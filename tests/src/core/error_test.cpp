#include "nova/core/error.h"

#include <gtest/gtest.h>

TEST(ErrorCodeTest, ErrorCodeToString) {
    EXPECT_STREQ("No error", nova::core::error_string(nova::core::ErrorCode::None).c_str());
    EXPECT_STREQ("Invalid argument", nova::core::error_string(nova::core::ErrorCode::InvalidArgument).c_str());
    EXPECT_STREQ("File not found", nova::core::error_string(nova::core::ErrorCode::FileNotFound).c_str());
    EXPECT_STREQ("Parse error", nova::core::error_string(nova::core::ErrorCode::ParseError).c_str());
    EXPECT_STREQ("Unknown error code", nova::core::error_string(static_cast<nova::core::ErrorCode>(99)).c_str());
}

TEST(ErrorCodeTest, NovaErrorWhat) {
    nova::core::NovaError err(nova::core::ErrorCode::InvalidArgument, "bad input");
    EXPECT_EQ(nova::core::ErrorCode::InvalidArgument, err.code());
    EXPECT_STREQ("bad input", err.what());
    EXPECT_STREQ("bad input", err.message().c_str());
}
