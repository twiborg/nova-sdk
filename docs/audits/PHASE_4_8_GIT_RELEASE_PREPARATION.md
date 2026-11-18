# Phase 4.8 — Git Release Preparation (BLOCKED)

Phase 4.8 cannot proceed as specified. The NOVA directory is **not a Git
repository** (no `.git` directory exists), and Git operations cannot be
performed without initializing a repository, which would require creating
commits — explicitly forbidden by the phase constraints.

## Blocker

| Item | Detail |
|------|--------|
| Git available | Yes (`C:\Program Files\Git\bin\git.exe`, version 2.56.1.windows.1) |
| NOVA is a Git repository | **No** — no `.git` directory exists |
| `git status` | `fatal: not a git repository` |
| `git tag -l` | Cannot run (not a repo) |
| Remote configuration | None |
| Existing tags | None |
| `v0.2.0` tag | Does not exist |

## Constraints Preventing Resolution

The following constraints in Phase 4.8 prevent initializing a Git repository:

- "Do not create commits."
- "Do not create Git tags."
- "Do not start Phase 4.8 automatically."

Initializing a Git repository (`git init`) would create a `.git` directory but
would not create a release state without commits and tags, both of which are
forbidden. Therefore, no Git release preparation can be performed in this
environment.

## Release File Inventory (Documentation Only)

The following files constitute the intended NOVA `0.2.0` release. This
inventory was compiled by direct filesystem inspection (read-only, no Git).

### Intended Release Files

**Project root:**
- `CMakeLists.txt` — Root build configuration (project version `0.2.0`)
- `CMakePresets.json` — CMake presets for MSVC x64
- `README.md` — Build, usage, versioning, and license instructions
- `LICENSE` — MIT License (Copyright (c) 2026 Artyom Filin (twiborg))
- `CHANGELOG.md` — Keep a Changelog format, `[0.2.0] - Unreleased`
- `RELEASE_NOTES.md` — Release notes for NOVA `0.2.0`

**Public headers** (`include/nova/`, 36 files):
- `nova.h`, `nova_version.h.in`
- `core/`: config.h, error.h, init.h, logger.h
- `loader/`: loader.h, loader_config.h, loader_result.h, loader_version.h
- `loader/module/`: module_enumerator.h, module_error.h, module_info.h
- `loader/pe/`: pe_headers.h, pe_parser.h, pe_section.h, pe_types.h
- `runtime/`: runtime.h, runtime_config.h, runtime_context.h,
  runtime_diagnostics.h, runtime_error.h, runtime_state.h
- `runtime/event/`: event.h, event_dispatcher.h
- `runtime/task/`: cancellation_token.h, task.h, task_scheduler.h
- `sdk/`: application.h, application_context.h, sdk.h, sdk_config.h,
  sdk_error.h, sdk_result.h, sdk_version.h

**Source code** (`src/nova/`, all `.cpp` and `CMakeLists.txt`):
- `core/`: config.cpp, error.cpp, init.cpp, logger.cpp, main.cpp
- `loader/`: loader.cpp, loader_result.cpp, loader_version.cpp
- `loader/module/`: module_enumerator.cpp, module_error.cpp, module_info.cpp
- `loader/pe/`: pe_parser.cpp
- `runtime/`: runtime.cpp, runtime_context.cpp, runtime_error.cpp,
  event/event_dispatcher.cpp, task/task_scheduler.cpp
- `sdk/`: application.cpp, sdk.cpp, sdk_error.cpp, sdk_version.cpp

**Examples** (`examples/external_application/`):
- `CMakeLists.txt`, `src/main.cpp`

**Tests** (`tests/`, all `.cpp` and `CMakeLists.txt`):
- `src/core/`: config_test, error_test, init_test, logger_test
- `src/loader/loader/`: loader_test
- `src/loader/module/`: module_enumerator_test
- `src/loader/pe/`: pe_parser_test
- `src/runtime/`: runtime_test, runtime_context_test,
  event/event_dispatcher_test, task/task_scheduler_test
- `src/sdk/`: application_test, sdk_test
- `src/integration/external_consumer/`: main.cpp

**Audit reports** (`docs/audits/`, 12 reports):
- PHASE_4_0_3_1_FINDING_VERIFICATION.md
- PHASE_4_0_3_2_DOCUMENTATION_CORRECTIONS.md
- PHASE_4_0_4_1_DOCUMENTATION_CORRECTIONS.md
- PHASE_4_0_4_DOCUMENTATION_EXAMPLES_REVIEW.md
- PHASE_4_0_5_1_TEST_COUNT_CORRECTION.md
- PHASE_4_0_5_RELEASE_CANDIDATE_VERIFICATION.md
- PHASE_4_1_RELEASE_PREPARATION.md
- PHASE_4_2_LICENSING_VERSIONING.md
- PHASE_4_3_VERSIONING_POLICY.md
- PHASE_4_4_CHANGELOG_RELEASE_NOTES.md
- PHASE_4_5_DISTRIBUTION_PACKAGE.md
- PHASE_4_6_RELEASE_ARCHIVE_VERIFICATION.md
- PHASE_4_7_FINAL_RELEASE_AUDIT.md

### Excluded from Release (Build Artifacts)

The following directories and their contents are **not** release files:
- `build/` — Debug build artifacts, GoogleTest FetchContent sources
- `build-release/` — Release build artifacts, GoogleTest FetchContent sources
- `build\rc-4.0.5\*`, `build\rc-4.0.5-install\*`, `build\rc-4.0.5-consumer\*`
  — Phase 4.0.5 verification directories
- `build\rc-4.5-dist-stage\*`, `build\rc-4.5-dist-install\*`,
  `build\rc-4.5-dist-consumer\*` — Phase 4.5 distribution directories
- `build\NOVA-0.2.0.zip` — Release archive (build artifact, not source)
- `build\*_deps\googletest-src\*` — GoogleTest FetchContent sources (not NOVA)

## Required Actions to Complete Phase 4.8

To complete Git release preparation, the following actions are required
(outside this environment's constraints):

1. Initialize a Git repository in the NOVA root (`git init`).
2. Add a `.gitignore` excluding `build/`, `build-release/`, and IDE artifacts.
3. Stage all intended release files listed above.
4. Create an initial commit (or release commit) for version `0.2.0`.
5. Create an annotated tag `v0.2.0` pointing to the release commit.
6. (Optional) Configure a remote and push (forbidden in this phase:
   "Do not publish or push anything").

None of these can be performed here due to the constraints against creating
commits and tags, and the absence of a Git repository.

## Final State

Phase 4.8 is **BLOCKED** — no Git repository exists, and the constraints
forbid the operations needed to create one. The NOVA `0.2.0` release artifacts
(source, headers, tests, documentation, license, and archive) are all present
and verified, but no Git release state can be established in this environment.

The release is ready for Git release preparation once a Git repository is
initialized and the constraints on commits/tags are lifted.
