#pragma once

// SDK configuration.
//
// Aggregates configuration for Core, Loader, and Runtime subsystems.
// Reasonable defaults are provided so that a default-constructed
// SdkConfig is valid and can initialize the SDK immediately.
#pragma once

#include "nova/core/logger.h"
#include "nova/loader/loader_config.h"
#include "nova/runtime/runtime_config.h"

#include <cstdint>
#include <string>

namespace nova::sdk {

// Configuration for the NOVA SDK.
struct SdkConfig {
    SdkConfig() = default;
    SdkConfig(const SdkConfig&) = default;
    SdkConfig& operator=(const SdkConfig&) = default;
    SdkConfig(SdkConfig&&) = default;
    SdkConfig& operator=(SdkConfig&&) = default;

    // Application name.
    std::string name = "nova-app";

    // Application version (semantic version string, e.g. "1.0.0").
    std::string version = "1.0.0";

    // Minimum log level used by the SDK.
    nova::core::LogLevel log_level = nova::core::LogLevel::Info;

    // Loader subsystem configuration.
    nova::loader::LoaderConfig loader_config{};

    // Runtime subsystem configuration.
    nova::runtime::RuntimeConfig runtime_config{};

    // Enable or disable the Core subsystem.
    bool enable_core = true;

    // Enable or disable the Loader subsystem.
    bool enable_loader = true;

    // Enable or disable the Runtime subsystem.
    bool enable_runtime = true;
};

}  // namespace nova::sdk
