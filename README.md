# NOVA

NOVA is an independent Geometry Dash modding platform.

## Status

This release establishes **NOVA Core**, **NOVA Loader** (PE parser, module enumeration, unified loader API), **NOVA Runtime** (lifecycle, task scheduling, event dispatching), and **NOVA SDK** (unified public developer interface).

The following are intentionally NOT implemented yet:
- Game loader
- Hooking / DLL injection
- Reverse engineering
- Game-specific functionality

## Technology

- C++20
- CMake (3.24+)
- MSVC
- Windows 10/11 x64
- GoogleTest (fetched automatically via CMake FetchContent)

## Prerequisites

- Visual Studio 2022 or 2026 (MSVC v143/v144) with the "Desktop development with C++" workload
- Windows 10/11 SDK
- CMake 3.24 or newer
- Git (to clone submodules / fetch dependencies)

## Project Structure

```
NoVA/
├── CMakeLists.txt              # Root build configuration
├── CMakePresets.json           # CMake presets for MSVC x64
├── README.md                   # This file
├── cmake/                      # CMake helper modules (future)
├── include/nova/core/          # Public Core headers
│   ├── config.h
│   ├── error.h
│   ├── init.h
│   └── logger.h
├── include/nova/loader/        # Public Loader headers
│   ├── loader.h
│   ├── loader_config.h
│   ├── loader_result.h
│   └── loader_version.h
├── include/nova/loader/pe/     # Public Loader (PE parser) headers
│   ├── pe_types.h
│   ├── pe_headers.h
│   ├── pe_section.h
│   └── pe_parser.h
├── include/nova/loader/module/ # Public Loader (module enumeration) headers
│   ├── module_enumerator.h
│   ├── module_error.h
│   └── module_info.h
├── include/nova/runtime/       # Public Runtime headers
│   ├── event/
│   │   ├── event.h
│   │   └── event_dispatcher.h
│   ├── runtime.h
│   ├── runtime_config.h
│   ├── runtime_context.h
│   ├── runtime_diagnostics.h
│   ├── runtime_error.h
│   ├── runtime_state.h
│   └── task/
│       ├── cancellation_token.h
│       ├── task.h
│       └── task_scheduler.h
├── include/nova/sdk/           # Public SDK headers
│   ├── application.h
│   ├── application_context.h
│   ├── sdk.h
│   ├── sdk_config.h
│   ├── sdk_error.h
│   ├── sdk_result.h
│   └── sdk_version.h
├── src/nova/core/              # Core library sources
│   ├── CMakeLists.txt
│   ├── config.cpp
│   ├── error.cpp
│   ├── init.cpp
│   ├── logger.cpp
│   └── main.cpp                # Minimal example executable
├── src/nova/loader/            # Loader library sources
│   ├── CMakeLists.txt
│   ├── loader.cpp
│   ├── loader_result.cpp
│   ├── loader_version.cpp
│   ├── module/
│   │   ├── CMakeLists.txt
│   │   ├── module_enumerator.cpp
│   │   ├── module_error.cpp
│   │   └── module_info.cpp
│   └── pe/
│       ├── CMakeLists.txt
│       └── pe_parser.cpp
├── src/nova/runtime/           # Runtime library sources
│   ├── CMakeLists.txt
│   ├── runtime.cpp
│   ├── runtime_context.cpp
│   ├── runtime_error.cpp
│   ├── event/
│   │   └── event_dispatcher.cpp
│   └── task/
│       └── task_scheduler.cpp
├── src/nova/sdk/               # SDK library sources
│   ├── CMakeLists.txt
│   ├── application.cpp
│   ├── sdk.cpp
│   ├── sdk_error.cpp
│   └── sdk_version.cpp
└── tests/                      # Unit tests
    ├── CMakeLists.txt
    ├── src/core/
    │   ├── error_test.cpp
    │   ├── config_test.cpp
    │   ├── init_test.cpp
    │   └── logger_test.cpp
    ├── src/loader/
    │   ├── loader/
    │   │   ├── CMakeLists.txt
│   │   │   └── loader_test.cpp
    │   ├── module/
    │   │   ├── CMakeLists.txt
│   │   │   └── module_enumerator_test.cpp
    │   └── pe/
    │       ├── CMakeLists.txt
│   │       └── pe_parser_test.cpp
    └── src/runtime/
        ├── CMakeLists.txt
        ├── event/
        │   └── event_dispatcher_test.cpp
        ├── runtime_context_test.cpp
        ├── runtime_test.cpp
        └── task/
            └── task_scheduler_test.cpp
    └── src/sdk/
        ├── CMakeLists.txt
        ├── application_test.cpp
        └── sdk_test.cpp
```

## Architecture

`nova_core` is a standalone, dependency-minimal library. It is intentionally independent of future **Loader**, **Runtime**, **SDK**, and **Mod Manager** modules. No game-specific or reverse-engineering code lives here.

### Dependency Direction

```
nova_core          (standalone)
    ↑
nova_loader        (depends on nova_core)
    ↑
nova_runtime       (depends on nova_core, nova_loader)
    ↑
nova_sdk           (depends on nova_core, nova_loader, nova_runtime)
```

### Module Responsibilities

**Core** (`nova_core`):
- **Logging** (`logger.h`/`logger.cpp`) — thread-safe structured logger with six levels: Trace, Debug, Info, Warning, Error, Critical.
- **Configuration** (`config.h`/`config.cpp`) — JSON-based configuration manager with typed access.
- **Error handling** (`error.h`/`error.cpp`) — `NovaError` exception type with `ErrorCode` classification.
- **Initialization** (`init.h`/`init.cpp`) — RAII core startup/shutdown state tracking.

**Loader** (`nova_loader`, `nova_loader_pe`, `nova_loader_module`):
- **PE Parser** (`nova_loader_pe`) — read-only PE parser. Inspects PE files without executing, loading, injecting, or modifying code. Supports PE32 and PE32+, parses DOS/COFF/optional headers and the section table, validates signatures and boundaries, detects integer overflow, and provides safe RVA-to-offset conversion and section lookup. Depends only on `nova_core`.
- **Module Enumeration** (`nova_loader_module`) — read-only enumeration of modules loaded in the current process using documented Windows APIs (`EnumProcessModulesEx`, `GetModuleInformation`, `GetModuleFileNameExW`, `GetModuleBaseNameW`). Does not enumerate external processes or perform injection/hooking.
- **Loader** (`nova_loader`) — unified facade over PE parser and module enumeration with lifecycle management.

**Runtime** (`nova_runtime`):
- **Lifecycle** (`runtime.h`) — `Core`-style initialized/running state management with `initialize()`/`start()`/`stop()`/`shutdown()`.
- **Configuration** (`runtime_config.h`) — `RuntimeConfig` with logging, scheduler, and behavior options.
- **Context** (`runtime_context.h`) — immutable snapshot of runtime identity, version, state, config, and creation timestamp.
- **Error Handling** (`runtime_error.h`) — `RuntimeError`/`RuntimeErrorCode` with structured error codes.
- **Task System** (`task/task.h`, `task/task_scheduler.h`) — `TaskScheduler` with priority-aware queuing (Critical > High > Normal > Low), FIFO within priorities, `std::thread`-based workers, bounded queue, `std::shared_ptr<CancellationState>`-based cooperative cancellation, and thread-safe statistics.
- **Event System** (`event/event.h`, `event/event_dispatcher.h`) — `EventDispatcher` with typed events via `std::any`, subscription management, event filtering, subscription metadata, nested dispatch support, exception isolation, and dispatch statistics.
- **Diagnostics** (`runtime_diagnostics.h`) — `RuntimeDiagnostics` snapshot with state, uptime, scheduler statistics, event statistics, and configuration summary.

**SDK** (`nova_sdk`):
- **Unified Facade** (`sdk.h`) — `Sdk` class composing Core, Loader, and Runtime with lifecycle management and typed accessors (`core()`, `loader()`, `runtime()`).
- **Configuration** (`sdk_config.h`) — `SdkConfig` aggregating Core, Loader, and Runtime configuration with enable/disable flags per subsystem.
- **Error Handling** (`sdk_error.h`) — `SdkError`/`SdkErrorCode` with structured error codes and `SdkResult` for lifecycle operations.
- **Result Types** (`sdk_result.h`) — `SdkResultT<T>` template for value-returning operations.
- **Version** (`sdk_version.h`) — `SdkVersion` and `sdk_version_string()`.
- **Application** (`application.h`, `application_context.h`) — `Application` class with full lifecycle (initialize/start/stop/shutdown), `ApplicationContext` immutable snapshot, and SDK access.

## Build

### Using CMake presets (recommended)

Open a developer command prompt for x64 (e.g. "x64 Native Tools Command Prompt for VS 2022 or VS 2026"):

```powershell
# Configure
cmake --preset msvc-x64-debug

# Build
cmake --build --preset msvc-x64-debug

# Run tests
ctest --preset msvc-x64-debug
```

### Manual configuration

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build --config Debug --output-on-failure
```

### Build targets

| Target | Description |
|--------|-------------|
| `nova_core` | Core static library |
| `nova_core_example` | Minimal executable that initializes NOVA Core |
| `nova_core_tests` | GoogleTest unit test executable |
| `nova_loader_pe` | PE parser static library |
| `nova_loader_pe_tests` | PE parser unit test executable |
| `nova_loader_module` | Module enumeration static library |
| `nova_loader_module_tests` | Module enumeration unit test executable |
| `nova_loader` | Unified Loader static library |
| `nova_loader_tests` | Loader unit test executable |
| `nova_runtime` | Runtime static library |
| `nova_runtime_tests` | Runtime unit test executable |
| `nova_sdk` | SDK static library |
| `nova_sdk_tests` | SDK unit test executable |

## SDK Usage Example

```cpp
#include "nova/sdk/sdk.h"
#include "nova/sdk/sdk_config.h"
#include <iostream>

int main() {
    nova::sdk::SdkConfig config;
    config.name = "MyApp";
    config.version = "1.0.0";
    config.enable_core = true;
    config.enable_loader = true;
    config.enable_runtime = true;

    nova::sdk::Sdk sdk;
    auto result = sdk.initialize(config);
    if (!result.ok) {
        std::cerr << "SDK init failed: " << result.error.message << std::endl;
        return 1;
    }

    // Parse a PE file
    auto peResult = sdk.loader().parse_pe_file("example.exe");
    if (peResult.ok) {
        std::cout << "PE machine: "
                  << (peResult.pe.nt_headers.is_pe32_plus ? "PE32+" : "PE32")
                  << std::endl;
    }

    // Enumerate modules
    auto modResult = sdk.loader().enumerate_modules();
    if (modResult.ok) {
        std::cout << "Loaded modules: " << modResult.modules.size() << std::endl;
    }

    // Start runtime task scheduler
    auto startResult = sdk.runtime().start();
    if (startResult.ok) {
        // Submit tasks via sdk.runtime().scheduler()
    }

    sdk.shutdown();
    return 0;
}
```

Alternatively, use the `Application` class for a higher-level lifecycle:

```cpp
#include "nova/sdk/application.h"
#include "nova/sdk/sdk_config.h"

int main() {
    nova::sdk::Application app;
    auto initResult = app.initialize();
    if (!initResult.ok) return 1;

    app.start();
    // ... application work ...
    app.stop();
    app.shutdown();
    return 0;
}
```

## Consuming NOVA as an installed package

NOVA can be consumed as an external dependency once it has been built and
installed (see the [installation](README.md#installation) step below). The
installed package provides CMake config files and imported targets.

### Prerequisites

- Install NOVA to a prefix (e.g. `cmake --install build --prefix <install-dir>`).
- The consuming project must use CMake 3.24+ and compile with C++20.

### find_package

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

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="<install-dir>"
```

### Imported targets

The installed package exposes these `STATIC IMPORTED` targets:

- `nova::nova_core`
- `nova::nova_loader_pe`
- `nova::nova_loader_module`
- `nova::nova_loader`
- `nova::nova_runtime`
- `nova::nova_sdk`

`nova::nova_sdk` transitively links `nova::nova_runtime`, `nova::nova_loader`,
and `nova::nova_core`, so linking `nova::nova_sdk` is sufficient for most
applications. The package installs static libraries (`.lib`) and public headers
under `include/nova`.

### Minimal C++ example

```cpp
#include "nova/sdk/sdk.h"

int main() {
    nova::sdk::SdkConfig config;
    config.name = "MyApp";
    auto result = nova::sdk::Sdk{}.initialize(config);
    if (!result.ok) return 1;
    return 0;
}
```

### Bundled examples

The examples bundled with NOVA (`src/nova/core/main.cpp`,
`examples/external_application/src/main.cpp`,
`tests/integration/external_consumer/src/main.cpp`) are built **in-tree** as
part of the NOVA source tree. They link against NOVA's source-tree targets
(e.g. `nova::sdk`) and use source-tree include paths. They are not standalone
consumers of an installed NOVA package. To consume an installed package from an
external project, use the `find_package(NOVA)` approach shown above.

## Running the example

```powershell
.\build\nova_core_example.exe
```

## Running tests

```powershell
ctest --test-dir build --config Debug --output-on-failure
```

## Test Results

| Suite | Tests | Status |
|-------|-------|--------|
| `nova_core_tests` | 60 | Passed |
| `nova_loader_pe_tests` | 19 | Passed |
| `nova_loader_module_tests` | 25 | Passed |
| `nova_loader_tests` | 17 | Passed |
| `nova_runtime_tests` | 48 | Passed |
| `nova_sdk_tests` | 47 | Passed |
| **Total** | **216** | **Passed** |

Test counts are derived from the compiled GoogleTest binaries (Debug configuration, `/W4 /WX`) and verified against the test source `TEST(` registrations. All six suites pass under `ctest --test-dir build --config Debug`.

## License

NOVA is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Versioning

NOVA uses two independent versioning levels:

- **Project version** (`0.2.0`) — the overall NOVA project and distribution
  release version, declared in the root `CMakeLists.txt` and used by CMake
  package versioning.
- **Subsystem API versions** (`0.1.0`) — individual public subsystem API
  compatibility versions for `nova_sdk`, `nova_loader`, and `nova_runtime`,
  declared in their respective public headers.

Project and subsystem API versions are intentionally independent and may
evolve at different rates. API version values are informational metadata and
are not currently used for automatic compatibility enforcement. No ABI
stability or backward compatibility is guaranteed.