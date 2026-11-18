#pragma once

// Unified loader error and result types.
//
// These describe failures that originate in the loader itself (lifecycle,
// configuration, disabled subsystems) rather than in the underlying PE parser
// or module enumerator. Detailed errors from those subsystems are preserved
// in their own result types (PeResult, ModuleEnumeratorResult) and are not
// duplicated here.

#include <string>

namespace nova::loader {

// Classification of loader-specific failures.
enum class LoaderErrorCode : int {
    None = 0,
    Unknown = 1,
    InvalidArgument = 2,
    NotInitialized = 3,
    AlreadyInitialized = 4,
    SubsystemDisabled = 5,
};

// A loader-specific error.
struct LoaderError {
    LoaderErrorCode code = LoaderErrorCode::None;
    std::string message;
};

// Human-readable description of a LoaderErrorCode.
[[nodiscard]] std::string loader_error_string(LoaderErrorCode code) noexcept;

// Result of a loader lifecycle or configuration operation.
struct LoaderResult {
    bool ok = false;
    LoaderError error;
};

}  // namespace nova::loader
