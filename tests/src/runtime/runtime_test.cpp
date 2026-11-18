#include "nova/runtime/runtime.h"
#include "nova/runtime/runtime_diagnostics.h"

#include <gtest/gtest.h>

TEST(RuntimeTest, Construction) {
    nova::runtime::Runtime runtime;
    EXPECT_EQ(runtime.state(), nova::runtime::RuntimeState::Created);
    EXPECT_FALSE(runtime.is_initialized());
    EXPECT_FALSE(runtime.is_running());
}

TEST(RuntimeTest, DefaultConfigurationValid) {
    nova::runtime::RuntimeConfig config;
    EXPECT_FALSE(config.name.empty());
    EXPECT_GT(config.max_concurrent_tasks, 0u);
    EXPECT_GT(config.task_queue_capacity, 0u);
    EXPECT_GT(config.event_queue_capacity, 0u);
}

TEST(RuntimeTest, Initialization) {
    nova::runtime::Runtime runtime;
    auto r = runtime.initialize();
    EXPECT_TRUE(r.ok) << r.error.message;
    EXPECT_TRUE(runtime.is_initialized());
    EXPECT_FALSE(runtime.is_running());
    EXPECT_EQ(runtime.state(), nova::runtime::RuntimeState::Initialized);
    runtime.shutdown();
}

TEST(RuntimeTest, RepeatedInitialization) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    auto r = runtime.initialize();
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::runtime::RuntimeErrorCode::AlreadyInitialized);
    runtime.shutdown();
}

TEST(RuntimeTest, StartAndStop) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    EXPECT_TRUE(runtime.start().ok);
    EXPECT_TRUE(runtime.is_running());
    EXPECT_EQ(runtime.state(), nova::runtime::RuntimeState::Running);
    runtime.stop();
    EXPECT_FALSE(runtime.is_running());
    EXPECT_EQ(runtime.state(), nova::runtime::RuntimeState::Stopped);
    runtime.shutdown();
}

TEST(RuntimeTest, StartWithoutInitialization) {
    nova::runtime::Runtime runtime;
    auto r = runtime.start();
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::runtime::RuntimeErrorCode::NotRunning);
}

TEST(RuntimeTest, StopBeforeStart) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    runtime.stop();
    EXPECT_FALSE(runtime.is_running());
    EXPECT_EQ(runtime.state(), nova::runtime::RuntimeState::Stopped);
}

TEST(RuntimeTest, RepeatedShutdown) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    runtime.shutdown();
    runtime.shutdown(); // must be safe
    EXPECT_FALSE(runtime.is_initialized());
}

TEST(RuntimeTest, ShutdownWithoutInitialization) {
    nova::runtime::Runtime runtime;
    runtime.shutdown(); // must be safe
    EXPECT_FALSE(runtime.is_initialized());
}

TEST(RuntimeTest, InvalidConfiguration) {
    nova::runtime::RuntimeConfig config;
    config.max_concurrent_tasks = 0;
    nova::runtime::Runtime runtime;
    auto r = runtime.initialize(config);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.error.code, nova::runtime::RuntimeErrorCode::InvalidConfiguration);
}

TEST(RuntimeTest, Context) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    auto ctx = runtime.context();
    EXPECT_FALSE(ctx.name().empty());
    EXPECT_EQ(ctx.state(), nova::runtime::RuntimeState::Initialized);
    EXPECT_EQ(ctx.config().name, runtime.config().name);
    EXPECT_GT(ctx.id(), 0u);
    runtime.shutdown();
}

TEST(RuntimeTest, Version) {
    auto v = nova::runtime::Runtime::version();
    EXPECT_GE(v.major, 0u);
    EXPECT_GE(v.minor, 0u);
}

TEST(RuntimeTest, SchedulerAndDispatcherAccess) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    EXPECT_TRUE(runtime.start().ok);
    auto& sched = runtime.scheduler();
    auto& disp = runtime.dispatcher();
    EXPECT_GE(sched.worker_count(), 0u);
    EXPECT_EQ(disp.total_subscribers(), 0u);
    runtime.shutdown();
}

TEST(RuntimeTest, Diagnostics) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    EXPECT_TRUE(runtime.start().ok);

    auto diag = runtime.diagnostics();
    EXPECT_EQ(diag.state, nova::runtime::RuntimeState::Running);
    EXPECT_FALSE(diag.name.empty());
    EXPECT_GT(diag.id, 0u);
    EXPECT_FALSE(diag.version.empty());
    EXPECT_GE(diag.uptime.count(), 0);
    EXPECT_EQ(diag.config.name, runtime.config().name);
    EXPECT_GE(diag.scheduler_stats.worker_count, 0u);

    runtime.shutdown();
}

TEST(RuntimeTest, DiagnosticsAfterStop) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    runtime.stop();

    auto diag = runtime.diagnostics();
    EXPECT_EQ(diag.state, nova::runtime::RuntimeState::Stopped);
}
