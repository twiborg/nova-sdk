#pragma once

// Version information for the NOVA Loader.
//
// This header exposes the version of the unified loader API independent of
// the underlying PE parser and module enumerator subsystems.

#include <cstdint>
#include <string>

namespace nova::loader {

// Semantic version of the loader API.
struct LoaderVersion {
    std::uint32_t major = 0;
    std::uint32_t minor = 1;
    std::uint32_t patch = 0;
};

// Return the loader API version.
[[nodiscard]] LoaderVersion loader_version() noexcept;

// Return the loader API version as a human-readable string (e.g. "0.1.0").
[[nodiscard]] std::string loader_version_string() noexcept;

}  // namespace nova::loader
