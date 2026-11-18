#include "nova/runtime/task/task_scheduler.h"

#include <algorithm>
#include <utility>

namespace nova::runtime {

TaskScheduler::TaskScheduler() = default;

TaskScheduler::~TaskScheduler() {
    stop();
}

void TaskScheduler::start(std::uint32_t worker_count,
                          std::uint32_t capacity,
                          const SchedulerConfig& config) {
    // Stop any existing workers first (joins them) before acquiring the lock
    // to avoid deadlock.
    stop();
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = true;
    worker_count_ = worker_count;
    capacity_ = capacity;
    config_ = config;
    running_count_ = 0;
    workers_.reserve(worker_count);
    for (std::uint32_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back(&TaskScheduler::worker_loop, this);
    }
}

void TaskScheduler::stop() {
    std::vector<std::thread> workers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
        queue_cv_.notify_all();
        workers.swap(workers_);
    }
    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    workers_.clear();
    running_count_ = 0;
}

std::optional<TaskId> TaskScheduler::submit(std::function<void()> callable,
                                            TaskPriority priority,
                                            CancellationToken token) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        return std::nullopt;
    }
    if (tasks_.size() >= capacity_) {
        return std::nullopt;
    }
    TaskId id = next_id_.fetch_add(1, std::memory_order_relaxed);
    std::uint64_t seq = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    tasks_.emplace(id,
                   Task{id, TaskState::Queued, priority, std::move(callable),
                        std::move(token), TaskResult{}, seq, {}});
    queue_.push_back(QueuedTask{id, priority, seq});
    std::push_heap(queue_.begin(), queue_.end(), QueuedTaskCompare{});
    queue_cv_.notify_one();

    ++total_submitted_;
    if (queue_.size() > peak_queue_size_) {
        peak_queue_size_ = queue_.size();
    }
    return id;
}

bool TaskScheduler::cancel(TaskId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return false;
    }
    if (it->second.state != TaskState::Queued) {
        return false;
    }
    it->second.state = TaskState::Cancelled;
    it->second.result.ok = false;
    it->second.result.error = "cancelled";
    task_cv_.notify_all();
    ++total_cancelled_;
    return true;
}

std::optional<TaskState> TaskScheduler::state(TaskId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return std::nullopt;
    }
    return it->second.state;
}

std::optional<TaskResult> TaskScheduler::result(TaskId id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(id);
    if (it == tasks_.end()) {
        return std::nullopt;
    }
    return it->second.result;
}

void TaskScheduler::wait(TaskId id) {
    std::unique_lock<std::mutex> lock(mutex_);
    task_cv_.wait(lock, [&] {
        auto it = tasks_.find(id);
        if (it == tasks_.end()) {
            return true;
        }
        return it->second.state == TaskState::Completed ||
               it->second.state == TaskState::Failed ||
               it->second.state == TaskState::Cancelled;
    });
}

std::uint32_t TaskScheduler::worker_count() const noexcept {
    return worker_count_;
}

std::size_t TaskScheduler::queued_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::size_t TaskScheduler::running_count() const noexcept {
    return running_count_.load(std::memory_order_acquire);
}

SchedulerStatistics TaskScheduler::statistics() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    SchedulerStatistics stats;
    stats.total_submitted = total_submitted_;
    stats.total_completed = total_completed_;
    stats.total_failed = total_failed_;
    stats.total_cancelled = total_cancelled_;
    stats.total_retained_cleaned_up = total_retained_cleaned_up_;
    stats.currently_queued = queue_.size();
    stats.currently_running = running_count_.load(std::memory_order_acquire);
    // Count only successfully completed tasks whose results remain available.
    // Cancelled and failed tasks are terminal but are not retained results.
    std::size_t retained = 0;
    for (const auto& entry : tasks_) {
        if (entry.second.state == TaskState::Completed) {
            ++retained;
        }
    }
    stats.retained_results = retained;
    stats.worker_count = worker_count_;
    stats.peak_queue_size = peak_queue_size_;
    return stats;
}

void TaskScheduler::cleanup_retained_results_locked() {
    if (config_.retention_policy == RetentionPolicy::None) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::vector<TaskId> to_remove;

    for (auto& pair : tasks_) {
        Task& task = pair.second;
        // Never remove running or queued tasks.
        if (task.state != TaskState::Completed &&
            task.state != TaskState::Failed &&
            task.state != TaskState::Cancelled) {
            continue;
        }

        bool remove = false;
        if (config_.retention_policy == RetentionPolicy::CountBased) {
            // Mark all terminal tasks for removal; count-based trimming
            // happens after sorting by completion time below.
            if (task.completed_time != std::chrono::system_clock::time_point{}) {
                remove = true;
            }
        } else if (config_.retention_policy == RetentionPolicy::TimeBased &&
                   task.completed_time !=
                       std::chrono::system_clock::time_point{} &&
                   now - task.completed_time > config_.retention_duration) {
            remove = true;
        }

        if (remove) {
            to_remove.push_back(pair.first);
        }
    }

    // For CountBased, sort candidates by completion time (oldest first) and
    // only remove enough to get under max_retained_results.
    if (config_.retention_policy == RetentionPolicy::CountBased &&
        !to_remove.empty()) {
        std::sort(to_remove.begin(), to_remove.end(),
                  [&](TaskId a, TaskId b) {
                      return tasks_.at(a).completed_time <
                             tasks_.at(b).completed_time;
                  });
        std::size_t current_retained = tasks_.size() - queue_.size() -
                                       running_count_.load();
        if (current_retained > config_.max_retained_results) {
            std::size_t to_remove_count =
                current_retained - config_.max_retained_results;
            if (to_remove_count > to_remove.size()) {
                to_remove_count = to_remove.size();
            }
            to_remove.resize(to_remove_count);
        } else {
            to_remove.clear();
        }
    }

    for (TaskId id : to_remove) {
        tasks_.erase(id);
        ++total_retained_cleaned_up_;
    }
}

void TaskScheduler::worker_loop() {
    for (;;) {
        TaskId id = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_cv_.wait(lock, [&] {
                return !queue_.empty() || !running_.load(std::memory_order_acquire);
            });
            if (queue_.empty() && !running_.load(std::memory_order_acquire)) {
                return;
            }
            // Pop the highest-priority task from the heap.
            std::pop_heap(queue_.begin(), queue_.end(), QueuedTaskCompare{});
            id = queue_.back().id;
            queue_.pop_back();
        }

        Task* task = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = tasks_.find(id);
            if (it == tasks_.end() || it->second.state == TaskState::Cancelled) {
                continue;
            }
            // Check cooperative cancellation before running.
            if (it->second.token.is_cancelled()) {
                it->second.state = TaskState::Cancelled;
                it->second.result.ok = false;
                it->second.result.error = "cancelled";
                ++total_cancelled_;
                task_cv_.notify_all();
                continue;
            }
            it->second.state = TaskState::Running;
            task = &it->second;
        }

        running_count_.fetch_add(1, std::memory_order_acq_rel);
        bool threw = false;
        bool cancelled_during_run = false;
        try {
            task->callable();
            // Check cancellation after callable returns (in case it didn't
            // check internally).
            if (task->token.is_cancelled()) {
                cancelled_during_run = true;
            }
        } catch (...) {
            threw = true;
        }
        running_count_.fetch_sub(1, std::memory_order_acq_rel);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (cancelled_during_run) {
                task->state = TaskState::Cancelled;
                task->result.ok = false;
                task->result.error = "cancelled";
                ++total_cancelled_;
            } else if (threw) {
                task->state = TaskState::Failed;
                task->result.ok = false;
                task->result.error = "task callable threw";
                ++total_failed_;
            } else {
                task->state = TaskState::Completed;
                task->result.ok = true;
                ++total_completed_;
            }
            task->completed_time = std::chrono::system_clock::now();
        }
        task_cv_.notify_all();

        // Periodically clean up old completed tasks.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cleanup_retained_results_locked();
        }
    }
}

}  // namespace nova::runtime
