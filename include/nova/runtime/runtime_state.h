#pragma once

// Runtime lifecycle state.
//
// Represents the high-level lifecycle of a NOVA Runtime instance. Transitions
// are predictable and are performed through the public initialize/start/stop/
// shutdown interface.
namespace nova::runtime {

enum class RuntimeState : int {
    Created = 0,     // Constructed, not yet initialized
    Initializing = 1,// initialize() in progress
    Initialized = 2, // initialize() completed, not yet started
    Running = 3,     // start() completed, tasks/events active
    Stopping = 4,    // stop() in progress
    Stopped = 5,     // stop() completed
    Failed = 6,      // A fatal error occurred
};

}  // namespace nova::runtime
