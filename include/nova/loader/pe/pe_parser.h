#pragma once

// Portable Executable (PE) parser public API.
//
// This is a read-only PE parser. It inspects PE files without executing,
// loading, injecting, or modifying executable code. It never depends on the
// Windows loader and never maps a PE into memory.
//
// Two entry points are provided:
//   - parse(std::span<const std::byte>)  parse an in-memory buffer
//   - parse_file(const std::filesystem::path&)  parse a file on disk
//
// Both return a PeResult holding either a successfully parsed PeFile or a
// PeError describing what went wrong.

#include "nova/loader/pe/pe_headers.h"
#include "nova/loader/pe/pe_section.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace nova::loader {

// Classification of PE parse failures.
enum class PeErrorCode : int {
    None = 0,
    Unknown = 1,
    EmptyInput = 2,
    TooSmall = 3,
    InvalidDosSignature = 4,
    InvalidNtSignature = 5,
    InvalidOptionalHeaderMagic = 6,
    TruncatedDosHeader = 7,
    TruncatedNtHeaders = 8,
    TruncatedOptionalHeader = 9,
    InvalidSectionTableOffset = 10,
    SectionTableBeyondEof = 11,
    IntegerOverflow = 12,
    InvalidSectionRawDataBounds = 13,
    InvalidSectionName = 14,
    FileNotFound = 15,
    IoError = 16,
};

// A PE parse error.
struct PeError {
    PeErrorCode code = PeErrorCode::None;
    std::string message;
};

// Result of parsing a PE file.
struct PeResult {
    bool ok = false;
    PeFile pe;
    PeError error;
};

// Parse a PE from a byte span.
// Returns PeResult with ok=true on success, or ok=false with an error.
[[nodiscard]] PeResult parse(std::span<const std::byte> buffer) noexcept;

// Parse a PE from a file on disk.
// Returns PeResult with ok=true on success, or ok=false with an error.
[[nodiscard]] PeResult parse_file(const std::filesystem::path& path) noexcept;

// Human-readable description of a PeErrorCode.
[[nodiscard]] std::string pe_error_string(PeErrorCode code) noexcept;

}  // namespace nova::loader
