#include "nova/sdk/sdk_error.h"

namespace nova::sdk {

std::string sdk_error_string(SdkErrorCode code) noexcept {
    switch (code) {
        case SdkErrorCode::None: return "No error";
        case SdkErrorCode::Unknown: return "Unknown error";
        case SdkErrorCode::InvalidArgument: return "Invalid argument";
        case SdkErrorCode::InvalidConfiguration: return "Invalid configuration";
        case SdkErrorCode::NotInitialized: return "SDK is not initialized";
        case SdkErrorCode::AlreadyInitialized: return "SDK is already initialized";
        case SdkErrorCode::NotRunning: return "SDK is not running";
        case SdkErrorCode::CoreFailure: return "Core subsystem failure";
        case SdkErrorCode::LoaderFailure: return "Loader subsystem failure";
        case SdkErrorCode::RuntimeFailure: return "Runtime subsystem failure";
        case SdkErrorCode::SubsystemDisabled: return "SDK subsystem is disabled";
        default: return "Unknown SDK error code";
    }
}

}  // namespace nova::sdk
