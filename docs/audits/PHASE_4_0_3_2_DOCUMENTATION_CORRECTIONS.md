# Phase 4.0.3.2 — Public API Documentation Corrections

Applied minimal documentation-only corrections based on the verified findings in
`docs/audits/PHASE_4_0_3_1_FINDING_VERIFICATION.md`. No source, header signature,
test, or public API behavior was changed.

---

## Files modified

1. `include/nova/core/config.h` — Finding H1 (ConfigValue scalar accessors)
2. `include/nova/loader/loader.h` — Findings M1 (module reference lifetime) and M4 (external synchronization)
3. `include/nova/runtime/task/task_scheduler.h` — Finding M2 (`TaskScheduler::wait` behavior)

## Correction 1 — ConfigValue scalar accessors (H1)

### File
`include/nova/core/config.h`, lines 57-62

### Change
Added a documentation comment block immediately after the `as_bool()`/`as_int()`
`/as_double()`/`as_string()`/`as_array()`/`as_object()` accessors:

```
// Unchecked scalar accessors. These do NOT perform runtime type validation:
// calling an accessor whose stored type does not match silently returns a
// default-initialized value (false, 0, 0.0, or "") with no error signal.
// Callers must verify the stored type with is_*() before using these
// accessors. When the stored type is uncertain, prefer the try_as_*()
// accessors below, which return std::nullopt / nullptr on type mismatch.
```

### Finding addressed
H1: `as_*` accessors silently return defaults on type mismatch with no error
signal. Callers must verify `is_*()` first; `try_as_*` is the supported safe path.

### Public API signature change
None. All `as_*` and `try_as_*` signatures unchanged.

---

## Correction 2 — Loader module reference lifetime (M1)

### File
`include/nova/loader/loader.h`, lines 92-96 and 103-107

### Change
Added reference-lifetime documentation to both `find_module_by_name` and
`find_module_by_path`:

```
// The returned reference refers to internal loader storage. A subsequent
// call to enumerate_modules() invalidates previously obtained references;
// do not retain a reference across re-enumeration. Copy the ModuleInfo if
// the data must outlive the current enumeration snapshot.
```

### Finding addressed
M1: `find_module_by_name`/`find_module_by_path` return `reference_wrapper<const
ModuleInfo>` into internal `modules_` storage; `enumerate_modules()` reallocates
that storage (module_enumerator.cpp:312), invalidating prior references.

### Public API signature change
None. Both `find_module_by_name` and `find_module_by_path` signatures unchanged.

---

## Correction 3 — Loader external synchronization (M4)

### File
`include/nova/loader/loader.h`, lines 40-44

### Change
Rewrote the class-level thread-safety note to make the external synchronization
requirement prominent:

```
// Repeated initialization/shutdown is handled safely. The PE parsing methods
// are stateless and thread-safe. Enumeration and lifecycle methods are
// guarded by an internal mutex, but concurrent initialize/shutdown/enumerate
// calls are NOT safe: the caller must externally synchronize them. This class
// is safe to use from a single thread without external synchronization.
```

### Finding addressed
M4: The previous wording implied the internal mutex made lifecycle/enumeration
safe for concurrent use; the implementation (module_enumerator.cpp has no
internal mutex) requires caller-side synchronization for concurrent
`initialize`/`shutdown`/`enumerate`.

### Public API signature change
None.

---

## Correction 4 — TaskScheduler::wait behavior (M2)

### File
`include/nova/runtime/task/task_scheduler.h`, lines 103-106

### Change
Replaced the inaccurate "Block until the task completes (or forever if
unknown)." comment with:

```
// Block until the task reaches a terminal state (Completed, Failed, or
// Cancelled), or returns immediately if the task id is unknown. Tasks that
// remain queued when stop() is called never reach a terminal state and will
// block wait() indefinitely.
```

### Finding addressed
M2: The implementation (`task_scheduler.cpp:114-125`) returns immediately for
unknown IDs (the predicate is satisfied immediately) and blocks only for tasks
that reach a terminal state. The previous comment incorrectly claimed unknown
IDs block forever.

### Public API signature change
None. `wait(TaskId id)` signature unchanged.

---

## Verification

1. **Exact diff**: All changes are confined to comment blocks in the three
   header files listed above. No function signatures, class definitions, or
   member declarations were altered.
2. **Only documentation/comments modified**: Confirmed by inspecting each edited
   region — every change is a `//` comment; no code tokens changed.
3. **All four findings addressed**:
   - H1 (config.h): scalar accessor type-validation documentation added.
   - M1 (loader.h): module reference lifetime documented.
   - M4 (loader.h): external synchronization requirement made prominent.
   - M2 (task_scheduler.h): `wait()` behavior corrected.
4. **No source, test, or public API signature changed**: The public API surface
   is identical before and after this phase.

No source files, tests, or public API signatures were modified in this phase.
