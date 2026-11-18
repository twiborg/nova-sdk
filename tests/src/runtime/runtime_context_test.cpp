#include "nova/runtime/runtime.h"
#include "nova/runtime/runtime_context.h"

#include <chrono>
#include <gtest/gtest.h>

TEST(RuntimeContextTest, ContextFields) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    auto ctx = runtime.context();
    EXPECT_EQ(ctx.name(), runtime.config().name);
    EXPECT_GT(ctx.id(), 0u);
    EXPECT_FALSE(ctx.version().empty());
    EXPECT_EQ(ctx.state(), nova::runtime::RuntimeState::Initialized);
    EXPECT_EQ(ctx.config().name, runtime.config().name);
    EXPECT_EQ(ctx.config().max_concurrent_tasks,
              runtime.config().max_concurrent_tasks);
    runtime.shutdown();
}

TEST(RuntimeContextTest, CreationTimestamp) {
    nova::runtime::Runtime runtime;
    EXPECT_TRUE(runtime.initialize().ok);
    auto ctx = runtime.context();
    EXPECT_NE(ctx.creation_time(), std::chrono::system_clock::time_point{});
    runtime.shutdown();
}
