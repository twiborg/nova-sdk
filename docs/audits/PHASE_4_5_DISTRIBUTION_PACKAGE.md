# Phase 4.5 — Distribution Package Preparation

Prepared a clean, reproducible distribution package for NOVA `0.2.0` that
allows an external CMake project to consume NOVA without accessing the NOVA
source tree.

This phase is packaging and verification only. No new functionality was added.

## Distribution Layout

```
NOVA-0.2.0/
├── LICENSE
├── README.md
├── CHANGELOG.md
├── RELEASE_NOTES.md
├── include/
│   └── nova/
│       ├── nova.h
│       ├── nova_version.h.in
│       ├── core/
│       │   ├── config.h
│       │   ├── error.h
│       │   ├── init.h
│       │   └── logger.h
│       ├── loader/
│       │   ├── loader.h
│       │   ├── loader_config.h
│       │   ├── loader_result.h
│       │   └── loader_version.h
│       ├── loader/module/
│       │   ├── module_enumerator.h
│       │   ├── module_error.h
│       │   └── module_info.h
│       ├── loader/pe/
│       │   ├── pe_headers.h
│       │   ├── pe_parser.h
│       │   ├── pe_section.h
│       │   └── pe_types.h
│       ├── runtime/
│       │   ├── runtime.h
│       │   ├── runtime_config.h
│       │   ├── runtime_context.h
│       │   ├── runtime_diagnostics.h
│       │   ├── runtime_error.h
│       │   └── runtime_state.h
│       ├── runtime/event/
│       │   ├── event.h
│       │   └── event_dispatcher.h
│       ├── runtime/task/
│       │   ├── cancellation_token.h
│       │   ├── task.h
│       │   └── task_scheduler.h
│       └── sdk/
│           ├── application.h
│           ├── application_context.h
│           ├── sdk.h
│           ├── sdk_config.h
│           ├── sdk_error.h
│           ├── sdk_result.h
│           └── sdk_version.h
├── lib/
│   ├── nova_core.lib
│   ├── nova_loader_pe.lib
│   ├── nova_loader_module.lib
│   ├── nova_loader.lib
│   ├── nova_runtime.lib
│   └── nova_sdk.lib
└── lib/cmake/NOVA/
    ├── novaConfig.cmake
    ├── novaConfigVersion.cmake
    ├── novaTargets.cmake
    └── novaTargets-release.cmake
```

## Files Staged

| Category | Count | Details |
|----------|-------|---------|
| Documentation | 4 | LICENSE, README.md, CHANGELOG.md, RELEASE_NOTES.md |
| Public headers | 36 | Under `include/nova/` (including `nova_version.h.in` template) |
| Static libraries | 6 | `nova_core.lib`, `nova_loader_pe.lib`, `nova_loader_module.lib`, `nova_loader.lib`, `nova_runtime.lib`, `nova_sdk.lib` |
| CMake package files | 4 | `novaConfig.cmake`, `novaConfigVersion.cmake`, `novaTargets.cmake`, `novaTargets-release.cmake` |
| **Total** | **49 files** | **3.75 MB (3,933,110 bytes)** |

## Staging Directory

`C:\Users\twiborg\Desktop\NoVA\build\rc-4.5-dist-stage`

## Installation Command Used

```powershell
cmake --install "C:\Users\twiborg\Desktop\NoVA\build\rc-4.0.5" --prefix "C:\Users\twiborg\Desktop\NoVA\build\rc-4.5-dist-install" --config Release
```

The install was performed from the already-verified Release build (`build/rc-4.0.5`, Phase 4.0.5). Only NOVA-specific files were copied to the staging directory; GoogleTest/GMock files installed by CMake's FetchContent rules were intentionally excluded.

## Package Contents Validation

| Check | Result |
|-------|--------|
| All public headers present | Pass (36 headers under `include/nova/`) |
| All required static libraries present | Pass (6 `.lib` files) |
| CMake package configuration files present | Pass (4 files) |
| All six exported targets available | Pass (`nova::nova_core`, `nova::nova_loader_pe`, `nova::nova_loader_module`, `nova::nova_loader`, `nova::nova_runtime`, `nova::nova_sdk`) |
| LICENSE included | Pass |
| README.md included | Pass |
| CHANGELOG.md included | Pass |
| RELEASE_NOTES.md included | Pass |
| No build artifacts included | Pass (no `.obj`, `.exe`, `.pdb`, etc.) |
| No test-only dependencies (GoogleTest/GMock) | Pass |

## Relocatability Findings

The package is **relocatable**:

- `novaTargets.cmake` computes `_IMPORT_PREFIX` by navigating 4 directory
  levels up from `CMAKE_CURRENT_LIST_FILE` (the location of
  `novaTargets.cmake` itself). No hardcoded absolute paths.
- `novaConfig.cmake` computes `PACKAGE_PREFIX_DIR` from
  `${CMAKE_CURRENT_LIST_DIR}/../../../`. Relative to the config file.
- Library paths in `novaTargets-release.cmake` use `${_IMPORT_PREFIX}/lib/*.lib`.

The package can be copied to any location and consumed via
`CMAKE_PREFIX_PATH` pointing to the package root. No references to the
original source tree, build directory, or machine-specific paths exist in the
package.

## External Consumer Test

### Consumer project

`C:\Users\twiborg\Desktop\NoVA\build\rc-4.5-dist-consumer\` (outside NOVA source tree)

### Consumer CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyDistConsumer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(NOVA REQUIRED)
add_executable(mydistconsumer src/main.cpp)
target_link_libraries(mydistconsumer PRIVATE nova::nova_sdk)
```

### Configure command

```powershell
cmake -S <consumer> -B <consumer>/build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<stage-dir>"
```

### Result

- `find_package(NOVA REQUIRED)` found the staged package successfully.
- Build succeeded with zero warnings and zero errors.
- Consumer executable ran successfully:

```
[INFO] Initializing NOVA Core
[INFO] Shutting down NOVA Core
Distribution consumer: SDK initialized successfully
```

Exit code: 0

The consumer used only the staged package — no references to NOVA source
directories, build directories, or files outside the staged package.

## Packaging Limitations

1. **GoogleTest/GMock contamination:** The existing CMake install rules
   install GoogleTest/GMock headers, libraries, and CMake config files
   alongside NOVA because GoogleTest is fetched via FetchContent and its
   install rules are processed during `cmake --install`. These are test-only
   dependencies and were intentionally excluded from the distribution stage.
   The smallest justified CMake correction would be to scope install rules so
   only NOVA targets are installed, or to use `cmake --install --component`
   if component-based install is introduced. This was not implemented in this
   phase per the constraint against silently modifying CMake configuration.

2. **`nova_version.h.in` template included:** The staging directory includes
   `include/nova/nova_version.h.in`, a CMake configure template. It is not a
   consumer-facing header and is harmless, but it is not strictly necessary
   for consumers. It could be excluded in a future packaging refinement.

3. **No archive created:** Per constraints, no ZIP/archive was created in
   this phase. The staged directory is ready for archive creation.

## Package Readiness

The package is **ready for archive creation**. All required files are present,
the package is relocatable, and an external consumer successfully built and
executed using only the staged package via `find_package(NOVA REQUIRED)` and
`nova::nova_sdk`.

## Files Modified

None. No source code, public headers, tests, or CMake configuration files
were modified during this phase.
