# Phase 4.0.4.1 — Documentation Corrections

Applied documentation-only corrections to `README.md` based on the required
findings in `docs/audits/PHASE_4_0_4_DOCUMENTATION_EXAMPLES_REVIEW.md`.

This is a documentation-only phase. No source, header, test, CMake, or example
files were modified. The public API freeze established in Phase 4.0.3 is
preserved.

---

## 1. Correction R1 — README generator mismatch

### File modified
`README.md`, lines 195 and 211.

### Change
- Line 195 (developer command prompt hint): changed "VS 2022" to
  "VS 2022 or VS 2026".
- Line 211 (manual `cmake -G` generator): changed
  `Visual Studio 17 2022` to `Visual Studio 18 2026`, matching the verified
  toolchain (`build/CMakeCache.txt:487` `CMAKE_GENERATOR = Visual Studio 18 2026`,
  MSVC 14.52).

### Finding addressed
R1: The README manual-configuration snippet referenced Visual Studio 17 2022,
which does not match the verified environment and may not be installed on a
developer's machine.

### Public API signature change
None.

---

## 2. Correction R2/C1 — Installed-package consumption documentation

### File modified
`README.md`, new "Consuming NOVA as an installed package" section (lines ~300-371).

### Change
Added a dedicated section documenting how an external CMake project consumes an
installed NOVA package:

- Installation prerequisite: `cmake --install build --prefix <install-dir>`
- `find_package(NOVA REQUIRED)` with a minimal CMakeLists.txt example linking
  `nova::nova_sdk`
- `CMAKE_PREFIX_PATH` usage for custom install prefixes
- Imported target names (all `STATIC IMPORTED`, `cxx_std_20`):
  - `nova::nova_core`
  - `nova::nova_loader_pe`
  - `nova::nova_loader_module`
  - `nova::nova_loader`
  - `nova::nova_runtime`
  - `nova::nova_sdk`
- Note that `nova::nova_sdk` transitively links `nova::nova_runtime`,
  `nova::nova_loader`, and `nova::nova_core`
- Clarification that the package installs static libraries (`.lib`) and public
  headers under `include/nova`
- A minimal C++ example using `nova/sdk/sdk.h`

### Finding addressed
R2/C1: The README had no section describing how to consume NOVA as an installed
package via `find_package(NOVA)` and imported targets, despite Phase 4.0.2
verifying that this works.

### Public API signature change
None.

---

## 3. Correction R3/E1 — Bundled examples clarification

### File modified
`README.md`, new "Bundled examples" subsection (lines ~363-371).

### Change
Added a clarification paragraph stating that the bundled examples
(`src/nova/core/main.cpp`, `examples/external_application/src/main.cpp`,
`tests/integration/external_consumer/src/main.cpp`) are built **in-tree** as part
of the NOVA source tree, link against NOVA's source-tree targets, use source-tree
include paths, and are **not** standalone consumers of an installed NOVA
package. External consumption should use the `find_package(NOVA)` approach.

### Finding addressed
R3/E1: The bundled examples were not clearly distinguished from standalone
installed-package consumers, which could mislead external developers.

### Public API signature change
None.

---

## Verification

1. **Complete README diff**: All changes are confined to comment/prose and code
   fences in `README.md`. No source, header, test, CMake, or example code was
   modified.
2. **Imported target names**: Verified against the installed
   `lib/cmake/nova/novaTargets.cmake` (Phase 4.0.2) — all six target names match
   exactly.
3. **CMake snippets**: Match the actual package configuration (C++20 required,
   `find_package(NOVA REQUIRED)`, `nova::nova_sdk` transitive linking).
4. **Generator instructions**: No longer misleading — reference VS 18 2026 (the
   verified generator) while noting VS 2022 or VS 2026.
5. **Bundled examples**: Accurately described as in-tree builds that are not
   standalone installed-package consumers.
6. **Only documentation files changed**: Confirmed — only `README.md` was
   modified.

No source files, tests, or public API signatures were modified in this phase.
