#include "nova/sdk/sdk.h"

#include <chrono>
#include <string>

namespace nova::sdk {

Sdk::Sdk() = default;

Sdk::~Sdk() {
    shutdown();
}

SdkResult Sdk::initialize(SdkConfig config) noexcept {
    SdkResult result;
    result.ok = false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load(std::memory_order_acquire)) {
        result.error.code = SdkErrorCode::AlreadyInitialized;
        result.error.message = "SDK is already initialized";
        return result;
    }

    // Validate configuration.
    if (config.name.empty()) {
        result.error.code = SdkErrorCode::InvalidConfiguration;
        result.error.message = "SDK configuration name must not be empty";
        return result;
    }

    config_ = std::move(config);

    // Initialize Core.
    if (config_.enable_core) {
        core_ = std::make_unique<nova::core::Core>();
        if (!core_->initialize()) {
            result.error.code = SdkErrorCode::CoreFailure;
            result.error.message = "Core subsystem initialization failed";
            shutdown_locked();
            return result;
        }
    }

    // Initialize Loader.
    if (config_.enable_loader) {
        loader_ = std::make_unique<nova::loader::Loader>();
        auto loader_result = loader_->initialize(config_.loader_config);
        if (!loader_result.ok) {
            result.error.code = SdkErrorCode::LoaderFailure;
            result.error.message = "Loader subsystem initialization failed: " +
                                   loader_result.error.message;
            shutdown_locked();
            return result;
        }
    }

    // Initialize Runtime.
    if (config_.enable_runtime) {
        runtime_ = std::make_unique<nova::runtime::Runtime>();
        auto runtime_result =
            runtime_->initialize(config_.runtime_config);
        if (!runtime_result.ok) {
            result.error.code = SdkErrorCode::RuntimeFailure;
            result.error.message = "Runtime subsystem initialization failed: " +
                                   runtime_result.error.message;
            shutdown_locked();
            return result;
        }
    }

    initialized_.store(true, std::memory_order_release);
    result.ok = true;
    return result;
}

void Sdk::shutdown_locked() noexcept {
    // Shut down subsystems in reverse initialization order:
    // Runtime -> Loader -> Core. Disabled or never-initialized subsystems
    // have null unique_ptrs and are skipped. Idempotent and noexcept.
    if (runtime_) {
        runtime_->shutdown();
    }
    if (loader_) {
        loader_->shutdown();
    }
    if (core_) {
        core_->shutdown();
    }
    initialized_.store(false, std::memory_order_release);
}

void Sdk::shutdown() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_locked();
}

bool Sdk::is_initialized() const noexcept {
    return initialized_.load(std::memory_order_acquire);
}

const SdkConfig& Sdk::config() const noexcept { return config_; }

}  // namespace nova::sdk
