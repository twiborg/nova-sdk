# Phase 4.6 — Release Archive Creation & Final Verification

Created and verified the NOVA `0.2.0` release archive. Performed final
verification using the extracted archive with a fresh external CMake consumer.

## Archive

| Property | Value |
|----------|-------|
| Path | `C:\Users\twiborg\Desktop\NoVA\build\NOVA-0.2.0.zip` |
| Size | 679,259 bytes (~663 KB) |
| File count | 49 files (51 zip entries including 2 directory entries) |
| Format | ZIP |

## Archive Contents

```
NOVA-0.2.0/
├── LICENSE
├── README.md
├── CHANGELOG.md
├── RELEASE_NOTES.md
├── include/nova/          (34 .h headers + nova_version.h.in template)
├── lib/                   (6 static .lib libraries)
└── lib/cmake/NOVA/        (4 CMake package config files)
```

### Public headers (36 files)
`nova.h`, `nova_version.h.in`, and headers under `core/`, `loader/`,
`loader/module/`, `loader/pe/`, `runtime/`, `runtime/event/`,
`runtime/task/`, `sdk/`.

### Static libraries (6 files)
`nova_core.lib`, `nova_loader_pe.lib`, `nova_loader_module.lib`,
`nova_loader.lib`, `nova_runtime.lib`, `nova_sdk.lib`.

### CMake package files (4 files)
`novaConfig.cmake`, `novaConfigVersion.cmake`, `novaTargets.cmake`,
`novaTargets-release.cmake`.

### Documentation (4 files)
`LICENSE`, `README.md`, `CHANGELOG.md`, `RELEASE_NOTES.md`.

## Archive Integrity Verification

| Check | Result |
|-------|--------|
| ZIP archive opens cleanly | Pass |
| No CRC or extraction errors | Pass |
| All expected files present | Pass |
| Archive paths are relative | Pass |
| No absolute paths | Pass |
| No parent-directory traversal | Pass |
| Contents match staging directory | Pass |

All 51 zip entries were verified via `System.IO.Compression.ZipFile.OpenRead`.
All paths use relative `NOVA-0.2.0/` prefix with backslash separators. No
absolute paths (`C:\...`) or parent traversal (`..\`) entries exist.

## Relocatability

The package is **relocatable**:

- `novaTargets.cmake` computes `_IMPORT_PREFIX` by navigating directory
  levels up from `CMAKE_CURRENT_LIST_FILE`. No hardcoded absolute paths.
- `novaConfig.cmake` computes `PACKAGE_PREFIX_DIR` from
  `${CMAKE_CURRENT_LIST_DIR}/../../../`. Relative to the config file.
- Library paths in `novaTargets-release.cmake` use
  `${_IMPORT_PREFIX}/lib/*.lib`.

The package can be copied to any location and consumed via
`CMAKE_PREFIX_PATH` pointing to the package root. No references to the
original source tree, build directory, or machine-specific paths exist.

## Extracted Package Location

`C:\Users\twiborg\AppData\Local\Temp\kilo\NOVA-0.2.0-extract\NOVA-0.2.0\`

Extracted to an independent temporary directory outside the NOVA repository
and build tree. This is the only NOVA package used for the final consumer
test.

## Final External Consumer Test

### Consumer project
`C:\Users\twiborg\AppData\Local\Temp\kilo\NOVA-0.2.0-consumer\` (outside NOVA repository and build tree)

### Package path used
`C:\Users\twiborg\AppData\Local\Temp\kilo\NOVA-0.2.0-extract\NOVA-0.2.0\`

### Configure command
```powershell
cmake -S <consumer> -B <consumer>/build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<pkg-root>"
```

### Configure result
`find_package(NOVA REQUIRED)` found the extracted archive successfully.
Configuration done. (Expected `CMAKE_BUILD_TYPE` informational warning for
multi-config generator — not an error.)

### Build result
Build succeeded. `archiveconsumer.exe` produced at
`<consumer>/build/Release/archiveconsumer.exe`.
No compilation errors. (MSB8029 informational warnings about temp directory
location for incremental builds — not errors.)

### Execution result
```
[INFO] Initializing NOVA Core
[INFO] Shutting down NOVA Core
Archive consumer: SDK initialized successfully
```

### Exit code
0

The consumer used **only** the extracted archive — no references to NOVA
source directories, build directories, or files outside the staged package
(except standard compiler and Windows SDK dependencies).

## Remaining Limitations

1. **GoogleTest/GMock exclusion:** The existing CMake install rules install
   GoogleTest/GMock headers, libraries, and CMake config files alongside NOVA
   (GoogleTest is fetched via FetchContent). These test-only dependencies were
   intentionally excluded from the distribution package. The smallest
   justified CMake correction would be to scope NOVA install rules separately
   or use component-based install. This was not implemented per the
   constraint against modifying CMake configuration.

2. **No archive publication:** Per constraints, no ZIP archive was published,
   uploaded, or tagged. The archive exists locally only for verification.

3. **`nova_version.h.in` template:** Included in the package as it is part of
   the source `include/nova/` directory. It is a CMake configure template,
   not a consumer-facing header. Harmless but not strictly necessary for
   consumers.

## Final Release Readiness Verdict

**PASS — Release Archive Ready**

Evidence:
- Release build succeeded with zero warnings and zero errors
  (`/W4 /WX /permissive- /Zc:__cplusplus`).
- All 6 test suites pass, 216 tests total under Release.
- Clean installation to a fresh prefix succeeded.
- All 6 exported CMake targets verified with correct names.
- External consumer built and executed using `find_package(NOVA REQUIRED)`
  and `nova::nova_sdk` from the installed package (Phase 4.0.5).
- Release archive created successfully (49 files, 679,259 bytes).
- Archive integrity verified (no CRC errors, relative paths, no absolute
  paths).
- Archive extracted to an independent location outside the repository.
- Final external consumer built and executed successfully against the
  extracted archive (exit code 0).
- Package is relocatable — no hardcoded source tree or build directory
  references.

The NOVA `0.2.0` release archive is ready for archive creation (zip already
produced locally). No official publication, upload, or tagging was performed
per constraints.
