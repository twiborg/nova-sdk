#pragma once

// Section helper utilities.
//
// Provides safe, bounds-checked operations for working with PE sections:
// finding sections by name, testing whether an RVA falls within a section,
// and converting an RVA to a file offset. All operations gracefully handle
// header RVAs, virtual-only regions, overlapping sections, and out-of-range
// values without reading outside the supplied buffer.

#include "nova/loader/pe/pe_headers.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace nova::loader {

// Find the first section whose name matches (case-sensitive).
// Returns std::nullopt if no section matches.
[[nodiscard]] std::optional<std::size_t> find_section_by_name(
    const std::vector<SectionHeader>& sections,
    std::string_view name) noexcept;

// Return true if the given RVA falls within the virtual range of any section.
// Header RVAs (below the first section's virtual address) are not considered
// part of any section.
[[nodiscard]] bool rva_in_section(
    const std::vector<SectionHeader>& sections,
    std::uint32_t rva) noexcept;

// Convert an RVA to a file offset within the parsed PE buffer.
// Returns std::nullopt when the RVA cannot be mapped (e.g. it falls in a
// header-only region, a virtual-only section gap, or outside all sections).
// raw_data_size is used when present; virtual_size is used as a fallback.
[[nodiscard]] std::optional<std::uint32_t> rva_to_offset(
    const PeFile& pe,
    std::uint32_t rva) noexcept;

// Return the section index containing the given RVA, or std::nullopt.
[[nodiscard]] std::optional<std::size_t> section_containing_rva(
    const std::vector<SectionHeader>& sections,
    std::uint32_t rva) noexcept;

}  // namespace nova::loader
