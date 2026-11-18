#pragma once

// Module enumerator public API.
//
// This component enumerates the modules loaded in the current process in a
// read-only, informational manner. It does not enumerate arbitrary external
// processes, inject code, hook, or modify any module.
//
// All Windows-specific details are hidden behind this interface. The
// enumerator returns plain data structures (ModuleInfo) and never exposes
// raw Windows handles or structures.

#include "nova/loader/module/module_error.h"
#include "nova/loader/module/module_info.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nova::loader {

// Result of a module enumeration operation.
struct ModuleEnumeratorResult {
    bool ok = false;
    std::vector<ModuleInfo> modules;
    ModuleError error;
};

// Enumerates modules loaded in the current process.
//
// The class is moveable but not copyable. Each call to enumerate() performs a
// fresh snapshot of the module list.
class ModuleEnumerator {
public:
    ModuleEnumerator() = default;
    ModuleEnumerator(const ModuleEnumerator&) = delete;
    ModuleEnumerator& operator=(const ModuleEnumerator&) = delete;
    ModuleEnumerator(ModuleEnumerator&&) noexcept = default;
    ModuleEnumerator& operator=(ModuleEnumerator&&) noexcept = default;
    ~ModuleEnumerator() = default;

    // Enumerate modules in the current process.
    // Returns a result holding either the module list or an error. Partial
    // results are preserved whenever possible.
    [[nodiscard]] ModuleEnumeratorResult enumerate() noexcept;

    // Convert a wide-character (UTF-16) string to a UTF-8 encoded std::string.
    // Surrogate pairs are handled correctly by the Windows API. Returns true on
    // success; on failure out is left unchanged and the function returns false.
    // Provided so callers (and tests) can verify the conversion independently.
    [[nodiscard]] static bool wide_to_utf8(std::wstring_view wide, std::string& out) noexcept;

    // Find the first module whose base name matches (case-insensitive).
    // Returns std::nullopt if no module matches.
    [[nodiscard]] std::optional<std::reference_wrapper<const ModuleInfo>>
    find_by_name(std::string_view name) const noexcept;

    // Find the first module whose full path matches (case-insensitive).
    // Returns std::nullopt if no module matches.
    [[nodiscard]] std::optional<std::reference_wrapper<const ModuleInfo>>
    find_by_path(const std::filesystem::path& path) const noexcept;

    // The modules from the last successful enumeration, or empty if none has
    // been performed successfully.
    [[nodiscard]] const std::vector<ModuleInfo>& modules() const noexcept { return modules_; }

    [[nodiscard]] bool last_enumeration_ok() const noexcept { return last_ok_; }

private:
    std::vector<ModuleInfo> modules_;
    ModuleError last_error_;
    bool last_ok_ = false;
};

}  // namespace nova::loader
