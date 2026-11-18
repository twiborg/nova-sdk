#pragma once

// Public PE header models.
//
// These structs hold the *parsed* contents of a PE file. They are not
// overlaid on raw memory; fields are populated explicitly by the parser
// after bounds-checked reads. This keeps the parser safe regardless of
// input alignment or packing.

#include "nova/loader/pe/pe_types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace nova::loader {

// COFF file header (present in both PE32 and PE32+).
struct CoffFileHeader {
    PEMachine machine = PEMachine::Unknown;
    std::uint16_t number_of_sections = 0;
    std::uint32_t time_date_stamp = 0;
    std::uint32_t pointer_to_symbol_table = 0;
    std::uint32_t number_of_symbols = 0;
    std::uint16_t size_of_optional_header = 0;
    std::uint16_t characteristics = 0;
};

// Data directory entry from the optional header.
struct DataDirectory {
    std::uint32_t virtual_address = 0;
    std::uint32_t size = 0;
};

// PE optional header model (fields common to PE32 and PE32+).
struct OptionalHeader {
    PEOptionalMagic magic = PEOptionalMagic::PE32;
    std::uint8_t major_linker_version = 0;
    std::uint8_t minor_linker_version = 0;
    std::uint32_t size_of_code = 0;
    std::uint32_t size_of_initialized_data = 0;
    std::uint32_t size_of_uninitialized_data = 0;
    std::uint32_t address_of_entry_point = 0;
    std::uint32_t base_of_code = 0;
    // PE32 only.
    bool has_base_of_data = false;
    std::uint32_t base_of_data = 0;
    // PE32+ only (and PE32 image base is a 32-bit field).
    std::uint64_t image_base = 0;
    std::uint32_t section_alignment = 0;
    std::uint32_t file_alignment = 0;
    std::uint16_t major_operating_system_version = 0;
    std::uint16_t minor_operating_system_version = 0;
    std::uint16_t major_image_version = 0;
    std::uint16_t minor_image_version = 0;
    std::uint16_t major_subsystem_version = 0;
    std::uint16_t minor_subsystem_version = 0;
    std::uint32_t win32_version_value = 0;
    std::uint32_t size_of_image = 0;
    std::uint32_t size_of_headers = 0;
    std::uint32_t check_sum = 0;
    std::uint16_t subsystem = 0;
    std::uint16_t dll_characteristics = 0;
    std::uint64_t size_of_stack_reserve = 0;
    std::uint64_t size_of_stack_commit = 0;
    std::uint64_t size_of_heap_reserve = 0;
    std::uint64_t size_of_heap_commit = 0;
    std::uint32_t loader_flags = 0;
    std::uint32_t number_of_rva_and_sizes = 0;
    std::vector<DataDirectory> data_directory;
};

// Full NT headers (signature + COFF + optional).
struct NtHeaders {
    std::uint32_t signature_offset = 0;   // offset of "PE\0\0" in the file
    CoffFileHeader coff_file_header;
    OptionalHeader optional_header;
    bool is_pe32_plus = false;
};

// Section header model.
struct SectionHeader {
    std::string name;
    std::uint32_t virtual_address = 0;
    std::uint32_t virtual_size = 0;
    std::uint32_t raw_data_offset = 0;    // pointer to raw data
    std::uint32_t raw_data_size = 0;      // size of raw data
    std::uint32_t characteristics = 0;
};

// Top-level result of parsing a PE file.
struct PeFile {
    NtHeaders nt_headers;
    std::vector<SectionHeader> sections;
    // Offset in the source buffer where the section table begins.
    std::uint32_t section_table_offset = 0;
    // Total size of the source buffer that was parsed.
    std::size_t buffer_size = 0;
};

}  // namespace nova::loader
