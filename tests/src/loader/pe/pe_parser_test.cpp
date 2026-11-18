#include "nova/loader/pe/pe_parser.h"
#include "nova/loader/pe/pe_section.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Synthetic PE fixture builders.
// ---------------------------------------------------------------------------

void write_u16(std::vector<std::byte>& buf, std::size_t off, std::uint16_t v) {
    buf[off] = static_cast<std::byte>(v & 0xff);
    buf[off + 1] = static_cast<std::byte>((v >> 8) & 0xff);
}

void write_u32(std::vector<std::byte>& buf, std::size_t off, std::uint32_t v) {
    buf[off] = static_cast<std::byte>(v & 0xff);
    buf[off + 1] = static_cast<std::byte>((v >> 8) & 0xff);
    buf[off + 2] = static_cast<std::byte>((v >> 16) & 0xff);
    buf[off + 3] = static_cast<std::byte>((v >> 24) & 0xff);
}

void write_u64_le(std::vector<std::byte>& buf, std::size_t off, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf[off + i] = static_cast<std::byte>((v >> (8 * i)) & 0xff);
    }
}

// Build a minimal, well-formed PE file in memory.
// Layout:
//   0x000-0x03F : DOS header (64 bytes), e_lfanew at 0x3c
//   0x080       : NT headers ("PE\0\0" + COFF + optional)
//   optional header size: 96 (PE32) / 112 (PE32+) bytes before dirs
//   data directories: number_of_rva_and_sizes = 0
//   section table : after optional header
std::vector<std::byte> make_pe(nova::loader::PEMachine machine,
                               std::uint16_t num_sections,
                               bool pe32_plus,
                               std::uint32_t entry_point = 0x1000,
                               std::uint64_t image_base = 0x140000000ULL) {
    constexpr std::size_t kNtHeadersStart = 0x80;
    constexpr std::size_t kCoffHeaderSize = 20;
    constexpr std::size_t kSectionHeaderSize = 40;
    constexpr std::size_t kOptBeforeDirs32 = 96;
    constexpr std::size_t kOptBeforeDirs32Plus = 112;

    const std::size_t opt_before_dirs =
        pe32_plus ? kOptBeforeDirs32Plus : kOptBeforeDirs32;
    // NT headers: signature (4) + COFF (20) + optional header.
    const std::size_t opt_start = kNtHeadersStart + 4 + kCoffHeaderSize;
    const std::size_t opt_end = opt_start + opt_before_dirs;
    const std::size_t section_table_start = opt_end;
    constexpr std::size_t kRawDataSize = 0x200;
    const std::size_t raw_data_start =
        section_table_start + static_cast<std::size_t>(num_sections) * kSectionHeaderSize;
    const std::size_t file_size =
        raw_data_start + static_cast<std::size_t>(num_sections) * kRawDataSize;

    std::vector<std::byte> buf(file_size, std::byte{0});

    // ---- DOS header ----
    buf[0] = std::byte{0x4d};  // 'M'
    buf[1] = std::byte{0x5a};  // 'Z'
    write_u32(buf, 0x3c, static_cast<std::uint32_t>(kNtHeadersStart));  // e_lfanew

    // ---- NT headers ----
    buf[kNtHeadersStart + 0] = std::byte{0x50};  // 'P'
    buf[kNtHeadersStart + 1] = std::byte{0x45};  // 'E'
    buf[kNtHeadersStart + 2] = std::byte{0x00};
    buf[kNtHeadersStart + 3] = std::byte{0x00};

    // COFF file header
    std::size_t coff = kNtHeadersStart + 4;
    write_u16(buf, coff + 0, static_cast<std::uint16_t>(machine));
    write_u16(buf, coff + 2, num_sections);
    write_u32(buf, coff + 4, 0);                              // time_date_stamp
    write_u32(buf, coff + 8, 0);                              // pointer_to_symbol_table
    write_u32(buf, coff + 12, 0);                             // number_of_symbols
    write_u16(buf, coff + 16, static_cast<std::uint16_t>(opt_before_dirs));  // size_of_optional_header
    write_u16(buf, coff + 18, 0);                             // characteristics

    // Optional header
    std::size_t o = opt_start;
    write_u16(buf, o + 0, pe32_plus ? 0x020b : 0x010b);  // magic
    buf[o + 2] = std::byte{0x0f};                         // major_linker
    buf[o + 3] = std::byte{0x00};                         // minor_linker
    write_u32(buf, o + 4, 0x1000);                        // size_of_code
    write_u32(buf, o + 8, 0x1000);                        // size_of_init_data
    write_u32(buf, o + 12, 0);                            // size_of_uninit_data
    write_u32(buf, o + 16, entry_point);                  // address_of_entry_point
    write_u32(buf, o + 20, 0x1000);                       // base_of_code

    if (pe32_plus) {
        write_u64_le(buf, o + 24, image_base);            // image_base (8)
    } else {
        write_u32(buf, o + 24, 0);                        // base_of_data (4)
        write_u32(buf, o + 28,
                  static_cast<std::uint32_t>(image_base & 0xffffffffULL));  // image_base (4)
    }
    write_u32(buf, o + 32, 0x1000);                       // section_alignment
    write_u32(buf, o + 36, 0x200);                        // file_alignment
    write_u16(buf, o + 40, 6);                            // major_os
    write_u16(buf, o + 42, 0);                            // minor_os
    write_u16(buf, o + 44, 0);                            // major_image
    write_u16(buf, o + 46, 0);                            // minor_image
    write_u16(buf, o + 48, 6);                            // major_subsystem
    write_u16(buf, o + 50, 0);                            // minor_subsystem
    write_u32(buf, o + 52, 0);                            // win32_version
    write_u32(buf, o + 56, 0x2000);                       // size_of_image
    write_u32(buf, o + 60, static_cast<std::uint32_t>(opt_end));  // size_of_headers
    write_u32(buf, o + 64, 0);                            // check_sum
    write_u16(buf, o + 68, 3);                            // subsystem (GUI)
    write_u16(buf, o + 70, 0);                            // dll_characteristics

    if (pe32_plus) {
        write_u64_le(buf, o + 72, 0x100000);              // stack_reserve
        write_u64_le(buf, o + 80, 0x1000);                // stack_commit
        write_u64_le(buf, o + 88, 0x100000);              // heap_reserve
        write_u64_le(buf, o + 96, 0x1000);                // heap_commit
        write_u32(buf, o + 104, 0);                       // loader_flags
        write_u32(buf, o + 108, 0);                       // number_of_rva_and_sizes
    } else {
        write_u32(buf, o + 72, 0x100000);                 // stack_reserve
        write_u32(buf, o + 76, 0x1000);                   // stack_commit
        write_u32(buf, o + 80, 0x100000);                 // heap_reserve
        write_u32(buf, o + 84, 0x1000);                   // heap_commit
        write_u32(buf, o + 88, 0);                        // loader_flags
        write_u32(buf, o + 92, 0);                        // number_of_rva_and_sizes
    }

    // ---- Section table ----
    std::size_t sec = section_table_start;
    for (std::uint16_t i = 0; i < num_sections; ++i) {
        std::string name = ".text";
        for (std::size_t c = 0; c < name.size(); ++c) {
            buf[sec + c] = static_cast<std::byte>(name[c]);
        }
        write_u32(buf, sec + 8, 0x1000);                            // virtual_size
        write_u32(buf, sec + 12,
                  0x1000 + static_cast<std::uint32_t>(i) * 0x1000);  // virtual_address
        write_u32(buf, sec + 16, 0x200);                             // raw_data_size
        write_u32(buf, sec + 20,
                  static_cast<std::uint32_t>(
                      section_table_start +
                      static_cast<std::size_t>(num_sections) * kSectionHeaderSize +
                      static_cast<std::size_t>(i) * 0x200));  // raw_data_offset
        write_u32(buf, sec + 36,
                  static_cast<std::uint32_t>(
                      static_cast<std::uint32_t>(
                          nova::loader::PESectionCharacteristics::CNT_CODE) |
                      static_cast<std::uint32_t>(
                          nova::loader::PESectionCharacteristics::MEM_EXECUTE) |
                      static_cast<std::uint32_t>(
                          nova::loader::PESectionCharacteristics::MEM_READ)));
        sec += kSectionHeaderSize;
    }

    return buf;
}

}  // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

using nova::loader::PEMachine;
using nova::loader::parse;
using nova::loader::PeErrorCode;
using nova::loader::PeResult;

namespace {

TEST(PeParserTest, ValidPE32) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    auto result = parse(buf);
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_EQ(result.pe.nt_headers.coff_file_header.machine, PEMachine::I386);
    EXPECT_FALSE(result.pe.nt_headers.is_pe32_plus);
    EXPECT_EQ(result.pe.nt_headers.optional_header.magic,
              nova::loader::PEOptionalMagic::PE32);
    EXPECT_EQ(result.pe.sections.size(), 1u);
    EXPECT_EQ(result.pe.sections[0].name, ".text");
    EXPECT_EQ(result.pe.sections[0].virtual_address, 0x1000u);
}

TEST(PeParserTest, ValidPE32Plus) {
    auto buf = make_pe(nova::loader::PEMachine::AMD64, 2, true);
    auto result = parse(buf);
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_EQ(result.pe.nt_headers.coff_file_header.machine, PEMachine::AMD64);
    EXPECT_TRUE(result.pe.nt_headers.is_pe32_plus);
    EXPECT_EQ(result.pe.nt_headers.optional_header.magic,
              nova::loader::PEOptionalMagic::PE32Plus);
    EXPECT_EQ(result.pe.sections.size(), 2u);
}

TEST(PeParserTest, InvalidDosSignature) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    buf[0] = std::byte{0x00};  // clobber 'M'
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::InvalidDosSignature);
}

TEST(PeParserTest, InvalidNtSignature) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    buf[0x80] = std::byte{0x00};  // clobber 'P' of "PE\0\0"
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::InvalidNtSignature);
}

TEST(PeParserTest, TruncatedDosHeader) {
    std::vector<std::byte> buf(32, std::byte{0});
    buf[0] = std::byte{0x4d};
    buf[1] = std::byte{0x5a};
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::TruncatedDosHeader);
}

TEST(PeParserTest, TruncatedNtHeaders) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    buf.resize(0x84 + 1);
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::TruncatedNtHeaders);
}

TEST(PeParserTest, InvalidOptionalHeaderMagic) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    write_u32(buf, 0x98, 0xffff);
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::InvalidOptionalHeaderMagic);
}

TEST(PeParserTest, InvalidSectionTableOffset) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    write_u32(buf, 0x3c, static_cast<std::uint32_t>(buf.size() + 10));
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::InvalidSectionTableOffset);
}

TEST(PeParserTest, SectionTableBeyondEof) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    // Keep the optional header (ends at 0xF8) but cut off the section table
    // (which needs bytes up to 0x120).
    buf.resize(0x100);
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::SectionTableBeyondEof);
}

TEST(PeParserTest, IntegerOverflowAttempt) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    write_u16(buf, 0x86, 0xffff);
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::SectionTableBeyondEof);
}

TEST(PeParserTest, InvalidSectionRawDataBounds) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    write_u32(buf, 0x10c, static_cast<std::uint32_t>(buf.size() + 100));
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::InvalidSectionRawDataBounds);
}

TEST(PeParserTest, RVAConversion) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    auto result = parse(buf);
    ASSERT_TRUE(result.ok);
    auto offset = nova::loader::rva_to_offset(result.pe, 0x1000);
    ASSERT_TRUE(offset.has_value());
    EXPECT_GT(*offset, 0u);
    EXPECT_TRUE(nova::loader::rva_in_section(result.pe.sections, 0x1000));
    EXPECT_FALSE(nova::loader::rva_in_section(result.pe.sections, 0x9000));
}

TEST(PeParserTest, RVAOutsideAllSections) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    auto result = parse(buf);
    ASSERT_TRUE(result.ok);
    EXPECT_FALSE(nova::loader::rva_in_section(result.pe.sections, 0x5000));
    EXPECT_FALSE(nova::loader::rva_to_offset(result.pe, 0x5000).has_value());
}

TEST(PeParserTest, HeaderRVAConversion) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    auto result = parse(buf);
    ASSERT_TRUE(result.ok);
    EXPECT_FALSE(nova::loader::rva_to_offset(result.pe, 0).has_value());
    EXPECT_FALSE(nova::loader::rva_in_section(result.pe.sections, 0));
}

TEST(PeParserTest, EmptyInput) {
    std::vector<std::byte> buf;
    auto result = parse(buf);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, PeErrorCode::EmptyInput);
}

TEST(PeParserTest, UnknownMachineType) {
    auto buf = make_pe(static_cast<nova::loader::PEMachine>(0x9999), 1, false);
    auto result = parse(buf);
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_EQ(result.pe.nt_headers.coff_file_header.machine,
              static_cast<nova::loader::PEMachine>(0x9999));
}

TEST(PeParserTest, MalformedSectionNames) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    for (int i = 0; i < 8; ++i) {
        buf[0xF8 + i] = static_cast<std::byte>('A' + i);
    }
    auto result = parse(buf);
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_EQ(result.pe.sections[0].name, "ABCDEFGH");
}

TEST(PeParserTest, FindSectionByName) {
    auto buf = make_pe(nova::loader::PEMachine::I386, 1, false);
    auto result = parse(buf);
    ASSERT_TRUE(result.ok);
    auto idx = nova::loader::find_section_by_name(result.pe.sections, ".text");
    ASSERT_TRUE(idx.has_value());
    EXPECT_EQ(*idx, 0u);
    EXPECT_FALSE(
        nova::loader::find_section_by_name(result.pe.sections, ".nonexistent")
            .has_value());
}

TEST(PeParserTest, FindSectionByNameEmpty) {
    std::vector<nova::loader::SectionHeader> empty;
    EXPECT_FALSE(nova::loader::find_section_by_name(empty, ".text").has_value());
}

}  // namespace
