#pragma once

// Runtime configuration.
//
// Reasonable defaults are provided so that a default-constructed
// RuntimeConfig is valid and can initialize a runtime immediately.

#include "nova/core/logger.h"
#include "nova/runtime/task/task_scheduler.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace nova::runtime {

// Configuration for the NOVA Runtime.
struct RuntimeConfig {
    // Human-readable runtime name.
    std::string name = "nova-runtime";

    // Maximum number of concurrently executing tasks.
    std::uint32_t max_concurrent_tasks = 4;

    // Maximum number of tasks queued (waiting or running) at once.
    std::uint32_t task_queue_capacity = 1024;

    // Maximum number of events that may be queued for dispatch at once.
    std::uint32_t event_queue_capacity = 256;

    // Minimum log level used by the runtime.
    nova::core::LogLevel log_level = nova::core::LogLevel::Info;

    // Time to wait for tasks to complete during shutdown before forcing
    // termination.
    std::chrono::milliseconds shutdown_timeout = std::chrono::milliseconds(5000);

    // Task retention policy for completed task results.
    RetentionPolicy retention_policy = RetentionPolicy::CountBased;

    // Maximum number of completed task results to retain (used when
    // retention_policy is CountBased).
    std::uint32_t max_retained_results = 1000;

    // Maximum age of a completed task result before cleanup (used when
    // retention_policy is TimeBased).
    std::chrono::milliseconds retention_duration =
        std::chrono::minutes(5);

    // Enable priority-aware scheduling. When false, all tasks run in
    // submission order regardless of priority.
    bool priority_scheduling = true;

    // Enable starvation prevention: periodically boost priority of long-queued
    // low-priority tasks.
    bool starvation_prevention = true;

    // Track event dispatch statistics.
    bool event_statistics = true;

    // Collect runtime diagnostics snapshots.
    bool diagnostics_enabled = true;
};

}  // namespace nova::runtime
