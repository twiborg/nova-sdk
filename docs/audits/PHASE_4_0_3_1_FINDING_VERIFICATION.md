# Phase 4.0.3.1 — Public API Finding Verification

Focused verification of the four findings flagged in Phase 4.0.3:
- H1: `ConfigValue` scalar accessor silent defaults
- M1: Loader module reference lifetime
- M2: `TaskScheduler::wait` behavior
- M3: Runtime scheduler/dispatcher access after shutdown
Plus a brief review of M4 (Loader external synchronization).

This is an investigation-only phase. No source, header, test, or documentation files were modified.

---

## Finding 1 — ConfigValue scalar accessors (H1)

### Verdict: Confirmed

### Implementation evidence
`include/nova/core/config.h:50-53`:

```cpp
[[nodiscard]] bool as_bool() const noexcept { return bool_; }
[[nodiscard]] int64_t as_int() const noexcept { return int_; }
[[nodiscard]] double as_double() const noexcept { return double_; }
[[nodiscard]] const std::string& as_string() const noexcept { return string_; }
```

These read raw member storage with no `type_` check. A default-constructed `ConfigValue` is `Type::Null` with `int_ = 0`, `double_ = 0.0`, `bool_ = false`, `string_ = ""`. Calling `as_int()` on a string/stored value silently returns `0` with no way for the caller to detect the mismatch.

The safe nullable alternatives exist at `config.h:60-77` (`try_as_bool`/`try_as_int`/`try_as_double`/`try_as_string`), which return `std::nullopt`/`nullptr` on type mismatch.

The `Config`-layer `get_as_impl<T>` (`config.h:163-185`) throws `NovaError(ErrorCode::InvalidArgument, ...)` on mismatch, so the type-safety gap is confined to the `ConfigValue` level.

### Concrete usage scenario
```cpp
nova::core::ConfigValue v(std::string("8080"));
int64_t port = v.as_int(); // silently returns 0, no error signal
```

### Production-code dependence
Only `src/nova/core/config.cpp:333-336` uses the scalar `as_*` accessors, and each call is guarded by a `switch (v.type())` that matches the accessor to the stored type (e.g. `case Type::Int: v.as_int()`). No production caller outside `config.cpp` uses scalar accessors, and none rely on silent defaults. Tests cover `try_as_*` mismatch behavior (`config_test.cpp:272-280`) but do not pin the silent-default `as_*` behavior.

### Intent / documentation
Not documented as an intentional contract. It is a pre-nullable-API oversight: the codebase explicitly introduced `try_as_*` (`config.h:57-77`) as "Safe, nullable accessors... Never ... throw," implying the unchecked `as_*` accessors are the unsafe legacy path.

### Actual severity
High (latent data-integrity risk: a port read as `0` could bind to an invalid port; indistinguishable from a legitimate stored `0`).

### Minimal recommended action
Document that callers must verify `is_*()` before calling `as_*()`, and promote `try_as_*` as the supported safe path. Do not change `as_*` semantics in 1.0 (would break `noexcept` and return types on a public header).

### Public API signature change needed?
No. Documentation-only before 1.0. A throwing/checked scalar accessor would require new method names and a separately approved, breaking phase.

---

## Finding 2 — Loader module reference lifetime (M1)

### Verdict: Confirmed

### Implementation evidence
`include/nova/loader/module/module_enumerator.cpp`:

- `enumerate()` at line 312: `modules_ = std::move(collected);` — this move-assignment reallocates the `std::vector<ModuleInfo>`, invalidating all references, pointers, and iterators into the previous `modules_` storage.
- `find_by_name()` at lines 329-337: returns `std::cref(info)` where `info` is an element of `modules_`.
- `find_by_path()` at lines 339-348: returns `std::cref(info)` where `info` is an element of `modules_`.

`include/nova/loader/loader.h:92-99`:
`Loader::find_module_by_name` / `find_module_by_path` delegate to the enumerator and return `std::optional<std::reference_wrapper<const ModuleInfo>>`.

### Concrete usage scenario
```cpp
auto ref = loader.find_module_by_name("NOVA.exe");   // cref into modules_
loader.enumerate_modules();                            // move-assigns modules_, invalidating ref
ref->get().name;                                       // dangling reference -> UB
```

### Repeated enumeration / concurrency
- Repeated `enumerate_modules()` invalidates references from the previous enumeration (move-assignment reallocates).
- `ModuleEnumerator` has no internal mutex; `find_by_name`/`find_by_path` read `modules_` without locking. The `Loader` facade guards lifecycle/enumeration with `mutex_` (`loader.h:115`) and documents that concurrent `initialize`/`shutdown`/`enumerate` calls require external synchronization (`loader.h:42-44`). Calling `find_module_by_name` concurrently with `enumerate_modules()` is a data race on `modules_`.

### Actual severity
Medium (dangling reference is real and easily triggered by re-enumeration; concurrent use is a data race).

### Minimal recommended action
Add prominent lifetime documentation stating that references are invalidated by any subsequent `enumerate_modules()` call and must not be used concurrently with enumeration. Consider returning `std::optional<ModuleInfo>` by value in a post-1.0 phase.

### Public API signature change needed?
Not before 1.0 (documentation-only). A by-value return would be source-incompatible.

---

## Finding 3 — TaskScheduler wait behavior (M2)

### Verdict: Partially confirmed (the "indefinite block for unknown IDs" claim is NOT accurate)

### Implementation evidence
`include/nova/runtime/task/task_scheduler.cpp:114-125`:

```cpp
void TaskScheduler::wait(TaskId id) {
    std::unique_lock<std::mutex> lock(mutex_);
    task_cv_.wait(lock, [&] {
        auto it = tasks_.find(id);
        if (it == tasks_.end()) {
            return true;   // unknown ID -> predicate satisfied immediately
        }
        return it->second.state == TaskState::Completed ||
               it->second.state == TaskState::Failed ||
               it->second.state == TaskState::Cancelled;
    });
}
```

- **Unknown ID:** `tasks_.find(id) == end()` → predicate returns `true` immediately → `wait` returns immediately. It does **not** block forever.
- **Valid, completed/failed/cancelled ID:** predicate already true → returns immediately.
- **Valid, queued/running ID:** blocks until the worker sets the task to a terminal state and calls `task_cv_.notify_all()` (line 300).

`stop()` at lines 32-51 sets `running_ = false`, notifies `queue_cv_`, and joins workers. `worker_loop()` (lines 229-308) transitions every task to a terminal state (`Completed`/`Failed`/`Cancelled`) and notifies `task_cv_` (line 300), so waiters are unblocked before workers exit.

`state(TaskId)` (lines 96-103) and `result(TaskId)` (lines 105-112) return `std::nullopt` for unknown IDs — safe, no blocking.

### Concrete usage scenario
```cpp
scheduler().wait(99999); // unknown ID -> returns immediately (NOT indefinite)
```

### Shutdown interaction
Shutdown does not leave waiters blocked: workers complete/cancel all tasks and notify `task_cv_` before exiting. A `wait()` on a task that never completes because the scheduler was stopped before the task ran: if the task remains `Queued` (never picked up), `wait` would block indefinitely because `stop()` does not transition queued tasks to a terminal state nor notify `task_cv_` for them. This is a narrower, real edge case.

### Documentation accuracy
The header comment (`task_scheduler.h:103-104`) says "Block until the task completes (or forever if unknown)." The "forever if unknown" portion is **inaccurate** — unknown IDs return immediately.

### Actual severity
Medium for the documentation inaccuracy; Low for the genuine edge case (queued task never terminal after stop).

### Minimal recommended action
Correct the documentation: unknown IDs return immediately; `wait()` blocks only for tasks that reach a terminal state. Document that a `Queued` task never started before `stop()` may block `wait()` indefinitely.

### Public API signature change needed?
No. Documentation-only.

---

## Finding 4 — Runtime access after shutdown (M3)

### Verdict: Not confirmed as a safety defect (documentation gap only)

### Implementation evidence
`include/nova/runtime/runtime.h:77-78`:
`scheduler()` and `dispatcher()` return non-const references (`TaskScheduler&`, `EventDispatcher&`), marked `noexcept`.

`include/nova/runtime/runtime.cpp:93-96`:
```cpp
void Runtime::shutdown() noexcept {
    stop();
    atomic_state_.store(RuntimeState::Stopped, std::memory_order_release);
}
```
`stop()` (lines 79-91) calls `scheduler_.stop()`, which joins all worker threads (`task_scheduler.cpp:32-51`).

`Runtime::~Runtime()` (runtime.h:51) calls `shutdown()` — the subsystems (`scheduler_`, `dispatcher_`, declared at runtime.h:98-99) remain valid members for the entire lifetime of the `Runtime` object, including during destructor execution.

### Behavior after shutdown (Runtime still alive)
- `scheduler().submit(...)` returns `std::nullopt` when `running_ == false` (`task_scheduler.cpp:57-58`) — safe no-op.
- `scheduler().cancel(...)` returns `false` for non-queued tasks (`task_scheduler.cpp:79-94`) — safe no-op.
- `scheduler().state(...)` / `result(...)` return `std::nullopt` for unknown IDs (`task_scheduler.cpp:96-112`) — safe.
- `EventDispatcher::subscribe`/`dispatch`/`unsubscribe` remain functional after `stop()` (no state prevents them); `dispatch` catches handler exceptions (`event_dispatcher.cpp:85-89`).

### Behavior after Runtime object destruction
Accessing `scheduler()` or `dispatcher()` through a destroyed `Runtime` object is ordinary C++ use-after-free of the `Runtime` object — not a NOVA-specific defect, and not something the API can prevent without fundamental redesign.

### Concrete usage scenario
```cpp
runtime.shutdown();
runtime.scheduler().submit(...); // safe: returns nullopt (not running)
```

### Actual severity
Low — operations fail safely (no-op / nullopt) while the Runtime object is alive. Use-after-free of a destroyed Runtime is standard C++ UB, not a NOVA defect.

### Minimal recommended action
Document that `scheduler()`/`dispatcher()` references are valid only for the lifetime of the `Runtime` object and that submission after `stop()`/`shutdown()` is a safe no-op.

### Public API signature change needed?
No. Documentation-only.

---

## Brief review — M4 (Loader external synchronization)

### Verdict: Confirmed as accurately documented

`include/nova/loader/loader.h:42-44` states: "enumeration and lifecycle methods are protected by an internal mutex and are safe to call from any single thread at a time, but concurrent initialize/shutdown/enumerate calls are externally synchronized by the caller."

`loader.h:115` declares `mutable std::mutex mutex_`, and `module_enumerator.cpp` (`ModuleEnumerator`) has **no** internal mutex — its `modules_` vector is accessed unsynchronized in `find_by_name`/`find_by_path` (cpp:329-348) and mutated in `enumerate()` (cpp:312). This matches the documented requirement that the caller serialize concurrent lifecycle/enumeration calls. No discrepancy found.

---

## Summary table

| Finding | ID | Verdict | Actual severity | Signature change needed? |
|---|---|---|---|---|
| ConfigValue scalar silent defaults | H1 | Confirmed | High | No (doc only before 1.0) |
| Loader module reference lifetime | M1 | Confirmed | Medium | No (doc only before 1.0) |
| TaskScheduler wait indefinite block | M2 | Partially confirmed (unknown-ID block claim inaccurate) | Medium (doc) / Low (edge case) | No |
| Runtime scheduler/dispatcher after shutdown | M3 | Not confirmed as defect | Low | No |
| Loader external sync requirement | M4 | Confirmed accurate | Informational | No |

---

## Recommendation on API freeze

NOVA can proceed to API freeze with **minor changes**. None of the four verified findings requires a public API signature change before 1.0; all are addressable through documentation corrections and clarifications:

1. H1: document `as_*` precondition + promote `try_as_*`.
2. M1: document reference invalidation on re-enumeration and concurrent-use data race.
3. M2: correct `wait()` documentation (unknown IDs return immediately; queued-but-never-started tasks may block).
4. M3: document `scheduler()`/`dispatcher()` lifetime tied to the `Runtime` object and safe no-op behavior after `stop()`.

No source files were modified in this verification phase.

No source, header, test, or documentation files were modified in this phase.
