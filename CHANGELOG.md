# Changelog

All notable changes to NOVA are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.2.0] - Unreleased

### Added

- NOVA Core: thread-safe structured logger with six levels (Trace, Debug,
  Info, Warning, Error, Critical).
- NOVA Core: JSON-based configuration manager with typed accessors and safe
  nullable `try_as_*` accessors.
- NOVA Core: `NovaError` exception type with `ErrorCode` classification.
- NOVA Core: RAII core initialization/shutdown state tracking.
- NOVA Loader: read-only PE32/PE32+ PE parser with DOS/COFF/optional header
  and section table parsing, signature and boundary validation, integer
  overflow detection, and safe RVA-to-offset conversion.
- NOVA Loader: read-only module enumeration of modules loaded in the current
  process using documented Windows APIs.
- NOVA Loader: unified Loader facade over PE parser and module enumeration
  with lifecycle management.
- NOVA Runtime: lifecycle management (`initialize`/`start`/`stop`/`shutdown`).
- NOVA Runtime: priority-aware task scheduler (Critical > High > Normal >
  Low), FIFO within priorities, bounded queue, cooperative cancellation, and
  thread-safe statistics.
- NOVA Runtime: typed event dispatcher with subscription management, event
  filtering, nested dispatch support, exception isolation, and dispatch
  statistics.
- NOVA Runtime: runtime diagnostics snapshot.
- NOVA SDK: unified facade composing Core, Loader, and Runtime with lifecycle
  management and typed accessors (`core()`, `loader()`, `runtime()`).
- NOVA SDK: safe nullable subsystem accessors (`try_core()`, `try_loader()`,
  `try_runtime()`).
- NOVA SDK: `Application` class with full application-level lifecycle.
- NOVA SDK: configuration, error handling, result types, and versioning.
- Installation support: public headers, static libraries, and CMake package
  config with `find_package(NOVA)` and `nova::*` imported targets.
- MIT License (`LICENSE`).

### Changed

- Public API signatures frozen (Phase 4.0.3).
- Documentation corrections: CMake generator updated to Visual Studio 18
  2026; installed-package consumption instructions added; bundled examples
  clarified as in-tree builds (Phase 4.0.4.1).
- README licensing: replaced unsupported "Proprietary" wording with MIT
  License reference (Phase 4.2).
- README versioning: documented independent project and subsystem API
  versioning (Phase 4.3).

### Fixed

- SDK initialization rollback (Phase 3.x).
- Application SDK ownership synchronization (Phase 3.x).
- Safe nullable subsystem accessors (`try_core`/`try_loader`/`try_runtime`)
  (Phase 3.x).
- ConfigValue safe nullable accessors (`try_as_*`) (Phase 3.x).
- Scheduler retained-result statistics (Phase 3.x).
- Strict JSON parser correctness (Phase 3.x).
- UTF-8 module names (Phase 3.x).
- Config thread safety (Phase 3.x).
- Config accessor data races (`try_get_as<T>`, `get_as<T>`, `try_get`)
  (Phase 3.8.4).

### Documentation

- Independent versioning policy documented: project version `0.2.0`, subsystem
  API versions `0.1.0`, intentionally independent, informational metadata
  only, no automatic compatibility enforcement (Phase 4.3).
- Release Candidate verification: 216 tests across six suites, all passing
  under Release with `/W4 /WX` (Phase 4.0.5).
- Test count correction: `nova_core_tests` is 60 (not 59); project total is
  216 (not 215) (Phase 4.0.5.1).

### Not implemented (explicitly out of scope)

- Game loader.
- Hooking / DLL injection.
- Reverse engineering.
- Game-specific functionality.
- DLL injection, manual mapping, remote process access, process memory
  modification, arbitrary code execution.

NOVA provides read-only infrastructure only.
