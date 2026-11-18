#include "nova/sdk/application.h"
#include "nova/sdk/sdk_config.h"
#include "nova/sdk/sdk_error.h"

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

TEST(ApplicationTest, Construction) {
    nova::sdk::Application app;
    EXPECT_FALSE(app.is_initialized());
    EXPECT_FALSE(app.is_running());
}

TEST(ApplicationTest, DefaultConfigurationValid) {
    nova::sdk::SdkConfig config;
    EXPECT_FALSE(config.name.empty());
    EXPECT_FALSE(config.version.empty());
    EXPECT_TRUE(config.enable_core);
    EXPECT_TRUE(config.enable_loader);
    EXPECT_TRUE(config.enable_runtime);
}

TEST(ApplicationTest, Initialization) {
    nova::sdk::Application app;
    auto r = app.initialize();
    EXPECT_TRUE(r.ok) << r.error.message;
    EXPECT_TRUE(app.is_initialized());
    EXPECT_FALSE(app.is_running());
    app.shutdown();
}

TEST(ApplicationTest, RepeatedInitialization) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    auto r = app.initialize();
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::sdk::SdkErrorCode::AlreadyInitialized);
    app.shutdown();
}

TEST(ApplicationTest, StartAndStop) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    EXPECT_TRUE(app.start().ok);
    EXPECT_TRUE(app.is_running());
    app.stop();
    EXPECT_FALSE(app.is_running());
    app.shutdown();
}

TEST(ApplicationTest, StartWithoutInitialization) {
    nova::sdk::Application app;
    auto r = app.start();
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::sdk::SdkErrorCode::NotInitialized);
}

TEST(ApplicationTest, RepeatedShutdown) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    app.shutdown();
    app.shutdown(); // must be safe
    EXPECT_FALSE(app.is_initialized());
}

TEST(ApplicationTest, ShutdownWithoutInitialization) {
    nova::sdk::Application app;
    app.shutdown(); // must be safe
    EXPECT_FALSE(app.is_initialized());
}

TEST(ApplicationTest, InvalidConfiguration) {
    nova::sdk::SdkConfig config;
    config.name = "";
    nova::sdk::Application app;
    auto r = app.initialize(std::move(config));
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::sdk::SdkErrorCode::InvalidConfiguration);
}

TEST(ApplicationTest, ApplicationContext) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    auto ctx = app.context();
    EXPECT_FALSE(ctx.name().empty());
    EXPECT_FALSE(ctx.version().empty());
    EXPECT_GT(ctx.id(), 0u);
    EXPECT_FALSE(ctx.sdk_version().empty());
    app.shutdown();
}

TEST(ApplicationTest, RuntimeAccess) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    EXPECT_TRUE(app.start().ok);

    auto sdk = app.sdk();
    ASSERT_NE(sdk, nullptr);
    auto& runtime = sdk->runtime();
    (void)runtime; // Runtime is accessible

    app.shutdown();
}

TEST(ApplicationTest, ConcurrentStateInspection) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    bool ok = true;
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&app, &ok]() {
            for (int j = 0; j < 50; ++j) {
                if (!app.is_initialized()) {
                    ok = false;
                }
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_TRUE(ok);
    app.shutdown();
}

TEST(ApplicationTest, SdkHandleBeforeInitializationIsEmpty) {
    nova::sdk::Application app;
    EXPECT_FALSE(app.is_initialized());
    auto handle = app.sdk();
    EXPECT_EQ(handle, nullptr);
}

TEST(ApplicationTest, SdkHandleAfterShutdownIsEmpty) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    app.shutdown();
    EXPECT_FALSE(app.is_initialized());
    auto handle = app.sdk();
    EXPECT_EQ(handle, nullptr);
}

TEST(ApplicationTest, RetainedSdkHandleOutlivesShutdown) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);

    // Acquire a shared handle while the Application owns the Sdk.
    auto retained = app.sdk();
    ASSERT_NE(retained, nullptr);
    EXPECT_TRUE(retained->is_initialized());

    // Application releases its own ownership; the handle keeps the Sdk alive.
    app.shutdown();
    EXPECT_FALSE(app.is_initialized());
    EXPECT_EQ(app.sdk(), nullptr);

    // The retained handle remains valid; Sdk::shutdown() was never called by
    // Application, so the initialization flag is unchanged.
    EXPECT_NE(retained, nullptr);
    EXPECT_TRUE(retained->is_initialized());
}

TEST(ApplicationTest, ConcurrentSdkAccessAndShutdown) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);

    std::atomic<bool> stop_requested{false};
    std::barrier sync_point(9);  // 8 reader threads + 1 shutdown thread

    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&app, &stop_requested, &sync_point]() {
            sync_point.arrive_and_wait();
            while (!stop_requested.load(std::memory_order_acquire)) {
                auto handle = app.sdk();
                // Each handle is either valid (pre-shutdown) or empty
                // (post-shutdown); never a dangling reference.
                if (handle) {
                    EXPECT_TRUE(handle->is_initialized());
                }
            }
        });
    }

    std::thread shutdown_thread([&app, &stop_requested, &sync_point]() {
        sync_point.arrive_and_wait();
        app.shutdown();
        stop_requested.store(true, std::memory_order_release);
    });

    for (auto& t : readers) {
        t.join();
    }
    shutdown_thread.join();

    EXPECT_FALSE(app.is_initialized());
}

TEST(ApplicationTest, SdkHandleSubsystemAccessWhileInitialized) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    EXPECT_TRUE(app.start().ok);

    auto sdk = app.sdk();
    ASSERT_NE(sdk, nullptr);
    auto& runtime = sdk->runtime();
    (void)runtime; // Runtime is accessible

    app.stop();
    app.shutdown();
}

TEST(ApplicationTest, ConfigBeforeInitializationIsDefault) {
    nova::sdk::Application app;
    EXPECT_FALSE(app.is_initialized());
    auto cfg = app.config();
    EXPECT_EQ(cfg.name, "nova-app");
    EXPECT_EQ(cfg.version, "1.0.0");
    EXPECT_TRUE(cfg.enable_core);
    EXPECT_TRUE(cfg.enable_loader);
    EXPECT_TRUE(cfg.enable_runtime);
}

TEST(ApplicationTest, ConfigWhileInitialized) {
    nova::sdk::Application app;
    nova::sdk::SdkConfig config;
    config.name = "test-app";
    config.version = "2.0.0";
    config.enable_loader = false;

    EXPECT_TRUE(app.initialize(std::move(config)).ok);

    auto cfg = app.config();
    EXPECT_EQ(cfg.name, "test-app");
    EXPECT_EQ(cfg.version, "2.0.0");
    EXPECT_TRUE(cfg.enable_core);
    EXPECT_FALSE(cfg.enable_loader);
    EXPECT_TRUE(cfg.enable_runtime);

    app.shutdown();
}

TEST(ApplicationTest, ConfigAfterShutdownIsDefault) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);
    app.shutdown();

    EXPECT_FALSE(app.is_initialized());
    auto cfg = app.config();
    EXPECT_EQ(cfg.name, "nova-app");
    EXPECT_EQ(cfg.version, "1.0.0");
    EXPECT_TRUE(cfg.enable_core);
    EXPECT_TRUE(cfg.enable_loader);
    EXPECT_TRUE(cfg.enable_runtime);
}

TEST(ApplicationTest, ConcurrentConfigReadsAndShutdown) {
    nova::sdk::Application app;
    EXPECT_TRUE(app.initialize().ok);

    std::atomic<bool> stop_requested{false};
    std::barrier sync_point(9);  // 8 reader threads + 1 shutdown thread

    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&app, &stop_requested, &sync_point]() {
            sync_point.arrive_and_wait();
            while (!stop_requested.load(std::memory_order_acquire)) {
                // Each config() call returns a self-contained copy that is safe
                // to use regardless of concurrent shutdown.
                auto cfg = app.config();
                EXPECT_FALSE(cfg.name.empty());
            }
        });
    }

    std::thread shutdown_thread([&app, &stop_requested, &sync_point]() {
        sync_point.arrive_and_wait();
        app.shutdown();
        stop_requested.store(true, std::memory_order_release);
    });

    for (auto& t : readers) {
        t.join();
    }
    shutdown_thread.join();

    EXPECT_FALSE(app.is_initialized());
    auto cfg = app.config();
    EXPECT_EQ(cfg.name, "nova-app");
}

TEST(ApplicationTest, RollbackRuntimeFailureAfterCoreAndLoader) {
    nova::sdk::Application app;
    nova::sdk::SdkConfig config;
    config.name = "rollback-app";
    // InvalidConfiguration in Runtime::initialize() -> RuntimeFailure after
    // Core and Loader were already initialized.
    config.runtime_config.max_concurrent_tasks = 0;

    auto result = app.initialize(std::move(config));
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error.code, nova::sdk::SdkErrorCode::RuntimeFailure);
    EXPECT_FALSE(app.is_initialized());

    // Rollback restored a clean, retryable state: Core and Loader were torn
    // down and are reusable.
    config.name = "rollback-app";
    config.runtime_config.max_concurrent_tasks = 4;
    EXPECT_TRUE(app.initialize(std::move(config)).ok);
    EXPECT_TRUE(app.is_initialized());
    app.shutdown();
}

TEST(ApplicationTest, RollbackDisabledRuntimeSkipped) {
    nova::sdk::Application app;
    nova::sdk::SdkConfig config;
    config.name = "disablerollback-app";
    config.enable_runtime = false;
    // Runtime disabled, so its InvalidConfiguration is never triggered.
    config.runtime_config.max_concurrent_tasks = 0;

    auto result = app.initialize(std::move(config));
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(app.is_initialized());
    EXPECT_FALSE(config.enable_runtime);
    app.shutdown();
}

TEST(ApplicationTest, RollbackIsInitializedFalseAfterFailure) {
    nova::sdk::Application app;
    nova::sdk::SdkConfig config;
    config.name = "state-app";
    config.runtime_config.max_concurrent_tasks = 0;

    EXPECT_FALSE(app.initialize(std::move(config)).ok);
    EXPECT_FALSE(app.is_initialized());

    // A valid re-initialization succeeds, confirming the SDK is not stuck.
    config.name = "state-app";
    config.runtime_config.max_concurrent_tasks = 4;
    EXPECT_TRUE(app.initialize(std::move(config)).ok);
    EXPECT_TRUE(app.is_initialized());
    app.shutdown();
}
