# Phase 4.2 — MIT Licensing & Versioning Investigation

Implements the maintainer-approved MIT licensing decision and investigates the
version mismatch without making any unapproved version changes.

---

## 1. MIT License Preparation

### Status: Complete

A standard MIT License file was created at:

`LICENSE`

### Copyright holder

Provided by the maintainer: **Copyright (c) 2026 Artyom Filin (twiborg)**

This name was supplied directly by the maintainer; it was not invented or
substituted. The full standard MIT License text was used without modification
or added restrictions.

### License text

Complete standard MIT License text, including:
- Copyright notice line
- Permission grant (use, copy, modify, merge, publish, distribute, sublicense,
  sell)
- Conditions (copyright and permission notice inclusion)
- Disclaimer of warranty ("AS IS")
- Liability limitation

### Previous state

No LICENSE or COPYING file existed in the NOVA source tree. The only LICENSE
files present were GoogleTest's bundled license inside build `_deps`
directories (a third-party dependency, not NOVA's own license).

---

## 2. README Licensing Changes

### File modified

`README.md` — License section only.

### Change

Replaced:
```
## License

Proprietary — NOVA project.
```

With:
```
## License

NOVA is licensed under the MIT License. See [LICENSE](LICENSE) for details.
```

No other README changes were made. No unrelated content was touched.

---

## 3. Version Declaration Locations

### CMake project version

| Location | Line | Value |
|----------|------|-------|
| `CMakeLists.txt` | 4 | `project(NOVA VERSION 0.2.0 LANGUAGES CXX)` |

This is the package/build version. It is exposed to consumers via:
- Compile definitions: `NOVA_VERSION_MAJOR=0`, `NOVA_VERSION_MINOR=2`,
  `NOVA_VERSION_PATCH=0`, `NOVA_VERSION_STRING="0.2.0"` (`CMakeLists.txt:22-27`)
- Installed package version file: `novaConfigVersion.cmake`
  (`PACKAGE_VERSION "0.2.0"`)

### Header API versions

All three headers declare `0.1.0` using C++20 struct default member
initializers (not macros):

| Header | Lines | Struct | Value |
|--------|-------|--------|-------|
| `include/nova/sdk/sdk_version.h` | 13-15 | `SdkVersion{major, minor, patch}` | `{0, 1, 0}` |
| `include/nova/loader/loader_version.h` | 15-17 | `LoaderVersion{major, minor, patch}` | `{0, 1, 0}` |
| `include/nova/runtime/runtime.h` | 38-40 | `RuntimeVersion{major, minor, patch}` | `{0, 1, 0}` |

Runtime additionally has a string member at `runtime.h:94`:
- `version_ = "0.1.0"`

### Version exposure mechanism

- C++20 structs with `std::uint32_t` default member initializers.
- Accessor functions return the struct by value:
  - `sdk_version()` (`sdk_version.h:19`)
  - `loader_version()` (`loader_version.h:21`)
  - `Runtime::version()` static (`runtime.h:83-85`)
- String accessors provide human-readable form:
  - `sdk_version_string()` (`sdk_version.h:22`)
  - `loader_version_string()` (`loader_version.h:24`)
  - `version_` string member (`runtime.h:94`)

---

## 4. Meaning of Each Version

### CMake project version (`0.2.0`)

Describes the **package/release version** of the NOVA distribution as a whole.
It drives the installed CMake package version (`PACKAGE_VERSION`) and the
`NOVA_VERSION_*` compile definitions. It is a build-system-level concept.

### Header API versions (`0.1.0` for SDK, Loader, Runtime)

Each header documents its struct as "Semantic version of the X API". These
describe **per-subsystem API compatibility versions**, independent of the
CMake project version. They are declared in headers, not derived from CMake,
and are not synchronized with each other or with the project version.

### Distinct intended meanings

Yes. The CMake project version and the header API versions have distinct
intended meanings:
- Project version = package/release identity.
- Header API versions = per-subsystem API compatibility identity.

There is no existing documentation stating they must match, nor any code that
synchronizes them.

---

## 5. Code Consumption of Version Values

No internal code was found that consumes these version values for
compatibility checks, feature gating, or behavioral decisions. They are
exposed purely as informational APIs for consumers:
- `sdk_version()`, `loader_version()`, `Runtime::version()` return version
  structs by value.
- String variants return human-readable strings.

No `#if` guards, no compile-time assertions, no runtime compatibility logic
reference these values.

---

## 6. Existing Compatibility Policy

None. No document in the repository describes:
- What a major/minor/patch bump means for API stability.
- Whether header API versions are decoupled from the package version.
- Deprecation or backward-compatibility guarantees.

---

## 7. Recommended Versioning Policy

**Approach A: Keep project and API versions independent, documenting their meaning.**

Reasoning:

1. **They already serve distinct purposes.** The CMake project version
   identifies the distributable package; the header API versions identify
   per-subsystem API compatibility. Forcing them to match would conflate two
   different concepts.

2. **No consumer depends on synchronization.** No code consumes the values for
   compatibility checks, and no external contract requires them to match.

3. **Least disruptive.** This approach requires no version declaration changes
   (which the maintainer has not approved), only documentation.

**Implementation (documentation only):**
- In README or a versioning document, state that:
  - `CMake project VERSION` = NOVA package release version.
  - `SdkVersion` / `LoaderVersion` / `RuntimeVersion` = per-subsystem API
    semantic versions, independent of the package version.
  - API versions may evolve independently of the package version.
  - No ABI or API stability guarantee is currently offered.

**Alternative approaches not recommended at this time:**
- **B (Synchronize):** Would require changing version declarations, which the
  maintainer has not approved, and would erase the meaningful distinction
  between package and API versions.
- **C (Explicit compatibility policy while retaining project version):** A
  worthwhile future step, but it is a policy decision that should be made by
  the maintainer after deciding whether API stability is a goal. This phase
  only investigates and recommends; it does not implement a policy.

---

## 8. Remaining Maintainer Decisions

| Decision | Details |
|----------|---------|
| Version policy | Adopt Approach A (document independence), B (synchronize), or C (explicit policy). No version declarations should change without explicit approval. |
| API stability commitment | Decide whether NOVA offers ABI/API stability guarantees, which would drive how header API versions are bumped. |
| Header API version alignment | Decide whether the three header API versions (all `0.1.0`) should remain aligned or diverge per subsystem. |

---

## 9. Exact Files Modified

| File | Change |
|------|--------|
| `LICENSE` | Created — standard MIT License, Copyright (c) 2026 Artyom Filin (twiborg) |
| `README.md` | License section: replaced "Proprietary — NOVA project." with MIT License reference |

## 10. Files Not Modified

- `CMakeLists.txt` — version declarations unchanged.
- All public headers — API versions unchanged.
- No tests modified.
- No CMake configuration modified.
- No source files modified.
