# Phase 4.0.5 — Release Candidate Verification

## A. Environment

| Item | Value |
|------|-------|
| OS | Windows 11 x64 |
| CMake | 4.3.1-msvc1 (`C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) |
| C/C++ Compiler | MSVC 19.52.37029 for x64 (`C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.52.37025\bin\Hostx64\x64\cl.exe`) |
| Generator | Visual Studio 18 2026 |
| Architecture | x64 |
| Configuration | Release |
| Language standard | C++20 (`/Zc:__cplusplus`) |
| Compiler flags | `/W4 /WX /permissive- /Zc:__cplusplus` |
| Windows SDK | 10.0.26100.0 |
| Python3 | 3.15.2 (used by CMake for FetchContent/GTest) |

## B. Clean Directories

| Purpose | Path |
|---------|------|
| Source root | `C:\Users\twiborg\Desktop\NoVA` |
| Release build | `C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5` |
| Installation prefix | `C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5-install` |
| External consumer | `C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5-consumer` |

All three directories were newly created; no existing directories or user files were overwritten. No source files, headers, tests, or CMake configuration files were modified during this phase.

## C. Release Configuration

**Command:**

```powershell
cmake -S "C:\Users\twiborg\Desktop\NoVA" -B "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5" -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
```

**Result:** Configuration succeeded. Generated build files in `build/rc-4.0.5`.

**Notes:** CMake emitted a warning that `CMAKE_BUILD_TYPE` was manually specified but not used by the project — this is expected for multi-config generators (Visual Studio), where configuration is selected at build time via `--config Release`. Not a defect.

**Exit code:** 0

## D. Release Build

**Command:**

```powershell
cmake --build "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5" --config Release
```

**Targets built:**
- `nova_core` — static library
- `nova_loader_pe` — static library
- `nova_loader_module` — static library
- `nova_loader` — static library
- `nova_runtime` — static library
- `nova_sdk` — static library
- `nova_core_example` — executable
- `external_example` — executable (examples/external_application)
- `external_consumer` — executable (tests/integration/external_consumer)
- `nova_core_tests` — test executable
- `nova_loader_pe_tests` — test executable
- `nova_loader_module_tests` — test executable
- `nova_loader_tests` — test executable
- `nova_runtime_tests` — test executable
- `nova_sdk_tests` — test executable
- GoogleTest/GMock libraries (`gtest`, `gtest_main`, `gmock`, `gmock_main`)

**Warnings:** None under `/W4 /WX /permissive- /Zc:__cplusplus`.

**Errors:** None.

**Exit code:** 0

## E. Tests

**Command:**

```powershell
ctest --test-dir "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5" -C Release --output-on-failure
```

**Results:**

| Suite | Tests | Status |
|-------|-------|--------|
| `nova_core_tests` | 60 | Passed |
| `nova_loader_pe_tests` | 19 | Passed |
| `nova_loader_module_tests` | 25 | Passed |
| `nova_loader_tests` | 17 | Passed |
| `nova_runtime_tests` | 48 | Passed |
| `nova_sdk_tests` | 47 | Passed |
| **Total** | **216** | **Passed** |

CTest reported: 100% tests passed, 0 tests failed out of 6 suites.

**Discrepancy:** The documented expected total was 215. The authoritative source count (verified by counting `TEST()` registrations across all test files) is 216:
- `nova_core_tests`: 60 (documented as 59 — off by one)
- `nova_loader_pe_tests`: 19
- `nova_loader_module_tests`: 25
- `nova_loader_tests`: 17
- `nova_runtime_tests`: 48
- `nova_sdk_tests`: 47
- Total: 216

All 216 tests pass. The discrepancy is a documentation error in the README test count table, not a code defect.

## F. Installation

**Command:**

```powershell
cmake --install "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5" --prefix "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5-install" --config Release
```

**Result:** Installation succeeded.

**Installed public headers** (`include/nova/`):
- `nova/core/`: config.h, error.h, init.h, logger.h
- `nova/loader/`: loader.h, loader_config.h, loader_result.h, loader_version.h
- `nova/loader/module/`: module_enumerator.h, module_error.h, module_info.h
- `nova/loader/pe/`: pe_headers.h, pe_parser.h, pe_section.h, pe_types.h
- `nova/runtime/`: runtime.h, runtime_config.h, runtime_context.h, runtime_diagnostics.h, runtime_error.h, runtime_state.h
- `nova/runtime/event/`: event.h, event_dispatcher.h
- `nova/runtime/task/`: cancellation_token.h, task.h, task_scheduler.h
- `nova/sdk/`: application.h, application_context.h, sdk.h, sdk_config.h, sdk_error.h, sdk_result.h, sdk_version.h
- `nova/nova.h`

**Installed static libraries** (`lib/`):
- `nova_core.lib`, `nova_loader_pe.lib`, `nova_loader_module.lib`, `nova_loader.lib`, `nova_runtime.lib`, `nova_sdk.lib`

**Installed CMake package files** (`lib/cmake/nova/`):
- `novaConfig.cmake`
- `novaConfigVersion.cmake` (PACKAGE_VERSION "0.2.0")
- `novaTargets.cmake`
- `novaTargets-release.cmake`

**Imported targets** (all `STATIC IMPORTED`, `cxx_std_20`, verified in `novaTargets.cmake`):
- `nova::nova_core`
- `nova::nova_loader_pe`
- `nova::nova_loader_module`
- `nova::nova_loader`
- `nova::nova_runtime`
- `nova::nova_sdk`

**Transitive linking:** `nova::nova_sdk` → `nova::nova_runtime` → `nova::nova_loader` → `nova::nova_loader_pe` / `nova::nova_loader_module` → `nova::nova_core`

**Prefix handling:** Library locations use `${_IMPORT_PREFIX}/lib/*.lib`, computed relative to the installed `novaTargets.cmake` file — no dependency on the source tree or previous build directories.

## G. External Consumer

**Consumer project:** `C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5-consumer` (outside NOVA source tree)

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.24)
project(MyConsumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(NOVA REQUIRED)
add_executable(myconsumer src/main.cpp)
target_link_libraries(myconsumer PRIVATE nova::nova_sdk)
```

**main.cpp:** Includes `nova/sdk/sdk.h`, calls `Sdk::initialize()` and `Sdk::shutdown()`.

**Configure command:**

```powershell
cmake -S <consumer> -B <consumer>/build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5-install"
```

**Result:** `find_package(NOVA REQUIRED)` found the installed package successfully. Configuration succeeded.

**Build command:**

```powershell
cmake --build <consumer>/build --config Release
```

**Result:** Built `myconsumer.exe` with no warnings or errors.

**Run command:**

```powershell
<consumer>/build/Release/myconsumer.exe
```

**Output:**
```
[INFO] Initializing NOVA Core
[INFO] Shutting down NOVA Core
External consumer: SDK initialized successfully
```

**Exit code:** 0

**Verification:** The consumer linked only against the installed package (`nova::nova_sdk` via `CMAKE_PREFIX_PATH`), used installed headers under `include/nova`, and did not reference any source-tree or build-tree artifacts.

## H. Source Modifications

**None.** No source files, public headers, tests, CMake configuration files, or example files were modified during this phase. Only new directories were created under `build/`.

## I. Remaining Limitations

1. **Test count documentation discrepancy:** The README documents a total of 215 tests, but the actual count is 216 (off by one in `nova_core_tests`: documented as 59, actual is 60). This is a documentation error only; all tests pass. The README test-count table should be updated to reflect 216 total / 60 for `nova_core_tests`.

2. **Multi-config generator note:** `CMAKE_BUILD_TYPE` is not honored at configure time for the Visual Studio generator; configuration must be specified via `--config` at build/test time. This is standard CMake behavior, not a NOVA defect, but consumers should be aware.

3. **Version 0.2.0:** The package version is 0.2.0. The `novaConfigVersion.cmake` uses `SameMajorVersion` compatibility, so `find_package(NOVA)` without a version requirement will match, but version-constrained requests require matching major version.

4. **No public release claimed:** This verification establishes Release Candidate readiness only. No tags, packages, or public artifacts were created.

## J. Final Verdict

**PASS — Release Candidate Ready**

**Evidence:**
- Fresh Release configuration succeeded (exit code 0).
- Full Release build succeeded with zero warnings under `/W4 /WX` and zero errors (exit code 0).
- All six test suites passed; CTest reported 100% pass, 0 failures.
- Total test count is 216 (all pass; the documented 215 is a documentation off-by-one error).
- Fresh installation into a clean prefix succeeded; all headers, static libraries, and CMake package files present.
- All six imported targets verified with correct names, `STATIC IMPORTED` type, `cxx_std_20` feature, and Release configuration.
- External consumer project (outside source tree) successfully used `find_package(NOVA REQUIRED)`, linked `nova::nova_sdk`, compiled as C++20, and ran with exit code 0.
- No source-tree or build-tree artifacts were used by the consumer — only the installed package.
- No source files, headers, tests, or CMake files were modified.

The one identified issue (test count documentation discrepancy) is purely documentary and does not affect Release Candidate readiness.
