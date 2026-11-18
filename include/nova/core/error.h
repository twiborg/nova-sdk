#pragma once

#include <exception>
#include <string>
#include <string_view>

namespace nova::core {

enum class ErrorCode : int {
    None = 0,
    Unknown = 1,
    InvalidArgument = 2,
    OutOfRange = 3,
    NotImplemented = 4,
    FileNotFound = 5,
    ParseError = 6,
    IoError = 7,
    InvalidConfiguration = 8,
    AlreadyInitialized = 9,
    NotInitialized = 10,
};

class NovaError : public std::exception {
public:
    explicit NovaError(ErrorCode code, std::string message) noexcept
        : code_(code), message_(std::move(message)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const char* what() const noexcept override { return message_.c_str(); }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }

private:
    ErrorCode code_;
    std::string message_;
};

[[nodiscard]] inline std::string error_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::None: return "No error";
        case ErrorCode::Unknown: return "Unknown error";
        case ErrorCode::InvalidArgument: return "Invalid argument";
        case ErrorCode::OutOfRange: return "Out of range";
        case ErrorCode::NotImplemented: return "Not implemented";
        case ErrorCode::FileNotFound: return "File not found";
        case ErrorCode::ParseError: return "Parse error";
        case ErrorCode::IoError: return "I/O error";
        case ErrorCode::InvalidConfiguration: return "Invalid configuration";
        case ErrorCode::AlreadyInitialized: return "Already initialized";
        case ErrorCode::NotInitialized: return "Not initialized";
        default: return "Unknown error code";
    }
}

}  // namespace nova::core
