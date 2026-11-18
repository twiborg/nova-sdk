# NOVA Audit History

This document records the verified audit and remediation work performed on the
NOVA codebase during the Phase 3 / Phase 3.9 work sessions. It contains only
information that could be confirmed from the current repository and session
evidence.

## Historical record availability

The original NOVA Phase 3 audit report and the complete set of findings
NOVA-001 through NOVA-020 were **not available** in this repository. No
audit report, findings file, AGENTS.md, or `.kilo/` directory exists in the
workspace. Their complete historical descriptions could not be recovered and
are deliberately not fabricated here. This document records only the findings
and work verified in the current session.

## Phase 3.8.2 — Config API introduction (context)

- Introduced two public `Config` accessors:
  - `Config::try_get(std::string_view key)`
  - `Config::try_get_as<T>(std::string_view key)`
- Location: `include/nova/core/config.h`, `src/nova/core/config.cpp`

## Phase 3.8.3 — Read-only verification (context)

Discovered issues in the Config accessors:

1. `try_get()` locks `mutex_` correctly but returns a reference to internal
   storage that may become invalid after concurrent removal or clearing.
2. `try_get_as<T>()` accessed `values_` without acquiring `mutex_` (data race).
3. Existing `get_as<T>()` accessed `values_` without acquiring `mutex_`
   (pre-existing data race).
4. `get()` already locked correctly.
5. `try_get()` was declared `noexcept`, although string construction and
   mutex locking may theoretically throw.

Location: `include/nova/core/config.h`, `src/nova/core/config.cpp`

## Phase 3.8.4 — Config thread-safety fixes (FIXED AND VERIFIED)

Scope: `include/nova/core/config.h`, `src/nova/core/config.cpp`,
`tests/src/core/config_test.cpp`.

### Changes

1. **`try_get_as<T>()` data race fixed.** Acquires `mutex_` via
   `std::lock_guard<std::mutex>` before accessing `values_`; holds the lock
   during lookup, type checking, and construction/copying of the returned
   value. Missing key or type mismatch returns `std::nullopt`. Not marked
   `noexcept`. (`config.h` `try_get_as` definition)
2. **`get_as<T>()` data race verified already fixed.** The existing template
   already acquires `mutex_` once and performs lookup/conversion while the
   lock is held. Exception types, error codes, and messages preserved:
   `NovaError(ErrorCode::InvalidArgument, "Configuration key not found: ...")`
   for missing keys and `NovaError(ErrorCode::InvalidArgument,
   "Configuration key type mismatch")` for type mismatches. No source change
   was required for this accessor.
3. **`try_get()` `noexcept` corrected.** Removed `noexcept` from both the
   declaration (`config.h`) and definition (`config.cpp`). Return type and
   behavior preserved.
4. **Reference lifetime documented.** Doc comments added on `get()` and
   `try_get()`: returned references refer to internal Config storage; the
   mutex is released when the accessor returns; references must not be
   retained across concurrent mutations (`set`/`remove`/`clear`) that erase
   or clear the referenced entry. Does not claim the reference remains
   protected by the mutex.
5. **Deadlock checks.** Each accessor acquires the mutex exactly once; no
   public Config accessor is called while holding the same non-recursive
   mutex; no recursive locking; return-value construction happens while the
   lock is held in `get_as<T>()` and `try_get_as<T>()`.
6. **Tests added.** `ConcurrentGetAsAndTryGetAsWhileMutating` and
   `ConcurrentReadsDoNotCrash` in `config_test.cpp`, exercising
   `get_as<T>()`/`try_get_as<T>()` concurrently with mutating threads.

### Verification

- Full Debug build with `/W4 /WX /permissive-`: clean, no warnings or errors.
- All six test suites pass under `ctest --test-dir build --config Debug`:
  - nova_core_tests: 59 tests
  - nova_loader_pe_tests: 19 tests
  - nova_loader_module_tests: 25 tests
  - nova_loader_tests: 17 tests
  - nova_runtime_tests: 48 tests
  - nova_sdk_tests: 47 tests
  - **Total: 215 tests, 0 failed (6/6 suites passed)**

Test counts verified against test source `TEST(` registrations and compiled
binaries. Re-ran repeatedly; concurrency tests stable, no flakiness.

> **Correction note (Phase 4.0.5.1):** A subsequent Release Candidate
> verification (Phase 4.0.5) re-counted all `TEST(` registrations across the
> test source files and found `nova_core_tests` contains 60 tests, not 59, and
> the project total is 216, not 215. The counts above reflect the values
> recorded at the time of the Phase 3.8.4 verification. The current verified
> totals are: nova_core_tests: 60, nova_loader_pe_tests: 19,
> nova_loader_module_tests: 25, nova_loader_tests: 17, nova_runtime_tests: 48,
> nova_sdk_tests: 47, **Total: 216**. See `docs/audits/PHASE_4_0_5_1_TEST_COUNT_CORRECTION.md`.

## Phase 3.9.1 — ConfigValue scalar accessor audit (NOVA-006) (OPEN — design limitation)

**Finding:** `ConfigValue::as_bool()`, `as_int()`, `as_double()`, and
`as_string()` (`config.h:50-53`) read raw member storage with no type check.
On type mismatch they silently return zero-initialized defaults
(`false`, `0`, `0.0`, `""`) with no error signal. `as_string()` returns a
`const std::string&` to internal storage.

**Assessment:** Silent-default behavior is inconsistent with the safe nullable
API (`try_as_bool()`/`try_as_int()`/`try_as_double()`/`try_as_string()`,
`config.h:60-71`) and with the array/object nullable accessors. No production
caller outside `config.cpp` serialization relies on scalar accessors; none
rely on silent defaults. The `Config`-layer `get_as<T>()` is type-safe and
throws `NovaError(ErrorCode::InvalidArgument, ...)` on mismatch
(`config.h:163-185`). Changing `as_*` semantics is breaking (alters
`noexcept` guarantee and/or return types on a public header) and requires a
separately approved phase.

**Status:** Documented design limitation. No code change made in this audit.
Recommended: keep `as_*` as-is, document that callers must verify `is_*()`
first; the `try_as_*` API is the supported safe path. Any throwing/checked
scalar accessor would need new method names and separate approval.

## Phase 3.9.2 — Core destructor audit (NOVA-004) (CLOSED — no defect)

**Finding:** Reported inconsistency that `Core::~Core()` does not call
`shutdown()`.

**Assessment:** `Core` (`include/nova/core/init.h`, `src/nova/core/init.cpp`)
owns only an `std::atomic<bool> initialized_`. `initialize()`/`shutdown()`
only toggle that flag and log; they acquire no resources (no threads, handles,
memory, or OS handles). `initialize()` performs no resource acquisition. The
destructor therefore correctly does nothing beyond destroying the trivial
atomic member.

All destruction scenarios (before init, after init, after shutdown, after
partial/failure init) are leak-free and produce no observable inconsistent
state because `Core` owns nothing. `Sdk` and `main.cpp` explicitly call
`shutdown()` before `Core` destruction. `Sdk`/`Runtime` destructors *do*
delegate to `shutdown()` because they own real resources
(`unique_ptr` subsystems, joinable worker threads); `Core` does not.

**Status:** Closed — no actionable defect. The destructor/shutdown difference
is intentional and appropriate to a non-owning state flag. Adding destructor
shutdown would only introduce static-destruction-order logging risk without
benefit.

## Phase 3.9.3 — Findings consolidation (COMPLETE)

Consolidated all session-verified findings into this document. Confirmed that
the original NOVA-001–NOVA-020 audit report could not be recovered and is
marked unavailable. Distinguished fixed defects (Phase 3.8.4), closed
non-issues (NOVA-004), and documented design limitations (NOVA-006).

## Summary of current status

| Item | ID/Phase | Status |
|------|----------|--------|
| Original audit findings | NOVA-001–NOVA-020 | Unavailable — not recovered |
| Config API introduction | 3.8.2 | Context (pre-existing) |
| Config read-only verification | 3.8.3 | Context (pre-existing) |
| Config thread-safety fixes | 3.8.4 | Fixed and verified |
| ConfigValue scalar silent-defaults | NOVA-006 | Open — design limitation |
| Core destructor inconsistency | NOVA-004 | Closed — no defect |
| Findings consolidation | 3.9.3 | Complete |

No source code, tests, or API changes were made during the audit phases
(3.9.1–3.9.3). Source changes were made only in Phase 3.8.4.

## Phase 4.2 — MIT Licensing & Versioning Investigation (COMPLETE)

- MIT License introduced (`LICENSE`).
- Copyright holder: Artyom Filin (twiborg).
- README License section updated to reference MIT License.
- Independent versioning policy investigated; no version declarations changed.

## Phase 4.3 — Versioning Policy (COMPLETE)

- Independent versioning policy approved by maintainer:
  - Project version `0.2.0` (overall release/distribution version,
    `CMakeLists.txt`).
  - Subsystem API versions `0.1.0` (per-subsystem API compatibility versions,
    public headers).
- Versions are intentionally independent; API versions are informational and
  not used for automatic compatibility enforcement.
- README Versioning section added.
- No version declarations changed; no public API signatures changed.

