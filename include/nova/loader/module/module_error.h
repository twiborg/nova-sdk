#pragma once

// Module enumeration error classification.
//
// These errors describe failures that can occur while enumerating modules in
// the current process. They are intentionally distinct from PE parser errors
// (nova::loader::PeErrorCode) so that module enumeration failures are not
// conflated with PE file parsing failures.

#include <cstdint>
#include <string>

namespace nova::loader {

// Classification of module enumeration failures.
enum class ModuleErrorCode : int {
    None = 0,
    Unknown = 1,
    InvalidArgument = 2,
    NotFound = 3,
    ApiFailure = 4,
    EnumerationFailure = 5,
    MetadataFailure = 6,
    BufferTooSmall = 7,
    InvalidPath = 8,
    StateChanged = 9,
};

// A module enumeration error.
struct ModuleError {
    ModuleErrorCode code = ModuleErrorCode::None;
    std::string message;
};

// Human-readable description of a ModuleErrorCode.
[[nodiscard]] std::string module_error_string(ModuleErrorCode code) noexcept;

}  // namespace nova::loader
