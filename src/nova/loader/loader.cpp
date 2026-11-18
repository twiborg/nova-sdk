// Unified NOVA Loader implementation.
//
// Delegates to the existing PE parser (nova::loader::parse / parse_file) and
// module enumerator (ModuleEnumerator) implementations without reimplementing
// their functionality. All Windows-specific details remain confined to those
// subsystems.

#include "nova/loader/loader.h"

namespace nova::loader {

Loader::Loader() = default;

Loader::~Loader() {
    shutdown();
}

LoaderResult Loader::initialize(const LoaderConfig& config) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load(std::memory_order_acquire)) {
        LoaderResult result;
        result.ok = false;
        result.error.code = LoaderErrorCode::AlreadyInitialized;
        result.error.message = "Loader is already initialized";
        return result;
    }

    config_ = config;

    if (config_.enable_module_enumeration) {
        enumerator_ = std::make_unique<ModuleEnumerator>();
    }

    initialized_.store(true, std::memory_order_release);

    LoaderResult result;
    result.ok = true;
    return result;
}

void Loader::shutdown() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    enumerator_.reset();
    initialized_.store(false, std::memory_order_release);
}

bool Loader::is_initialized() const noexcept {
    return initialized_.load(std::memory_order_acquire);
}

LoaderResult Loader::require_initialized() const noexcept {
    LoaderResult result;
    if (!initialized_.load(std::memory_order_acquire)) {
        result.ok = false;
        result.error.code = LoaderErrorCode::NotInitialized;
        result.error.message = "Loader is not initialized";
    }
    return result;
}

PeResult Loader::parse_pe(std::span<const std::byte> buffer) noexcept {
    PeResult result;
    if (!initialized_.load(std::memory_order_acquire)) {
        result.ok = false;
        result.error.code = PeErrorCode::Unknown;
        result.error.message = "Loader is not initialized";
        return result;
    }
    if (!config_.enable_pe_parsing) {
        result.ok = false;
        result.error.code = PeErrorCode::Unknown;
        result.error.message = "PE parsing is disabled";
        return result;
    }
    return parse(buffer);
}

PeResult Loader::parse_pe_file(const std::filesystem::path& path) noexcept {
    PeResult result;
    if (!initialized_.load(std::memory_order_acquire)) {
        result.ok = false;
        result.error.code = PeErrorCode::Unknown;
        result.error.message = "Loader is not initialized";
        return result;
    }
    if (!config_.enable_pe_parsing) {
        result.ok = false;
        result.error.code = PeErrorCode::Unknown;
        result.error.message = "PE parsing is disabled";
        return result;
    }
    return parse_file(path);
}

ModuleEnumeratorResult Loader::enumerate_modules() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    ModuleEnumeratorResult result;
    if (!initialized_.load(std::memory_order_acquire)) {
        result.ok = false;
        result.error.code = ModuleErrorCode::Unknown;
        result.error.message = "Loader is not initialized";
        return result;
    }
    if (!config_.enable_module_enumeration || enumerator_ == nullptr) {
        result.ok = false;
        result.error.code = ModuleErrorCode::Unknown;
        result.error.message = "Module enumeration is disabled";
        return result;
    }
    return enumerator_->enumerate();
}

std::optional<std::reference_wrapper<const ModuleInfo>>
Loader::find_module_by_name(std::string_view name) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load(std::memory_order_acquire) ||
        !config_.enable_module_enumeration || enumerator_ == nullptr) {
        return std::nullopt;
    }
    return enumerator_->find_by_name(name);
}

std::optional<std::reference_wrapper<const ModuleInfo>>
Loader::find_module_by_path(const std::filesystem::path& path) const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load(std::memory_order_acquire) ||
        !config_.enable_module_enumeration || enumerator_ == nullptr) {
        return std::nullopt;
    }
    return enumerator_->find_by_path(path);
}

}  // namespace nova::loader
