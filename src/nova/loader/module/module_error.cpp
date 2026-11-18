#include "nova/loader/module/module_error.h"

#include <string>

namespace nova::loader {

std::string module_error_string(ModuleErrorCode code) noexcept {
    switch (code) {
        case ModuleErrorCode::None: return "No error";
        case ModuleErrorCode::Unknown: return "Unknown error";
        case ModuleErrorCode::InvalidArgument: return "Invalid argument";
        case ModuleErrorCode::NotFound: return "Module not found";
        case ModuleErrorCode::ApiFailure: return "Windows API failure";
        case ModuleErrorCode::EnumerationFailure: return "Module enumeration failure";
        case ModuleErrorCode::MetadataFailure: return "Module metadata retrieval failure";
        case ModuleErrorCode::BufferTooSmall: return "Buffer too small";
        case ModuleErrorCode::InvalidPath: return "Invalid module path";
        case ModuleErrorCode::StateChanged: return "Module list changed during enumeration";
        default: return "Unknown module error code";
    }
}

}  // namespace nova::loader
