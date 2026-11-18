# Phase 4.1 — Release Preparation

Repository metadata findings, versioning assessment, proposed changelog,
proposed release notes, and distribution structure proposal for NOVA's first
formal release.

This phase is preparation only. No publishing, uploading, tagging, or
distribution was performed. No source code was modified. No version
declarations were changed. No license was invented or selected.

---

## 1. Repository Metadata Findings

### Project version

- **CMake project version:** `0.2.0` (`CMakeLists.txt:4`)
- **Compile definitions:** `NOVA_VERSION_MAJOR=0`, `NOVA_VERSION_MINOR=2`,
  `NOVA_VERSION_PATCH=0`, `NOVA_VERSION_STRING="0.2.0"` (`CMakeLists.txt:22-27`)
- **Installed package version:** `0.2.0` (from `novaConfigVersion.cmake`,
  `PACKAGE_VERSION "0.2.0"`)

### Header-defined API versions (independent of CMake project version)

| Header | Struct/Field | Declared value |
|--------|--------------|----------------|
| `include/nova/sdk/sdk_version.h:12-16` | `SdkVersion{major, minor, patch}` | `{0, 1, 0}` |
| `include/nova/loader/loader_version.h:14-18` | `LoaderVersion{major, minor, patch}` | `{0, 1, 0}` |
| `include/nova/runtime/runtime.h:37-41` | `RuntimeVersion{major, minor, patch}` | `{0, 1, 0}` |
| `include/nova/runtime/runtime.h:94` | `version_` string | `"0.1.0"` |

**Inconsistency:** The CMake project version (`0.2.0`) does not match the
header-defined API versions (`0.1.0` for SDK, Loader, and Runtime). These are
declared independently in headers and are not synchronized with the CMake
project version or with each other's semantic meaning (API version vs package
version). This is a pre-existing condition, not introduced in this phase.

### Build system

- **CMake minimum:** 3.24 (`CMakeLists.txt:1`)
- **CMake presets:** `msvc-x64-debug`, `msvc-x64-release` inheriting
  `base` (Visual Studio 18 2026, x64, C++20) (`CMakePresets.json`)
- **Compiler flags:** `/W4 /WX /permissive- /Zc:__cplusplus` for MSVC
  (`CMakeLists.txt:14`)
- **Language standard:** C++20, required, extensions off
  (`CMakeLists.txt:17-19`)

### Components

- `nova_core` — Core static library (logging, configuration, error handling,
  initialization)
- `nova_loader_pe` — PE parser static library
- `nova_loader_module` — Module enumeration static library
- `nova_loader` — Unified Loader static library
- `nova_runtime` — Runtime static library (lifecycle, scheduler, events,
  diagnostics)
- `nova_sdk` — SDK static library (unified facade, Application)

### Test coverage

- 6 test suites, **216 tests total**, all passing under Release with `/W4 /WX`
- `nova_core_tests`: 60
- `nova_loader_pe_tests`: 19
- `nova_loader_module_tests`: 25
- `nova_loader_tests`: 17
- `nova_runtime_tests`: 48
- `nova_sdk_tests`: 47

### Installation

- Public headers installed under `include/nova/`
- Static libraries installed under `lib/` (`.lib`)
- CMake package config installed under `lib/cmake/nova/`
- Imported targets: `nova::nova_core`, `nova::nova_loader_pe`,
  `nova::nova_loader_module`, `nova::nova_loader`, `nova::nova_runtime`,
  `nova::nova_sdk`

---

## 2. License Review

### Finding

**No LICENSE or COPYING file exists in the NOVA source tree.**

The only `LICENSE` files present are GoogleTest's bundled license, located
inside build `_deps` directories (a third-party dependency, not NOVA's own
license).

### README claim

`README.md:401`: "Proprietary — NOVA project."

This is a statement of intent, not a license grant. There is no license text
attached to the repository, no copyright notice in any source or header file,
and no SPDX identifier anywhere in the source tree.

### Assessment

The repository has **no valid, complete license**. The README's "Proprietary"
claim is unsupported by an actual license file. A maintainer decision is
required before release: either add a license file (e.g. a proprietary license
text, or an open-source license selected by the maintainer) or remove the
unsupported claim.

**Action required:** Maintainer must decide on a license. Do not invent or
select one on behalf of the project.

---

## 3. Versioning Assessment

### Current state

- CMake project version: `0.2.0`
- Header API versions: `0.1.0` (SDK, Loader, Runtime)
- Package version file: `0.2.0` (SameMajorVersion compatibility)

### Readiness for 1.0.0

**Not recommended to jump automatically to 1.0.0.** Reasoning:

1. **Version declarations are unsynchronized.** The CMake project version
   (`0.2.0`) does not match the header-defined API versions (`0.1.0`). Before
   declaring a stable 1.0.0, these should be reconciled so that package version
   and API versions are coherent.

2. **No license.** A formal release should ship with a verified license file.
   The current "Proprietary" claim has no license text behind it.

3. **Public API surface is young.** The API was reviewed in Phase 4.0.3 and
   frozen, but the library is at version 0.2.0 with limited subsystem coverage
   (no game loader, no hooking, no injection — explicitly out of scope). Moving
   to 1.0.0 implies stability commitments that should be matched by version
   discipline.

4. **No changelog or release notes exist.** A 1.0.0 release should have
   documented release history.

### Recommendation

- **Do not change the version automatically.**
- If the maintainer decides to proceed toward a first stable release, the
  version should be set deliberately (e.g. `0.3.0` as a pre-1.0 stability
  milestone, or `1.0.0` only after version declarations are synchronized, a
  license is added, and a changelog is written).
- Any version change must update both `CMakeLists.txt` `project(VERSION ...)`
  and the header-defined API versions consistently.

---

## 4. Proposed CHANGELOG.md

The following changelog is based exclusively on verified project history from
audit records and verification sessions. It distinguishes verified
functionality from planned work. Nothing here is invented.

```markdown
# CHANGELOG

## [Unreleased]

### Added
- NOVA Core: thread-safe structured logger (6 levels), JSON-based
  configuration manager, NovaError exception type, RAII core
  initialization/shutdown state tracking.
- NOVA Loader: read-only PE parser (PE32/PE32+), module enumeration via
  documented Windows APIs, unified Loader facade with lifecycle management.
- NOVA Runtime: lifecycle management, priority-aware task scheduler, typed
  event dispatcher, runtime diagnostics, configuration.
- NOVA SDK: unified facade over Core/Loader/Runtime, SdkConfig, SdkError,
  SdkResult, SdkVersion, Application class with full lifecycle.
- Installation support: public headers, static libraries, CMake package
  config with `find_package(NOVA)` and `nova::*` imported targets.

### Changed
- Public API signatures frozen (Phase 4.0.3).
- Documentation corrections: CMake generator updated to Visual Studio 18
  2026, installed-package consumption instructions added, bundled examples
  clarified as in-tree builds (Phase 4.0.4.1).
- Test count corrected: 216 total (nova_core_tests: 60) (Phase 4.0.5.1).

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

### Verified
- Release build with `/W4 /WX /permissive- /Zc:__cplusplus`: zero warnings,
  zero errors (Phase 4.0.5).
- All 6 test suites pass, 216 tests total (Phase 4.0.5).
- Clean installation to a fresh prefix (Phase 4.0.5).
- External consumer builds and runs using `find_package(NOVA REQUIRED)`
  and `nova::nova_sdk` from the installed package (Phase 4.0.5).

### Not implemented (explicitly out of scope)
- Game loader
- Hooking / DLL injection
- Reverse engineering
- Game-specific functionality
- DLL injection, manual mapping, remote process access, process memory
  modification, arbitrary code execution
```

---

## 5. Proposed Release Notes

```markdown
# NOVA Release Notes

## What NOVA is

NOVA is a Windows x64 C++20 infrastructure library providing read-only
infrastructure components: Core (logging, configuration, errors,
initialization), Loader (PE parsing, module enumeration, unified loader
API), Runtime (lifecycle, task scheduling, event dispatching, diagnostics),
and SDK (unified facade, Application class).

NOVA does NOT implement: game loader, hooking, DLL injection, reverse
engineering, game-specific functionality, DLL injection, manual mapping,
remote process access, process memory modification, or arbitrary code
execution.

## Main components

- **nova_core** — Logging, JSON configuration, error handling, initialization.
- **nova_loader_pe** — Read-only PE32/PE32+ parser.
- **nova_loader_module** — Read-only module enumeration via documented
  Windows APIs (current process only).
- **nova_loader** — Unified Loader facade over PE parser and module
  enumeration.
- **nova_runtime** — Lifecycle, priority-aware task scheduler, typed event
  dispatcher, diagnostics.
- **nova_sdk** — Unified SDK facade, Application class, configuration,
  error handling, versioning.

## Supported environment

- Windows 10/11 x64
- Visual Studio 2022 or 2026 (MSVC v143/v144) with "Desktop development
  with C++" workload
- Windows 10/11 SDK
- CMake 3.24 or newer
- C++20

## Build requirements

Configure with Visual Studio 18 2026, x64:

```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --config Release
```

Strict compiler flags: `/W4 /WX /permissive- /Zc:__cplusplus`.

## Installation and dependency consumption

Install to a prefix:

```
cmake --install build --prefix <install-dir>
```

Consume from an external CMake project:

```cmake
find_package(NOVA REQUIRED)
target_link_libraries(myapp PRIVATE nova::nova_sdk)
```

Point CMake at a custom prefix:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH="<install-dir>"
```

Installed targets (all `STATIC IMPORTED`, C++20):

- `nova::nova_core`
- `nova::nova_loader_pe`
- `nova::nova_loader_module`
- `nova::nova_loader`
- `nova::nova_runtime`
- `nova::nova_sdk`

## Test coverage

6 test suites, **216 tests**, all passing under Release with `/W4 /WX`:

| Suite | Tests |
|-------|-------|
| nova_core_tests | 60 |
| nova_loader_pe_tests | 19 |
| nova_loader_module_tests | 25 |
| nova_loader_tests | 17 |
| nova_runtime_tests | 48 |
| nova_sdk_tests | 47 |
| **Total** | **216** |

## Known limitations

- No LICENSE file exists in the source tree. README states "Proprietary —
  NOVA project" but no license text is provided. A maintainer decision on
  licensing is required before distribution.
- CMake project version (0.2.0) does not match header-defined API versions
  (0.1.0 for SDK, Loader, Runtime). Version declarations are not
  synchronized.
- Public headers contain no copyright or license notices.
- The `nova_config.h.in` template and `nova_version.h.in` are referenced by
  CMake but were not found as separate files in the source tree (they may be
  generated at configure time).

## Explicitly unsupported functionality

- Game loader
- Hooking / DLL injection
- Reverse engineering
- Game-specific functionality
- DLL injection, manual mapping, remote process access, process memory
  modification, arbitrary code execution

NOVA provides read-only infrastructure only. It does not execute, inject,
hook, or modify executable code.

## No claims

- No cross-platform support (Windows x64 only).
- No ABI stability guarantee.
- No production-readiness guarantee.
- No official release has been made; this document describes a release
  candidate, not a published release.
```

---

## 6. Distribution Structure Proposal

A clean release archive should contain:

```
NOVA-<version>/
├── CMakeLists.txt              # Root build configuration
├── CMakePresets.json           # CMake presets for MSVC x64
├── README.md                   # Build and usage instructions
├── LICENSE                     # (to be added by maintainer decision)
├── CHANGELOG.md                # (proposed above)
├── RELEASE_NOTES.md            # (proposed above)
├── cmake/
│   └── novaConfig.cmake.in     # CMake package config template
├── include/nova/               # Public headers
│   ├── nova.h
│   ├── nova_version.h.in
│   ├── core/
│   ├── loader/
│   ├── runtime/
│   └── sdk/
├── src/nova/                   # Library sources
│   ├── core/
│   ├── loader/
│   ├── runtime/
│   └── sdk/
├── examples/                   # Source-tree examples (in-tree builds)
│   └── external_application/
└── tests/                      # Unit tests
    ├── src/core/
    ├── src/loader/
    ├── src/runtime/
    └── src/sdk/
```

**Note:** The archive contains source only. Built static libraries, CMake
package files, and installed headers are produced by the consumer via
`cmake --build` and `cmake --install`. No binaries should be distributed in
the source archive.

---

## 7. Remaining Decisions Before Release

| Decision | Status | Details |
|----------|--------|---------|
| License | **Required** | No LICENSE file exists. Maintainer must select/add a license file or remove the unsupported "Proprietary" claim from README. |
| Version | **Recommended** | Current 0.2.0 is unsynchronized with header API versions (0.1.0). Decide whether to release as 0.2.0, 0.3.0, or 1.0.0, and synchronize declarations. |
| Version synchronization | **Recommended** | Align CMake project version with header-defined SDK/Loader/Runtime API versions before any release. |
| Copyright headers | **Optional** | No source or header files contain copyright notices. Consider adding them. |
| `nova_version.h.in` / `nova_config.h.in` | **Investigate** | Referenced by CMake but not found as separate source files; confirm they are generated at configure time. |

---

**Final status:** Release preparation findings documented. No source modified.
No version changed. No license invented. No distribution performed. Maintainer
action required on license and version decisions before any formal release.
