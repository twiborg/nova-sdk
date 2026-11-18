#include "nova/runtime/runtime.h"

#include <atomic>
#include <chrono>

namespace {

// Global unique runtime id generator.
std::atomic<std::uint64_t> g_next_runtime_id{1};

}  // namespace

namespace nova::runtime {

Runtime::Runtime() = default;

Runtime::~Runtime() {
    shutdown();
}

RuntimeResult Runtime::initialize(const RuntimeConfig& config) noexcept {
    RuntimeResult result;
    result.ok = false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (atomic_state_.load(std::memory_order_acquire) !=
            RuntimeState::Created &&
        atomic_state_.load(std::memory_order_acquire) != RuntimeState::Stopped) {
        result.error.code = RuntimeErrorCode::AlreadyInitialized;
        result.error.message = "Runtime is already initialized";
        return result;
    }

    if (config.max_concurrent_tasks == 0 ||
        config.task_queue_capacity == 0 || config.event_queue_capacity == 0) {
        result.error.code = RuntimeErrorCode::InvalidConfiguration;
        result.error.message = "Invalid runtime configuration";
        return result;
    }

    atomic_state_.store(RuntimeState::Initializing, std::memory_order_release);
    config_ = config;
    name_ = config.name;
    id_ = g_next_runtime_id.fetch_add(1, std::memory_order_relaxed);
    creation_time_ = std::chrono::system_clock::now();
    metadata_.clear();

    atomic_state_.store(RuntimeState::Initialized, std::memory_order_release);
    result.ok = true;
    return result;
}

RuntimeResult Runtime::start() noexcept {
    RuntimeResult result;
    result.ok = false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (atomic_state_.load(std::memory_order_acquire) !=
        RuntimeState::Initialized) {
        result.error.code = RuntimeErrorCode::NotRunning;
        result.error.message = "Runtime is not initialized";
        return result;
    }

    atomic_state_.store(RuntimeState::Running, std::memory_order_release);
    SchedulerConfig sched_config;
    sched_config.max_concurrent_tasks = config_.max_concurrent_tasks;
    sched_config.task_queue_capacity = config_.task_queue_capacity;
    sched_config.retention_policy = config_.retention_policy;
    sched_config.max_retained_results = config_.max_retained_results;
    sched_config.retention_duration = config_.retention_duration;
    sched_config.starvation_prevention = config_.starvation_prevention;
    scheduler_.start(config_.max_concurrent_tasks, config_.task_queue_capacity,
                     sched_config);
    result.ok = true;
    return result;
}

void Runtime::stop() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current = atomic_state_.load(std::memory_order_acquire);
    if (current != RuntimeState::Running &&
        current != RuntimeState::Initialized &&
        current != RuntimeState::Stopping) {
        return;
    }

    atomic_state_.store(RuntimeState::Stopping, std::memory_order_release);
    scheduler_.stop();
    atomic_state_.store(RuntimeState::Stopped, std::memory_order_release);
}

void Runtime::shutdown() noexcept {
    stop();
    atomic_state_.store(RuntimeState::Stopped, std::memory_order_release);
}

RuntimeState Runtime::state() const noexcept {
    return atomic_state_.load(std::memory_order_acquire);
}

bool Runtime::is_initialized() const noexcept {
    auto s = atomic_state_.load(std::memory_order_acquire);
    return s == RuntimeState::Initialized || s == RuntimeState::Running ||
           s == RuntimeState::Stopping;
}

bool Runtime::is_running() const noexcept {
    return atomic_state_.load(std::memory_order_acquire) ==
           RuntimeState::Running;
}

RuntimeContext Runtime::context() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return RuntimeContext(name_, id_, version_, state(), config_, metadata_,
                          creation_time_);
}

RuntimeDiagnostics Runtime::diagnostics() const noexcept {
    RuntimeDiagnostics diag;
    diag.state = state();
    diag.name = name_;
    diag.id = id_;
    diag.version = version_;
    diag.creation_time = creation_time_;
    if (creation_time_ != std::chrono::system_clock::time_point{}) {
        diag.uptime =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - creation_time_);
    }
    diag.config = config_;
    diag.scheduler_stats = scheduler_.statistics();
    diag.event_stats = dispatcher_.statistics();
    return diag;
}

const RuntimeConfig& Runtime::config() const noexcept { return config_; }

TaskScheduler& Runtime::scheduler() noexcept { return scheduler_; }

EventDispatcher& Runtime::dispatcher() noexcept { return dispatcher_; }

}  // namespace nova::runtime
