#include "nova/core/error.h"
#include "nova/core/init.h"

#include <gtest/gtest.h>

TEST(CoreTest, InitializeAndShutdown) {
    nova::core::Core core;
    EXPECT_FALSE(core.is_initialized());
    EXPECT_TRUE(core.initialize());
    EXPECT_TRUE(core.is_initialized());
    EXPECT_FALSE(core.initialize());  // already initialized
    core.shutdown();
    EXPECT_FALSE(core.is_initialized());
}

TEST(CoreTest, ShutdownWhenNotInitialized) {
    nova::core::Core core;
    EXPECT_NO_THROW(core.shutdown());
}
