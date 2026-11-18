#pragma once

// SDK version information.
#pragma once

#include <cstdint>
#include <string>

namespace nova::sdk {

// Semantic version of the SDK API.
struct SdkVersion {
    std::uint32_t major = 0;
    std::uint32_t minor = 1;
    std::uint32_t patch = 0;
};

// Return the SDK API version.
[[nodiscard]] SdkVersion sdk_version() noexcept;

// Return the SDK API version as a human-readable string (e.g. "0.1.0").
[[nodiscard]] std::string sdk_version_string() noexcept;

}  // namespace nova::sdk
