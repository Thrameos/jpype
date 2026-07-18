# Debug: reverse-bridge `exec()`-raises SIGSEGV, Maven-only

## Status (2026-07-18): ROOT-CAUSED AND FIXED. Two real bugs, both fixed.
`mvn -o verify -Dpython.executable=python3.10`: 662/662 tests, 0 failures, 0
errors, 14 skipped (same baseline as before), confirmed stable across
repeated runs. Ready to move `plan/Coverage.md`'s `python.exceptions` pass
out of "blocked."

## Root cause: two independent bugs stacked on top of each other

### Bug 1 (Java side, the actual trigger): `PackageManager` can't see an
application module's own packages under `--module-path` launch

`native/jpype_module/src/main/java/org/jpype/pkg/PackageManager.java`'s
`getContentMap`/`isPackage` enumerate a Java package's contents via two
paths: `getJarContents` (classpath/unnamed-module, via
`ClassLoader.getSystemClassLoader().getResources(name)`) and
`getModuleContents` (JDK platform modules, via the `jrt:/` filesystem).
Neither path covers a **named application module's own packages** when the
JVM is launched with `--module-path=.../org.jpype-*.jar
--add-modules=org.jpype` (exactly how Maven Surefire launches the reverse-
bridge test JVM, and how `org.jpype.Launcher` is meant to be embedded in
general). `ClassLoader.getResources()` does not reliably walk a named
module's own resources - only classpath entries and anything separately
patched in via `--patch-module` (which is why a `--patch-module`-injected
test class like a scratch `MiniExcNGTest` in package `python.exceptions`
*was* visible while the real `python.exceptions.Py*Error` classes, compiled
into the module jar itself, were not).

Consequence: `jpype/_jbridge.py`'s `initialize()` builds `_jpype._exc` by
walking `dir(JPackage("python.exceptions"))` **once**, at interpreter
startup (before any test method has run). Under `mvn test`'s module-path
launch this walk found **zero** real exception classes (confirmed via
direct diagnostic: `len(_jpype._exc) == 0`), deterministically and on every
run - not flaky, not order-dependent, not related to which test happened to
run first (there's no "run first" for a startup-time walk anyway). The
662/662-passing baseline recorded earlier in `plan/Coverage.md` this same
session did not actually exercise this path meaningfully - once the fail-
fast crash (bug 2, below) was fixed and the full suite could run to
completion without aborting, **15 pre-existing tests** across
`python.exceptions`, `python.lang` (`PyExcNGTest`, `PyRangeNGTest`), `python.
collections` (`PyChainMapNGTest`), `DispatchFallbackNGTest`, and
`JPypeScriptEngineNGTest` turned out to already be silently getting the
wrong exception type (`java.lang.RuntimeException` wrapping a repr string,
instead of the correct `python.exceptions.Py*Error` subtype) - this was
always broken under `mvn test`, just never surfaced because bug 2 crashed
the JVM before any of these assertions' real failure could be reported by
Surefire.

**Fix**: added `isNamedModulePackage`/`getNamedModuleContents` to
`PackageManager`, using `ModuleLayer.boot().configuration().modules()` to
find which resolved module declares the target package (via
`ModuleDescriptor.packages()`), then `ModuleReader.list()`/`.find()` to
enumerate and resolve its class-file resources - the module-path
counterpart to the existing `getJarContents`. Wired into both `isPackage`
and `getContentMap` alongside the existing jar/jrt paths.

### Bug 2 (native, the crash mechanism): wrong `JPPyObject` policy turns any
conversion failure into an unrecoverable fail-fast

`JPypeException::convertPythonToJava` (`native/common/jp_exception.cpp`)
calls `_jpype._pyexc_convert(exc)` to turn a raised Python exception into
its matching Java throwable, then does:
```cpp
JPPyObject proxy_res = JPPyObject::claim(PyObject_CallFunctionObjArgs(pyexc_convert_fn, exc.get(), nullptr));
if (proxy_res.isNull()) { ... fail(frame, msg); return; }
```
The `if (proxy_res.isNull())` fallback makes it obvious the author expected
`_pyexc_convert` might legitimately fail and wanted to degrade gracefully to
a generic Java `RuntimeException` via `fail()`. But `JPPyObject::claim()`
(`native/python/jp_pythontypes.cpp`) asserts its argument is non-null and
**throws** if it isn't (`ASSERT_NOT_NULL` → `JP_RAISE_PYTHON()`) - it never
returns a null-wrapping `JPPyObject` on failure the way `JPPyObject::accept()`
does. So the moment `_pyexc_convert` actually raised (which, per bug 1,
was *every single time* under `mvn test`), `claim()` threw a **second**
`JPypeException` from inside `toJava`'s try block, landing in its
`catch (JPypeException&)` block - which is a deliberate, by-design fail-fast
for "exception handling itself failed catastrophically" (`int *i = nullptr;
*i = 0;`, `GCOVR_EXCL_START`-marked). That fail-fast is correct for its
intended trigger (something going wrong badly enough that Python/Java error
state can no longer be trusted); it was never meant to fire just because a
normal, anticipated "conversion failed, fall back to a generic exception"
path took a wrong turn through the wrong `JPPyObject` policy.

Confirmed via disassembly earlier this session (see prior status below,
kept for the record) that the crashing instruction really was this
fail-fast, not something else; confirmed via targeted `fprintf`
instrumentation (added temporarily, removed after use - see "how this was
found" below) that the second exception was thrown from exactly this
`claim()` call, and that it originated from `_pyexc_convert` raising
`RecursionError: maximum recursion depth exceeded while calling a Python
object` - because `_jbridge.py`'s `_pyexc_convert` falls back to calling
*itself* on an `AssertionError` when no match is found in `_jpype._exc`,
which recurses forever when `_jpype._exc` is itself empty (bug 1).

**Fix**: changed `claim()` to `accept()` for that one call. `accept()`
clears the Python error state and returns a null-wrapping `JPPyObject`
instead of throwing, so `proxy_res.isNull()` now actually triggers as
originally intended, and the existing `fail()` fallback (a plain Java
`RuntimeException`) surfaces instead of a JVM crash. This fix is valuable
independent of bug 1 - it makes the exception-conversion path robust
against *any* future reason `_pyexc_convert` might fail, not just this one.

## How this was found (for the debugging-method record)

Given a reliable Maven-only reproducer and a battery of standalone repros
that all failed to reproduce it (see "prior status" below), the next
session judged that further gdb-wrapping of Surefire's fork (previously
shown to corrupt Surefire's own JVM probing and silently change its launch
mode - see "Debugging tooling notes" below) was less promising than direct
instrumentation of the suspect code path, compiled into the real binary and
exercised via the real, reliable `mvn test` reproducer (which needs no
launch-command changes at all, sidestepping the Surefire-probing pitfall
entirely). Added temporary `fprintf(stderr, ...)` checkpoints (not gated by
the `JP_TRACE`/`JP_TRACING_ENABLE` machinery, which is compiled out
entirely in a normal build and would have required a separate, behavior-
changing rebuild) at each step of `convertPythonToJava` and in `toJava`'s
catch blocks, rebuilt via `make -f project/dev.mk`, and ran the existing
`mvn -o test -Dtest=<TestClass>` reproducer directly - no gdb, no `-Djvm=`
wrapping. This pinpointed the exact call (`claim()` on the `_pyexc_convert`
result) and, via one more targeted `PyErr_Fetch`-based checkpoint, the
exact underlying Python exception (`RecursionError`), which led directly
to the `_jpype._exc` len-zero finding via a small diagnostic `context.exec()`
script run from the same Maven test. All instrumentation was removed after
use; the two fixes above are the only production-code changes.

## Prior status (kept for the record, superseded by the above)

Confirmed via disassembly (`objdump -d -C` on the installed `_jpype.so`,
matching the `hs_err_pid*.log` crash offset exactly) that the faulting
instruction was the deliberate `int *i = nullptr; *i = 0;` inside `toJava`'s
own `catch (JPypeException& ex)` block. Reproduced deterministically via
`mvn -o test -Dtest=<TestClass>` in `native/jpype_module`, 3/3 times, with
three different triggers - explained now: any raised Python exception hits
this, since `_jpype._exc` was unconditionally empty under this launch mode.

Ruled out (still true, just no longer the active mystery): JaCoCo, plain
JPMS module/patch-module boundaries in isolation, small-scale repetition.
The real missing ingredient (per the "what wasn't reproduced outside
Maven" section of the original writeup) was candidate 4: the real
`--module-path`+`--add-modules` launch of the actual `org.jpype` module
jar itself (not just a `--patch-module`-injected single class) - because
the bug isn't really about Surefire/TestNG at all, it's about how
`PackageManager` resolves an application module's package contents.

**Debugging tooling notes worth keeping**: `-Djvm=<path>` to Surefire is
unreliable for gdb-wrapping (corrupts Surefire's own JVM probing, can
silently change launch mode); WSL intercepts core dumps (no local core file
ever produced); disassembly-only diagnosis (`nm -C` + `objdump -d --no-show-
raw-insn -C`, cross-referenced against `hs_err_pid*.log`'s offset) is a
good first step before reaching for gdb; a minimal non-Maven repro needs
`target/org.jpype-1.7.2.dev0.jar` specifically, not the repo-root
`org.jpype.jar` (confirmed stale, see
[[jpype_feedback_use_devmk_not_adhoc_ninja]]).

## Verification

`mvn -o verify -Dpython.executable=python3.10`: 662/662, 0 failures, 0
errors, 14 skipped (same as the pre-existing baseline), run twice to
confirm determinism (not flaky). JaCoCo report regenerates cleanly.
python3.12 not re-verified here - it has its own separate, pre-existing,
unrelated environment issue noted in `plan/Coverage.md`.
