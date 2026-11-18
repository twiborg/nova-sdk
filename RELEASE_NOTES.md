# NOVA Release Notes — 0.2.0

## Overview

NOVA is a Windows x64 C++20 infrastructure library providing read-only
infrastructure components: Core (logging, configuration, errors,
initialization), Loader (PE parsing, module enumeration, unified loader API),
Runtime (lifecycle, task scheduling, event dispatching, diagnostics), and SDK
(unified facade, Application class).

NOVA does NOT implement: game loader, hooking, DLL injection, reverse
engineering, game-specific functionality, DLL injection, manual mapping,
remote process access, process memory modification, or arbitrary code
execution.

NOVA provides read-only infrastructure only. It does not execute, inject,
hook, or modify executable code.

## Architecture

NOVA consists of four major components:

- **nova_core** — Logging, JSON configuration, error handling,
  initialization.
- **nova_loader** — Read-only PE32/PE32+ parser, read-only module
  enumeration, unified Loader facade.
- **nova_runtime** — Lifecycle management, priority-aware task scheduler,
  typed event dispatcher, diagnostics.
- **nova_sdk** — Unified SDK facade composing Core, Loader, and Runtime;
  Application class; configuration; error handling; result types; versioning.

## Capabilities

The following capabilities are implemented and verified:

- Thread-safe structured logging with six levels.
- JSON-based configuration with typed and safe nullable accessors.
- `NovaError` exception type with structured `ErrorCode` classification.
- RAII core initialization and shutdown state tracking.
- Read-only PE32/PE32+ parsing with signature, boundary, and integer
  overflow validation; safe RVA-to-offset conversion.
- Read-only enumeration of modules loaded in the current process using
  documented Windows APIs.
- Unified Loader lifecycle management over PE parser and module enumeration.
- Runtime lifecycle (`initialize`/`start`/`stop`/`shutdown`).
- Priority-aware task scheduler with bounded queue and cooperative
  cancellation.
- Typed event dispatcher with subscription management, filtering, nested
  dispatch, exception isolation, and statistics.
- Runtime diagnostics snapshot.
- Unified SDK facade with subsystem accessors and safe nullable accessors.
- Application class with full application-level lifecycle.
- CMake package installation with `find_package(NOVA)` and imported targets
  `nova::nova_core`, `nova::nova_loader_pe`, `nova::nova_loader_module`,
  `nova::nova_loader`, `nova::nova_runtime`, `nova::nova_sdk`.

## Technical requirements

- Windows 10/11 x64 (verified on Windows 11 x64).
- Visual Studio 2022 or 2026 (MSVC v143/v144) with "Desktop development
  with C++" workload (verified with Visual Studio 18 2026, MSVC 19.52).
- Windows 10/11 SDK (verified 10.0.26100.0).
- CMake 3.24 or newer (verified 4.3.1).
- C++20.

## Build

Configure and build in Release:

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --config Release
```

Strict compiler flags: `/W4 /WX /permissive- /Zc:__cplusplus`.

## Installation and integration

Install NOVA to a prefix:

```
cmake --install build --prefix <install-dir>
```

Consume NOVA from an external CMake project:

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(NOVA REQUIRED)

add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE nova::nova_sdk)
```

When NOVA was installed to a custom prefix, point CMake at it:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH="<install-dir>"
```

The installed package exposes these `STATIC IMPORTED` targets:

- `nova::nova_core`
- `nova::nova_loader_pe`
- `nova::nova_loader_module`
- `nova::nova_loader`
- `nova::nova_runtime`
- `nova::nova_sdk`

`nova::nova_sdk` transitively links `nova::nova_runtime`,
`nova::nova_loader`, and `nova::nova_core`.

## Testing

Six test suites, **216 tests total**, all passing under Release with
`/W4 /WX`:

| Suite | Tests |
|-------|-------|
| nova_core_tests | 60 |
| nova_loader_pe_tests | 19 |
| nova_loader_module_tests | 25 |
| nova_loader_tests | 17 |
| nova_runtime_tests | 48 |
| nova_sdk_tests | 47 |
| **Total** | **216** |

All suites passed during Release Candidate verification (Phase 4.0.5).

## Versioning

NOVA uses two independent versioning levels:

- **Project version** (`0.2.0`) — overall NOVA project and distribution
  release version, declared in the root `CMakeLists.txt` and used by CMake
  package versioning.
- **Subsystem API versions** (`0.1.0`) — individual public subsystem API
  compatibility versions for `nova_sdk`, `nova_loader`, and `nova_runtime`,
  declared in their respective public headers.

Project and subsystem API versions are intentionally independent and may
evolve at different rates. API version values are informational metadata and
are not currently used for automatic compatibility enforcement.

## License

NOVA is licensed under the MIT License. See [LICENSE](LICENSE) for details.

Copyright (c) 2026 Artyom Filin (twiborg).

## Limitations

- Verified environment: Windows 11 x64 with Visual Studio 2026 Build Tools,
  MSVC 19.52, CMake 4.3.1, Windows SDK 10.0.26100.0. No cross-platform
  support has been demonstrated.
- No ABI stability guarantee.
- No automatic API compatibility enforcement.
- No claim of production readiness.
- This document describes a Release Candidate; no official public release
  has been made.

## Explicitly unsupported scope

The following are explicitly NOT implemented and are out of scope:

- Game loader.
- Hooking / DLL injection.
- Reverse engineering.
- Game-specific functionality.
- DLL injection, manual mapping, remote process access, process memory
  modification, arbitrary code execution.

NOVA is read-only infrastructure only.
