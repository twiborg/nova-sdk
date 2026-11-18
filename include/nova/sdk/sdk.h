#pragma once

// NOVA SDK public API.
//
// The SDK is a unified facade that composes the Core, Loader, and Runtime
// subsystems into a single, easy-to-use interface for application
// developers. It does not reimplement any subsystem functionality; it
// delegates to the existing implementations and presents a consistent,
// lifecycle-managed interface.
//
// The SDK is read-only with respect to game modification: it never executes,
// injects, hooks, or modifies executable code.
#pragma once

#include "nova/core/init.h"
#include "nova/loader/loader.h"
#include "nova/runtime/runtime.h"
#include "nova/sdk/sdk_config.h"
#include "nova/sdk/sdk_error.h"
#include "nova/sdk/sdk_result.h"
#include "nova/sdk/sdk_version.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

namespace nova::sdk {

// Unified SDK facade.
//
// The SDK owns and manages the lifecycle of the Core, Loader, and Runtime
// subsystems. Applications interact with the SDK through this class or
// through the Application class, which adds application-level lifecycle
// management.
//
// Lifecycle:
//   1. Construct.
//   2. initialize(config)   -> prepare all enabled subsystems
//   3. Use API              -> access Core, Loader, Runtime
//   4. shutdown()           -> release all subsystems
//
// Repeated initialization/shutdown is handled safely.
class Sdk {
public:
    Sdk();
    Sdk(const Sdk&) = delete;
    Sdk& operator=(const Sdk&) = delete;
    Sdk(Sdk&&) noexcept = default;
    Sdk& operator=(Sdk&&) noexcept = default;
    ~Sdk();

    // Initialize the SDK with the given configuration.
    // Each enabled subsystem is initialized in order: Core, Loader, Runtime.
    // If a subsystem fails to initialize, initialization stops and the error
    // is returned.
    [[nodiscard]] SdkResult initialize(SdkConfig config = SdkConfig{}) noexcept;

    // Shut down all subsystems. Safe after stop().
    void shutdown() noexcept;

    // Return true if initialize() has been called and shutdown() has not
    // since been called.
    [[nodiscard]] bool is_initialized() const noexcept;

    // Return the effective configuration.
    [[nodiscard]] const SdkConfig& config() const noexcept;

    // Return the SDK API version.
    [[nodiscard]] static SdkVersion version() noexcept {
        return sdk_version();
    }

    // Access to the Core subsystem.
    //
    // Requires the SDK to be initialized and Core to be enabled
    // (core_enabled() == true); otherwise behavior is undefined.
    [[nodiscard]] nova::core::Core& core() noexcept { return *core_; }

    // Access to the Loader subsystem.
    //
    // Requires the SDK to be initialized and Loader to be enabled
    // (loader_enabled() == true); otherwise behavior is undefined.
    [[nodiscard]] nova::loader::Loader& loader() noexcept { return *loader_; }

    // Access to the Runtime subsystem.
    //
    // Requires the SDK to be initialized and Runtime to be enabled
    // (runtime_enabled() == true); otherwise behavior is undefined.
    [[nodiscard]] nova::runtime::Runtime& runtime() noexcept {
        return *runtime_;
    }

    // Safe, nullable access to the Core subsystem.
    //
    // Returns nullptr when the SDK has not been initialized or Core is
    // disabled; otherwise returns a pointer to the Core subsystem.
    [[nodiscard]] nova::core::Core* try_core() noexcept { return core_.get(); }

    // Safe, nullable access to the Loader subsystem.
    //
    // Returns nullptr when the SDK has not been initialized or Loader is
    // disabled; otherwise returns a pointer to the Loader subsystem.
    [[nodiscard]] nova::loader::Loader* try_loader() noexcept { return loader_.get(); }

    // Safe, nullable access to the Runtime subsystem.
    //
    // Returns nullptr when the SDK has not been initialized or Runtime is
    // disabled; otherwise returns a pointer to the Runtime subsystem.
    [[nodiscard]] nova::runtime::Runtime* try_runtime() noexcept {
        return runtime_.get();
    }

    // Return true if Core is enabled.
    [[nodiscard]] bool core_enabled() const noexcept {
        return config_.enable_core;
    }

    // Return true if Loader is enabled.
    [[nodiscard]] bool loader_enabled() const noexcept {
        return config_.enable_loader;
    }

    // Return true if Runtime is enabled.
    [[nodiscard]] bool runtime_enabled() const noexcept {
        return config_.enable_runtime;
    }

private:
    [[nodiscard]] SdkResult require_initialized() const noexcept;
    // Shut down all successfully initialized subsystems in reverse order
    // (Runtime -> Loader -> Core). Assumes mutex_ is already held by the
    // caller; does not acquire it. Safe to call when some subsystems are null
    // (disabled or never initialized). Idempotent and noexcept.
    void shutdown_locked() noexcept;

    SdkConfig config_;
    std::unique_ptr<nova::core::Core> core_;
    std::unique_ptr<nova::loader::Loader> loader_;
    std::unique_ptr<nova::runtime::Runtime> runtime_;
    std::atomic<bool> initialized_{false};
    mutable std::mutex mutex_;
};

}  // namespace nova::sdk
