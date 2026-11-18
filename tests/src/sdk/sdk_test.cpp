#include "nova/sdk/sdk.h"
#include "nova/sdk/sdk_config.h"
#include "nova/sdk/sdk_error.h"
#include "nova/sdk/sdk_result.h"
#include "nova/sdk/sdk_version.h"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>

#include <gtest/gtest.h>

namespace {

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

TEST(SdkTest, Construction) {
    nova::sdk::Sdk sdk;
    EXPECT_FALSE(sdk.is_initialized());
}

TEST(SdkTest, DefaultConfigurationValid) {
    nova::sdk::SdkConfig config;
    EXPECT_FALSE(config.name.empty());
    EXPECT_FALSE(config.version.empty());
    EXPECT_TRUE(config.enable_core);
    EXPECT_TRUE(config.enable_loader);
    EXPECT_TRUE(config.enable_runtime);
}

TEST(SdkTest, VersionInformation) {
    auto v = nova::sdk::Sdk::version();
    EXPECT_GE(v.major, 0u);
    EXPECT_GE(v.minor, 0u);
    auto s = nova::sdk::sdk_version_string();
    EXPECT_FALSE(s.empty());
}

TEST(SdkTest, Initialization) {
    nova::sdk::Sdk sdk;
    auto r = sdk.initialize();
    EXPECT_TRUE(r.ok) << r.error.message;
    EXPECT_TRUE(sdk.is_initialized());
    sdk.shutdown();
    EXPECT_FALSE(sdk.is_initialized());
}

TEST(SdkTest, RepeatedInitialization) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    auto r = sdk.initialize();
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::sdk::SdkErrorCode::AlreadyInitialized);
    sdk.shutdown();
}

TEST(SdkTest, Shutdown) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    sdk.shutdown();
    EXPECT_FALSE(sdk.is_initialized());
}

TEST(SdkTest, RepeatedShutdown) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    sdk.shutdown();
    sdk.shutdown(); // must be safe
    EXPECT_FALSE(sdk.is_initialized());
}

TEST(SdkTest, ShutdownWithoutInitialization) {
    nova::sdk::Sdk sdk;
    sdk.shutdown(); // must be safe
    EXPECT_FALSE(sdk.is_initialized());
}

TEST(SdkTest, InvalidConfiguration) {
    nova::sdk::SdkConfig config;
    config.name = "";
    nova::sdk::Sdk sdk;
    auto r = sdk.initialize(std::move(config));
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::sdk::SdkErrorCode::InvalidConfiguration);
}

TEST(SdkTest, CoreAccess) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    EXPECT_TRUE(sdk.core_enabled());
    auto& core = sdk.core();
    (void)core; // Core is accessible
    sdk.shutdown();
}

TEST(SdkTest, LoaderAccess) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    EXPECT_TRUE(sdk.loader_enabled());
    auto& loader = sdk.loader();
    (void)loader; // Loader is accessible
    sdk.shutdown();
}

TEST(SdkTest, RuntimeAccess) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    EXPECT_TRUE(sdk.runtime_enabled());
    auto& runtime = sdk.runtime();
    (void)runtime; // Runtime is accessible
    sdk.shutdown();
}

TEST(SdkTest, DisabledLoader) {
    nova::sdk::SdkConfig config;
    config.enable_loader = false;
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize(std::move(config)).ok);
    EXPECT_FALSE(sdk.loader_enabled());
    sdk.shutdown();
}

TEST(SdkTest, DisabledRuntime) {
    nova::sdk::SdkConfig config;
    config.enable_runtime = false;
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize(std::move(config)).ok);
    EXPECT_FALSE(sdk.runtime_enabled());
    sdk.shutdown();
}

TEST(SdkTest, ConcurrentStateInspection) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);
    bool ok = true;
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&sdk, &ok]() {
            for (int j = 0; j < 50; ++j) {
                if (!sdk.is_initialized()) {
                    ok = false;
                }
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_TRUE(ok);
    sdk.shutdown();
}

TEST(SdkTest, TryCoreBeforeInitializationIsNull) {
    nova::sdk::Sdk sdk;
    EXPECT_FALSE(sdk.is_initialized());
    EXPECT_EQ(sdk.try_core(), nullptr);
}

TEST(SdkTest, TryLoaderBeforeInitializationIsNull) {
    nova::sdk::Sdk sdk;
    EXPECT_FALSE(sdk.is_initialized());
    EXPECT_EQ(sdk.try_loader(), nullptr);
}

TEST(SdkTest, TryRuntimeBeforeInitializationIsNull) {
    nova::sdk::Sdk sdk;
    EXPECT_FALSE(sdk.is_initialized());
    EXPECT_EQ(sdk.try_runtime(), nullptr);
}

TEST(SdkTest, TryAccessDisabledCoreIsNull) {
    nova::sdk::SdkConfig config;
    config.enable_core = false;
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize(std::move(config)).ok);
    EXPECT_FALSE(sdk.core_enabled());
    EXPECT_EQ(sdk.try_core(), nullptr);
    sdk.shutdown();
}

TEST(SdkTest, TryAccessDisabledLoaderIsNull) {
    nova::sdk::SdkConfig config;
    config.enable_loader = false;
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize(std::move(config)).ok);
    EXPECT_FALSE(sdk.loader_enabled());
    EXPECT_EQ(sdk.try_loader(), nullptr);
    sdk.shutdown();
}

TEST(SdkTest, TryAccessDisabledRuntimeIsNull) {
    nova::sdk::SdkConfig config;
    config.enable_runtime = false;
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize(std::move(config)).ok);
    EXPECT_FALSE(sdk.runtime_enabled());
    EXPECT_EQ(sdk.try_runtime(), nullptr);
    sdk.shutdown();
}

TEST(SdkTest, TryAccessEnabledSubsystemsNonNull) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);

    EXPECT_NE(sdk.try_core(), nullptr);
    EXPECT_NE(sdk.try_loader(), nullptr);
    EXPECT_NE(sdk.try_runtime(), nullptr);

    EXPECT_EQ(sdk.try_core(), &sdk.core());
    EXPECT_EQ(sdk.try_loader(), &sdk.loader());
    EXPECT_EQ(sdk.try_runtime(), &sdk.runtime());

    sdk.shutdown();
}

TEST(SdkTest, TryAccessAfterShutdownIsNonNull) {
    nova::sdk::Sdk sdk;
    EXPECT_TRUE(sdk.initialize().ok);

    EXPECT_NE(sdk.try_core(), nullptr);
    EXPECT_NE(sdk.try_loader(), nullptr);
    EXPECT_NE(sdk.try_runtime(), nullptr);

    sdk.shutdown();
    EXPECT_FALSE(sdk.is_initialized());

    // Sdk::shutdown() shuts down the subsystems but does not reset the
    // owning unique_ptrs, so the subsystem objects remain alive and the
    // try_*() accessors still return non-null pointers.
    EXPECT_NE(sdk.try_core(), nullptr);
    EXPECT_NE(sdk.try_loader(), nullptr);
    EXPECT_NE(sdk.try_runtime(), nullptr);
}
