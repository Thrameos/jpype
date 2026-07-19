# GCReactivation: re-enable the Java<->Python cross-triggered GC

## Status (2026-07-18): CLOSED. GC reactivated, real root cause found and
fixed (see #3 below - it wasn't actually a main/sub-interpreter
interaction bug, it was a broken JNI method lookup that would abort
interpreter startup for *every* interpreter, main or sub, the instant GC
init ran - which is exactly why re-enabling it previously looked like a
crash "when working with subinterpreters": subinterpreter tests are the
first thing in the suite order to spin up a second interpreter and hit
this path again). Verified via both `make -f project/dev.mk test`
(python3.10 + python3.12, 1752/1752 pytest) and `mvn -o verify` under both
`-Dpython.executable=python3.10` and `=python3.12` (940/940 NGTest,
including `SubInterpreterNGTest`/`SubInterpreterBuilderNGTest` under
3.12 where subinterpreters actually run rather than skip).

## Background
`JPGarbageCollection` (`native/common/jp_gc.cpp`) hooks Python's `gc`
module callbacks to Java's `System.gc()` and vice versa (RSS-based
heuristic to decide when to trigger). It was fully implemented but never
wired up: `JPContext::initializeResources` (`jp_context.cpp:649`) has
`//m_GC->init(frame);` commented out, with a FIXME saying it depends on
resources (`PyJPModuleState::gc_callbacks`/`collect`) that aren't
populated yet at that point in the init sequence - see
[[jpype_dead_code_audit_status]] (this was originally flagged as "dead
code" in the audit, then corrected: it's disabled-pending-reactivation,
not abandoned - see [[jpype_feedback_flag_gated_not_dead]]).

The user's ask: reactivate it, and resolve a previously-seen crash
involving interaction between the main interpreter and subinterpreters.

## Root cause #1 (confirmed): init-ordering bug, exactly as the FIXME says
`JPContext::attachJVM()` calls `initializeResources()` (native/common/
jp_context.cpp:443), which is where the commented-out `m_GC->init(frame)`
call lives. But `PyJPModule_loadResources()` - which populates
`st->gc_callbacks` and `st->collect` (native/python/pyjp_module.cpp:174,
181) - only runs *after* `attachJVM()` returns, back in `launch()`
(native/common/jp_bridge.cpp:184-190). So at the point the commented-out
call would have fired, `st->gc_callbacks`/`st->collect` are still
null/empty - `JPGarbageCollection::init`'s own "defensive check" (jp_gc.
cpp:160) would have silently skipped hooking up the callback every single
time, meaning even if reactivated in-place it would never actually
register with Python's `gc.callbacks`.

**Fix**: move the `m_GC->init(frame)` call out of `JPContext::
initializeResources` and into `launch()` itself, right after
`PyJPModule_loadResources()` runs (jp_bridge.cpp:190) - since `launch()` is
the single entry point used for *both* the main interpreter
(`Java_org_jpype_internal_NativeLauncherControl_startMain`) and every
subinterpreter (`..._startSubInterpreter`), this naturally gives every
interpreter (main and sub) its own correctly-initialized
`JPGarbageCollection` tied to its own `JPContext`/module state - no
special-casing needed for "is this the main interpreter or a sub."

## Root cause #2 (suspected, not yet confirmed): process-wide statics used
by a per-interpreter subsystem
`getWorkingSize()`'s `USE_PROC_INFO` path (`jp_gc.cpp:56-63`, generic
`__linux__` without `__GLIBC__`) uses **file-static** `statm_fd`/
`page_size`, opened in `JPGarbageCollection::init()` and closed in
`::shutdown()`. Since each interpreter (main + every sub) gets its own
`JPGarbageCollection` instance but they'd all share this one process-wide
fd: interpreter A's `shutdown()` (e.g. `SubInterpreter.close()`) closing
`statm_fd` while interpreter B is still running would make B's next
`getWorkingSize()` call read from a closed fd. `read()` returns -1/EBADF
(not a segfault) and the function's fallthrough logic returns 0 for that
case - wrong stats, not a crash, but still a real cross-interpreter bug in
that code path. **Not the reproduction path on this dev machine** (glibc
is defined here, so `USE_MALLINFO` is compiled instead, which has no such
shared-fd state) but worth fixing while touching this file, since the
compiled-out branch is still a live correctness bug on non-glibc Linux
(e.g. musl/Alpine) builds.

**Fix**: make `statm_fd`/`page_size` members of `JPGarbageCollection`
instead of file statics, so each interpreter's instance owns its own fd.

## Root cause #3 (CONFIRMED - this was "the crash"): broken JNI method
lookup in `JPGarbageCollection::init()`, wrong class entirely
Once fix #1 made `init()` actually run (it never had, previously - see
root cause #1), `mvn -o verify` immediately failed **every single test**
at `setUpClass`, first surfacing as
`org.jpype.script.JPypeScriptEngineNGTest.setUpClass ... NoSuchMethodError:
static Lorg/jpype/internal/NativeContext;.getTotalMemory()J`, cascading
into `NullPointerException: ... Interpreter.getBuiltIn() is null`
everywhere else (interpreter startup aborted before finishing, so nothing
downstream was initialized).

`init()` (jp_gc.cpp, now-removed lines) did:
```cpp
jclass ctxt = frame.getContext()->m_ContextClass;
_ContextClass = ctxt;
_totalMemoryID = frame.GetStaticMethodID(ctxt, "getTotalMemory", "()J");
// ...and freeMemory/maxMemory/usedMemory/heapMemory the same way
```
`m_ContextClass` is `NativeContext`, which has **no** memory-stat methods
at all - `getTotalMemory`/`getFreeMemory`/`getMaxMemory`/`getUsedMemory`/
`getHeapMemory` actually live as public static methods on
`org.jpype.internal.Support` (`Support.java:216-243`), and `JPContext::
initializeResources` **already** resolves them correctly against that
class earlier in the same init sequence (`jp_context.cpp:565-589`, into
`m_SupportClass`/`m_Support_GetTotalMemoryID`/etc - fields that already
exist on `JPContext` for exactly this purpose). `JPGarbageCollection::
init()` was doing its own redundant *and* wrong lookup instead of just
using those. `GetStaticMethodID` for a genuinely nonexistent method throws
a Java `NoSuchMethodError` - uncaught here, this propagates straight
through interpreter startup and aborts it for every interpreter, not just
subinterpreters. (The result of that broken lookup - `_ContextClass`/
`_totalMemoryID`/etc - was only ever read from inside an `#if 0`-disabled
debug-print block in `onEnd()`, so this bug had zero chance of being
caught by normal use even if `init()` hadn't crashed outright: it was
purely a self-inflicted crash on call, with no working payoff even had it
"succeeded.")

This explains the "crash when working with main vs sub interpreters"
framing precisely: main-interpreter-only test runs/usages of jpype
wouldn't necessarily hit `NativeContext.getTotalMemory` at all if nothing
else in that particular session's code path forced full startup through
this line before something else failed first, but *any* session that
spins up a second interpreter (a subinterpreter) forces `launch()` to run
again, hits `init()` again, and reliably blows up the moment GC was
switched on - consistent with GC looking like it specifically broke
subinterpreters when what it actually broke was every interpreter's
startup, subinterpreters just being the reliable second trigger in a
session that creates one.

**Fix**: deleted the broken lookup and the dead `#if 0` block using it,
along with the now-fully-unused `_ContextClass`/`_totalMemoryID`/
`_freeMemoryID`/`_maxMemoryID`/`_usedMemoryID`/`_heapMemoryID` private
fields on `JPGarbageCollection` (`jp_gc.h`) - `JPGarbageCollection` never
needed its own copy of these method IDs; if real memory-stat diagnostics
are wanted from GC code in the future, reuse `JPContext::
m_Support_Get*MemoryID` directly rather than re-resolving them.

## Fixes applied (all three root causes)
1. Moved `m_GC->init(frame)` from the commented-out call in `JPContext::
   initializeResources` (`jp_context.cpp:648`) to `launch()`
   (`jp_bridge.cpp`, right after `PyJPModule_loadResources()`) - runs once
   per interpreter (main or sub), after the Python-side state it depends
   on is actually populated. Needed adding `#include "jp_gc.h"` to
   `jp_bridge.cpp` (wasn't previously needed there).
2. Made `statm_fd`/`page_size` (`getWorkingSize()`'s `USE_PROC_INFO`/
   generic-Linux path) instance members instead of file-statics, so each
   interpreter's `JPGarbageCollection` owns its own fd instead of sharing
   one process-wide (latent cross-interpreter close-before-read bug on
   non-glibc Linux; not reproducible on this glibc dev machine, which
   compiles the `USE_MALLINFO` path instead, but fixed while touching the
   file regardless).
3. Deleted the broken `getTotalMemory`/etc. JNI lookup against the wrong
   class (`m_ContextClass` instead of `Support`) and the dead `#if 0`
   block that was its only reader - this was the actual reported crash.

No stress test was added beyond the existing `SubInterpreterNGTest`/
`SubInterpreterBuilderNGTest` suite, since root cause #3 reproduced
(as a hard, deterministic `NoSuchMethodError`, not an intermittent race)
on the very first real run with GC turned on via the existing test suite -
no additional stress scenario was needed to surface it, and the existing
subinterpreter tests now exercise the fixed path directly (both suites
create a subinterpreter, which re-enters `launch()` -> `m_GC->init()`, on
every run).
