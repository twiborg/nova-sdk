#include "nova/runtime/runtime_error.h"

#include <string>

namespace nova::runtime {

std::string runtime_error_string(RuntimeErrorCode code) noexcept {
    switch (code) {
        case RuntimeErrorCode::None: return "No error";
        case RuntimeErrorCode::Unknown: return "Unknown error";
        case RuntimeErrorCode::InvalidArgument: return "Invalid argument";
        case RuntimeErrorCode::InvalidConfiguration:
            return "Invalid configuration";
        case RuntimeErrorCode::InvalidTransition:
            return "Invalid lifecycle transition";
        case RuntimeErrorCode::NotInitialized:
            return "Runtime is not initialized";
        case RuntimeErrorCode::AlreadyInitialized:
            return "Runtime is already initialized";
        case RuntimeErrorCode::NotRunning: return "Runtime is not running";
        case RuntimeErrorCode::SchedulerFailure: return "Scheduler failure";
        case RuntimeErrorCode::QueueCapacityExceeded:
            return "Queue capacity exceeded";
        case RuntimeErrorCode::TaskCancelled: return "Task cancelled";
        case RuntimeErrorCode::TaskExecutionFailure:
            return "Task execution failure";
        case RuntimeErrorCode::EventDispatchFailure:
            return "Event dispatch failure";
        case RuntimeErrorCode::ShutdownTimeout:
            return "Shutdown timeout";
        default: return "Unknown runtime error code";
    }
}

}  // namespace nova::runtime
