# NOVA Known Limitations

This document lists confirmed limitations of NOVA `0.2.0` (Release Candidate
`v0.2.0-rc.1`). Only limitations verified through code inspection, build
verification, and Release Candidate testing are documented here.

Resolved issues are not described as current defects. Unverified limitations
are not speculated about.

## 1. Platform and environment

- **Windows x64 only.** NOVA has been built and tested exclusively on
  Windows 11 x64 with Visual Studio 2026 Build Tools (MSVC 19.52),
  CMake 4.3.1, and Windows SDK 10.0.26100.0. No cross-platform support has
  been demonstrated.
- **C++20 required.** NOVA compiles with `/Zc:__cplusplus` and
  `CMAKE_CXX_EXTENSIONS OFF`. Earlier language standards are not supported.

## 2. Versioning

- **Subsystem API versions are informational.** The SDK, Loader, and Runtime
  headers each declare their own API version (`0.1.0`) independently of the
  CMake project version (`0.2.0`). These values are metadata only and are not
  used for automatic compatibility enforcement.
- **No ABI stability guarantee.** NOVA makes no claims about binary
  compatibility across versions.

## 3. Loader

- **Read-only module enumeration of the current process only.** NOVA
  enumerates modules loaded in the current process using documented Windows
  APIs. It does not enumerate external processes.
- **Loader references are invalidated by module re-enumeration.** References
  returned by `find_module_by_name()` and `find_module_by_path()` refer to
  internal loader storage. A subsequent call to `enumerate_modules()`
  invalidates previously obtained references. Do not retain a reference
  across re-enumeration; copy the `ModuleInfo` if the data must outlive the
  current enumeration snapshot.
- **Loader lifecycle operations require external synchronization.** The PE
  parsing methods are thread-safe, but concurrent `initialize()`,
  `shutdown()`, and `enumerate_modules()` calls are not safe; the caller must
  externally synchronize them.

## 4. Runtime scheduler

- **TaskScheduler wait and shutdown semantics.** The scheduler uses
  cooperative cancellation via `std::shared_ptr<CancellationState>`. Tasks
  must check cancellation tokens and exit promptly. There is no forced
  termination of running tasks.
- **Scheduler statistics are retained results.** Statistics reflect retained
  results and may not represent real-time state under concurrent access.

## 5. SDK

- **Subsystem accessors require initialization.** `core()`, `loader()`, and
  `runtime()` require the SDK to be initialized and the corresponding
  subsystem to be enabled; otherwise behavior is undefined. Use `try_core()`,
  `try_loader()`, `try_runtime()` for safe nullable access.
- **SDK lifecycle operations require external synchronization.** While the
  SDK handles repeated initialize/shutdown safely, concurrent lifecycle
  operations from multiple threads require external synchronization.

## 6. Configuration

- **ConfigValue scalar accessors silently return defaults on type mismatch.**
  `as_bool()`, `as_int()`, `as_double()`, and `as_string()` read raw member
  storage with no type check. On type mismatch they silently return
  zero-initialized defaults. Callers should check `is_*()` or use `try_as_*`
  for safe nullable access.

## 7. Scope

- **Read-only infrastructure only.** NOVA does not implement a game loader,
  hooking, DLL injection, reverse engineering, game-specific functionality,
  manual mapping, remote process access, process memory modification, or
  arbitrary code execution. It never executes, injects, hooks, or modifies
  executable code.
