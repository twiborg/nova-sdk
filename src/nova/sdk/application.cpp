#include "nova/sdk/application.h"
#include "nova/sdk/sdk.h"

#include <chrono>

namespace nova::sdk {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

SdkResult Application::initialize(SdkConfig config) noexcept {
    SdkResult result;
    result.ok = false;

    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (initialized_) {
        result.error.code = SdkErrorCode::AlreadyInitialized;
        result.error.message = "Application is already initialized";
        return result;
    }

    if (config.name.empty()) {
        result.error.code = SdkErrorCode::InvalidConfiguration;
        result.error.message = "Application configuration name must not be empty";
        return result;
    }

    sdk_ = std::make_shared<Sdk>();
    config_ = config;
    auto sdk_result = sdk_->initialize(std::move(config));
    if (!sdk_result.ok) {
        result.error.code = SdkErrorCode::RuntimeFailure;
        result.error.message = "SDK initialization failed: " +
                               sdk_result.error.message;
        return result;
    }

    initialized_ = true;
    name_ = sdk_->config().name;
    version_ = sdk_->config().version;
    id_ = 1;  // Single application per process
    initialization_time_ = std::chrono::system_clock::now();

    result.ok = true;
    return result;
}

SdkResult Application::start() noexcept {
    SdkResult result;
    result.ok = false;

    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (!initialized_) {
        result.error.code = SdkErrorCode::NotInitialized;
        result.error.message = "Application is not initialized";
        return result;
    }

    if (running_) {
        // Already running; idempotent.
        result.ok = true;
        return result;
    }

    if (sdk_->runtime_enabled()) {
        auto runtime_result = sdk_->runtime().start();
        if (!runtime_result.ok) {
            result.error.code = SdkErrorCode::RuntimeFailure;
            result.error.message = "Runtime start failed: " +
                                   runtime_result.error.message;
            return result;
        }
    }

    running_ = true;
    result.ok = true;
    return result;
}

void Application::stop() noexcept {
    std::lock_guard<std::shared_mutex> lock(mutex_);

    if (!running_) {
        return;
    }

    if (sdk_ && sdk_->runtime_enabled()) {
        sdk_->runtime().stop();
    }

    running_ = false;
}

void Application::shutdown() noexcept {
    stop();

    std::lock_guard<std::shared_mutex> lock(mutex_);
    sdk_.reset();
    config_ = SdkConfig{};
    initialized_ = false;
}

bool Application::is_initialized() const noexcept { return initialized_; }

bool Application::is_running() const noexcept { return running_; }

ApplicationContext Application::context() const noexcept {
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return ApplicationContext(name_, version_, id_, sdk_version_string(),
                              initialization_time_);
}

std::shared_ptr<Sdk> Application::sdk() {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return sdk_;
}

SdkConfig Application::config() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return config_;
}

}  // namespace nova::sdk
