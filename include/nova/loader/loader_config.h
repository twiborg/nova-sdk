#pragma once

// Loader configuration.
//
// Reasonable defaults are provided so that a default-constructed
// LoaderConfig enables both the PE parser and module enumeration subsystems.

#include "nova/core/logger.h"

namespace nova::loader {

// Configuration for the unified loader.
struct LoaderConfig {
    // Enable PE file parsing via the PE parser subsystem.
    bool enable_pe_parsing = true;

    // Enable current-process module enumeration.
    bool enable_module_enumeration = true;

    // Minimum log level used by the loader (and forwarded to NOVA Core's
    // logger when logging is desired).
    nova::core::LogLevel log_level = nova::core::LogLevel::Info;

    // Whether to validate PE file structure strictly during parsing.
    bool strict_validation = true;
};

}  // namespace nova::loader
