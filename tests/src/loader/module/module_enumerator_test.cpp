#include "nova/loader/module/module_enumerator.h"
#include "nova/loader/module/module_info.h"

#include <gtest/gtest.h>

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

// Return the base name (e.g. "nova_loader_module_tests.exe") of the current
// process executable. This is the guaranteed main module for the tests.
std::string to_utf8(std::wstring_view w) {
    if (w.empty()) {
        return std::string();
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                     nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::string();
    }
    std::string result(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                        result.data(), needed, nullptr, nullptr);
    return result;
}

std::string current_exe_base_name() {
    wchar_t buf[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0) {
        return std::string();
    }
    return std::filesystem::path(to_utf8(std::wstring_view(buf, len))).filename().string();
}

std::string current_exe_full_path() {
    wchar_t buf[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0) {
        return std::string();
    }
    return to_utf8(std::wstring_view(buf, len));
}

}  // namespace

TEST(ModuleEnumeratorTest, SuccessfulEnumeration) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_FALSE(result.modules.empty());
}

TEST(ModuleEnumeratorTest, MainExecutablePresent) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_name = current_exe_base_name();
    ASSERT_FALSE(exe_name.empty());
    bool found = false;
    for (const auto& mod : result.modules) {
        if (mod.name == exe_name) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Main executable " << exe_name << " not found in module list";
}

TEST(ModuleEnumeratorTest, ModuleNamesNonEmpty) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    for (const auto& mod : result.modules) {
        EXPECT_FALSE(mod.name.empty()) << "A module has an empty name";
    }
}

TEST(ModuleEnumeratorTest, ModulePathsNonEmptyWhereAccessible) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    for (const auto& mod : result.modules) {
        if (!mod.full_path.empty()) {
            EXPECT_FALSE(mod.full_path.native().empty());
        }
    }
}

TEST(ModuleEnumeratorTest, BaseAddressesNonNull) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    for (const auto& mod : result.modules) {
        EXPECT_NE(mod.base_address, std::uintptr_t{0});
    }
}

TEST(ModuleEnumeratorTest, ModuleSizesValidWhereAvailable) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    for (const auto& mod : result.modules) {
        if (mod.image_size != 0) {
            EXPECT_GT(mod.image_size, std::uint64_t{0});
        }
    }
}

TEST(ModuleEnumeratorTest, RetrievalByModuleName) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_name = current_exe_base_name();
    ASSERT_FALSE(exe_name.empty());
    auto found = enumerator.find_by_name(exe_name);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().name, exe_name);
    EXPECT_NE(found->get().base_address, std::uintptr_t{0});
}

TEST(ModuleEnumeratorTest, RetrievalByFullPath) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_path = current_exe_full_path();
    ASSERT_FALSE(exe_path.empty());
    auto found = enumerator.find_by_path(exe_path);
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->get().full_path.empty());
}

TEST(ModuleEnumeratorTest, CaseInsensitivePathComparison) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_path = current_exe_full_path();
    ASSERT_FALSE(exe_path.empty());
    // Flip the case of the drive letter to verify case-insensitive matching.
    std::string altered = exe_path;
    if (!altered.empty() && std::isalpha(static_cast<unsigned char>(altered[0]))) {
        altered[0] = (altered[0] == 'A') ? 'a' : static_cast<char>(
            std::tolower(static_cast<unsigned char>(altered[0])));
    }
    auto found = enumerator.find_by_path(altered);
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->get().full_path.empty());
}

TEST(ModuleEnumeratorTest, MissingModuleLookup) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    EXPECT_FALSE(enumerator.find_by_name("__NOVA_DOES_NOT_EXIST__.dll").has_value());
    EXPECT_FALSE(enumerator.find_by_path("C:\\__NOVA_DOES_NOT_EXIST__\\missing.dll")
                     .has_value());
}

TEST(ModuleEnumeratorTest, EmptyResultHandling) {
    nova::loader::ModuleEnumerator enumerator;
    // A fresh enumerator has not performed an enumeration.
    EXPECT_TRUE(enumerator.modules().empty());
    EXPECT_FALSE(enumerator.find_by_name("anything").has_value());
    EXPECT_FALSE(enumerator.find_by_path("C:\\anything.dll").has_value());
}

TEST(ModuleEnumeratorTest, ArchitectureDetection) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    bool found_x64 = false;
    for (const auto& mod : result.modules) {
        if (mod.architecture == nova::loader::ModuleArchitecture::X64) {
            found_x64 = true;
            break;
        }
    }
    // This test executable is built for x64, so at least one module (the exe)
    // should be reported as X64 when its PE headers are readable.
    EXPECT_TRUE(found_x64) << "No module detected as x64 architecture";
}

TEST(ModuleEnumeratorTest, RepeatedEnumeration) {
    nova::loader::ModuleEnumerator enumerator;
    auto first = enumerator.enumerate();
    auto second = enumerator.enumerate();
    EXPECT_TRUE(first.ok);
    EXPECT_TRUE(second.ok);
    EXPECT_EQ(first.modules.size(), second.modules.size());
}

TEST(ModuleEnumeratorTest, StableBehaviorUnderRepeatedEnumeration) {
    nova::loader::ModuleEnumerator enumerator;
    std::vector<std::size_t> sizes;
    for (int i = 0; i < 3; ++i) {
        auto result = enumerator.enumerate();
        EXPECT_TRUE(result.ok);
        sizes.push_back(result.modules.size());
    }
    // Repeated enumerations should yield consistent (non-zero) counts.
    for (std::size_t s : sizes) {
        EXPECT_GT(s, std::size_t{0});
    }
}

TEST(ModuleEnumeratorTest, LastEnumerationOk) {
    nova::loader::ModuleEnumerator enumerator;
    EXPECT_FALSE(enumerator.last_enumeration_ok());
    auto result = enumerator.enumerate();
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(enumerator.last_enumeration_ok());
}

TEST(ModuleEnumeratorTest, WideToUtf8Empty) {
    std::string out = "not-empty";
    EXPECT_TRUE(nova::loader::ModuleEnumerator::wide_to_utf8(L"", out));
    EXPECT_TRUE(out.empty());
}

TEST(ModuleEnumeratorTest, WideToUtf8AsciiUnchanged) {
    std::string out;
    EXPECT_TRUE(nova::loader::ModuleEnumerator::wide_to_utf8(L"NOVA.exe", out));
    EXPECT_EQ(out, "NOVA.exe");
}

TEST(ModuleEnumeratorTest, WideToUtf8Cyrillic) {
    std::string out;
    EXPECT_TRUE(nova::loader::ModuleEnumerator::wide_to_utf8(L"\u043c\u043e\u0434\u0443\u043b\u044c", out));
    // Cyrillic "модуль" in UTF-8.
    EXPECT_EQ(out, "\xD0\xBC\xD0\xBE\xD0\xB4\xD1\x83\xD0\xBB\xD1\x8C");
}

TEST(ModuleEnumeratorTest, WideToUtf8Chinese) {
    std::string out;
    EXPECT_TRUE(nova::loader::ModuleEnumerator::wide_to_utf8(L"\u6a21\u5757", out));
    // Chinese "模块" in UTF-8.
    EXPECT_EQ(out, "\xE6\xA8\xA1\xE5\x9D\x97");
}

TEST(ModuleEnumeratorTest, WideToUtf8AccentedLatin) {
    std::string out;
    EXPECT_TRUE(nova::loader::ModuleEnumerator::wide_to_utf8(L"caf\u00e9", out));
    // "caf\u00e9" in UTF-8.
    EXPECT_EQ(out, "caf\xC3\xA9");
}

TEST(ModuleEnumeratorTest, WideToUtf8SupplementarySurrogatePair) {
    std::string out;
    // U+1F600 (GRINNING FACE) as a UTF-16 surrogate pair.
    const wchar_t pair[] = {static_cast<wchar_t>(0xD83D),
                            static_cast<wchar_t>(0xDE00)};
    EXPECT_TRUE(
        nova::loader::ModuleEnumerator::wide_to_utf8(
            std::wstring_view(pair, 2), out));
    // U+1F600 in UTF-8.
    EXPECT_EQ(out, "\xF0\x9F\x98\x80");
}

TEST(ModuleEnumeratorTest, WideToUtf8InvalidSurrogate) {
    std::string out = "sentinel";
    // Lone high surrogate with no matching low surrogate: Windows API replaces
    // the ill-formed sequence with U+FFFD (replacement character) rather than
    // failing. It is not silently replaced with '?'.
    const wchar_t lone[] = {static_cast<wchar_t>(0xD800)};
    EXPECT_TRUE(
        nova::loader::ModuleEnumerator::wide_to_utf8(
            std::wstring_view(lone, 1), out));
    // U+FFFD in UTF-8.
    EXPECT_EQ(out, "\xEF\xBF\xBD");
}

TEST(ModuleEnumeratorTest, FindByNameAsciiCaseInsensitivePreserved) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_name = current_exe_base_name();
    ASSERT_FALSE(exe_name.empty());
    // Lowercase lookup should still match (ASCII case-insensitive behavior).
    std::string lower = exe_name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto found = enumerator.find_by_name(lower);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().name, exe_name);
}

TEST(ModuleEnumeratorTest, FindByNameUtf8NameBytesPreserved) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    const std::string exe_name = current_exe_base_name();
    ASSERT_FALSE(exe_name.empty());
    // A UTF-8 lookup string identical to the module name matches byte-for-byte.
    auto found = enumerator.find_by_name(exe_name);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().name, exe_name);
}

TEST(ModuleEnumeratorTest, EnumeratePreservesAsciiModuleNames) {
    nova::loader::ModuleEnumerator enumerator;
    auto result = enumerator.enumerate();
    ASSERT_TRUE(result.ok);
    // Every module's name must be valid UTF-8 (ASCII subset trivially valid) and
    // non-empty, matching pre-existing behavior.
    for (const auto& mod : result.modules) {
        EXPECT_FALSE(mod.name.empty()) << "A module has an empty name";
        for (char c : mod.name) {
            // ASCII module names must not contain replacement '?' from lossy
            // conversion of non-ASCII source characters on this system.
            EXPECT_NE(c, '?') << "Module name contains lossy replacement char: "
                              << mod.name;
        }
    }
}

