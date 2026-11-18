#pragma once

// Runtime context snapshot.
//
// Provides an immutable, point-in-time view of a NOVA Runtime instance.
// Captured at construction; all members are const after initialization.

#include "nova/runtime/runtime_config.h"
#include "nova/runtime/runtime_state.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace nova::runtime {

// Immutable snapshot of a running runtime.
class RuntimeContext {
public:
    RuntimeContext(std::string name,
                   std::uint64_t id,
                   std::string version,
                   RuntimeState state,
                   RuntimeConfig config,
                   std::string metadata,
                   std::chrono::system_clock::time_point creation_time);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] const std::string& version() const noexcept { return version_; }
    [[nodiscard]] RuntimeState state() const noexcept { return state_; }
    [[nodiscard]] const RuntimeConfig& config() const noexcept { return config_; }
    [[nodiscard]] const std::string& metadata() const noexcept { return metadata_; }
    [[nodiscard]] std::chrono::system_clock::time_point creation_time() const
        noexcept {
        return creation_time_;
    }

private:
    std::string name_;
    std::uint64_t id_;
    std::string version_;
    RuntimeState state_;
    RuntimeConfig config_;
    std::string metadata_;
    std::chrono::system_clock::time_point creation_time_;
};

}  // namespace nova::runtime
