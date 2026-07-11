# Jep parity survey — corrected findings and follow-on plans

## Status (2026-07-11): survey done, corrected by user, follow-on work scoped into sub-plans below, nothing implemented yet

Background: jep 4.3.1 (`~/jcef/jep`) is a mature Java-calls-Python bridge,
same direction as jpype's reverse bridge. jep's own test suite only runs one
test in this environment, so confidence that jpype's reverse bridge actually
covers jep's use cases was low. An Explore-agent survey compared jep's public
API against `python.lang`/`python.io`/`python.exceptions`. The raw survey is
saved in memory (`jpype_jep_parity_survey`); this doc is the corrected,
user-reviewed version driving actual follow-on work.

## Corrections to the raw survey

1. **Thread attach is NOT a gap — it's a deliberate design win, don't build it.**
   The survey flagged jep's `Interpreter.attach(shareGlobals)` (manual
   per-thread attach/detach of a running interpreter) as something jpype
   lacks. User: jpype automatically handles ALL thread attachment; manual
   attach in jep was "a segfault magnet" — "every IDE managed to crash it."
   jpype's automatic model is the correct design. No action item here beyond
   not regressing it.

2. **numpy interop is likely NOT a real gap — verify, don't build numpy ties.**
   The survey claimed `python.lang.PyBuffer`/`PyMemoryView` don't support
   shape/strides/dtype-aware bridging the way jep's `NDArray`/`jep_numpy.c`
   do. This was wrong: `python/lang/PyMemoryView.java` already exposes
   `getFormat()`, `getShape()`, `getStrides()`, `getSubOffsets()`,
   `isReadOnly()`, `getSlice(...)` — the full Python buffer-protocol surface.
   User's point: the correct architecture is to expose a rich buffer *view*
   and let numpy build its own array over it (`np.frombuffer`/`np.asarray`
   on anything implementing the buffer protocol) — NOT to hand-roll
   numpy-specific conversion code in jpype itself. Building direct numpy
   ties is a known trap: "we end up tied to numpy at the hip and users
   complain."
   - **Action**: write a real test that hands a `PyMemoryView` (or a
     `PyBuffer` from a `PyBytes`/`PyByteArray`) to actual numpy
     (`numpy.frombuffer(memoryview, dtype=...)` or `np.asarray(...)`) from a
     launched-script test and confirm it round-trips correctly, including a
     multi-dimensional case (shape/strides non-trivial). This proves or
     disproves the gap empirically instead of guessing from reading headers.
     No new production code unless the test finds a real hole (e.g. a
     missing multi-dim `PyBuffer` construction path from the Java side).
     Scoped in detail as its own plan doc: `plan/NumpyBufferBench.md`.

3. **stdout/stderr capture — real gap, scope into `plan/IO.md`.**
   jep's `JepConfig.redirectStdout/redirectStdErr` has no jpype equivalent.
   User: once the Java-side `python.io` work lands (asReader()/asWriter()
   etc., see `plan/IO.md`), also build a duck-typed Python `io`-shaped object
   backed by a Java `OutputStream`/`Writer`, installable as `sys.stdout`/
   `sys.stderr`, so Python `print()`/writes get captured on the Java side.
   Added as a new section to `plan/IO.md` (section F) — sequenced *after*
   the existing `asReader()`/`asWriter()` promotion work, since it's the
   mirror-image direction (Java stream duck-typed as Python file object,
   rather than Python file object promoted to Java stream) and reuses the
   same buffering/encoding lessons.

4. **Configurable sub-interpreter knobs — CLOSED, see `plan/SubInterpreterBuilder.md`.**
   jep's `SubInterpreterOptions` (obmalloc, allowFork/Exec/Threads,
   ownGIL, checkMultiInterpExtensions) now has a jpype equivalent:
   `org.jpype.SubInterpreterBuilder`, a `ProcessBuilder`-style mutable
   builder (`EnumSet<Option>` for the seven `PyInterpreterConfig` flags,
   plain setters for stdio) with `.start()`/`.asSupplier()` terminators.
   `native/common/jp_bridge.cpp`'s `startSubInterpreter` no longer
   hardcodes the config — the seven flags thread down from Java. Per
   `plan/MultiPhaseInit.md` (COMPLETE), `_jpype` is multi-phase-init
   eligible, so `SubInterpreterBuilder.ownGil()` reaches a genuinely
   isolated own-GIL/own-obmalloc subinterpreter, confirmed by
   `SubInterpreterBuilderNGTest.testOwnGilLaunchesAndImportsJpype`.

## Where jpype is already ahead (no action needed, confirmed by survey)

- Default-method proxy handling (`ProxyInstance.java:99`).
- Cross-language exception cause-chaining (`jp_exception.cpp`).
- GC-driven memory management (`jref`/`GlobalPool`, `plan/Globals.md`) vs
  jep's manual `WeakReference`+`ReferenceQueue` polling.
- `python.lang`'s ~55-interface type surface vs jep's thin
  `PyObject`/`PyCallable`/`PyBuiltins`.

## Not gaps (scope differences)

- jep's `ClassEnquirer`/`java_import_hook.py` (Python-side Java imports) —
  belongs to jep's classic/interactive direction, orthogonal to the reverse
  bridge.
- jep's REPL console (`console.py`) — low value for an embedding library's
  core API.
