#pragma once

// Task scheduler.
//
// Executes application-supplied C++ callables on a pool of worker threads
// with a bounded, priority-aware queue. No script or arbitrary native code
// execution is performed. All operations are thread-safe.
//
// Features:
//   - Priority-aware scheduling (Critical > High > Normal > Low)
//   - FIFO ordering within equal priorities
//   - Cooperative cancellation via CancellationToken
//   - Configurable task retention (time- or count-based)
//   - Thread-safe statistics
#pragma once

#include "nova/runtime/task/cancellation_token.h"
#include "nova/runtime/task/task.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace nova::runtime {

// Statistics for the task scheduler. All counters are thread-safe and
// internally consistent (snapshotted under a single lock).
struct SchedulerStatistics {
    std::uint64_t total_submitted = 0;
    std::uint64_t total_completed = 0;
    std::uint64_t total_failed = 0;
    std::uint64_t total_cancelled = 0;
    std::uint64_t total_retained_cleaned_up = 0;
    std::size_t currently_queued = 0;
    std::size_t currently_running = 0;
    std::size_t retained_results = 0;
    std::uint32_t worker_count = 0;
    std::size_t peak_queue_size = 0;
};

// Retention policy for completed task results.
enum class RetentionPolicy : int {
    None = 0,        // Do not automatically clean up completed tasks
    TimeBased = 1,   // Clean up tasks older than retention_duration
    CountBased = 2,  // Keep at most max_retained_results completed tasks
};

// Configuration hints for the scheduler (superset of RuntimeConfig fields).
struct SchedulerConfig {
    std::uint32_t max_concurrent_tasks = 4;
    std::uint32_t task_queue_capacity = 1024;
    RetentionPolicy retention_policy = RetentionPolicy::CountBased;
    std::uint32_t max_retained_results = 1000;
    std::chrono::milliseconds retention_duration =
        std::chrono::minutes(5);
    bool starvation_prevention = true;
};

// Schedules and executes tasks on a fixed pool of worker threads.
class TaskScheduler {
public:
    TaskScheduler();
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) noexcept = default;
    TaskScheduler& operator=(TaskScheduler&&) noexcept = default;
    ~TaskScheduler();

    // Start the given number of worker threads with the given queue capacity.
    // Must be called before submit(). Safe to call repeatedly (restarts
    // workers).
    void start(std::uint32_t worker_count,
               std::uint32_t capacity = 1024,
               const SchedulerConfig& config = SchedulerConfig{});

    // Submit a callable for execution. Returns the assigned task id, or
    // std::nullopt if the scheduler is not running or the queue is full.
    [[nodiscard]] std::optional<TaskId> submit(std::function<void()> callable,
                                               TaskPriority priority =
                                                   TaskPriority::Normal,
                                               CancellationToken token = {});

    // Cancel a queued (not yet running) task. Returns true if cancelled.
    [[nodiscard]] bool cancel(TaskId id);

    // Return the current state of a task, or std::nullopt if unknown.
    [[nodiscard]] std::optional<TaskState> state(TaskId id) const;

    // Return the result of a task, or std::nullopt if unknown/not finished.
    [[nodiscard]] std::optional<TaskResult> result(TaskId id) const;

    // Block until the task reaches a terminal state (Completed, Failed, or
    // Cancelled), or returns immediately if the task id is unknown. Tasks that
    // remain queued when stop() is called never reach a terminal state and will
    // block wait() indefinitely.
    void wait(TaskId id);

    // Return the number of worker threads.
    [[nodiscard]] std::uint32_t worker_count() const noexcept;

    // Return the number of tasks currently queued (not yet running).
    [[nodiscard]] std::size_t queued_count() const noexcept;

    // Return the number of tasks currently running.
    [[nodiscard]] std::size_t running_count() const noexcept;

    // Return a snapshot of scheduler statistics.
    [[nodiscard]] SchedulerStatistics statistics() const noexcept;

    // Graceful shutdown: stop accepting new work, drain the queue, join
    // workers. Safe to call repeatedly.
    void stop();

private:
    struct QueuedTask {
        TaskId id;
        TaskPriority priority;
        std::uint64_t sequence;
    };

    // Compare function for priority queue: higher priority first, then lower
    // sequence number (FIFO within equal priority).
    struct QueuedTaskCompare {
        bool operator()(const QueuedTask& a, const QueuedTask& b) const {
            if (a.priority != b.priority) {
                return static_cast<int>(a.priority) <
                       static_cast<int>(b.priority);
            }
            return a.sequence > b.sequence;
        }
    };

    void worker_loop();
    void cleanup_retained_results_locked();

    mutable std::mutex mutex_;
    std::condition_variable queue_cv_;
    std::condition_variable task_cv_;
    // Priority-ordered queue of task ids waiting to run.
    std::vector<QueuedTask> queue_;
    std::unordered_map<TaskId, Task> tasks_;
    std::vector<std::thread> workers_;
    std::atomic<TaskId> next_id_{1};
    std::atomic<std::size_t> running_count_{0};
    std::atomic<bool> running_{false};
    std::uint32_t worker_count_ = 0;
    std::uint32_t capacity_ = 1024;
    std::atomic<std::uint64_t> next_sequence_{0};
    SchedulerConfig config_;

    // Statistics (updated under mutex_).
    std::uint64_t total_submitted_ = 0;
    std::uint64_t total_completed_ = 0;
    std::uint64_t total_failed_ = 0;
    std::uint64_t total_cancelled_ = 0;
    std::uint64_t total_retained_cleaned_up_ = 0;
    std::size_t peak_queue_size_ = 0;
};

}  // namespace nova::runtime
