#pragma once

// PE type definitions and constants.
//
// This header defines the PE-related constants and fixed-width integer
// aliases used by the PE parser. It deliberately does NOT define C++
// structs that mirror the Windows PE header layouts, because directly
// reinterpreting an arbitrary input buffer as a Windows struct is unsafe
// (alignment, packing, and buffer-boundary issues). All PE header fields
// are parsed explicitly from a byte span in pe_parser.cpp.

#include <cstdint>
#include <cstddef>

namespace nova::loader {

// Machine types from the COFF file header.
enum class PEMachine : std::uint16_t {
    Unknown = 0x0000,
    I386 = 0x014c,    // Intel 386 (PE32)
    AMD64 = 0x8664,   // AMD64 (PE32+)
};

// Optional header magic values.
enum class PEOptionalMagic : std::uint16_t {
    PE32 = 0x010b,   // PE32
    PE32Plus = 0x020b,  // PE32+
};

// Section characteristics flags (subset).
enum class PESectionCharacteristics : std::uint32_t {
    None = 0x00000000,
    CNT_CODE = 0x00000020,
    CNT_INITIALIZED_DATA = 0x00000040,
    CNT_UNINITIALIZED_DATA = 0x00000080,
    MEM_EXECUTE = 0x20000000,
    MEM_READ = 0x40000000,
    MEM_WRITE = 0x80000000,
};

// Fixed-width byte type used throughout the parser.
using byte = std::byte;

// Fixed-size type aliases used to represent PE header fields.
using pe_uint8 = std::uint8_t;
using pe_uint16 = std::uint16_t;
using pe_uint32 = std::uint32_t;
using pe_uint64 = std::uint64_t;

// Signature constants stored as 4-byte arrays (little-endian).
constexpr std::uint8_t dos_signature[2] = {0x4d, 0x5a};   // "MZ"
constexpr std::uint8_t nt_signature[4] = {0x50, 0x45, 0x00, 0x00};  // "PE\0\0"

}  // namespace nova::loader
