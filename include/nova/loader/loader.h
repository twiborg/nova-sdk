#pragma once

// Unified NOVA Loader public API.
//
// This header provides a single entry point to the NOVA Loader subsystems
// (PE parser and module enumerator). It does not reimplement their
// functionality; it delegates to the existing implementations and presents a
// consistent, lifecycle-managed interface.
//
// The Loader is read-only: it inspects PE files and enumerates modules in the
// current process. It never executes, injects, hooks, or modifies executable
// code, and it does not depend on the Windows loader.

#include "nova/loader/loader_config.h"
#include "nova/loader/loader_result.h"
#include "nova/loader/loader_version.h"
#include "nova/loader/module/module_enumerator.h"
#include "nova/loader/module/module_info.h"
#include "nova/loader/pe/pe_parser.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>

namespace nova::loader {

// Unified loader facade.
//
// Lifecycle:
//   1. Construct.
//   2. initialize(config)   -> prepare subsystems
//   3. Use API              -> parse PE files, enumerate modules
//   4. shutdown()           -> release subsystems
//
    // Repeated initialization/shutdown is handled safely. The PE parsing methods
    // are stateless and thread-safe. Enumeration and lifecycle methods are
    // guarded by an internal mutex, but concurrent initialize/shutdown/enumerate
    // calls are NOT safe: the caller must externally synchronize them. This class
    // is safe to use from a single thread without external synchronization.
class Loader {
public:
    Loader();
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    Loader(Loader&&) noexcept = default;
    Loader& operator=(Loader&&) noexcept = default;
    ~Loader();

    // Initialize the loader with the given configuration.
    // Returns LoaderResult with ok=true on success.
    [[nodiscard]] LoaderResult initialize(const LoaderConfig& config = LoaderConfig{}) noexcept;

    // Shut down the loader and release subsystems.
    void shutdown() noexcept;

    // Return true if initialize() has been called and shutdown() has not
    // since been called.
    [[nodiscard]] bool is_initialized() const noexcept;

    // Return the effective configuration.
    [[nodiscard]] const LoaderConfig& config() const noexcept { return config_; }

    // Return the loader API version.
    [[nodiscard]] static LoaderVersion version() noexcept { return loader_version(); }

    // Parse a PE file from an in-memory buffer.
    // Delegates to nova::loader::parse when PE parsing is enabled and the
    // loader is initialized; otherwise returns a PeResult with
    // PeErrorCode::SubsystemDisabled.
    [[nodiscard]] PeResult parse_pe(std::span<const std::byte> buffer) noexcept;

    // Parse a PE file from disk.
    // Delegates to nova::loader::parse_file when PE parsing is enabled and
    // the loader is initialized; otherwise returns a PeResult with
    // PeErrorCode::SubsystemDisabled.
    [[nodiscard]] PeResult parse_pe_file(const std::filesystem::path& path) noexcept;

    // Enumerate modules loaded in the current process.
    // Delegates to ModuleEnumerator::enumerate when module enumeration is
    // enabled and the loader is initialized; otherwise returns a
    // ModuleEnumeratorResult with ModuleErrorCode::SubsystemDisabled.
    [[nodiscard]] ModuleEnumeratorResult enumerate_modules() noexcept;

    // Find the first module whose base name matches (case-insensitive).
    // Returns std::nullopt if the loader is not initialized, enumeration is
    // disabled, or no module matches.
    //
    // The returned reference refers to internal loader storage. A subsequent
    // call to enumerate_modules() invalidates previously obtained references;
    // do not retain a reference across re-enumeration. Copy the ModuleInfo if
    // the data must outlive the current enumeration snapshot.
    [[nodiscard]] std::optional<std::reference_wrapper<const ModuleInfo>>
    find_module_by_name(std::string_view name) const noexcept;

    // Find the first module whose full path matches (case-insensitive).
    // Returns std::nullopt if the loader is not initialized, enumeration is
    // disabled, or no module matches.
    //
    // The returned reference refers to internal loader storage. A subsequent
    // call to enumerate_modules() invalidates previously obtained references;
    // do not retain a reference across re-enumeration. Copy the ModuleInfo if
    // the data must outlive the current enumeration snapshot.
    [[nodiscard]] std::optional<std::reference_wrapper<const ModuleInfo>>
    find_module_by_path(const std::filesystem::path& path) const noexcept;

    // Return true if PE parsing is enabled.
    [[nodiscard]] bool pe_parsing_enabled() const noexcept { return config_.enable_pe_parsing; }

    // Return true if module enumeration is enabled.
    [[nodiscard]] bool module_enumeration_enabled() const noexcept {
        return config_.enable_module_enumeration;
    }

private:
    [[nodiscard]] LoaderResult require_initialized() const noexcept;

    LoaderConfig config_;
    std::unique_ptr<ModuleEnumerator> enumerator_;
    std::atomic<bool> initialized_{false};
    mutable std::mutex mutex_;
};

}  // namespace nova::loader
