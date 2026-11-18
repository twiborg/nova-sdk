#pragma once

// NOVA Application public API.
//
// An Application is the top-level entry point for a program built on NOVA.
// It composes the SDK (Core, Loader, Runtime) and provides a simple
// lifecycle: construct, initialize, start, stop, shutdown.
//
// The Application is read-only with respect to game modification: it never
// executes, injects, hooks, or modifies executable code.
#pragma once

#include "nova/sdk/application_context.h"
#include "nova/sdk/sdk.h"
#include "nova/sdk/sdk_config.h"
#include "nova/sdk/sdk_error.h"
#include "nova/sdk/sdk_result.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace nova::sdk {

class Sdk;

// Application lifecycle facade.
class Application {
public:
    Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) noexcept = default;
    Application& operator=(Application&&) noexcept = default;
    ~Application();

    // Initialize the application with the given configuration.
    [[nodiscard]] SdkResult initialize(SdkConfig config = SdkConfig{}) noexcept;

    // Start execution (runtime workers and event dispatcher become active).
    [[nodiscard]] SdkResult start() noexcept;

    // Stop execution gracefully.
    void stop() noexcept;

    // Release all subsystems. Safe after stop().
    void shutdown() noexcept;

    // Return true if initialize() has been called and shutdown() has not
    // since been called.
    [[nodiscard]] bool is_initialized() const noexcept;

    // Return true if start() has been called and stop() has not
    // since been called.
    [[nodiscard]] bool is_running() const noexcept;

    // Immutable snapshot of the application context.
    [[nodiscard]] ApplicationContext context() const noexcept;

    // Access to the underlying SDK (for advanced use).
    //
    // Returns a shared handle to the SDK object. The returned handle keeps the
    // Sdk instance alive even after Application::shutdown() releases the
    // Application's own ownership. Lifetime is guaranteed; thread-safe access
    // to subsystem internals is NOT guaranteed by this handle and remains the
    // caller's responsibility (see Sdk and subsystem documentation).
    //
    // Returns an empty shared_ptr before initialize() or after shutdown().
    [[nodiscard]] std::shared_ptr<Sdk> sdk();

    // Return a copy of the effective configuration. The copy is taken while
    // holding a shared lock, so the returned value is a consistent snapshot
    // that is safe to use after the call returns, including across shutdown().
    // Returns a default-constructed SdkConfig before initialize() or after
    // shutdown() when no Sdk exists.
    [[nodiscard]] SdkConfig config() const;

    [[nodiscard]] static SdkVersion version() noexcept {
        return sdk_version();
    }

private:
    [[nodiscard]] SdkResult require_initialized() const noexcept;

    bool initialized_ = false;
    bool running_ = false;
    std::string name_;
    std::uint64_t id_ = 0;
    std::string version_;
    std::chrono::system_clock::time_point initialization_time_;
    SdkConfig config_;

    std::shared_ptr<Sdk> sdk_;
    mutable std::shared_mutex mutex_;
};

}  // namespace nova::sdk
