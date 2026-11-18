#pragma once

// Runtime diagnostics snapshot.
//
// Provides a consistent, point-in-time view of runtime health. Safe to
// retrieve concurrently from multiple threads.
#pragma once

#include "nova/runtime/event/event_dispatcher.h"
#include "nova/runtime/runtime_config.h"
#include "nova/runtime/runtime_state.h"
#include "nova/runtime/task/task_scheduler.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace nova::runtime {

// Snapshot of runtime diagnostics.
struct RuntimeDiagnostics {
    RuntimeState state = RuntimeState::Created;
    std::string name;
    std::uint64_t id = 0;
    std::string version;
    std::chrono::system_clock::time_point creation_time;
    std::chrono::milliseconds uptime{0};
    RuntimeConfig config;
    SchedulerStatistics scheduler_stats;
    EventDispatcher::Statistics event_stats;
};

}  // namespace nova::runtime
