# Phase 4.3 — Versioning Policy

Documents the approved independent versioning model for NOVA and records the
documentation changes made to support it.

## Approved Policy

The maintainer explicitly approved **Approach A: Independent Versioning**.

NOVA uses two independent versioning levels:

### Project version

- **Current value:** `0.2.0`
- **Location:** Root `CMakeLists.txt`, line 4 (`project(NOVA VERSION 0.2.0 ...)`)
- **Meaning:** Overall NOVA project version; release and distribution version;
  used by CMake package versioning.
- **Exposed via:** Compile definitions `NOVA_VERSION_MAJOR/MINOR/PATCH` and
  `NOVA_VERSION_STRING`; installed `novaConfigVersion.cmake`.

### Subsystem API versions

- **Current value:** `0.1.0`
- **Locations:**
  - `include/nova/sdk/sdk_version.h` — `SdkVersion{major=0, minor=1, patch=0}`
  - `include/nova/loader/loader_version.h` — `LoaderVersion{major=0, minor=1, patch=0}`
  - `include/nova/runtime/runtime.h` — `RuntimeVersion{major=0, minor=1, patch=0}`
- **Meaning:** Individual subsystem API compatibility versions, exposed through
  public C++20 API structures.
- **Relationship to project version:** Independent; may evolve at different
  rates.

### Key principles

- Project and subsystem API versions are intentionally independent.
- API version values are informational metadata.
- API versions are not currently used for automatic compatibility enforcement.
- No ABI stability or backward compatibility is guaranteed.

---

## Exact Version Declaration Locations

| Version | File | Line(s) | Value |
|---------|------|---------|-------|
| Project version | `CMakeLists.txt` | 4 | `0.2.0` |
| Project version macros | `CMakeLists.txt` | 22-27 | `0.2.0` |
| SDK API version | `include/nova/sdk/sdk_version.h` | 13-15 | `0.1.0` |
| Loader API version | `include/nova/loader/loader_version.h` | 15-17 | `0.1.0` |
| Runtime API version | `include/nova/runtime/runtime.h` | 38-40 | `0.1.0` |
| Runtime version string | `include/nova/runtime/runtime.h` | 94 | `"0.1.0"` |

---

## Documentation Changes

### `README.md`

Added a `Versioning` section after the `License` section explaining:
- Project version represents the overall project/distribution release.
- Subsystem API versions represent individual public subsystem APIs.
- Project and subsystem API versions are intentionally independent.
- They may evolve at different rates.
- Current values: project `0.2.0`, subsystem API `0.1.0`.
- API version values are informational; no automatic compatibility enforcement.
- No ABI stability or backward compatibility guaranteed.

### `AUDIT_HISTORY.md`

Added entries for:
- Phase 4.2 — MIT Licensing & Versioning Investigation (COMPLETE)
- Phase 4.3 — Versioning Policy (COMPLETE)

---

## Confirmations

- **Version values unchanged:** CMake project version remains `0.2.0`; subsystem
  API versions remain `0.1.0`.
- **Public API signatures unchanged:** No header, source, test, or CMake file
  was modified.
- **No ABI stability claimed:** The documentation explicitly states no ABI
  stability or backward compatibility is guaranteed.
- **No compatibility guarantees invented:** Only informational metadata is
  described.
- **Historical audit records preserved:** Previous entries in
  `AUDIT_HISTORY.md` were not modified; new entries were appended.

## Files Modified

| File | Change |
|------|--------|
| `README.md` | Added `Versioning` section |
| `AUDIT_HISTORY.md` | Added Phase 4.2 and Phase 4.3 entries |

## Files Not Modified

- `CMakeLists.txt` — version declarations unchanged.
- All public headers — API versions unchanged.
- No source files, tests, or CMake configuration modified.

## Verification Results

1. README accurately describes both version levels — verified.
2. Current version values remain unchanged — verified (`0.2.0` project,
   `0.1.0` subsystem APIs).
3. LICENSE remains standard MIT with approved copyright holder — verified
   (Phase 4.2).
4. AUDIT_HISTORY contains new entries — verified.
5. No source/header/test/CMake files were modified — verified.

Phase 4.3 complete. No further action required for this phase.
