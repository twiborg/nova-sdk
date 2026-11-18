# NOVA Testing Guide

This document is a tester-facing guide for independently verifying the NOVA
`0.2.0` Release Candidate (`v0.2.0-rc.1`). It describes how to build, install,
and consume NOVA as an external CMake dependency, and provides concrete test
scenarios for validating the public API.

This guide is documentation only. It does not modify NOVA source code or the
release candidate.

## 1. Requirements

### Verified environment

- Windows 10/11 x64 (verified on Windows 11 x64)
- Visual Studio 2022 or 2026 Build Tools with "Desktop development with C++"
  workload (verified with Visual Studio 18 2026, MSVC 19.52)
- Windows 10/11 SDK (verified 10.0.26100.0)
- CMake 3.24 or newer (verified 4.3.1)

### Language and architecture

- C++20 (required, extensions off)
- x64 only (NOVA is verified as Windows x64 infrastructure; no cross-platform
  support has been demonstrated)

## 2. Installation

### Extract the release archive

Extract `NOVA-0.2.0.zip` to a directory of your choice. The archive contains:

```
NOVA-0.2.0/
├── LICENSE
├── README.md
├── CHANGELOG.md
├── RELEASE_NOTES.md
├── include/nova/          # Public headers
├── lib/                   # Static libraries (.lib)
└── lib/cmake/NOVA/        # CMake package config files
```

### Consume from an external CMake project

Create a CMake project outside the NOVA source tree:

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(NOVA REQUIRED)

add_executable(mytest src/main.cpp)
target_link_libraries(mytest PRIVATE nova::nova_sdk)
```

Configure, pointing CMake at the extracted NOVA package:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<path-to-NOVA-0.2.0>"
cmake --build build --config Release
```

The following `STATIC IMPORTED` targets are available:

- `nova::nova_core`
- `nova::nova_loader_pe`
- `nova::nova_loader_module`
- `nova::nova_loader`
- `nova::nova_runtime`
- `nova::nova_sdk`

`nova::nova_sdk` transitively links `nova::nova_runtime`,
`nova::nova_loader`, and `nova::nova_core`.

Public headers are installed under `include/nova/` and are included as:

```cpp
#include "nova/sdk/sdk.h"
#include "nova/loader/loader.h"
#include "nova/runtime/runtime.h"
#include "nova/core/init.h"
```

## 3. Verification Scenarios

All examples use the public headers only. They do not rely on any internal
implementation detail.

### 3.1 Package discovery

**Goal:** Verify that `find_package(NOVA REQUIRED)` locates the installed
package and that all imported targets resolve.

**Steps:**
1. Create a CMake project as shown in Section 2.
2. Set `CMAKE_PREFIX_PATH` to the extracted `NOVA-0.2.0` directory.
3. Configure and build.

**Expected result:**
- `find_package(NOVA REQUIRED)` succeeds.
- All six imported targets are available.
- `nova::nova_sdk` links successfully.

### 3.2 Header inclusion

**Goal:** Verify that public headers are self-contained and compile.

**Steps:**
1. Include each public header in a translation unit:
   ```cpp
   #include "nova/sdk/sdk.h"
   #include "nova/loader/loader.h"
   #include "nova/runtime/runtime.h"
   #include "nova/core/init.h"
   #include "nova/loader/pe/pe_parser.h"
   #include "nova/loader/module/module_enumerator.h"
   ```
2. Compile with `/W4 /WX /permissive- /Zc:__cplusplus` as C++20.

**Expected result:**
- Compilation succeeds with no warnings or errors.

### 3.3 SDK initialization and shutdown

**Goal:** Verify the SDK lifecycle.

**Steps:**
1. Construct a `nova::sdk::Sdk`.
2. Call `initialize()` with default configuration.
3. Check `is_initialized()` returns `true`.
4. Call `shutdown()`.
5. Check `is_initialized()` returns `false`.

**Expected result:**
- `initialize()` returns `SdkResult{ok: true}`.
- `is_initialized()` returns `true` after initialize, `false` after shutdown.
- No crashes or exceptions.

### 3.4 Repeated initialization/shutdown

**Goal:** Verify that repeated initialize/shutdown cycles are safe.

**Steps:**
1. Construct a `nova::sdk::Sdk`.
2. Call `initialize()`, then `shutdown()`.
3. Repeat steps 2 two more times.
4. Verify `is_initialized()` returns `false` after the final shutdown.

**Expected result:**
- All `initialize()` calls return `SdkResult{ok: true}`.
- All `shutdown()` calls complete without error.
- `is_initialized()` returns `false` after each shutdown.

### 3.5 Safe nullable subsystem accessors

**Goal:** Verify that `try_core()`, `try_loader()`, and `try_runtime()`
return `nullptr` before initialization.

**Steps:**
1. Construct a `nova::sdk::Sdk` (do not initialize).
2. Call `try_core()`, `try_loader()`, `try_runtime()`.
3. Verify each returns `nullptr`.

**Expected result:**
- All three accessors return `nullptr` before initialization.

### 3.6 Subsystem access after initialization

**Goal:** Verify that subsystem accessors return valid references after
initialization with the corresponding subsystem enabled.

**Steps:**
1. Construct a `nova::sdk::Sdk` with all subsystems enabled (default).
2. Call `initialize()`.
3. Call `core()`, `loader()`, `runtime()`.
4. Verify each returns a non-null reference.
5. Call `shutdown()`.

**Expected result:**
- All three accessors return valid references after initialization.

### 3.7 PE file parsing (read-only)

**Goal:** Verify that the PE parser can read a PE32 or PE32+ file without
executing or modifying it.

**Steps:**
1. Initialize the SDK with Loader enabled.
2. Call `sdk.loader().parse_pe_file("some_valid_pe32.exe")`.
3. Check `PeResult.ok`.
4. If `ok`, inspect `peResult.pe.nt_headers.is_pe32_plus` to distinguish
   PE32 from PE32+.
5. Shut down the SDK.

**Expected result:**
- `parse_pe_file()` returns `PeResult{ok: true}` for a valid PE file.
- The `is_pe32_plus` field correctly reflects the PE format.
- No code is executed, injected, or modified (NOVA is read-only).

### 3.8 Invalid PE file rejection

**Goal:** Verify that the PE parser rejects malformed PE files.

**Steps:**
1. Initialize the SDK with Loader enabled.
2. Call `sdk.loader().parse_pe_file("not_a_pe_file.exe")` on a non-PE file.
3. Check `PeResult.ok`.

**Expected result:**
- `parse_pe_file()` returns `PeResult{ok: false}` with an appropriate error.
- No crash or exception.

### 3.9 Module enumeration (current process only)

**Goal:** Verify that module enumeration works on the current process.

**Steps:**
1. Initialize the SDK with Loader enabled.
2. Call `sdk.loader().enumerate_modules()`.
3. Check `ModuleEnumeratorResult.ok`.
4. If `ok`, verify `modules.size() > 0` (at least the current executable).

**Expected result:**
- `enumerate_modules()` returns `ModuleEnumeratorResult{ok: true}`.
- At least one module is reported.
- Only modules in the current process are enumerated (NOVA does not
  enumerate external processes).

### 3.9 Module name lookup

**Goal:** Verify `find_module_by_name()` locates a loaded module.

**Steps:**
1. Initialize the SDK with Loader enabled.
2. Call `enumerate_modules()`.
3. Call `find_module_by_name()` with the name of a known loaded module
   (e.g., the current executable's base name).
4. Verify the result is non-nullopt and contains expected fields.

**Expected result:**
- `find_module_by_name()` returns `std::optional` containing a
  `ModuleInfo` for a known module.
- The reference is valid only until the next `enumerate_modules()` call;
  do not retain it across re-enumeration.

### 3.10 Runtime lifecycle

**Goal:** Verify the Runtime lifecycle states.

**Steps:**
1. Initialize the SDK with Runtime enabled.
2. Obtain `sdk.runtime()`.
3. Call `initialize()` then `start()`.
4. Check `is_running()` returns `true`.
5. Call `stop()`.
6. Check `is_running()` returns `false`.
7. Call `shutdown()`.

**Expected result:**
- `initialize()` and `start()` return `RuntimeResult{ok: true}`.
- `is_running()` returns `true` after start, `false` after stop.
- `shutdown()` completes without error.

### 3.11 Error handling

**Goal:** Verify that invalid operations return structured errors rather
than throwing exceptions.

**Steps:**
1. Construct a `nova::sdk::Sdk` (do not initialize).
2. Call `sdk.core()`, `sdk.loader()`, or `sdk.runtime()`.
3. Observe undefined behavior per the API contract (these require
   initialization).

**Expected result:**
- Accessing subsystems before initialization is undefined behavior per the
  public API contract. Use `try_core()`, `try_loader()`, `try_runtime()`
  for safe nullable access instead.
- All SDK lifecycle operations return `SdkResult` with `ok` and `error`
  fields rather than throwing.

### 3.2 Scheduler behavior

**Goal:** Verify that the task scheduler accepts and executes tasks.

**Steps:**
1. Initialize the SDK with Runtime enabled.
2. Obtain `sdk.runtime().scheduler()`.
3. Submit a task via the scheduler's public API.
4. Verify the task executes.

**Expected result:**
- Tasks submitted to the scheduler execute as expected.
- Task priority and FIFO ordering within a priority are respected.

> The exact scheduler task-submission API is exposed through
> `nova::runtime::TaskScheduler`. Refer to `include/nova/runtime/task/task_scheduler.h`
> for the public method signatures.

## 4. Reporting Results

When reporting test results, include the following information:

- **Environment:** OS version and architecture (e.g., Windows 11 x64).
- **Compiler:** MSVC version (e.g., 19.52.37029).
- **CMake:** Version (e.g., 4.3.1).
- **NOVA version:** `0.2.0` (tag `v0.2.0-rc.1`).
- **Build configuration:** Release with `/W4 /WX /permissive- /Zc:__cplusplus`.
- **Steps to reproduce:** Exact commands and code used.
- **Expected result:** What you expected to happen.
- **Actual result:** What actually happened.
- **Logs and error messages:** Full compiler output, test output, and any
  error codes or messages returned by NOVA APIs.
- **Minimal reproduction:** The smallest code snippet that demonstrates the
  issue, if applicable.

Use the feedback template in `FEEDBACK.md` when submitting issues.

## 5. Scope Reminder

NOVA is read-only infrastructure. The following are explicitly out of scope
and not tested:

- Game loader
- Hooking / DLL injection
- Reverse engineering
- Game-specific functionality
- DLL injection, manual mapping, remote process access, process memory
  modification, arbitrary code execution

NOVA does not execute, inject, hook, or modify executable code.
