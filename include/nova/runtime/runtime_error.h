#pragma once

// Runtime error classification.
//
// These errors originate in the runtime itself (lifecycle, configuration,
// scheduler, dispatcher) rather than in Core or Loader. Detailed errors from
// Core/Loader are preserved in their own types.

#include <exception>
#include <string>
#include <string_view>

namespace nova::runtime {

// Classification of runtime failures.
enum class RuntimeErrorCode : int {
    None = 0,
    Unknown = 1,
    InvalidArgument = 2,
    InvalidConfiguration = 3,
    InvalidTransition = 4,
    NotInitialized = 5,
    AlreadyInitialized = 6,
    NotRunning = 7,
    SchedulerFailure = 8,
    QueueCapacityExceeded = 9,
    TaskCancelled = 10,
    TaskExecutionFailure = 11,
    EventDispatchFailure = 12,
    ShutdownTimeout = 13,
};

// A runtime-specific error.
struct RuntimeError : public std::exception {
    RuntimeError() = default;
    RuntimeError(RuntimeErrorCode code, std::string message) noexcept
        : code(code), message(std::move(message)) {}

    RuntimeErrorCode code = RuntimeErrorCode::None;
    std::string message;

    [[nodiscard]] const char* what() const noexcept override {
        return message.c_str();
    }
};

// Human-readable description of a RuntimeErrorCode.
[[nodiscard]] std::string runtime_error_string(RuntimeErrorCode code) noexcept;

// Result of a runtime lifecycle or configuration operation.
struct RuntimeResult {
    bool ok = false;
    RuntimeError error;
};

}  // namespace nova::runtime
