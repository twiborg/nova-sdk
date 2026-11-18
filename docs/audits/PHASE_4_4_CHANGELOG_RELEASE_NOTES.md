# Phase 4.4 — Changelog & Release Notes

Documentation-only phase. Created `CHANGELOG.md` and `RELEASE_NOTES.md` for
NOVA `0.2.0` based exclusively on verified project history.

## Files Created

| File | Description |
|------|-------------|
| `CHANGELOG.md` | Keep a Changelog formatted changelog with `[0.2.0] - Unreleased` entry |
| `RELEASE_NOTES.md` | Release notes for NOVA `0.2.0` |
| `docs/audits/PHASE_4_4_CHANGELOG_RELEASE_NOTES.md` | This audit report |

## Files Modified

None. Documentation-only changes; no source, header, test, or CMake files
modified.

## Historical Sources Inspected

| Source | Purpose |
|--------|---------|
| `AUDIT_HISTORY.md` | Verified audit history, test counts, fixes, design limitations |
| `README.md` | Project description, build/test instructions, versioning, license |
| `CMakeLists.txt` | Project version `0.2.0`, build configuration |
| `include/nova/sdk/sdk_version.h` | SDK API version `0.1.0` |
| `include/nova/loader/loader_version.h` | Loader API version `0.1.0` |
| `include/nova/runtime/runtime.h` | Runtime API version `0.1.0` |
| `LICENSE` | MIT License, Copyright (c) 2026 Artyom Filin (twiborg) |
| `docs/audits/PHASE_4_0_5_RELEASE_CANDIDATE_VERIFICATION.md` | RC verification results |
| `docs/audits/PHASE_4_0_5_1_TEST_COUNT_CORRECTION.md` | Test count correction (216 total) |

## Changelog Contents Summary

`CHANGELOG.md` follows Keep a Changelog format with a single `[0.2.0] -
Unreleased` entry containing:

- **Added** — Core logging/config/errors/init; Loader PE parsing/module
  enumeration/unified facade; Runtime lifecycle/scheduler/events/diagnostics;
  SDK facade/Application/config/error/result/versioning; installation support;
  MIT License.
- **Changed** — API freeze; documentation corrections (generator,
  installed-package consumption, bundled examples); README licensing (MIT);
  README versioning (independent model).
- **Fixed** — SDK init rollback; SDK ownership sync; nullable accessors;
  ConfigValue try_as; scheduler statistics; JSON parser; UTF-8 module names;
  Config thread safety; Config accessor data races.
- **Documentation** — Independent versioning policy; RC verification results;
  test count correction.
- **Not implemented** — explicitly out-of-scope items (game loader,
  hooking/injection, reverse engineering, etc.).

No `0.1.0` release entry was created because no historical release exists.

## Release Notes Contents Summary

`RELEASE_NOTES.md` contains:

- **Overview** — what NOVA is and explicitly unsupported functionality.
- **Architecture** — four components (Core, Loader, Runtime, SDK).
- **Capabilities** — implemented and verified capabilities only.
- **Technical requirements** — Windows x64, VS 2022/2026, Windows SDK,
  CMake 3.24+, C++20.
- **Build** — verified configure/build/test commands with strict flags.
- **Installation and integration** — install command, `find_package(NOVA
  REQUIRED)`, `nova::nova_sdk` linking, `CMAKE_PREFIX_PATH`, all six exact
  imported target names.
- **Testing** — six suites, 216 tests, RC verification results.
- **Versioning** — project `0.2.0`, subsystem API `0.1.0`, independent,
  informational, no automatic enforcement.
- **License** — MIT, references LICENSE file, correct copyright holder.
- **Limitations** — Windows x64 verified only; no cross-platform; no ABI
  stability; no automatic compatibility; no production readiness claim;
  Release Candidate, not official release.
- **Explicitly unsupported scope** — no DLL injection, hooking, manual
  mapping, remote process access, memory modification, arbitrary code
  execution; read-only infrastructure only.

## Verification Results

| Check | Result |
|-------|--------|
| Version matches CMake `0.2.0` | Pass |
| Subsystem API versions match `0.1.0` | Pass |
| License matches LICENSE (MIT) | Pass |
| Copyright holder correct | Pass |
| Test count is 216 | Pass |
| Six suite names correct | Pass |
| Imported CMake targets exact | Pass |
| README and release notes consistent | Pass |
| No unsupported feature claims | Pass |

No contradictions found between CHANGELOG.md, RELEASE_NOTES.md, README.md,
LICENSE, CMakeLists.txt, or public headers.

## Unresolved Historical Information

The original NOVA Phase 3 audit report (findings NOVA-001–NOVA-020) was not
available in the repository and could not be recovered. This is documented in
`AUDIT_HISTORY.md`. No earlier release entry was fabricated to compensate.

## Constraints Honored

- Documentation-only changes.
- No source code modified.
- No public headers modified.
- No tests modified.
- No CMake configuration modified.
- No version declarations changed.
- No publishing or distribution.
- No historical releases invented.
- No ABI stability claimed.

Phase 4.4 complete.
