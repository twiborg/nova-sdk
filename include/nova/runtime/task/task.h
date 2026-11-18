#pragma once

// Task abstraction.
//
// Tasks are controlled C++ callables supplied by the application. No script
// or arbitrary native code execution is performed by the scheduler.
#pragma once

#include "nova/runtime/task/cancellation_token.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace nova::runtime {

// Unique identifier for a task.
using TaskId = std::uint64_t;

// Priority of a task. Higher numeric values are scheduled first.
// Priority cannot be changed after a task begins execution.
enum class TaskPriority : int {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3,
};

// Lifecycle state of a task.
enum class TaskState : int {
    Queued = 0,      // Waiting to be picked up by a worker
    Running = 1,     // Currently executing on a worker thread
    Completed = 2,   // Finished successfully
    Failed = 3,      // Finished with an error
    Cancelled = 4,   // Removed before execution
};

// Result of a task execution.
struct TaskResult {
    bool ok = false;
    std::string error;
};

// A single unit of work submitted to the TaskScheduler.
struct Task {
    TaskId id = 0;
    TaskState state = TaskState::Queued;
    TaskPriority priority = TaskPriority::Normal;
    std::function<void()> callable;
    CancellationToken token;
    TaskResult result;
    // Monotonically increasing sequence number assigned at submission time
    // to preserve FIFO ordering within equal priorities.
    std::uint64_t sequence = 0;
    // Time point when the task completed (or was cancelled/failed).
    std::chrono::system_clock::time_point completed_time{};
};

}  // namespace nova::runtime
