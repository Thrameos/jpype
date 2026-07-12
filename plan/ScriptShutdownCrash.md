# Full-suite-only native crash on interpreter shutdown

## Status (2026-07-11): reproduced and bisected, root cause not diagnosed, not fixed

Found while testing `plan/JSR223.md`'s `org.jpype.script.JPypeScriptEngine`. Not a defect in
that feature - the JSR223 code is functionally complete and its test class
(`JPypeScriptEngineNGTest`) passes 6/6 when run in isolation
(`mvn -o test -Dpython.executable=python3.10 -Dtest=JPypeScriptEngineNGTest`). This is a
separate, pre-existing native lifecycle bug that the JSR223 work happened to trip because it's
the first thing in the suite to create a *second* `org.jpype.Script` (a persistent Python
`globals`/`locals` namespace) beyond `PyTestHarness`'s own shared singleton `context`.

## Symptom

Running the *whole* module's test suite (`mvn -o test -Dpython.executable=python3.10`, no
`-Dtest` filter) reliably `Aborted (core dumped)`s, always *after* every test has already
finished and `PyTestHarness`'s `@AfterSuite` has already successfully closed the interpreter
(`>>> Shutting down JPype Bridge... / Close bridge / Bridge down` all print normally first).
The crash itself:

```
[INFO] JPype shutdown sequence initiated.
Fatal Python error: PyThreadState_Get: the function must be called with the GIL held, but the
GIL is released (the current Python thread state is NULL)
Python runtime state: finalizing (tstate=0x...)

Aborted (core dumped)
```

"JPype shutdown sequence initiated" is logged by `NativeContext.shutdown()`
(`native/jpype_module/src/main/java/org/jpype/internal/NativeContext.java`, ~line 287), which is
a *second*, independent shutdown path fired by a JVM shutdown hook - separate from
`MainInterpreter.close()`, which already ran cleanly moments earlier. The method's own doc
comment calls this "a very dangerous time as portions of Java have already been deactivated."
The crash indicates this hook is touching Python's C API after CPython has already begun (or
finished) its own finalization.

This crash is contained entirely to the disposable Maven/Surefire test-fork subprocess - it
doesn't affect the shell, the build, or anything outside that one process.

## Bisection

- Baseline (this branch before the JSR223 work): full suite passes clean, 3/3 runs.
- Adding `org.jpype.script.JPypeScriptEngine` (which internally does
  `new Script(MainInterpreter.getInstance())` once, held for the engine's lifetime) plus its test
  class: full suite crashes, 2/2 runs (reliable, not flaky at that point).
- Minimal repro found by stripping the test class down to nothing but this:

  ```java
  package org.jpype.script;

  import org.jpype.MainInterpreter;
  import org.jpype.Script;
  import org.testng.annotations.Test;
  import python.lang.PyTestHarness;

  public class MinimalReproNGTest extends PyTestHarness
  {
    private static Script sharedScope;

    @Test
    public void testEngineDiscovery()
    {
      if (sharedScope == null)
        sharedScope = new Script(MainInterpreter.getInstance());
    }
  }
  ```

  This alone (full suite run, this class included) reproduces the crash.

- **Package placement changes the outcome**, which is the confusing part: the *identical*
  one-liner, run as part of the full suite, does **not** crash when placed in package
  `python.lang` instead of `org.jpype.script`:

  ```java
  package python.lang;   // <-- only difference

  import org.jpype.MainInterpreter;
  import org.jpype.Script;
  import org.testng.annotations.Test;

  public class MinimalReproNGTest extends PyTestHarness
  {
    private static Script sharedScope;

    @Test
    public void testCreateSecondScript()
    {
      sharedScope = new Script(MainInterpreter.getInstance());
    }
  }
  ```

  Both were tested by physically moving the same file between the two packages/directories,
  same suite, same command. Since Maven/Surefire/TestNG's test class run order is influenced by
  classpath/package scan order, and `org.jpype.*` sorts ahead of `python.lang.*`, the leading
  theory is this is a **timing/GC race**, not a deterministic logic bug: *when* in the run the
  second `Script`/`PyDict` gets created (and therefore when its native resources are still live
  relative to the rest of the suite's heap churn) determines whether the shutdown-hook race gets
  hit. This was not confirmed further (would need to force GC pauses or add explicit
  instrumentation to the shutdown path to observe ordering directly).

## Not reproduced as a standalone committed test

Deliberately **not** landed as a live test in `src/test/java`, since it reliably crashes a full
`mvn test` run - anyone running the whole suite without `-Dtest` filtering would hit an
unconditional core dump. The two repro classes above were used transiently during bisection and
removed afterward. Paste one back in under `native/jpype_module/src/test/java/` to reproduce.

## Next steps (not started)

- Needs gdb-level native debugging to see exactly what `NativeContext.shutdown()` /
  `onShutdown()` touches after Python's finalization has begun - same class of investigation as
  the previously-resolved subinterpreter GIL bugs (see the `jpype_subinterpreter_difficulty` and
  `jpype_gil_reacquire_bug` memory entries from that earlier session).
- Consider whether `NativeContext.shutdown()`'s JVM-shutdown-hook path should simply no-op if
  `MainInterpreter.close()` (or equivalent) has already run - it's not clear why a second
  independent shutdown path is needed at all when the interpreter was already closed in an
  orderly fashion.
- Until fixed, treat `mvn test` (full suite, no `-Dtest`) on this branch as unreliable; verify
  individual test classes with `-Dtest=<ClassName>` instead.
