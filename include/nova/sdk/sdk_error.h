#pragma once

// SDK error classification.
//
// These errors originate in the SDK itself (lifecycle, configuration,
// composition failures) rather than in Core, Loader, or Runtime. Detailed
// errors from those subsystems are preserved in their own types and can be
// inspected through the SDK error message.
#pragma once

#include <exception>
#include <string>

namespace nova::sdk {

// Classification of SDK failures.
enum class SdkErrorCode : int {
    None = 0,
    Unknown = 1,
    InvalidArgument = 2,
    InvalidConfiguration = 3,
    NotInitialized = 4,
    AlreadyInitialized = 5,
    NotRunning = 6,
    CoreFailure = 7,
    LoaderFailure = 8,
    RuntimeFailure = 9,
    SubsystemDisabled = 10,
};

// An SDK-specific error.
struct SdkError {
    SdkError() = default;
    SdkError(SdkErrorCode code, std::string message) noexcept
        : code(code), message(std::move(message)) {}

    SdkErrorCode code = SdkErrorCode::None;
    std::string message;

    [[nodiscard]] const char* what() const noexcept { return message.c_str(); }
};

// Human-readable description of a SdkErrorCode.
[[nodiscard]] std::string sdk_error_string(SdkErrorCode code) noexcept;

}  // namespace nova::sdk
