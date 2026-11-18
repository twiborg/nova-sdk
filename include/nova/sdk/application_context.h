#pragma once

// Application context snapshot.
//
// Provides an immutable, point-in-time view of a NOVA application running
// through the SDK. All members are const after initialization.
#pragma once

#include "nova/sdk/sdk_version.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

namespace nova::sdk {

// Immutable snapshot of an application running through the SDK.
class ApplicationContext {
public:
    ApplicationContext(std::string name,
                       std::string version,
                       std::uint64_t id,
                       std::string sdk_version,
                       std::chrono::system_clock::time_point initialization_time)
        : name_(std::move(name)),
          version_(std::move(version)),
          id_(id),
          sdk_version_(std::move(sdk_version)),
          initialization_time_(initialization_time) {}

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& version() const noexcept { return version_; }
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] const std::string& sdk_version() const noexcept {
        return sdk_version_;
    }
    [[nodiscard]] std::chrono::system_clock::time_point initialization_time()
        const noexcept {
        return initialization_time_;
    }

private:
    std::string name_;
    std::string version_;
    std::uint64_t id_;
    std::string sdk_version_;
    std::chrono::system_clock::time_point initialization_time_;
};

}  // namespace nova::sdk
