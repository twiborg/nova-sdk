#include "nova/loader/loader_result.h"

#include <string>

namespace nova::loader {

std::string loader_error_string(LoaderErrorCode code) noexcept {
    switch (code) {
        case LoaderErrorCode::None: return "No error";
        case LoaderErrorCode::Unknown: return "Unknown error";
        case LoaderErrorCode::InvalidArgument: return "Invalid argument";
        case LoaderErrorCode::NotInitialized: return "Loader is not initialized";
        case LoaderErrorCode::AlreadyInitialized: return "Loader is already initialized";
        case LoaderErrorCode::SubsystemDisabled: return "Loader subsystem is disabled";
        default: return "Unknown loader error code";
    }
}

}  // namespace nova::loader
