# Plan: convert `_jpype` to multi-phase init (`Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`)

## Status (2026-07-10): scoped, not started

## Context

Subinterpreters currently work in **legacy-style** mode only: shared process
GIL, shared object allocator (`use_main_obmalloc=1`,
`check_multi_interp_extensions=0`), per [[jpype_subinterpreter_difficulty]]
(resolved for lifecycle correctness — start/register/eval/close verified
with two subinterpreters + main running concurrently, no cross-talk, no
hangs). The reason it's legacy-style rather than true PEP 684 own-GIL
isolation is that `_jpype` is a **single-phase-init** extension
(`PyModule_Create(&moduledef)` called directly in `PyInit__jpype`, no
`PyModuleDef_Slot` array) — CPython requires multi-phase init
(`Py_mod_exec` + declaring `Py_mod_multiple_interpreters` support) before an
extension is even eligible to opt into `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`.

The Java-core side of global-state removal is **already done** (see
[[jpype_final_mission_status]] / `plan/Globals.md`, DONE 2026-07-10,
475/475) — that work converted `native/common/`'s JNI global refs into
per-`JPContext` `jref`/`GlobalPool`-routed handles or true process-wide
immortals, and threads `JPContext` through explicitly rather than via a
singleton. This plan is the follow-on: the **Python C-extension side**
(`native/python/`) — the module-init mechanism itself, plus a handful of
smaller `native/python/`-local globals never audited by the Globals.md pass
because they're Python-C-API state, not JNI global refs.

An Explore survey (this session, file:line-grounded, no code changes made)
found the codebase already unusually well-prepared: **no static
`PyTypeObject`s to convert** (every module type is already a heap type
built via `PyType_FromSpec`/`PyType_FromSpecWithBases` and stored on
`PyJPModuleState`), and **no cached global state pointer** (every call site
fetches state via `PyModule_GetState()`). The actual remaining work is
narrow — one mechanical restructuring of the init function, plus cleanup of
a few small process-global variables.

## A. Module init structure — the actual single→multi-phase conversion

`native/python/pyjp_module.cpp:1202-1212` — current `PyModuleDef`:
```cpp
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT, "_jpype", "jpype module",
    sizeof(PyJPModuleState), moduleMethods,
    nullptr,  // m_slots - THIS is what needs to become a real slot array
    PyJPModule_traverse, PyJPModule_clear, PyJPModule_free
};

PyMODINIT_FUNC PyInit__jpype()
{
    PyObject* module = PyModule_Create(&moduledef);   // single-phase call
    ...                                                 // all init work inline
    return module;
}
```

**Change**:
1. Add a `static PyModuleDef_Slot jpype_slots[] = { {Py_mod_exec,
   PyJPModule_exec}, {Py_mod_multiple_interpreters,
   Py_MOD_PER_INTERPRETER_GIL_SUPPORTED}, {0, nullptr} };` and set
   `moduledef.m_slots = jpype_slots` (replacing the `nullptr`).
2. Split `PyInit__jpype` into two functions:
   - `PyInit__jpype()` becomes a thin `return PyModuleDef_Init(&moduledef);`
   - A new `PyJPModule_exec(PyObject *module)` (signature: `int(PyObject*)`,
     returns 0 on success / -1 on failure per `Py_mod_exec` contract) takes
     everything currently in the body of `PyInit__jpype`
     (`pyjp_module.cpp:1227-1284`: state zeroing, `module_dict`/
     `interp_state`/`is_main_interpreter` setup, `__version__`,
     `__builtins__`, `strings_dict`/`Py_JP_CALL`/`class_magic` setup, the
     twelve `*_initType` calls, `st->context = new JPContext(st)`) — same
     logic, just relocated and adjusted to return `int` instead of
     `PyObject*` (error paths become `return -1;` instead of `return
     nullptr;`, no `Py_DECREF(module)` needed since `Py_mod_exec` failure is
     handled by the interpreter, not the callee).
3. **`Py_GIL_DISABLED`/`PyUnstable_Module_SetGIL` call**
   (`pyjp_module.cpp:1222-1224`) needs revisiting: today it's called
   post-hoc on the `PyModule_Create`-built object. With slot-based init this
   should become a static `{Py_mod_gil, Py_MOD_GIL_NOT_USED}` slot entry
   instead (conditionally compiled under `#ifdef Py_GIL_DISABLED`), matching
   the same "declare capability as a slot, don't mutate after creation"
   pattern the new `Py_mod_multiple_interpreters` slot also requires. Needs
   verification against the CPython docs for whether `Py_mod_gil` requires
   the same treatment or can stay post-hoc — flag as an open question if the
   two mechanisms turn out to be inconsistent (free-threading GIL slot vs.
   subinterpreter GIL slot may have been introduced in different CPython
   versions with different constraints; check version gating needed here
   too, since this repo supports Python versions before 3.12/3.13).
4. `moduledef.m_size = sizeof(PyJPModuleState)` is **already correct** for
   multi-phase init (per-module-instance allocation, effectively
   per-interpreter) — no change needed.

## B. Static `PyTypeObject` → heap type conversions

**None needed.** Confirmed via grep — no `static PyTypeObject`/file-scope
`PyTypeObject` struct literals exist anywhere in `native/python/`. Every
module-level type (`PyJPClass_Type`, `PyJPObject_Type`,
`PyJPException_Type`, `PyJPComparable_Type`, `PyJPArray_Type`,
`PyJPArrayPrimitive_Type`, `PyJPBuffer_Type`, `PyJPChar_Type`,
`PyJPField_Type`, `PyJPMethod_Type`, `PyJPMonitor_Type`, `PyJPProxy_Type`,
`PyJPNumberLong_Type`/`Float`/`Bool`, `PyJPClassHints_Type`,
`PyJPPackage_Type`) is built via `PyType_FromSpec`/
`PyType_FromSpecWithBases` inside a `*_initType(module, st)` call and stored
exclusively on `PyJPModuleState` (`native/python/include/pyjp.h:146-162`) —
already correct for multi-phase init, since `Py_mod_exec` re-runs these
`*_initType` calls once per interpreter, producing a fresh heap type per
interpreter as required.

The `PyType_Spec`/`PyType_Slot`/`PyMethodDef`/`PyGetSetDef` tables
themselves correctly remain `static` at file scope — immutable read-only
metadata, exactly CPython's own recommended pattern. No change.

One thing to retest under multi-phase init specifically: the pre-3.12
fallback path `PyJPClass_FromSpecWithBases`
(`native/python/pyjp_class.cpp:83-140`), which hand-builds a heap type off
`st->PyJPClass_Type->tp_alloc` to smuggle extra Java-slot space onto
arbitrary objects. Already per-`st`/heap-allocated, so structurally fine,
but it pokes at `tp_*` slots directly and should get explicit test coverage
once `Py_mod_exec` starts running this per-interpreter instead of once at
process startup.

## C. Other global/process-mutable state to fix

1. **`native/python/pyjp_module.cpp:386`** — `static string jarTmpPath;`,
   written in `PyJPModule_startup` (line 419), read/deleted in
   `PyJPModule_shutdown` (lines 490-494). Genuine cross-interpreter race:
   two subinterpreters each starting a JVM via the jar-path launch option
   would clobber each other's temp path. **Fix**: add a `std::string
   jarTmpPath;` member to `PyJPModuleState` (`pyjp.h`), replace the two call
   sites to use `st->jarTmpPath`. Small, mechanical.

2. **`native/python/pyjp_module.cpp:986`** — `int _PyJPModule_trace;`
   (`extern "C"` in `native/common/include/jp_tracer.h:88`, consumed by
   `native/common/jp_tracer.cpp:87-193`), toggled by the debug-only
   `trace()` Python method. Existing code comment explicitly accepts this as
   shared: *"used only in debugging and does not need to be subinterpreter
   safe."* **Decision needed, not a blocking fix**: either (a) keep as an
   intentionally-shared process-wide debug knob (document explicitly in the
   multi-phase-init changeset so a future auditor doesn't flag it as an
   oversight), or (b) make it an atomic/TLS if strict
   `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED` correctness is interpreted to
   require zero shared mutable state even for debug-only paths. Recommend
   (a) — CPython's own multi-phase-init contract is about correctness of
   the module's *user-facing* behavior across interpreters, not about
   eliminating every debug hook; document and move on.

3. **`native/python/pyjp_array.cpp:460`** — `PyTypeObject
   *PyJPArrayPrimitive_Type = nullptr;` — dead file-scope non-static global,
   never read (the real value lives at `st->PyJPArrayPrimitive_Type`, set
   line 482). Delete for cleanliness; would otherwise look like exactly the
   kind of leftover single-phase-init artifact a future audit trips over.

4. **`native/python/include/pyjp.h:232-234`** — `#ifdef JP_INSTRUMENTATION`
   header-scope `int fault_code = 0;` — not `static`/`inline`, so including
   this header in more than one TU with `JP_INSTRUMENTATION` defined is an
   ODR violation. Appears dead; the real mechanism is `st->fault_code`
   (`pyjp.h:229`, a `uint32_t` member of `PyJPModuleState`), consumed by
   `PyJPModule_fault` (`pyjp_module.cpp:1000-1022`). **Fix**: delete the
   header-scope declaration.

5. **`native/python/pyjp_misc.cpp:208-242`** — two consecutive
   `#ifdef JP_INSTRUMENTATION` blocks both (re)defining
   `PyJPModuleFault_check`/`PyJPModuleFault_throw`. The first
   (lines 208-225) declares its own shadow `int fault_code = 0;` and
   references an undeclared `st`; the second (227-242) references an
   undeclared `_PyJPModule_fault_code`. Since `JP_INSTRUMENTATION` is
   defined whenever `-DENABLE_COVERAGE=ON` is set (`CMakeLists.txt:87`),
   **this file will fail to compile under coverage builds as currently
   written** — a pre-existing bug, not something the multi-phase-init
   rewrite introduces, but directly adjacent to the global state being
   audited here. **Fix**: delete both stale blocks; `st->fault_code` is the
   one working mechanism and needs no duplicate.

6. **`native/python/pyjp_module.cpp:1025-1035`** — `#ifdef ANDROID`
   `PyJPModule_bootstrap` references an undefined `JPContext_global`
   (a would-be process-global `JPContext*` singleton — grep confirms it's
   never declared/defined anywhere) and an undeclared `st`. Dead/
   non-compiling Android-only code path, evidently unbuilt/untested today.
   **Not part of this rewrite's scope** — flag only: if this path is ever
   resurrected, `JPContext_global` is precisely the anti-pattern
   per-interpreter isolation requires eliminating, and it should be
   rewritten to thread state through like the rest of the file (mirroring
   `st->context`) rather than fixed as a singleton.

## D. Already fine / no change needed

- `PyJPModuleState` fetched exclusively via `PyModule_GetState()` at every
  call site (`pyjp_module.cpp:67-75`) — no cached global state pointer
  exists anywhere.
- `JPContext` is allocated once per module-init call
  (`st->context = new JPContext(st)`) and threaded through explicitly on
  the `native/common/` side via `context->modulestate` at every relevant
  call site (`jp_bridge.cpp`, `jp_typefactory.cpp`, `jp_exception.cpp`,
  `jp_reference_queue.cpp`, `jp_class.cpp`, `jp_classhints.cpp`,
  `jp_functional.cpp`, `jp_chartype.cpp`, `jp_booleantype.cpp`) — no
  singleton reach-through.
- GIL/thread-state handling (`jp_pythontypes.cpp:406,418,438,475,496,501`)
  uses `PyGILState_Ensure/Release` and `PyEval_SaveThread/RestoreThread`,
  both thread-state-scoped APIs that already operate correctly against
  whichever interpreter's thread state is current — no change needed for
  multi-phase init itself (this is the exact code already fixed for
  lifecycle correctness this session, see [[jpype_gil_reacquire_bug]]).
- No C-side module-global Python exception singletons exist.
  `PyJPException_Type` is a heap type (see B) built with `PyExc_Exception`
  (a CPython static *builtin* — explicitly permitted as a base; builtin
  static types are shared read-only singletons by design) plus
  `st->PyJPObject_Type`.
- `allocSpec`/`local_alloc_type` (`pyjp_value.cpp:328-398`) is deliberately
  thread-local (`PyThreadState_GetDict()`), not a global cache — already
  correctly scoped.
- `PyJPModule_loadResources`/`_clearResources`/`_ready`/`_traverse`/
  `_clear`/`_free` all operate exclusively on their `st`/`module`
  parameters — already structurally ready to run once per interpreter under
  `Py_mod_exec` with no modification.

## Known non-goal / hard external constraint

The JNI specification only supports **one JVM per process** in practice —
`JNI_CreateJavaVM`/`JNI_GetCreatedJavaVMs` are resolved per-`JPContext` on
the `native/common/` side, with no `JavaVM*` cache in `native/python/`, but
this is a constraint of the JNI layer itself, not something fixable by this
rewrite. Even after `_jpype` is proven per-interpreter-GIL-safe at the
Python-C-API level, running JPype in more than one subinterpreter
*simultaneously, each independently starting its own embedded JVM*, remains
bounded by this. **Scope of this plan is limited to making `_jpype` a
correct, safe multi-phase-init extension** (i.e., what CPython requires to
even offer `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`); it does not promise
multi-JVM-per-process, which is a separate, likely much harder problem (or
possibly a non-problem if the existing single-JVM-shared-across-
subinterpreters model, already working today per
[[jpype_subinterpreter_difficulty]], remains the intended usage pattern —
own-GIL isolation is valuable even with one shared JVM, since it removes
Python-level cross-talk risk between subinterpreters' non-Java state).

## Suggested execution order

1. Item C.3/C.4/C.5 dead-code cleanup first — zero-risk, immediately makes
   the subsequent grep/audit trail cleaner (no noise from dead globals that
   look real but aren't).
2. Item C.1 (`jarTmpPath` → `PyJPModuleState`) — small, mechanical,
   independently testable/committable.
3. Item A (the actual `PyModuleDef_Slot`/`PyJPModule_exec` split) — the core
   rewrite. Do this as its own commit once (1) and (2) are in, so the
   before/after diff on the real structural change is minimal and reviewable.
4. Item C.2 (`_PyJPModule_trace` policy decision) — documentation-only if
   option (a) is chosen (recommended); otherwise implement alongside (3).
5. Retarget the `PyGIL_DISABLED` slot question (A.3) — needs a version-gated
   CPython-docs check before implementing; do this as part of step 3 once
   the answer is confirmed, not left dangling after.

## Verification

- Full `mvn -q test -Dpython.executable=python3.12-dbg` (this is a
  3.12+-gated feature; python3.10 build should be unaffected and used as
  the regression baseline — expect unchanged pass count there, since
  `#if PY_VERSION_HEX>=0x030c0000` gating already exists for the
  subinterpreter-specific code paths).
- Rerun the full `SubInterpreterNGTest` suite (7 tests, including the two
  stress tests added this session —
  `testMixAndMatchNoCrossTalkNoHangs`/`testConcurrentThreadDoesNotBlockAfterSubEval`)
  under `python3.12-dbg` after the rewrite — these are the existing
  ground-truth correctness tests and must stay green.
- New test to add once multi-phase init lands: switch a subinterpreter's
  `PyInterpreterConfig` from the current legacy config
  (`use_main_obmalloc=1`, `check_multi_interp_extensions=0`) to a real
  isolated config (`use_main_obmalloc=0`, `check_multi_interp_extensions=1`,
  `.gil=PyInterpreterConfig_OWN_GIL`) and confirm `_jpype` imports/runs
  without CPython refusing the extension (the actual proof the rewrite
  achieved its goal — today this config combination would fail at import
  time precisely because `_jpype` doesn't declare
  `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED`).
- `python3.12-dbg`'s extra runtime assertions (the reason it was fetched
  this session in the first place, per [[jpype_subinterpreter_difficulty]])
  should stay in the loop for this work — own-GIL isolation bugs are
  exactly the class of memory-safety issue that build catches that a
  release build silently tolerates.
