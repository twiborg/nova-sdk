// Portable Executable (PE) parser implementation.
//
// This is a read-only PE parser. It inspects PE files without executing,
// loading, injecting, or modifying executable code. All fields are parsed
// explicitly from a std::span<const std::byte> with explicit bounds checks
// and integer-overflow detection. No Windows headers are included and the
// Windows loader is never used.

#include "nova/loader/pe/pe_parser.h"
#include "nova/loader/pe/pe_section.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace nova::loader {

namespace {

// ---------------------------------------------------------------------------
// Low-level, bounds-checked readers.
// ---------------------------------------------------------------------------

// Read a little-endian std::uint8 from [offset, offset+1).
[[nodiscard]] bool read_u8(std::span<const std::byte> buffer,
                           std::size_t offset,
                           std::uint8_t& out) noexcept {
    if (offset + 1 > buffer.size()) {
        return false;
    }
    out = static_cast<std::uint8_t>(buffer[offset]);
    return true;
}

// Read a little-endian std::uint16 from [offset, offset+2).
[[nodiscard]] bool read_u16(std::span<const std::byte> buffer,
                            std::size_t offset,
                            std::uint16_t& out) noexcept {
    if (offset + 2 > buffer.size()) {
        return false;
    }
    out = static_cast<std::uint16_t>(
        static_cast<std::uint8_t>(buffer[offset]) |
        (static_cast<std::uint8_t>(buffer[offset + 1]) << 8));
    return true;
}

// Read a little-endian std::uint32 from [offset, offset+4).
[[nodiscard]] bool read_u32(std::span<const std::byte> buffer,
                            std::size_t offset,
                            std::uint32_t& out) noexcept {
    if (offset + 4 > buffer.size()) {
        return false;
    }
    out = static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[offset])) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[offset + 1])) << 8) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[offset + 2])) << 16) |
          (static_cast<std::uint32_t>(static_cast<std::uint8_t>(buffer[offset + 3])) << 24);
    return true;
}

// Read a little-endian std::uint64 from [offset, offset+8).
[[nodiscard]] bool read_u64(std::span<const std::byte> buffer,
                            std::size_t offset,
                            std::uint64_t& out) noexcept {
    if (offset + 8 > buffer.size()) {
        return false;
    }
    out = static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset])) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 1])) << 8) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 2])) << 16) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 3])) << 24) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 4])) << 32) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 5])) << 40) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 6])) << 48) |
          (static_cast<std::uint64_t>(static_cast<std::uint8_t>(buffer[offset + 7])) << 56);
    return true;
}

// Read N bytes from [offset, offset+n) into dest.
// Returns false if the range extends beyond buffer size.
[[nodiscard]] bool read_bytes(std::span<const std::byte> buffer,
                              std::size_t offset,
                              std::size_t n,
                              std::byte* dest) noexcept {
    if (offset > buffer.size() || n > buffer.size() - offset) {
        return false;
    }
    if (n == 0) {
        return true;
    }
    std::memcpy(dest, buffer.data() + offset, n);
    return true;
}

// Read a null-terminated (or 8-byte) section name from [offset, offset+8).
// PE section names are 8 bytes, not necessarily null-terminated.
[[nodiscard]] bool read_section_name(std::span<const std::byte> buffer,
                                     std::size_t offset,
                                     std::string& out) noexcept {
    std::byte raw[8] = {};
    if (!read_bytes(buffer, offset, 8, raw)) {
        return false;
    }
    std::string name;
    name.reserve(8);
    for (std::size_t i = 0; i < 8; ++i) {
        if (raw[i] == std::byte{0}) {
            break;
        }
        name.push_back(static_cast<char>(raw[i]));
    }
    out = std::move(name);
    return true;
}

// Check for overflow in a + b for std::uint32_t.
[[nodiscard]] bool checked_add_u32(std::uint32_t a,
                                   std::uint32_t b,
                                   std::uint32_t& out) noexcept {
    std::uint64_t sum = static_cast<std::uint64_t>(a) +
                        static_cast<std::uint64_t>(b);
    if (sum > static_cast<std::uint64_t>(
                std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    out = static_cast<std::uint32_t>(sum);
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// pe_error_string
// ---------------------------------------------------------------------------

std::string pe_error_string(PeErrorCode code) noexcept {
    switch (code) {
        case PeErrorCode::None: return "No error";
        case PeErrorCode::Unknown: return "Unknown error";
        case PeErrorCode::EmptyInput: return "Empty input";
        case PeErrorCode::TooSmall: return "Input too small";
        case PeErrorCode::InvalidDosSignature: return "Invalid DOS signature (not MZ)";
        case PeErrorCode::InvalidNtSignature: return "Invalid NT signature (not PE\\0\\0)";
        case PeErrorCode::InvalidOptionalHeaderMagic:
            return "Invalid optional header magic";
        case PeErrorCode::TruncatedDosHeader: return "Truncated DOS header";
        case PeErrorCode::TruncatedNtHeaders: return "Truncated NT headers";
        case PeErrorCode::TruncatedOptionalHeader:
            return "Truncated optional header";
        case PeErrorCode::InvalidSectionTableOffset:
            return "Invalid section table offset";
        case PeErrorCode::SectionTableBeyondEof:
            return "Section table extends beyond EOF";
        case PeErrorCode::IntegerOverflow: return "Integer overflow";
        case PeErrorCode::InvalidSectionRawDataBounds:
            return "Invalid section raw-data bounds";
        case PeErrorCode::InvalidSectionName: return "Invalid section name";
        case PeErrorCode::FileNotFound: return "File not found";
        case PeErrorCode::IoError: return "I/O error";
        default: return "Unknown PE error code";
    }
}

// ---------------------------------------------------------------------------
// parse
// ---------------------------------------------------------------------------

PeResult parse(std::span<const std::byte> buffer) noexcept {
    PeResult result;

    // Empty input.
    if (buffer.empty()) {
        result.ok = false;
        result.error.code = PeErrorCode::EmptyInput;
        result.error.message = "Input buffer is empty";
        return result;
    }

    // A valid PE must be at least large enough for a DOS header (64 bytes)
    // and an e_lfanew pointer read at offset 0x3c.
    constexpr std::size_t kMinDosHeaderSize = 0x40;  // 64 bytes
    if (buffer.size() < kMinDosHeaderSize) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedDosHeader;
        result.error.message = "Input too small for a DOS header";
        return result;
    }

    // Validate DOS signature "MZ" at offset 0.
    if (static_cast<std::uint8_t>(buffer[0]) != dos_signature[0] ||
        static_cast<std::uint8_t>(buffer[1]) != dos_signature[1]) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidDosSignature;
        result.error.message = "Invalid DOS signature (expected MZ)";
        return result;
    }

    // Read e_lfanew at offset 0x3c (4 bytes).
    std::uint32_t e_lfanew = 0;
    if (!read_u32(buffer, 0x3c, e_lfanew)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedDosHeader;
        result.error.message = "Failed to read e_lfanew from DOS header";
        return result;
    }

    // The NT signature must be present.
    constexpr std::size_t kNtSignatureSize = 4;
    if (e_lfanew > buffer.size() - kNtSignatureSize) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidSectionTableOffset;
        result.error.message = "e_lfanew points beyond the end of the file";
        return result;
    }

    // Validate NT signature "PE\0\0".
    bool nt_sig_ok = true;
    for (std::size_t i = 0; i < kNtSignatureSize; ++i) {
        if (static_cast<std::uint8_t>(buffer[e_lfanew + i]) != nt_signature[i]) {
            nt_sig_ok = false;
            break;
        }
    }
    if (!nt_sig_ok) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidNtSignature;
        result.error.message = "Invalid NT signature (expected PE\\0\\0)";
        return result;
    }

    // COFF file header starts immediately after the NT signature.
    constexpr std::size_t kCoffHeaderSize = 20;
    std::size_t coff_header_offset = e_lfanew + kNtSignatureSize;
    if (coff_header_offset > buffer.size() - kCoffHeaderSize) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedNtHeaders;
        result.error.message = "COFF file header extends beyond EOF";
        return result;
    }

    NtHeaders nt;
    nt.signature_offset = e_lfanew;

    // Parse COFF file header.
    std::uint16_t machine = 0;
    std::uint16_t number_of_sections = 0;
    std::uint16_t size_of_optional_header = 0;
    std::uint32_t time_date_stamp = 0;
    std::uint32_t pointer_to_symbol_table = 0;
    std::uint32_t number_of_symbols = 0;
    std::uint16_t characteristics = 0;

    std::uint32_t dummy32 = 0;
    if (!read_u16(buffer, coff_header_offset + 0, machine) ||
        !read_u16(buffer, coff_header_offset + 2, number_of_sections) ||
        !read_u32(buffer, coff_header_offset + 4, time_date_stamp) ||
        !read_u32(buffer, coff_header_offset + 8, pointer_to_symbol_table) ||
        !read_u32(buffer, coff_header_offset + 12, number_of_symbols) ||
        !read_u16(buffer, coff_header_offset + 16, size_of_optional_header) ||
        !read_u16(buffer, coff_header_offset + 18, characteristics)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedNtHeaders;
        result.error.message = "Failed to parse COFF file header";
        return result;
    }

    nt.coff_file_header.machine = static_cast<PEMachine>(machine);
    nt.coff_file_header.number_of_sections = number_of_sections;
    nt.coff_file_header.time_date_stamp = time_date_stamp;
    nt.coff_file_header.pointer_to_symbol_table = pointer_to_symbol_table;
    nt.coff_file_header.number_of_symbols = number_of_symbols;
    nt.coff_file_header.size_of_optional_header = size_of_optional_header;
    nt.coff_file_header.characteristics = characteristics;

    // Optional header follows the COFF header.
    std::size_t optional_header_offset = coff_header_offset + kCoffHeaderSize;
    std::uint64_t opt_header_end =
        static_cast<std::uint64_t>(optional_header_offset) +
        static_cast<std::uint64_t>(size_of_optional_header);
    if (opt_header_end > static_cast<std::uint64_t>(buffer.size())) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Optional header extends beyond EOF";
        return result;
    }

    // Read the optional header magic (first 2 bytes) to determine PE32 vs PE32+.
    std::uint16_t opt_magic = 0;
    if (size_of_optional_header < 2 ||
        !read_u16(buffer, optional_header_offset, opt_magic)) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidOptionalHeaderMagic;
        result.error.message = "Optional header too small to read magic";
        return result;
    }

    nt.is_pe32_plus = (opt_magic == static_cast<std::uint16_t>(PEOptionalMagic::PE32Plus));
    if (opt_magic != static_cast<std::uint16_t>(PEOptionalMagic::PE32) &&
        opt_magic != static_cast<std::uint16_t>(PEOptionalMagic::PE32Plus)) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidOptionalHeaderMagic;
        result.error.message = "Invalid optional header magic";
        return result;
    }
    nt.optional_header.magic =
        nt.is_pe32_plus ? PEOptionalMagic::PE32Plus : PEOptionalMagic::PE32;

    // ---- Optional header standard fields (PE32 and PE32+) ----
    // Offset 0:  Magic (2)
    // Offset 2:  MajorLinkerVersion (1), MinorLinkerVersion (1)
    // Offset 4:  SizeOfCode (4)
    // Offset 8:  SizeOfInitializedData (4)
    // Offset 12: SizeOfUninitializedData (4)
    // Offset 16: AddressOfEntryPoint (4)
    // Offset 20: BaseOfCode (4)
    constexpr std::size_t kPe32OptHeaderSizeBeforeDirs = 96;
    constexpr std::size_t kPe32PlusOptHeaderSizeBeforeDirs = 112;
    std::size_t opt_header_before_dirs =
        nt.is_pe32_plus ? kPe32PlusOptHeaderSizeBeforeDirs : kPe32OptHeaderSizeBeforeDirs;

    if (size_of_optional_header < opt_header_before_dirs) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message =
            std::string("Optional header too small for PE") +
            (nt.is_pe32_plus ? "32+" : "32") + " fields";
        return result;
    }

    std::size_t o = optional_header_offset;
    std::uint8_t major_linker = 0, minor_linker = 0;
    std::uint32_t size_of_code = 0, size_of_init_data = 0, size_of_uninit_data = 0;
    std::uint32_t address_of_entry_point = 0, base_of_code = 0;

    if (!read_u8(buffer, o + 2, major_linker) ||
        !read_u8(buffer, o + 3, minor_linker) ||
        !read_u32(buffer, o + 4, size_of_code) ||
        !read_u32(buffer, o + 8, size_of_init_data) ||
        !read_u32(buffer, o + 12, size_of_uninit_data) ||
        !read_u32(buffer, o + 16, address_of_entry_point) ||
        !read_u32(buffer, o + 20, base_of_code)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Failed to parse optional header standard fields";
        return result;
    }

    nt.optional_header.major_linker_version = major_linker;
    nt.optional_header.minor_linker_version = minor_linker;
    nt.optional_header.size_of_code = size_of_code;
    nt.optional_header.size_of_initialized_data = size_of_init_data;
    nt.optional_header.size_of_uninitialized_data = size_of_uninit_data;
    nt.optional_header.address_of_entry_point = address_of_entry_point;
    nt.optional_header.base_of_code = base_of_code;

    // PE32: BaseOfData (4) at o+24, ImageBase (4) at o+28.
    // PE32+: ImageBase (8) at o+24 (no BaseOfData).
    if (nt.is_pe32_plus) {
        std::uint64_t image_base = 0;
        if (!read_u64(buffer, o + 24, image_base)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedOptionalHeader;
            result.error.message = "Failed to read PE32+ image_base";
            return result;
        }
        nt.optional_header.image_base = image_base;
    } else {
        std::uint32_t base_of_data = 0;
        std::uint32_t image_base32 = 0;
        if (!read_u32(buffer, o + 24, base_of_data) ||
            !read_u32(buffer, o + 28, image_base32)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedOptionalHeader;
            result.error.message = "Failed to read PE32 base_of_data/image_base";
            return result;
        }
        nt.optional_header.has_base_of_data = true;
        nt.optional_header.base_of_data = base_of_data;
        nt.optional_header.image_base = image_base32;
    }

    // SectionAlignment (4) at o+32, FileAlignment (4) at o+36.
    std::uint32_t section_alignment = 0, file_alignment = 0;
    if (!read_u32(buffer, o + 32, section_alignment) ||
        !read_u32(buffer, o + 36, file_alignment)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Failed to read alignment fields";
        return result;
    }
    nt.optional_header.section_alignment = section_alignment;
    nt.optional_header.file_alignment = file_alignment;

    // OS version (40), image version (44), subsystem version (48).
    std::uint16_t major_os = 0, minor_os = 0;
    std::uint16_t major_img = 0, minor_img = 0;
    std::uint16_t major_sub = 0, minor_sub = 0;
    if (!read_u16(buffer, o + 40, major_os) ||
        !read_u16(buffer, o + 42, minor_os) ||
        !read_u16(buffer, o + 44, major_img) ||
        !read_u16(buffer, o + 46, minor_img) ||
        !read_u16(buffer, o + 48, major_sub) ||
        !read_u16(buffer, o + 50, minor_sub)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Failed to read version fields";
        return result;
    }
    nt.optional_header.major_operating_system_version = major_os;
    nt.optional_header.minor_operating_system_version = minor_os;
    nt.optional_header.major_image_version = major_img;
    nt.optional_header.minor_image_version = minor_img;
    nt.optional_header.major_subsystem_version = major_sub;
    nt.optional_header.minor_subsystem_version = minor_sub;

    // Win32VersionValue (52), SizeOfImage (56), SizeOfHeaders (60), CheckSum (64).
    std::uint32_t win32_version = 0, size_of_image = 0, size_of_headers = 0;
    std::uint32_t check_sum = 0;
    if (!read_u32(buffer, o + 52, win32_version) ||
        !read_u32(buffer, o + 56, size_of_image) ||
        !read_u32(buffer, o + 60, size_of_headers) ||
        !read_u32(buffer, o + 64, check_sum)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Failed to read optional header image fields";
        return result;
    }
    nt.optional_header.win32_version_value = win32_version;
    nt.optional_header.size_of_image = size_of_image;
    nt.optional_header.size_of_headers = size_of_headers;
    nt.optional_header.check_sum = check_sum;

    // Subsystem (68), DllCharacteristics (70).
    std::uint16_t subsystem = 0, dll_characteristics = 0;
    if (!read_u16(buffer, o + 68, subsystem) ||
        !read_u16(buffer, o + 70, dll_characteristics)) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Failed to read subsystem fields";
        return result;
    }
    nt.optional_header.subsystem = subsystem;
    nt.optional_header.dll_characteristics = dll_characteristics;

    // Stack/heap reserve/commit and loader flags.
    std::uint64_t stack_reserve = 0, stack_commit = 0;
    std::uint64_t heap_reserve = 0, heap_commit = 0;
    std::uint32_t loader_flags = 0, number_of_rva_and_sizes = 0;

    if (nt.is_pe32_plus) {
        // PE32+: StackReserve (8) at o+72, StackCommit (8) at o+80,
        //        HeapReserve (8) at o+88, HeapCommit (8) at o+96,
        //        LoaderFlags (4) at o+104, NumberOfRvaAndSizes (4) at o+108.
        if (!read_u64(buffer, o + 72, stack_reserve) ||
            !read_u64(buffer, o + 80, stack_commit) ||
            !read_u64(buffer, o + 88, heap_reserve) ||
            !read_u64(buffer, o + 96, heap_commit) ||
            !read_u32(buffer, o + 104, loader_flags) ||
            !read_u32(buffer, o + 108, number_of_rva_and_sizes)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedOptionalHeader;
            result.error.message = "Failed to read PE32+ optional header fields";
            return result;
        }
    } else {
        // PE32: StackReserve (4) at o+72, StackCommit (4) at o+76,
        //       HeapReserve (4) at o+80, HeapCommit (4) at o+84,
        //       LoaderFlags (4) at o+88, NumberOfRvaAndSizes (4) at o+92.
        std::uint32_t stack_reserve32 = 0, stack_commit32 = 0;
        std::uint32_t heap_reserve32 = 0, heap_commit32 = 0;
        if (!read_u32(buffer, o + 72, stack_reserve32) ||
            !read_u32(buffer, o + 76, stack_commit32) ||
            !read_u32(buffer, o + 80, heap_reserve32) ||
            !read_u32(buffer, o + 84, heap_commit32) ||
            !read_u32(buffer, o + 88, loader_flags) ||
            !read_u32(buffer, o + 92, number_of_rva_and_sizes)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedOptionalHeader;
            result.error.message = "Failed to read PE32 optional header fields";
            return result;
        }
        stack_reserve = stack_reserve32;
        stack_commit = stack_commit32;
        heap_reserve = heap_reserve32;
        heap_commit = heap_commit32;
    }
    nt.optional_header.size_of_stack_reserve = stack_reserve;
    nt.optional_header.size_of_stack_commit = stack_commit;
    nt.optional_header.size_of_heap_reserve = heap_reserve;
    nt.optional_header.size_of_heap_commit = heap_commit;
    nt.optional_header.loader_flags = loader_flags;
    nt.optional_header.number_of_rva_and_sizes = number_of_rva_and_sizes;

    // Data directories.
    std::size_t data_dir_offset =
        optional_header_offset + opt_header_before_dirs;
    std::uint64_t data_dir_end =
        static_cast<std::uint64_t>(data_dir_offset) +
        static_cast<std::uint64_t>(number_of_rva_and_sizes) * 8ULL;
    if (data_dir_end > opt_header_end) {
        result.ok = false;
        result.error.code = PeErrorCode::TruncatedOptionalHeader;
        result.error.message = "Data directories extend beyond optional header";
        return result;
    }
    nt.optional_header.data_directory.reserve(number_of_rva_and_sizes);
    for (std::uint32_t i = 0; i < number_of_rva_and_sizes; ++i) {
        std::uint32_t va = 0, sz = 0;
        std::size_t dir_off = data_dir_offset + static_cast<std::size_t>(i) * 8ULL;
        if (!read_u32(buffer, dir_off, va) || !read_u32(buffer, dir_off + 4, sz)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedOptionalHeader;
            result.error.message = "Failed to read data directory entry";
            return result;
        }
        nt.optional_header.data_directory.push_back(DataDirectory{va, sz});
    }

    // ---- Section table ----
    // Section table starts immediately after the optional header.
    std::uint64_t section_table_offset64 = opt_header_end;
    if (section_table_offset64 > static_cast<std::uint64_t>(buffer.size())) {
        result.ok = false;
        result.error.code = PeErrorCode::InvalidSectionTableOffset;
        result.error.message = "Section table offset extends beyond EOF";
        return result;
    }
    std::size_t section_table_offset = static_cast<std::size_t>(section_table_offset64);

    std::uint32_t number_of_sections_val = nt.coff_file_header.number_of_sections;
    constexpr std::size_t kSectionHeaderSize = 40;
    std::uint64_t section_table_end =
        static_cast<std::uint64_t>(section_table_offset) +
        static_cast<std::uint64_t>(number_of_sections_val) *
        static_cast<std::uint64_t>(kSectionHeaderSize);
    if (section_table_end > static_cast<std::uint64_t>(buffer.size())) {
        result.ok = false;
        result.error.code = PeErrorCode::SectionTableBeyondEof;
        result.error.message = "Section table extends beyond EOF";
        return result;
    }

    result.pe.nt_headers = nt;
    result.pe.section_table_offset = static_cast<std::uint32_t>(section_table_offset);
    result.pe.buffer_size = buffer.size();

    // Parse each section header.
    for (std::uint32_t i = 0; i < number_of_sections_val; ++i) {
        std::size_t sec_off = section_table_offset + static_cast<std::size_t>(i) * kSectionHeaderSize;

        SectionHeader sec;
        if (!read_section_name(buffer, sec_off, sec.name)) {
            result.ok = false;
            result.error.code = PeErrorCode::InvalidSectionName;
            result.error.message = "Failed to read section name";
            return result;
        }

        // Reject empty names as malformed.
        if (sec.name.empty()) {
            result.ok = false;
            result.error.code = PeErrorCode::InvalidSectionName;
            result.error.message = "Section has an empty name";
            return result;
        }

        std::uint32_t virtual_size = 0;
        std::uint32_t virtual_address = 0;
        std::uint32_t raw_data_size = 0;
        std::uint32_t raw_data_offset = 0;
        std::uint32_t section_characteristics = 0;

        if (!read_u32(buffer, sec_off + 8, virtual_size) ||
            !read_u32(buffer, sec_off + 12, virtual_address) ||
            !read_u32(buffer, sec_off + 16, raw_data_size) ||
            !read_u32(buffer, sec_off + 20, raw_data_offset) ||
            !read_u32(buffer, sec_off + 24, dummy32) ||
            !read_u32(buffer, sec_off + 28, dummy32) ||
            !read_u32(buffer, sec_off + 32, dummy32) ||
            !read_u32(buffer, sec_off + 36, section_characteristics)) {
            result.ok = false;
            result.error.code = PeErrorCode::TruncatedNtHeaders;
            result.error.message = "Failed to parse section header";
            return result;
        }

        sec.virtual_size = virtual_size;
        sec.virtual_address = virtual_address;
        sec.raw_data_size = raw_data_size;
        sec.raw_data_offset = raw_data_offset;
        sec.characteristics = section_characteristics;

        // Validate raw data bounds.
        std::uint64_t raw_end =
            static_cast<std::uint64_t>(raw_data_offset) +
            static_cast<std::uint64_t>(raw_data_size);
        if (raw_end > static_cast<std::uint64_t>(buffer.size())) {
            result.ok = false;
            result.error.code = PeErrorCode::InvalidSectionRawDataBounds;
            result.error.message = "Section '" + sec.name +
                                   "' raw data extends beyond EOF";
            return result;
        }

        result.pe.sections.push_back(std::move(sec));
    }

    result.ok = true;
    return result;
}

// ---------------------------------------------------------------------------
// parse_file
// ---------------------------------------------------------------------------

PeResult parse_file(const std::filesystem::path& path) noexcept {
    PeResult result;
    std::error_code ec;
    std::uintmax_t file_size = std::filesystem::file_size(path, ec);
    if (ec) {
        result.ok = false;
        result.error.code = PeErrorCode::FileNotFound;
        result.error.message = "Cannot open file: " + path.string();
        return result;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        result.ok = false;
        result.error.code = PeErrorCode::FileNotFound;
        result.error.message = "Cannot open file: " + path.string();
        return result;
    }

    std::vector<std::byte> buffer;
    buffer.resize(static_cast<std::size_t>(file_size));
    if (!buffer.empty()) {
        file.read(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size()));
    }
    if (file.fail() && !file.eof()) {
        result.ok = false;
        result.error.code = PeErrorCode::IoError;
        result.error.message = "Failed to read file: " + path.string();
        return result;
    }

    return parse(std::span<const std::byte>(buffer));
}

}  // namespace nova::loader

// ---------------------------------------------------------------------------
// Section helper implementations.
// ---------------------------------------------------------------------------

#include "nova/loader/pe/pe_section.h"

namespace nova::loader {

std::optional<std::size_t> find_section_by_name(
    const std::vector<SectionHeader>& sections,
    std::string_view name) noexcept {
    for (std::size_t i = 0; i < sections.size(); ++i) {
        if (sections[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> section_containing_rva(
    const std::vector<SectionHeader>& sections,
    std::uint32_t rva) noexcept {
    for (std::size_t i = 0; i < sections.size(); ++i) {
        const SectionHeader& sec = sections[i];
        std::uint64_t sec_end =
            static_cast<std::uint64_t>(sec.virtual_address) +
            static_cast<std::uint64_t>(sec.virtual_size);
        if (rva >= sec.virtual_address && rva < sec_end) {
            return i;
        }
    }
    return std::nullopt;
}

bool rva_in_section(const std::vector<SectionHeader>& sections,
                    std::uint32_t rva) noexcept {
    return section_containing_rva(sections, rva).has_value();
}

std::optional<std::uint32_t> rva_to_offset(const PeFile& pe,
                                           std::uint32_t rva) noexcept {
    auto idx = section_containing_rva(pe.sections, rva);
    if (!idx.has_value()) {
        return std::nullopt;
    }
    const SectionHeader& sec = pe.sections[*idx];
    std::uint64_t delta =
        static_cast<std::uint64_t>(rva) - static_cast<std::uint64_t>(sec.virtual_address);
    if (sec.raw_data_size == 0 || sec.raw_data_offset == 0) {
        return std::nullopt;
    }
    if (delta >= static_cast<std::uint64_t>(sec.raw_data_size)) {
        return std::nullopt;
    }
    std::uint64_t offset =
        static_cast<std::uint64_t>(sec.raw_data_offset) + delta;
    if (offset > static_cast<std::uint64_t>(pe.buffer_size)) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(offset);
}

}  // namespace nova::loader
