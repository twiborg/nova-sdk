#include "nova/core/logger.h"

#include <gtest/gtest.h>

TEST(LoggerTest, DefaultLevelIsInfo) {
    nova::core::Logger& logger = nova::core::Logger::instance();
    EXPECT_EQ(nova::core::LogLevel::Info, logger.level());
}

TEST(LoggerTest, SetAndGetLevel) {
    nova::core::Logger& logger = nova::core::Logger::instance();
    logger.set_level(nova::core::LogLevel::Trace);
    EXPECT_EQ(nova::core::LogLevel::Trace, logger.level());
    logger.set_level(nova::core::LogLevel::Critical);
    EXPECT_EQ(nova::core::LogLevel::Critical, logger.level());
    logger.set_level(nova::core::LogLevel::Info);
}

TEST(LoggerTest, LogLevels) {
    nova::core::Logger& logger = nova::core::Logger::instance();
    logger.trace("trace message");
    logger.debug("debug message");
    logger.info("info message");
    logger.warning("warning message");
    logger.error("error message");
    logger.critical("critical message");
    SUCCEED();
}
