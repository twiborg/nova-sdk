#include "nova/loader/loader.h"
#include "nova/loader/loader_config.h"
#include "nova/loader/loader_result.h"
#include "nova/loader/loader_version.h"

#include <gtest/gtest.h>

#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace {

// Return the full path of the current test executable (a valid PE file).
std::string current_exe_path() {
    wchar_t buf[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len == 0) {
        return std::string();
    }
    int needed = WideCharToMultiByte(CP_UTF8, 0, buf, len, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::string();
    }
    std::string result(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, buf, len, result.data(), needed, nullptr, nullptr);
    return result;
}

}  // namespace

TEST(LoaderTest, Construction) {
    nova::loader::Loader loader;
    EXPECT_FALSE(loader.is_initialized());
}

TEST(LoaderTest, DefaultConfiguration) {
    nova::loader::LoaderConfig config;
    EXPECT_TRUE(config.enable_pe_parsing);
    EXPECT_TRUE(config.enable_module_enumeration);
}

TEST(LoaderTest, Initialization) {
    nova::loader::Loader loader;
    auto result = loader.initialize();
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(loader.is_initialized());
    loader.shutdown();
    EXPECT_FALSE(loader.is_initialized());
}

TEST(LoaderTest, InitializationWithConfig) {
    nova::loader::LoaderConfig config;
    config.enable_pe_parsing = false;
    config.enable_module_enumeration = false;

    nova::loader::Loader loader;
    auto result = loader.initialize(config);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(loader.is_initialized());
    EXPECT_FALSE(loader.pe_parsing_enabled());
    EXPECT_FALSE(loader.module_enumeration_enabled());
    loader.shutdown();
}

TEST(LoaderTest, RepeatedInitialization) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);
    auto second = loader.initialize();
    EXPECT_FALSE(second.ok);
    EXPECT_EQ(second.error.code, nova::loader::LoaderErrorCode::AlreadyInitialized);
    loader.shutdown();
}

TEST(LoaderTest, Shutdown) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);
    loader.shutdown();
    EXPECT_FALSE(loader.is_initialized());
}

TEST(LoaderTest, RepeatedShutdown) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);
    loader.shutdown();
    // Shutting down again must be safe.
    loader.shutdown();
    EXPECT_FALSE(loader.is_initialized());
}

TEST(LoaderTest, StateInspection) {
    nova::loader::Loader loader;
    EXPECT_FALSE(loader.is_initialized());
    EXPECT_TRUE(loader.initialize().ok);
    EXPECT_TRUE(loader.is_initialized());
    loader.shutdown();
    EXPECT_FALSE(loader.is_initialized());
}

TEST(LoaderTest, Version) {
    auto v = nova::loader::Loader::version();
    EXPECT_GE(v.major, 0u);
    EXPECT_GE(v.minor, 0u);
    auto s = nova::loader::loader_version_string();
    EXPECT_FALSE(s.empty());
}

TEST(LoaderTest, PeParserAccess) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);

    std::string path = current_exe_path();
    ASSERT_FALSE(path.empty());
    auto result = loader.parse_pe_file(path);
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_EQ(result.pe.nt_headers.coff_file_header.machine,
              nova::loader::PEMachine::AMD64);

    loader.shutdown();
}

TEST(LoaderTest, PeParserAccessWhenDisabled) {
    nova::loader::LoaderConfig config;
    config.enable_pe_parsing = false;

    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize(config).ok);

    std::string path = current_exe_path();
    ASSERT_FALSE(path.empty());
    auto result = loader.parse_pe_file(path);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, nova::loader::PeErrorCode::Unknown);

    loader.shutdown();
}

TEST(LoaderTest, ModuleEnumerationAccess) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);

    auto result = loader.enumerate_modules();
    EXPECT_TRUE(result.ok) << result.error.message;
    EXPECT_FALSE(result.modules.empty());

    loader.shutdown();
}

TEST(LoaderTest, ModuleEnumerationWhenDisabled) {
    nova::loader::LoaderConfig config;
    config.enable_module_enumeration = false;

    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize(config).ok);

    auto result = loader.enumerate_modules();
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, nova::loader::ModuleErrorCode::Unknown);

    loader.shutdown();
}

TEST(LoaderTest, ErrorPropagationNotInitialized) {
    nova::loader::Loader loader;  // not initialized

    auto pe = loader.parse_pe_file(current_exe_path());
    EXPECT_FALSE(pe.ok);
    EXPECT_EQ(pe.error.code, nova::loader::PeErrorCode::Unknown);

    auto me = loader.enumerate_modules();
    EXPECT_FALSE(me.ok);
    EXPECT_EQ(me.error.code, nova::loader::ModuleErrorCode::Unknown);
}

TEST(LoaderTest, FindModuleByName) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);

    auto enum_result = loader.enumerate_modules();
    ASSERT_TRUE(enum_result.ok);
    ASSERT_FALSE(enum_result.modules.empty());

    const std::string& name = enum_result.modules.front().name;
    auto found = loader.find_module_by_name(name);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get().name, name);

    loader.shutdown();
}

TEST(LoaderTest, FindModuleMissing) {
    nova::loader::Loader loader;
    EXPECT_TRUE(loader.initialize().ok);

    EXPECT_FALSE(
        loader.find_module_by_name("__NOVA_DOES_NOT_EXIST__.dll").has_value());

    loader.shutdown();
}

TEST(LoaderTest, ThreadSafeLifecycle) {
    nova::loader::Loader loader;
    bool ok = true;
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&loader, &ok]() {
            for (int j = 0; j < 50; ++j) {
                auto r = loader.initialize();
                if (!r.ok) {
                    // AlreadyInitialized is acceptable under concurrency.
                    if (r.error.code !=
                        nova::loader::LoaderErrorCode::AlreadyInitialized) {
                        ok = false;
                    }
                }
                loader.shutdown();
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_TRUE(ok);
}
