#pragma once

// Unified NOVA Runtime public API.
//
// The Runtime owns the lifecycle of the process-wide execution context:
// task scheduling and event dispatching. It is infrastructure only and does
// not perform arbitrary code execution, injection, hooking, or process
// modification.
//
// Lifecycle:
//   1. Construct          -> Created
//   2. initialize(config) -> Created/Stopped -> Initializing -> Initialized
//   3. start()            -> Initialized -> Running
//   4. stop()             -> Running/Initialized -> Stopping -> Stopped
//   5. shutdown()         -> release subsystems
//
// Repeated calls are safe and invalid transitions produce structured errors.

#include "nova/runtime/runtime_config.h"
#include "nova/runtime/runtime_context.h"
#include "nova/runtime/runtime_diagnostics.h"
#include "nova/runtime/runtime_error.h"
#include "nova/runtime/runtime_state.h"
#include "nova/runtime/event/event_dispatcher.h"
#include "nova/runtime/task/task_scheduler.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace nova::runtime {

// Semantic version of the runtime API.
struct RuntimeVersion {
    std::uint32_t major = 0;
    std::uint32_t minor = 1;
    std::uint32_t patch = 0;
};

// The runtime facade. Moveable but not copyable.
class Runtime {
public:
    Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;
    Runtime(Runtime&&) noexcept = default;
    Runtime& operator=(Runtime&&) noexcept = default;
    ~Runtime();

    // Initialize the runtime with the given configuration.
    [[nodiscard]] RuntimeResult initialize(
        const RuntimeConfig& config = RuntimeConfig{}) noexcept;

    // Start execution (workers and dispatcher become active).
    [[nodiscard]] RuntimeResult start() noexcept;

    // Stop execution gracefully (drain queue, join workers).
    void stop() noexcept;

    // Release all subsystems. Safe after stop().
    void shutdown() noexcept;

    // Current lifecycle state.
    [[nodiscard]] RuntimeState state() const noexcept;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] bool is_running() const noexcept;

    // Immutable snapshot of the current runtime state.
    [[nodiscard]] RuntimeContext context() const noexcept;

    [[nodiscard]] const RuntimeConfig& config() const noexcept;

    TaskScheduler& scheduler() noexcept;
    EventDispatcher& dispatcher() noexcept;

    // Return a diagnostics snapshot of the current runtime state.
    [[nodiscard]] RuntimeDiagnostics diagnostics() const noexcept;

    [[nodiscard]] static RuntimeVersion version() noexcept {
        return RuntimeVersion{};
    }

private:
    [[nodiscard]] RuntimeResult transition_to(RuntimeState next) noexcept;

    RuntimeConfig config_;
    std::atomic<RuntimeState> atomic_state_{RuntimeState::Created};
    std::string name_;
    std::uint64_t id_ = 0;
    std::string version_ = "0.1.0";
    std::string metadata_;
    std::chrono::system_clock::time_point creation_time_;

    TaskScheduler scheduler_;
    EventDispatcher dispatcher_;

    mutable std::mutex mutex_;
};

}  // namespace nova::runtime
