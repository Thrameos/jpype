# Debug: reverse-bridge `exec()`-raises SIGSEGV, Maven-only

## Status (2026-07-18): CONFIRMED REAL, NOT ROOT-CAUSED, REPRO NARROWED TO MAVEN/SUREFIRE/TESTNG

Discovered while starting the `python.exceptions` pass of [[plan/Coverage.md]].
Blocks writing more `python.exceptions` tests via `exec()` until understood -
see that plan's "Concrete known low spots" section for the coverage-side
writeup. This file is purely the debugging log/next-steps for the crash
itself.

## The bug

Any Python exception raised during a reverse-bridge call whose Java-side
method returns `void` (concretely: `Script.exec(String)` /
`PyBuiltIn.exec(...)` → `Backend.exec(...)`, a JDK dynamic-proxy call
dispatched through `org.jpype.proxy.ProxyInstance.hostInvoke`) crashes the
JVM with `SIGSEGV` inside `JPypeException::toJava(JPJavaFrame&) [clone
.cold]` (`native/common/jp_exception.cpp`).

Confirmed via disassembly (`objdump -d -C` on the installed `_jpype.so`,
matching the `hs_err_pid*.log` crash offset exactly) that the faulting
instruction is the deliberate `int *i = nullptr; *i = 0;` inside `toJava`'s
own `catch (JPypeException& ex)` block (compiled to a literal `movl
$0x0,0x0`). That block only exists as a "we're already too corrupted to
unwind safely" fail-fast for when exception-handling *itself* throws a
second `JPypeException` - so the real bug is whatever throws that second
exception from inside `convertPythonToJava`/`toJava`'s try block, not this
catch block itself (which is working as designed, just for an
unanticipated trigger).

Reproduced deterministically via `mvn -o test -Dtest=<TestClass>` in
`native/jpype_module`, 3/3 times, with three different triggers:
`context.exec("raise AssertionError('test')")`,
`context.exec("raise ValueError('test')")`, and the organically-raised
`context.exec("int('not a number')")` - i.e. not tied to one exception
type or to explicit `raise` vs. a naturally-raised error. A separate,
apparently-unrelated *flaky* crash at the same `toJava` frame was also
observed once from `JPypeScriptEngineNGTest.testBindingsRoundTrip` (a
non-raising `eval` call, via `ScriptEngine.eval`/`.put`), but it did not
reproduce on a clean rebuild - noted here so it isn't lost, not chased
further yet. It may be the same root cause manifesting on a later,
unrelated call (see "corrupted state, not corrupted call" theory below).

## What's ruled out (each confirmed by a real, run repro, not guesswork)

- **Not caused by JaCoCo.** Reproduced the exact same call sequence
  (`org.jpype.Script.exec_(...)` raising, driven from Python via
  `jpype.startJVM()` against the current `target/org.jpype-1.7.2.dev0.jar`)
  with JaCoCo's `-javaagent` attached from the Python-side JVM launch -
  20 iterations, no crash. See `/tmp/repro4.py`-style script (not checked
  in - scratch only) for the pattern; the key ingredients were
  `-javaagent:.../org.jacoco.agent-0.8.14-runtime.jar=destfile=...` plus
  `--enable-native-access=ALL-UNNAMED` on `jpype.startJVM()`.
- **Not caused by JPMS module boundaries alone.** Reproduced with a
  hand-built `java` invocation using `--module-path`, `--add-modules=
  org.jpype`, and `--patch-module=org.jpype=<dir>` (patching a test class
  directly into the `org.jpype` module, matching how Surefire patches
  `target/test-classes` in) - still no crash, plain Java `main()`, no
  TestNG involved.
- **Not caused by repetition/allocator wraparound at a small scale.**
  50 sequential raising `exec()` calls through a single reused
  `org.jpype.Script` instance (matching how `PyTestHarness`'s static
  `context` field is shared across all NGTest methods in a suite run) -
  no crash. Doesn't rule out a *much* larger scale (662-test suite worth
  of prior allocation churn) still mattering; see GlobalPool note below.
- **Root cause is *not* "the `_jpype._exc` dict is missing an entry" or
  similar Python-side config gap** - `PyExcNGTest`'s existing
  `eval("1/0")`/`eval("int('not a number')")` tests already exercise the
  identical `convertPythonToJava` → `_pyexc_convert` → `_jpype._exc[cls](...)`
  path successfully for `PyZeroDivisionError`/`PyValueError`, so the
  lookup/construction machinery itself works; only the caller being a
  `void`-returning `exec()` call, run under Maven specifically, differs.

## What was NOT successfully reproduced outside Maven

Every hand-built repro outside `mvn test` succeeded (no crash), including
one that matched the real command line closely: `--enable-native-access`,
`--module-path`, `--add-modules=org.jpype`, `--patch-module=org.jpype=...`,
`--add-opens=org.jpype/org.jpype=ALL-UNNAMED`, `--add-reads=org.jpype=
ALL-UNNAMED`, JaCoCo's `-javaagent`, all together, calling
`org.jpype.MainInterpreter`/`Script` directly from a plain `main()` (no
TestNG). This means the remaining, unreproduced ingredient is specific to
**Surefire's actual forked-JVM launch and/or TestNG's reflective test
invocation** - not any single flag combination tried so far.

Candidates not yet tried:
1. **TestNG reflection dispatch itself** - `Method.invoke()` through
   TestNG's `MethodInvocationHelper`/`TestInvoker` machinery, as opposed to
   a directly-called `main()`. A quick reflection-wrapped call
   (`Method.invoke` on a lambda-backed method, no TestNG) was tried and did
   *not* crash, but that's not the same as TestNG's actual invocation path
   (extra listener/interceptor machinery, `Object[]` boxing of args, etc.).
2. **JIT/tiering state at the time of the call.** Maven's fork loads far
   more classes and runs far more code before reaching any given test
   method than a minimal repro does; if this is a JIT-compiled-code bug
   (e.g. a miscompilation of `hostInvoke` or `toJava` at C2, or of the JNI
   trampoline), a minimal repro may simply never warm up enough to trigger
   it. Worth trying `-Xcomp` or `-XX:-TieredCompilation` on a minimal repro
   to force early C2 compilation and see if that alone triggers it without
   Maven/TestNG at all.
3. **Accumulated native-side allocator state from the *whole* prior test
   suite**, not just repeated raises - the session's own `GlobalPool`
   ("stamp mismatch... prefix wraparound reuse") warnings appear in normal
   (non-crashing) runs too and are believed benign
   ([[jpype_globalpool_allocator_status]]), but that belief predates this
   bug; worth a repro that runs a large, varied chunk of the *real* NGTest
   suite first (not just 50 identical raises) before attempting the
   crashing call, to see if heavier/more varied prior allocation churn is
   the missing ingredient rather than TestNG/Surefire specifically.
4. **The exact `--patch-module` *target directory*.** Real Surefire patches
   in `target/test-classes` (many classes: all NGTests, not just one) plus
   uses `--add-opens` scoped per-package based on which test packages were
   *scanned*, not just the one under test. The repro only ever patched in
   a single tiny class - a `--patch-module` pointing at the *real*
   `target/test-classes` directory (untried) would be closer to the real
   condition and cheap to try next.

## Debugging tooling notes (so the next session doesn't rediscover these)

- **`-Djvm=<path>` to Surefire is unreliable for gdb-wrapping.** Surefire
  uses the configured `jvm` path for its own internal probing (module
  descriptor / version detection) in addition to the actual forked test
  run. A gdb wrapper's extra stdout/stderr silently corrupts that probing,
  which makes Surefire fall back to a different launch mode entirely
  (manifest-only-jar classpath instead of `--module-path` +
  `--patch-module`) - so the "crash" you're chasing may just stop
  reproducing because you changed the launch mode, not because you fixed
  anything. If retrying this path: filter the wrapper script so it only
  invokes gdb when the args contain `ForkedBooter`/the real test-run
  marker, and pass everything else straight through untouched (see
  `/tmp/fakejdk/bin/java`-style script from this session, not checked in).
  Even with that filter, one attempt still fell back to manifest-jar mode
  for unclear reasons and needs re-verifying with
  `-Dsurefire.useManifestOnlyJar=false -Dsurefire.useSystemClassLoader=false`
  added.
- **WSL intercepts core dumps** (`/proc/sys/kernel/core_pattern` is
  `|/wsl-capture-crash ...`, not writable without root) - no local core
  file is produced on crash, so live gdb attach (`-p <pid>`) or wrapping
  the launch command is the only option, not post-mortem core analysis.
- **Disassembly-only diagnosis is possible and was useful here**: `nm -C
  _jpype.so | grep toJava` to get the `.cold` clone's symbol, then
  `objdump -d --no-show-raw-insn -C _jpype.so` sliced to that symbol,
  cross-referenced against the `hs_err_pid*.log`'s `+0x4a` offset -
  confirmed exactly which C++ line faults without needing a live debugger
  at all. Good first step before reaching for gdb.
- A minimal non-Maven repro needs, at minimum: `target/org.jpype-1.7.2.dev0.jar`
  (**not** the repo-root `org.jpype.jar` - confirmed stale this session,
  missing `Launcher.class`/`Runner.class` entirely, see
  [[jpype_feedback_use_devmk_not_adhoc_ninja]]) plus
  `net.java.dev.jna:jna:5.17.0` on the classpath/module-path.

## Next step

Try candidate 4 first (real `--patch-module=org.jpype=target/test-classes`
pointing at the actual, full compiled test tree, not a synthetic one-class
directory) since it's the cheapest to try and closest to the untested gap;
fall back to candidate 1 (real TestNG dispatch, not `main()`) if that
doesn't reproduce it, then candidate 2 (`-Xcomp`) if neither does.
