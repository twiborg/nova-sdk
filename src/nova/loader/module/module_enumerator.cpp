// Module enumerator implementation.
//
// This component enumerates the modules loaded in the current process in a
// read-only, informational manner. It uses only documented Windows APIs and
// never enumerates external processes, injects code, hooks, or modifies any
// module. All Windows-specific headers and handles are confined to this
// implementation file.

#include "nova/loader/module/module_enumerator.h"
#include "nova/loader/pe/pe_types.h"

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "psapi.lib")

namespace nova::loader {

namespace {

// Architecture mapping from a PE machine type.
[[nodiscard]] ModuleArchitecture machine_to_architecture(PEMachine machine) noexcept {
    switch (machine) {
        case PEMachine::I386: return ModuleArchitecture::X86;
        case PEMachine::AMD64: return ModuleArchitecture::X64;
        default: return ModuleArchitecture::Unknown;
    }
}

// Read a little-endian std::uint16 from [ptr, ptr+2) via memcpy (alignment
// safe). Returns false if the pointer is null.
[[nodiscard]] bool read_u16_from_memory(const void* ptr, std::uint16_t& out) noexcept {
    if (ptr == nullptr) {
        return false;
    }
    std::uint8_t bytes[2] = {};
    std::memcpy(bytes, ptr, 2);
    out = static_cast<std::uint16_t>(bytes[0] | (static_cast<std::uint16_t>(bytes[1]) << 8));
    return true;
}

// Read a little-endian std::uint32 from [ptr, ptr+4) via memcpy.
[[nodiscard]] bool read_u32_from_memory(const void* ptr, std::uint32_t& out) noexcept {
    if (ptr == nullptr) {
        return false;
    }
    std::uint8_t bytes[4] = {};
    std::memcpy(bytes, ptr, 4);
    out = static_cast<std::uint32_t>(bytes[0]) |
          (static_cast<std::uint32_t>(bytes[1]) << 8) |
          (static_cast<std::uint32_t>(bytes[2]) << 16) |
          (static_cast<std::uint32_t>(bytes[3]) << 24);
    return true;
}

// Peek at the PE machine type of a module loaded at base_address by reading
// its in-memory DOS + NT headers (read-only access to our own process memory).
// Returns PEMachine::Unknown when the headers cannot be read or the machine
// type is unrecognized; this does not fail the enumeration.
[[nodiscard]] PEMachine read_pe_machine(std::uintptr_t base_address) noexcept {
    if (base_address == 0) {
        return PEMachine::Unknown;
    }

    const std::byte* base = reinterpret_cast<const std::byte*>(base_address);

    // DOS signature "MZ" at offset 0.
    std::uint16_t dos_sig = 0;
    if (!read_u16_from_memory(base, dos_sig) ||
        dos_sig != static_cast<std::uint16_t>(0x5a4d)) {  // "MZ" little-endian
        return PEMachine::Unknown;
    }

    // e_lfanew at offset 0x3c.
    std::uint32_t e_lfanew = 0;
    if (!read_u32_from_memory(base + 0x3c, e_lfanew)) {
        return PEMachine::Unknown;
    }

    const std::byte* nt_header = base + e_lfanew;

    // NT signature "PE\0\0".
    std::uint32_t nt_sig = 0;
    if (!read_u32_from_memory(nt_header, nt_sig) ||
        nt_sig != 0x00004550) {  // "PE\0\0" little-endian
        return PEMachine::Unknown;
    }

    // COFF Machine field immediately after the NT signature.
    std::uint16_t machine = 0;
    if (!read_u16_from_memory(nt_header + 4, machine)) {
        return PEMachine::Unknown;
    }
    return static_cast<PEMachine>(machine);
}

// Case-insensitive comparison of two narrow strings (ASCII-focused; module
// names and typical Windows paths are ASCII-compatible).
[[nodiscard]] bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        char ca = static_cast<char>(a[i]);
        char cb = static_cast<char>(b[i]);
        if (ca >= 'a' && ca <= 'z') ca = static_cast<char>(ca - ('a' - 'A'));
        if (cb >= 'a' && cb <= 'z') cb = static_cast<char>(cb - ('a' - 'A'));
        if (ca != cb) {
            return false;
        }
    }
    return true;
}

// Case-insensitive comparison of two wide strings.
[[nodiscard]] bool iequals_wide(std::wstring_view a, std::wstring_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        wchar_t ca = a[i];
        wchar_t cb = b[i];
        if (ca >= L'a' && ca <= L'z') ca = static_cast<wchar_t>(ca - (L'a' - L'A'));
        if (cb >= L'a' && cb <= L'z') cb = static_cast<wchar_t>(cb - (L'a' - L'A'));
        if (ca != cb) {
            return false;
        }
    }
    return true;
}

// Read a wide-character module name/path into a growable buffer.
// Returns false if the value could not be retrieved at all.
[[nodiscard]] bool read_module_string(
    HANDLE process,
    HMODULE module,
    DWORD (WINAPI* getter)(HANDLE, HMODULE, LPWSTR, DWORD),
    std::wstring& out) noexcept {
    DWORD size = 4096;
    std::vector<wchar_t> buffer(size);
    for (int attempt = 0; attempt < 5; ++attempt) {
        DWORD written = getter(process, module, buffer.data(), size);
        if (written == 0) {
            return false;
        }
        if (written < size - 1) {
            out.assign(buffer.data(), written);
            return true;
        }
        // Buffer was too small; grow and retry.
        size *= 2;
        if (size > 32768) {
            size = 32768;
        }
        buffer.resize(size);
    }
    return false;
}

}  // namespace

// Convert a wide-character (UTF-16) string to a UTF-8 encoded std::string.
[[nodiscard]] bool ModuleEnumerator::wide_to_utf8(std::wstring_view wide,
                                                  std::string& out) noexcept {
    if (wide.empty()) {
        out.clear();
        return true;
    }
    const int needed = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return false;
    }
    std::string result(static_cast<std::size_t>(needed), '\0');
    const int written = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
        result.data(), needed, nullptr, nullptr);
    if (written <= 0 || written != needed) {
        return false;
    }
    out = std::move(result);
    return true;
}

// ---------------------------------------------------------------------------
// ModuleEnumerator
// ---------------------------------------------------------------------------

ModuleEnumeratorResult ModuleEnumerator::enumerate() noexcept {
    ModuleEnumeratorResult result;
    result.ok = false;

    const HANDLE process = GetCurrentProcess();  // pseudo-handle, do not close

    // First pass: determine the required buffer size.
    DWORD needed = 0;
    if (!EnumProcessModulesEx(process, nullptr, 0, &needed, LIST_MODULES_ALL)) {
        result.error.code = ModuleErrorCode::ApiFailure;
        result.error.message =
            "EnumProcessModulesEx (size query) failed: " +
            std::to_string(static_cast<unsigned long>(GetLastError()));
        last_error_ = result.error;
        last_ok_ = false;
        modules_.clear();
        return result;
    }

    if (needed == 0) {
        // Empty module list is valid.
        result.ok = true;
        result.modules.clear();
        modules_.clear();
        modules_.shrink_to_fit();
        last_error_ = ModuleError{};
        last_ok_ = true;
        return result;
    }

    const std::size_t module_count = static_cast<std::size_t>(needed) / sizeof(HMODULE);
    std::vector<HMODULE> modules(module_count);

    // Second pass: retrieve the module handles.
    DWORD needed2 = 0;
    if (!EnumProcessModulesEx(
            process,
            modules.data(),
            static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
            &needed2,
            LIST_MODULES_ALL)) {
        result.error.code = ModuleErrorCode::EnumerationFailure;
        result.error.message =
            "EnumProcessModulesEx (retrieval) failed: " +
            std::to_string(static_cast<unsigned long>(GetLastError()));
        last_error_ = result.error;
        last_ok_ = false;
        modules_.clear();
        return result;
    }

    // If the module list grew between calls, retry once with a larger buffer.
    if (needed2 > needed) {
        const std::size_t retry_count =
            static_cast<std::size_t>(needed2) / sizeof(HMODULE);
        modules.resize(retry_count);
        if (!EnumProcessModulesEx(
                process,
                modules.data(),
                static_cast<DWORD>(modules.size() * sizeof(HMODULE)),
                &needed2,
                LIST_MODULES_ALL)) {
            result.error.code = ModuleErrorCode::StateChanged;
            result.error.message = "Module list changed during enumeration";
            last_error_ = result.error;
            last_ok_ = false;
            modules_.clear();
            return result;
        }
    }

    // Collect information for each module. A single inaccessible module does
    // not invalidate the whole enumeration; partial results are preserved.
    std::vector<ModuleInfo> collected;
    collected.reserve(modules.size());
    bool any_metadata_failure = false;

    for (HMODULE hmod : modules) {
        ModuleInfo info;

        MODULEINFO modinfo = {};
        if (!GetModuleInformation(process, hmod, &modinfo, sizeof(modinfo))) {
            // Cannot query this module's base/size; skip it but keep going.
            any_metadata_failure = true;
            continue;
        }

        info.base_address = reinterpret_cast<std::uintptr_t>(modinfo.lpBaseOfDll);
        info.image_size = static_cast<std::uint64_t>(modinfo.SizeOfImage);

        // Detect architecture from the module's in-memory PE headers.
        PEMachine machine = read_pe_machine(info.base_address);
        info.pe_machine = machine;
        info.architecture = machine_to_architecture(machine);

        // Full filesystem path.
        std::wstring path_wide;
        if (read_module_string(process, hmod, GetModuleFileNameExW, path_wide)) {
            info.full_path = std::filesystem::path(path_wide);
        }

        // Module base name.
        std::wstring name_wide;
        if (read_module_string(process, hmod, GetModuleBaseNameW, name_wide)) {
            if (!wide_to_utf8(name_wide, info.name)) {
                // Conversion failure: leave name empty rather than corrupting it.
                info.name.clear();
            }
        }

        collected.push_back(std::move(info));
    }

    modules_ = std::move(collected);
    result.modules = modules_;
    last_ok_ = true;
    last_error_ = ModuleError{};
    result.ok = true;
    result.error = ModuleError{};

    if (any_metadata_failure) {
        // Preserve partial results but note that some metadata was missing.
        result.error.code = ModuleErrorCode::MetadataFailure;
        result.error.message = "Metadata for one or more modules could not be retrieved";
        last_error_ = result.error;
    }

    return result;
}

std::optional<std::reference_wrapper<const ModuleInfo>>
ModuleEnumerator::find_by_name(std::string_view name) const noexcept {
    for (const ModuleInfo& info : modules_) {
        if (iequals(info.name, name)) {
            return std::cref(info);
        }
    }
    return std::nullopt;
}

std::optional<std::reference_wrapper<const ModuleInfo>>
ModuleEnumerator::find_by_path(const std::filesystem::path& path) const noexcept {
    const std::wstring_view target = path.native();
    for (const ModuleInfo& info : modules_) {
        if (iequals_wide(info.full_path.native(), target)) {
            return std::cref(info);
        }
    }
    return std::nullopt;
}

}  // namespace nova::loader
