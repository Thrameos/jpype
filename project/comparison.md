# jpype vs. jpy vs. jep vs. pyjnius: feature comparison

Scope: if jpype's test suite were ported to jpy/jep/pyjnius, how much of it
would have something to run against (see the testbench-porting discussion
this doc grew out of). Grounded in each library's own source: jpy's C
source (`~/devel/jpy/src/main/c`, 21-file/151-test suite), jep's C source
(`~/devel/jep/src/main/c`, 34-file/247-test suite), and pyjnius's Cython
source (`~/devel/pyjnius/jnius/*.pxi`, `reflect.py`, 37-file/160-test
suite). See `project/benchmark/RESULTS.md` for the jpype/jpy/jep/pyjnius
speed comparison.

jpy is architecturally a thin C extension with essentially no Python-side
wrapper layer — most of jpy's rows below follow from that: jpype spends
effort on Python-idiomatic ergonomics and correctness breadth that jpy's
design doesn't take on. jep embeds Python inside the JVM (the reverse of
jpype/jpy's architecture) and, unlike jpy, gives Java collections real
Python protocol support (`pyjlist.c`, `pyjmap.c`, `pyjcollection.c`,
`pyjiterable.c`) plus functional-interface duck typing and general
multi-method proxy support (`pyjtype.c`'s `functionalInterface`
detection, `jep/Proxy.java` + `java_access/Proxy.c`) — closer to jpype in
scope than jpy, though still narrower. pyjnius is closer still in *scope*
(real collection-protocol, `Comparable`, functional-interface, and
general-proxy support, all verified working — see below), but its
multi-dimensional/buffer array support is the narrowest of the three, and
its general-proxy implementation has a reproducible crash under a
specific input that jpy/jep don't share.

## Feature matrix: all four libraries

A single-glance summary of the pairwise sections below, which carry the
full evidence/citations for every row. "Yes"/"No" means fully working and
verified (empirically, where practical) or confirmed absent from source;
anything more nuanced gets a footnote.

### Language / object-model integration

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Collection protocols (`List`/`Map`/`Iterator` as native Python `list`/`dict`/iterator) | Yes | No | Yes (`pyjlist.c`/`pyjmap.c`/etc.) | Yes (`protocol_map`) |
| `Comparable`/`Iterable` duck-typing (`<`, `for x in`, `hash()`) | Yes | No | Yes | Yes (`protocol_map`) |
| `AutoCloseable` → Python context manager (`with obj:`) | Yes (`jpype/_jio.py`) | No | Yes (`pyjautocloseable.c`) | Yes (`protocol_map`) |
| Functional-interface duck typing (bare `lambda`/callable as a Java SAM arg, no proxy class needed) | Yes | No (explicit proxy object only) | Yes (`pyjtype.c`'s `functionalInterface`) | Yes (`jnius_conversion.pxi`) |
| General proxy (Python object implementing an arbitrary Java interface) | Yes, multithread-safe | Did not produce a usable object in this checkout\* | Yes | Yes for the common case, with a reproduced crash on one input shape\*\* |

\* jpy's `PyObject.createProxy()` exists but didn't produce a usable
object from Python in this checkout (see jpy section below).

\*\* pyjnius's proxy mechanism works for the common case, but a
Python-implemented interface method receiving a **null** `Object`
argument segfaults the JVM (`jni_GetObjectClass` on a null jobject), and
the non-null case returns `None` instead of the real object — see the
dedicated section below.

### Conversion / arrays

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Class hints / custom conversions (`@JConversion`) | Yes (54 tests) | No | No | No |
| Buffer-protocol (numpy) array push, flat 1D | Yes, bulk path, value-correct across ~14 source formats (see dtype matrix below); the dtype coercion runs in Java (`Support.fillFlatFromBuffer`) via a single JNI call | Yes, via a separate argument-matching fast path, not `jpy.array()` itself — no dtype check: same-width dtype mismatches are bit-reinterpreted rather than rejected, see dtype matrix below | Yes, genuine bulk path, closed 8-dtype allowlist (no `float16`) | No, rejected unconditionally (`"Expecting a python list/tuple"`) |
| Buffer-protocol (numpy) array push, multi-dimensional (`int[][]`+) | Yes, bulk path | No (`TypeError`, falls back to a per-element path) | No (`TypeError: Error matching ndarray.dtype...`) | No (same rejection as 1D) |
| Per-class `findJavaConversion` caching w/ invalidation | Yes | N/A — matching is already cheap per call | N/A — short-circuits on arity before any per-arg work | No equivalent per-call cost to cache |

### numpy scalar argument dispatch (`Math.max(int,int)`, i.e. an overloaded method)

| numpy scalar type | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| `numpy.int32` | OK | Fails ("ambiguous Java method call") | OK | Fails ("no matching method") |
| `numpy.int64` | OK | Fails (same) | OK | Fails (same) |
| `numpy.float32` | OK | Fails (same) | Fails ("cannot be interpreted as an integer") | Fails (same) |
| `numpy.float64` | OK | OK (genuine Python `float` subclass) | OK | OK (same reason) |

Verified empirically for all four. jpype resolves every numpy scalar type
against an overloaded method correctly. jep resolves `int32`/`int64`
correctly (`jep_numpy.c` has an explicit numpy-scalar path) but not
`float32`. jpy and pyjnius share the same failure shape (only a genuine
Python `int`/`float` passes their fast dispatch, no numpy-aware fallback
for `int`/`long` params), with differently-worded errors.

### Buffer dtype conversion: the full matrix

Tested every combination empirically, with source read where the
behavior needed explaining.

**jpype's fast-path matrix covers 14 source buffer format codes** —
`?`/`c`/`b` (bool/int8), `B` (uint8), `h`/`H` (int16/uint16), `i`/`l`/`I`/`L`
(int32/uint32), `q`/`Q` (int64/uint64), `f`/`d` (float32/float64), `n`/`N`
(native ssize_t/size_t), and `e` (IEEE 754 half-precision `float16`, via a
dedicated bit-level decoder, `Half<Convert<float>::toX>`, reusing the same
`Convert<float>::toX` machinery every other type shares). Tested all 12
realistic numpy dtypes (`bool`/`int8`/`uint8`/`int16`/`uint16`/`int32`/
`uint32`/`int64`/`uint64`/`float16`/`float32`/`float64`) against all 8 Java
primitive array types (`JArray(JType)(arr)`): 94 of 96 combinations
succeed with genuinely converted values (`float32([1.0,2.0,3.0]) ->
int[]` gives `[1, 2, 3]`, not bit garbage). The 2 failures are
`boolean`/`float*` → `char`, both rejections consistent with Java having
no implicit boolean-to-char or float-to-char narrowing. `float8_e4m3`
(via `ml_dtypes`) has no recognized format code — it still succeeds for
`float`/`double` targets via the general per-element fallback
(`ml_dtypes` scalars support `__float__`), not the buffer fast path.

**jpy's buffer path does not inspect the source dtype.** Its argument
buffer handler (`jpy_jtype.c` ~line 2026) does `memcpy(arrayItems,
pyBuffer->buf, itemCount*itemSize)` after checking only that the byte
length matches — it never reads `pyBuffer->format`. Concretely: passing
`numpy.float32([1.0, 2.0, 3.0])` where `DeepBench.sumIntArray(int[])`
expects `int[]` returns `3217031168` — no error, and not `6` — the raw
IEEE-754 bit patterns of `1.0f`/`2.0f`/`3.0f` reinterpreted as `int32` and
summed (`1065353216 + 1073741824 + 1077936128 = 3217031168`). This
happens specifically when the source dtype and the target's byte width
match but the dtype itself doesn't (`float32`↔`int32`, `float64`↔`int64`);
different-width mismatches are caught by the length check
(`uint8`/`float16` → `int[]` both raise "no matching Java method
overloads found"). Net effect: a wrong-dtype argument at matching byte
width produces a plausible-looking wrong number with no exception raised
— worth flagging distinctly from a rejection, since a rejection is at
least visible at the call site whereas this case is not.

**jep's numpy fast path checks dtype identity against a closed
allowlist** — `float32` → `int[]` and `float16` → `int[]` both cleanly
fail (`"Error matching ndarray.dtype to Java primitive type"`), matching
`convert_pyndarray_jprimitivearray`'s exact-match check against 8
`NPY_*` constants (no `NPY_FLOAT16`, so `float16` is a hard, permanent
gap for jep rather than an untested case). No same-width reinterpretation
risk, since dtype identity is checked before data is touched.

**pyjnius** rejects all numpy input unconditionally regardless of dtype
(established above).

### Non-contiguous buffer sources

A numpy column slice or a transposed array is a valid buffer-protocol
object that can't offer a C-contiguous view.
`project/benchmark/{jpype,jpy,jep,pyjnius}/array_noncontig.py` measures
what happens when one is pushed as a Java array argument, at flat (1D)
and multi-dimensional depths, across all four primitive types.

**jpy fails on the 1D case, for every type and size** (confirmed by
running it): `DeepBench.sumIntArray(non_contiguous_column)` raises
`RuntimeError: no matching Java method overloads found`. Root cause
(`jpy_jtype.c`, `JType_ConvertPyArgToJObjectArg`): jpy's flat-array
buffer-argument path requests the source buffer with `flags =
PyBUF_SIMPLE` — no `PyBUF_ND`/`PyBUF_STRIDES` — so numpy's own
`bf_getbuffer` refuses the request against a non-contiguous array. The
error text that surfaces reads as an overload-resolution problem with
the Java method signature, not a contiguity problem with the input array
— jpy's overload matcher treats the failed buffer request the same as
"argument doesn't match any candidate," so nothing in the message points
at transposing or slicing the array as the fix. jpy's multi-dimensional
buffer argument matching is unaffected — an `int[][]`-or-deeper target
never enters this buffer branch (confirmed by source and by running it;
every transposed multi-dimensional case pushed correctly, at the same
cost as the contiguous case).

jpype handles a non-contiguous source correctly at every depth; the flat
(1D) case reaches the same single-JNI-call bulk path as a contiguous
source — `int[100000]` non-contiguous column slice: 80,715ns, close to
jep's non-contiguous number below (see `project/benchmark/RESULTS.md`
Section 4/8).

**jep handles the flat (1D) case correctly, through its numpy fast
path** (confirmed by running `project/benchmark/jep/array_noncontig.py`
against this branch's harness): a non-contiguous column slice pushes
successfully at every size and type, close to jep's own contiguous
`buffer->array` numbers (`int[100000]`: 75,448ns non-contiguous vs.
~51,150ns contiguous, `array_flat.py`) — jep's numpy fast path doesn't
require contiguity the way jpy's flat buffer-argument path does. jep has
no automatic multi-dimensional numpy push at all (established above), so
the ND/transposed case is instead measured via the same manual per-row
assembly used for jep's ordinary multi-dimensional `buffer->array` push;
this completes successfully at every depth (e.g. `double[][][][][]`
(10^5): 21.5M ns), consistent with jep's contiguous manual-assembly
numbers at the same depth, since each per-row leaf call is itself a
small, independently-contiguous 1D slice.

pyjnius has no `buffer->array` push at any size or depth, contiguous or
not; its `array_noncontig.py` is a stub rather than a benchmark.

### Fast bulk-transfer path coverage

The sections above establish each path individually; laid out as one
matrix, this counts how many of these eight push/pull paths are genuine
bulk operations versus a per-element fallback that still succeeds.
"Genuine" here means what the sections above verified empirically — a
call that succeeds via a per-element/per-row walk is marked as such
rather than counted as a bulk-path pass.

| Path | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| `list->array` push, flat (1D) | Y | Y | ⚠ | Y |
| `list->array` push, multi-dim (rectangular) | Y | Y | ⚠ | Y |
| `buffer->array` push, flat (1D), contiguous | Y | Y\* | Y | N |
| `buffer->array` push, flat (1D), non-contiguous | Y | N | Y | N |
| `buffer->array` push, multi-dim, contiguous | Y | N | N | N |
| `buffer->array` push, multi-dim, non-contiguous (transposed) | Y | N | ⚠ | N |
| `array->buffer` pull, flat (1D) | Y | Y | N | N |
| `array->buffer` pull, multi-dim | Y | Y | N | N |
| **Genuine bulk paths (of 8)** | **8** | **5** | **2** | **2** |

Legend: **Y** = a real, verified bulk path. **N** = no bulk path —
rejected outright, or falls back to a fully general per-element walk
with no dedicated fast-path code (jep's/pyjnius's `array->buffer` rows
return real data but by paying `array->list`'s per-element cost plus a
redundant wrapping step, landing worse than the library's own list-pull
number). **⚠** = a path exists and executes but isn't bulk the way the
others are: jep's `list->array` rows are incomplete at multi-dim depth
(its own `float`/`double` sweeps didn't finish — see the OOM finding
below, so the Y for jep's flat `list->array` row is provisional on
`int`/`long` only), and jep's transposed-ND `buffer->array` push is a
manual, user-written per-row Python walk rather than an automatic
argument-conversion path. \* jpy's flat contiguous `buffer->array` push
is real and bulk, but see the dtype-check gap above — read with that
caveat.

Framed this way: jpype has all eight paths, jpy has five of eight (one
of those five silently wrong on dtype mismatch), and jep and pyjnius
each have two of eight with no caveats. The "Other" table below adds the
non-buffer feature gaps on top; this table is scoped specifically to
bulk numeric array transfer.

### Other

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Pickling / `copyreg` support | Yes (9 tests) | No | No | No |
| Caller-sensitive JDK method handling | Yes (20 tests) | No | No | No |
| Javadoc-derived docstrings / Jedi / typing-stub generation | Yes (~37 tests) | No | No (bare `dir()` only) | No (bare `dir()` only, `__doc__ is None`) |
| Rich JVM-finder / startup-options API | Yes | Different, narrower (`jpy.create_jvm`) | N/A — embeds Python inside the JVM, reverse architecture | Auto-starts on first `autoclass()` call from a preset classpath (`jnius_config`) — simplest of the four, least configurable |
| Late class loading (add a jar/path to the classpath after the JVM is already running) | Yes — `addClassPath()` live-injects into `org.jpype.JPypeContext`'s custom classloader via JNI | No — `jvm_classpath` is only settable as an argument to `init_jvm()`, no post-startup API found | No — architectural: jep doesn't start its own JVM, it's embedded inside one already launched via `java -classpath ...`; `JepConfig.setClassLoader()` supplies a pre-built `ClassLoader` to a new sub-interpreter at construction time, not a live "add this jar now" call | No: `add_classpath()` calls `check_vm_running()` and raises `ValueError` if the JVM has already started — functionally identical to `set_classpath()`, both pre-startup only |

### Test suite size

| | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| test files | 90 | 21 | 34 | 37 |
| tests | 1,884 | 151 | 247 | 160 |

Collection-protocol/Comparable/functional-interface/general-proxy support
means jep and pyjnius both have something to port a meaningfully larger
fraction of jpype's suite against than jpy does. `test_classhints.py`/
`test_hints.py`/`test_customizer.py`, `test_pickle.py`/`test_serial.py`,
and the introspection-ergonomics files are gaps for all three of
jpy/jep/pyjnius. Multi-dimensional/buffer array tests are a gap for jpy
(partial) and pyjnius (total) but not jep (partial, same as jpy).

## jpype vs. jpy

### Features jpype has that jpy does not

| Feature | jpype | jpy | Evidence |
|---|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/etc. | `list`/`dict`-like `__getitem__`, iteration, `len()` | None — Java collections are opaque wrapper objects | `jpy_jtype.c:2690-91` sets `tp_as_sequence`/`tp_as_mapping` to `NULL` unconditionally |
| `Comparable`/`Iterable`/`Hashable` duck-typing | Java objects implementing these participate in Python's `<`, `for x in`, `hash()`, etc. | None found | no `Comparable`/`Iterable` handling in `jpy_jtype.c` |
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (`test_classhints.py`, `test_hints.py`, `test_customizer.py`, 54 tests) | No equivalent subsystem | grep of `jpy/src/main/c` for hints/customizer machinery: nothing |
| Functional-interface duck typing (pass a Python `lambda`/callable directly as a Java SAM interface arg) | Supported (`test_functional.py`, `test_lambdas.py`) | Only via explicit proxy objects, not implicit lambda conversion | no functional-interface matcher in `jpy_jtype.c`/`jpy_jmethod.c` |
| Proxy (`@JImplements`, Python object implementing a Java interface) | Supported, multithread-safe (`test_proxy.py`, `test_proxy_multithreaded.py`) | Present (`PyObject.createProxy()`) but did not produce a usable object in this checkout — see `project/benchmark/RESULTS.md` footnote | reproduced this session |
| Multi-dimensional numpy array push (`int[][]` etc. from an ndarray) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | reproduced via a `bench_deep_jep.py`-style harness (jpy hits its own equivalent failure) |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jpy source or test suite |
| Introspection ergonomics: docstrings from Javadoc, `repr()`, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | None | no analogous test files or source in jpy |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference in jpy source |
| JVM lifecycle ergonomics (`jvmfinder`, startup options, `test_startup.py`/`test_opts.py`, 40 tests) | Rich, discoverable JVM-finding + startup-option API | Different, narrower launch API (`jpy.create_jvm`) — a different shape, not a subset | jpy has no equivalent finder/options surface |
| Per-class conversion caching with generation-based invalidation | Yes | N/A — jpy's matching is already unconditionally cheap per call (see below), so caching wasn't a gap to close | `jpy_jtype.c`/`jpy_jmethod.c` |

### Where jpy does less work per call

Not gaps — places jpy is faster because it skips work jpype does
deliberately.

| Behavior | jpype | jpy |
|---|---|---|
| Array-argument matching | Validates every element up front (needed for correct Java-style overload disambiguation) | Does not inspect elements before committing to a conversion — confirmed by reading `jpy_jtype.c`/`jpy_jmethod.c` |
| Scalar/dispatch/proxy matching | More abstraction layers, broader general-purpose machinery (implicit numeric widening, the full `JPConversion` chain) | Leaner architecturally; not found to skip correctness in doing so — see `RESULTS.md` |

### Not compared here (excluded, no jpy equivalent to port to)

Fault-injection (`test_fault.py`, 88 tests) and coverage-instrumentation
tests (`test_coverage.py`, `test_javacoverage.py`, 50 tests) exercise
jpype's own internal error paths, not a portable behavior — excluded per
the original ask, not because jpy lacks the feature.

### jpy suite-size context

jpype: 1,884 tests across 90 files (~21k lines), `test/jpypetest/`. jpy:
151 tests across 21 Python test files, `~/devel/jpy/src/test/python/`.

Roughly 250+ of jpype's tests exercise features with no jpy counterpart
(the table above) — those can only be noted as gaps, not ported. The
remainder (conversion, arrays, strings, exceptions, fields/properties,
overloads/varargs, reflect, jclass/jpackage/imports, numeric/boxing,
buffers, inherit, hash, synchronized) is the realistic portable subset if
this comparison is ever turned into an actual ported test run.

### jpy: differences in restart, boxing, exceptions, thread attachment, and shutdown

Read against jpy's own source, five behaviors where jpy's API shape
suggests one thing and the underlying mechanism does another. Recorded
as comparisons, not verdicts — each includes the concrete mechanism so
the reader can judge how much it matters for a given use case.

**Restarting the interpreter is disabled by default in jpy's own test
config.** `PyLib.stopPython()`'s own javadoc
(`~/devel/jpy/src/main/java/org/jpy/PyLib.java:243-259`) states that
stopping the interpreter again after a restart "currently causes a fatal
error in the Java Runtime Environment," linking jpy's own
[issue #70](https://github.com/bcdev/jpy/issues/70). `STOP_IS_NO_OP`
(`PyLib.java:57`, `Boolean.getBoolean("jpy.stopIsNoOp") ||
ON_WINDOWS`) makes `stopPython()` skip `Py_Finalize` when set, so "stop"
becomes a no-op that leaves the interpreter alive — no module teardown,
no `sys.modules` clear. jpy's own `setup.py:226-232` (`test_maven`) sets
`-Djpy.stopIsNoOp=true` for every Maven test run, because, per its own
comment, multiple start/stop cycles in the same JVM crash CPython. The
one test that exercises a real stop/start/stop cycle,
`LifeCycleTest.testCanStartAndStopWithoutException`, self-skips under
that flag (`Assume.assumeFalse(..., "jpy.stopIsNoOp")`) — it doesn't run
under the config the suite's own Maven run uses. jpype doesn't offer
interpreter restart either, but takes a different approach to the same
underlying constraint: it exposes PEP 684 subinterpreters
(`Py_NewInterpreterFromConfig`/`Py_EndInterpreter`,
`native/common/jp_bridge.cpp:454-560`) as independently disposable
instances (`org.jpype.SubInterpreter`) rather than a single root
interpreter meant to be torn down and revived.

**Boxed-type selection for a generic `Object`/`Number` argument depends
on the value's magnitude, not its declared type.**
`JType_CreateJavaNumberFromPythonInt`
(`~/devel/jpy/src/main/c/jpy_jtype.c:542-563`) picks `Byte`/`Short`/
`Integer`/`Long` by testing whether the value survives a narrowing cast
(`b`/`s`/`i` vs. `j`). The same Python `int` literal boxes to a different
Java runtime class depending purely on its magnitude at a given call:
`foo(5)` boxes as `Byte`, `foo(5000)` as `Short`, `foo(5_000_000)` as
`Integer`, `foo(5_000_000_000)` as `Long`. Since Java-side overload
resolution and `instanceof` both dispatch on the boxed object's runtime
class, `someMethod(x)`'s effective behavior can change based on how
large `x` happens to be, for arguments that are all equally plain
Python `int`s — worth flagging because the boundary is a magnitude
threshold rather than anything visible at the call site. jpype's
equivalent path, `JPConversionBoxLong::convert`
(`native/common/jp_classhints.cpp:1662-1697`), boxes a plain Python `int`
to `java.lang.Long` unconditionally, independent of magnitude. jpype does
vary the box class for numpy scalar types (`numpy.int32` → `Integer`,
`numpy.int16` → `Short`), driven by the input's dtype identity rather
than its magnitude.

**Java exceptions surface as a single Python `RuntimeError`, without
per-type distinction.** `JPy_HandleJavaException`
(`~/devel/jpy/src/main/c/jpy_module.c:1244-1420`) is jpy's only
Java-to-Python exception path and always ends in
`PyErr_Format(PyExc_RuntimeError, ...)` (lines 1398/1411), regardless of
whether the underlying exception was a `NullPointerException`, an
`IllegalArgumentException`, or an application-defined checked exception.
`getCause()` is walked (line 1394) to splice `"caused by "` text into
that one message (lines 1259-1284) — there is no `__cause__`, no
`__context__`, and no distinct Python exception object per cause. The
detailed message text (the stack-trace walk) only runs when
`JPy_VerboseExceptions` is set (line 1255); otherwise the message is
`error.toString()`. Net effect: `except SomeSpecificException` isn't
available through jpy — every failure is a `RuntimeError` to the caller.
jpype's `JException` (`jpype/_jexception.py`, `@JImplementationFor
("java.lang.Throwable", base=True)`) maps each Java exception class onto
its own Python exception type, mirroring the Java `Throwable` hierarchy,
so `except java.lang.NullPointerException` and `except
java.lang.IllegalArgumentException` are distinguishable, and
`getCause()`/`getMessage()`/`printStackTrace()` remain available as
methods on the exception object.

**Threads that call from Python into Java attach to the JVM as
non-daemon, with no detach call anywhere in jpy's source.**
`JPy_GetJNIEnv` (`jpy_module.c:267-298`) calls plain
`AttachCurrentThread` (line 281), not `AttachCurrentThreadAsDaemon`, on
`JNI_EDETACHED`. There is no `DetachCurrentThread` call anywhere in jpy's
C or Java source (confirmed by grep across `src/main/c/*.c`). Two
consequences follow: every Python thread that calls a Java method keeps
its JVM-side thread registration for the thread's whole life, with no
jpy API to release it; and because the attach is non-daemon,
`DestroyJavaVM` (JNI-mandated to block until all non-daemon threads
exit) will wait on any such thread that's still alive but idle — a
Python thread that made one Java call and is now sitting in
`time.sleep()` can hold up JVM shutdown, for a reason not visible from
the call site that triggered the attach. jpype also auto-attaches, but
as a daemon, with the tradeoff named in its own API:
`JPContext::getEnv()` (`native/common/jp_context.cpp:870-894`) attaches
via `AttachCurrentThreadAsDaemon` "so that the newly attached thread does
not deadlock the shutdown" (comment, line 885-886), and
`java.lang.Thread.isAttached()`/`.attach()`/`.attachAsDaemon()`/
`.detach()` (`jpype/_jthread.py:22-84`) let a long-running thread detach
explicitly.

**`DestroyJavaVM` has no guard against a daemon thread mid-callback into
Python.** `JPy_destroy_jvm` (`jpy_module.c:510-521`) calls
`DestroyJavaVM()` with no `isRunning()`/shutting-down check anywhere in
jpy's proxy-invocation path, in either direction. jpy's only
shutdown-race guard is `Py_IsFinalizing()` (`org_jpy_PyLib.c:57-86`),
checked at the top of every native entry point for the opposite
direction (a Java thread calling into Python while Python is
finalizing) — and its own comment states it "doesn't completely prevent
the race condition (TOCTOU), but... mitigates the risk significantly."
jpype's model (`[[jvm_shutdown_daemon_thread_safety]]`) relies on
`DestroyJavaVM`'s own JNI-mandated block on non-daemon threads for the
general case (`jp_context.cpp:97-101`: "VM_Exit parks all remaining
daemon threads at the final safepoint; nothing executes Java code after
DestroyJavaVM returns"), plus one targeted check
(`jp_proxy.cpp:161`, `context->isRunning() || ...is_shutting_down`) for
the one gap that guarantee doesn't cover — a daemon-thread proxy
callback still parked when shutdown completes underneath it.

### jpy: GIL discipline and native-handle lifetime — two places the approaches match

Two properties of jpy's Java-side/threading engineering, separate from
the C-side conversion behaviors above, where jpy and jpype land on
equivalent (if differently-implemented) safety.

| Concern | jpy's approach | jpype's approach | Evidence |
|---|---|---|---|
| GIL acquisition at native entry points, called from arbitrary (Java) threads | `PyGILState_Ensure`/`Release` around every JNI entry point, ~35 call sites; rejects a Python call made mid-interpreter-shutdown rather than racing it | Same primitive, same discipline: `PyGILState_Ensure`/`Release` around Python calls from Java threads, including the reentrant case (`PyGILState_LOCKED` → release is a documented no-op) and `PyGILState_Check()`, chosen because it stays reliable across subinterpreters | jpy: `~/devel/jpy/src/main/c/jni/org_jpy_PyLib.c:65-86` and ~35 call sites (e.g. 284, 407, 501, 881). jpype: `native/common/jp_bridge.cpp:280-390,601-624`, `native/python/jp_pythontypes.cpp:407-476` |
| Preventing a native pointer from being reclaimed/reused while a JNI call using it is in flight | Reads the native pointer off the live wrapper object, then relies on `Reference.reachabilityFence(this)` (`PyObject.java:33-40`) as an explicit guard against the JIT deciding the wrapper is dead early | Never reads the pointer off a live object at cleanup time: `org.jpype.ref.NativeReference` is a `PhantomReference` that copies the native handle onto itself at construction (`hostReference` field); a `PhantomReference` is only enqueued once the referent is already proven unreachable, so there's no live-object race to fence against | jpy: `~/devel/jpy/src/main/java/org/jpy/PyObject.java:33-40`. jpype: `native/jpype_module/src/main/java/org/jpype/ref/NativeReference.java:62-108` |

Same outcome (no use-after-free of the native handle, no unsafe call
during shutdown) reached two different ways: jpy adds an explicit guard
against a hazard its design creates; jpype's design doesn't create that
hazard shape. The non-daemon thread-attachment behavior above is a
separate point in the same subsystem — GIL discipline and the
reachability fence hold up independently of it.

## jpype vs. jep

### Features jpype has that jep does not

| Feature | jpype | jep | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | no hints/customizer machinery found in `jep/src/main/c` |
| Multi-dimensional numpy array *push* (Python ndarray → `int[][]` etc. as a method argument) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | `jep_numpy.c`'s only `PyArray_NDIM` use (line 399) is on the opposite direction (Java array → Python ndarray return value) |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jep source or test suite |
| Javadoc-derived docstrings, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_module.py`, `test_module2.py` | Only bare `dir()` listing of method names (`test_dir.py`) — no docstrings, no stub generation | `pyjobject`/`pyjtype` expose method names via `dir()` but no docstring text sourced from Javadoc |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in jep source |
| Per-class conversion caching with generation-based invalidation | Yes | Not applicable the same way — jep's overload resolution already short-circuits on arity before per-argument type work (see below) | `pyjmultimethod.c:118-155`, `pyjmethod.c:284` |

### jep: threading, lifecycle, and exception handling

The same lens applied to jpy above (thread attachment, GIL discipline,
sub-interpreter shutdown, exception fidelity, boxed-type selection,
shutdown-vs-daemon-thread guarding, native-object lifetime) applied to
jep's own source (`~/devel/jep/src/main/c/Jep/*.c`,
`src/main/java/jep/**/*.java`).

**Java-to-Python exceptions collapse into one type, `JepException`.**
`process_py_exception` (`src/main/c/Jep/jep_exceptions.c:42-175`) is
jep's only Python-to-Java exception path; every Python exception becomes
a `JepException(String, long)` built from `"ExcType: message"` string
concatenation (lines 148-163), not a distinct Java exception class per
Python exception type — `catch SomeSpecificPythonException` isn't
available, the same shape as jpy's `RuntimeError` collapse above. One
case jpy's version doesn't cover: if the Python exception is itself
wrapping a Java exception that crossed into Python and back
(a `PyJObject`-backed exception), jep preserves that as a real
`Throwable` cause via a second constructor, `JepException(String,
Throwable)` (lines 162-167) — cause-chaining for that round-trip case
specifically, not for exceptions that originate natively in Python.
jpype's per-class `JException` mapping (`jpype/_jexception.py`) has no
counterpart in jep for either case.

**The proxy-invocation path has no liveness check at shutdown.**
`jep.python.InvocationHandler.invoke()`
(`src/main/java/jep/python/InvocationHandler.java:132-141`) calls
straight into native code with no liveness check, and the native side,
`Java_jep_python_InvocationHandler_invoke`
(`src/main/c/Jep/python/invocationhandler.c`), has no
`Py_IsFinalizing()`/interpreter-liveness check anywhere in the file. A
Java thread mid-callback into a Python-implemented proxy
(`jep.jproxy()`) when the interpreter is closing has no equivalent of
jpype's `jp_proxy.cpp:161` `isRunning()`/`is_shutting_down` guard, and
not even jpy's TOCTOU-checked `Py_IsFinalizing()` on the other
direction — this is the one place neither jpy's nor jpype's guard has a
counterpart in jep.

**No check that a `PyObject` is being touched by the interpreter that
created it.** jpype's guard for this (`JPClass::convertToPythonObject`,
`native/common/jp_class.cpp:379-403`) exists because own-GIL
subinterpreters have separate allocators/arenas — handing one
interpreter's `PyObject*` to another's Python code is memory corruption,
so `proxy->m_Context != context` is checked explicitly and raises a
`RuntimeError` rather than touching the pointer. jep documents the same
constraint without enforcing it: `jep.python.PyObject`'s javadoc states
"This class is not thread safe and PyObjects can only be used on the
Thread where they were created. When an Interpreter instance is closed
all PyObjects from that instance will be invalid"
(`src/main/java/jep/python/PyObject.java:36-38`). Tracing the call path:
`PyObject.tstate()` → `MemoryManager.getThreadState()` →
`getThreadLocalJep()` (`src/main/java/jep/python/MemoryManager.java:124-134`)
looks up whatever `Jep` instance is bound to the calling thread via a
plain `ThreadLocal<Jep>`, throwing only if none is bound
("`Invalid thread access.`") — it doesn't check that this specific
`PyObject`'s originating interpreter matches. That `tstate` is then
passed into native code together with the object's raw pointer
(`Java_jep_python_PyObject_getAttr` and five sibling functions,
`src/main/c/Jep/python/jep_object.c:36-278`) with no ownership check.
The general shape of the hazard: a Java-side collection that outlives
the interpreter call which produced the stored `PyObject`/proxy is a
potential site for this, not only a cross-thread one.

Two reproduction attempts on the same thread: (1) opening a second
`SharedInterpreter` while a first is still open is blocked outright —
`JepException: "Unsafe reuse of thread main for another Python
Interpreter. Please close() the previous Interpreter to ensure
stability"` — a real guard, though incidental rather than an ownership
check. (2) Closing the first interpreter, then opening a second on the
freed thread and touching the first's stashed object returned a
correct-looking result rather than crashing; the reason is that
`SharedInterpreter` instances aren't separate CPython-level
subinterpreters at all (`SharedInterpreter`'s own javadoc: instances
"share all imported modules" and only keep *globals* distinct — the
same underlying construct as jpype's `Script`, one interpreter with N
independent globals dicts, not `Py_NewInterpreter`/
`Py_NewInterpreterFromConfig` isolation). A genuine own-GIL
`SubInterpreter`-vs-`SubInterpreter` reproduction — the case that would
actually exercise this gap, since each `new SubInterpreter()` normally
gets its own `MemoryManager` — wasn't attempted. So: the structural gap
is confirmed by source (no `proxy->m_Context != context`-style check
anywhere in jep), but the two concrete repro attempts tried here didn't
produce a live crash, for reasons specific to which jep class each one
exercised — an open question rather than a demonstrated crash.

**`SharedInterpreter` and `Script` are the same underlying construct
under different names.** Checked directly: opening a `SharedInterpreter`
does `globals = PyDict_New(); PyDict_SetItemString(globals,
"__builtins__", ...)` (`src/main/c/Jep/pyembed.c:766-768`) — a fresh
Python dict, not `Py_NewInterpreter`/`Py_NewInterpreterFromConfig`
anywhere in that path. Every `SharedInterpreter` instance runs in the
same underlying CPython interpreter, one GIL, one `sys.modules`, with
only its own `globals` dict — the same pattern as jpype's `Script`
(`org.jpype.Script`, "a scope of variables in the Python interpreter...
housed in Java space"), distinct from `jep.SubInterpreter` or jpype's
own `SubInterpreter` (both `Py_NewInterpreter`-backed isolation). jep
gives this shared-globals pattern its own class in a naming family that
otherwise reads as "isolated interpreter" (`SubInterpreter`/
`SharedInterpreter` share the `Interpreter` API), where jpype keeps the
always-cheap, never-isolated version (`Script`) visibly separate from
the opt-in real-isolation one (`SubInterpreter`).

**Three comparisons where jep's approach matches jpype's or jpy's:**

| Concern | jep's approach | Comparison | Evidence |
|---|---|---|---|
| Thread attachment for calls crossing into Java | `AttachCurrentThreadAsDaemon`, with a comment reasoning through why: "there are no hooks to detach the thread later[, so] daemon is the only way to let the process exit normally" | Matches jpype's daemon-attach approach; addresses the hazard jpy's plain-`AttachCurrentThread`-without-detach creates | `src/main/c/Jep/pyembed.c:817-834` |
| Boxed numeric type selection for a generic Java target | `pylong_as_jobject` dispatches on the declared/expected Java type via `IsAssignableFrom` checks (Long → Integer → Byte → Short → BigInteger fallback on overflow), not the Python value's magnitude | Type-stable, matching jpype's fixed-boxing approach rather than jpy's magnitude-dependent behavior above | `src/main/c/Jep/convert_p2j.c:317-368` |
| Sub-interpreter shutdown | `pyembed_thread_close` calls `Py_EndInterpreter(jepThread->tstate)` when closing a non-main interpreter thread — no `stopIsNoOp`-style flag in the source | Matches the honest shape of jpype's `SubInterpreter.close()`; jep's multi-`Jep`-instance isolation is real, not a no-op flag around a crash | `src/main/c/Jep/pyembed.c:794-805` |

**Object lifetime: a manual-only model, distinct from both jpy's and
jpype's.** `jep.python.PyObject` (Java) has no `PhantomReference`/
`Cleaner`/`reachabilityFence` — cleanup is exclusively manual, via
`close()`; a `PyObject` becomes invalid once its owning interpreter
closes (per its own javadoc). This sidesteps jpy's live-pointer race
(there's no GC-triggered decref racing a JNI call, because there's no
GC-triggered decref at all), but trades it for a pure manual-lifetime
contract: forgetting to call `close()` leaks the native object until its
sub-interpreter tears down. A third design point, distinct from jpy's
fence-guarded live-pointer model and jpype's phantom-reference model.

**GIL discipline uses a different mechanism.** jep acquires/releases via
`PyEval_AcquireThread(jepThread->tstate)`/`PyEval_ReleaseThread` against
a per-`JepThread`-cached `PyThreadState` (12+ call sites in
`pyembed.c`), not the `PyGILState_*` TLS API jpy and jpype both use — a
choice consistent with jep predating PEP 684's sub-interpreter story. No
shutdown-race guard comparable to jpy's `Py_IsFinalizing()` check was
found in this path.

**Concurrency: jep's own adversarial tests pass, for a narrower claim
than jpype/jpy's.** Built jep's existing native lib
(`build/lib.linux-x86_64-cpython-312`) and ran its two adversarial
multithreading tests directly: `jep.test.synchronization.TestCrossLangSync`
(16 Python-sub-interpreter threads + 16 Java threads on one shared
lock/`AtomicInteger` via `obj.synchronized()`) and
`jep.test.TestSharedModulesThreads` (16 threads concurrently creating
`SubInterpreter`s and importing the same shared module). Both exited 0.
Neither is the same claim as
`GilConcurrencyParityNGTest`
(`native/jpype_module/src/test/java/org/jpype/GilConcurrencyParityNGTest.java`)
or jpy's `MultiThreadedEvalTestFixture` test check: N uncoordinated
threads mutating one shared interpreter's globals with no explicit
lock, relying on the automatic per-call GIL guard for correctness. jep
has no construct for that scenario — `SharedInterpreter`'s javadoc
states each instance "still maintains distinct global variables" even
though modules are shared, and mixing `Interpreter` instances on the
same thread at the same time is unsupported
(`SharedInterpreter.java:34-44`); `MainInterpreter` is bootstrap
machinery for GIL-deadlock avoidance, not something application code
runs against directly. jep's concurrency safety here comes from
architecturally not sharing mutable interpreter state across threads,
rather than from a guard proven safe under shared mutable state the way
jpype's is (`[[jvm_shutdown_daemon_thread_safety]]`-adjacent).

**From the benchmark side:**

- A reproducible `OutOfMemoryError` in jep's own multi-dimensional array
  benchmark, `array_multidim.py`, partway through its combined
  `int`/`long`/`float`/`double` sweep, even at `-Xmx3g` (raising the heap
  to 4GB then 6GB only postpones it). Specific to that one long-running
  combined-sweep process — the narrower single-scenario jep scripts
  (`array_ragged.py`, `array_noncontig.py`, `array_shape.py`) complete
  cleanly at the same depths/sizes under the stock default heap,
  consistent with an allocation-rate-vs-GC-throughput issue rather than
  a fixed working-set size (`RESULTS.md` Section 10). Not root-caused at
  the source level here.
- jep has no `array->buffer` (Java array → numpy) bulk return path at
  any depth — its numbers are `array->list`'s per-element cost plus a
  redundant `np.asarray()` wrap, landing worse than its own list-pull
  number: ~180x slower than jpype/jpy at `long[100000]` (10.7M ns vs.
  111-116K ns, `RESULTS.md` Section 3). Same architectural gap as
  pyjnius; see the fast-bulk-path-coverage table above for the full
  accounting.

### Features jep has that jpy lacks

Noted because it changes the porting-effort picture from the jpy table
above — these jpype-suite categories that had no jpy counterpart do have
something to port to for jep.

| Feature | jep | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterable` | Real `__getitem__`/`__setitem__`/slicing/iteration, backed by the actual Java collection | `pyjlist.c`, `pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`, `pyjiterator.c` |
| `Comparable`/hashing duck-typing | `tp_richcompare` delegates to `Comparable.compareTo`, `tp_hash` to `hashCode` | `pyjobject.c:155-181`, `pyjobject.c:310-312` |
| Functional-interface duck typing (pass a Python callable directly as a Java SAM interface arg) | Detected via `isInterface` + single-abstract-method check | `pyjtype.c:259-335` (`functionalInterface`) |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | Full `InvocationHandler`-style proxy, own test file | `jep/Proxy.java`, `java_access/Proxy.c`, `test_jproxy.py` |
| `synchronized` block support | `pyjmonitor.c`, `test_synchronized.py` | jpy has no equivalent test or source |

### Where jep does less work per call

Verified by reading `pyjmultimethod.c`/`pyjmethod.c`, not inferred from
timing. jep's overload resolution filters candidates by parameter count
only (O(1), no type inspection); the per-argument type-compatibility
check (`PyJMethod_CheckArguments`) only runs when two or more candidates
share the same arity. For a method with a single overload (the common
case, and the case for every array/scalar benchmark in `RESULTS.md`),
jep converts arguments directly against the one known parameter type in
a single pass. jpype always runs a full `matches()` scoring scan then a
separate `convert()`, even with no overload ambiguity, since its
architecture doesn't special-case "only one candidate" — this is also
why jep falls behind on the 16-overload dispatch benchmark in
`RESULTS.md` (all candidates share arity there, so jep's shortcut can't
fire and the more expensive check runs repeatedly).

### Not compared here (excluded, no jep equivalent to port to)

Same exclusion as jpy: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
exercise jpype's own internals, not portable behavior.

### jep suite-size context

jep: 34 Python test files, 247 tests, `~/devel/jep/src/test/python/`.
Larger than jpy's suite, still under a fifth of jpype's 1,884. The
collection-protocol, Comparable, functional-interface, and proxy support
above mean a larger fraction of jpype's suite has something to port
against for jep than for jpy — but `test_classhints.py`/
`test_hints.py`/`test_customizer.py`, `test_pickle.py`/`test_serial.py`,
and the introspection-ergonomics files remain gaps for jep too.

## jpype vs. pyjnius

### Features jpype has that pyjnius does not

| Feature | jpype | pyjnius | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | grep of `jnius/*.pxi`/`reflect.py` for hints/customizer/register-conversion machinery: nothing |
| Multi-dimensional / buffer-protocol array push (`int[]`/`int[][]` etc. from an ndarray) | Bulk buffer path (`caching-multidim-push` branch), any dimension | Not supported at any dimension, including flat 1D — narrower than jpy or jep, which at least accept a 1D buffer object | `DeepBench.sumIntArray(numpy.arange(...))` raises `JavaException('Expecting a python list/tuple, got array(...)')` unconditionally, for any ndim |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | no `__reduce__`/pickle-registration logic in pyjnius's own source (the one `pickle` hit in the built `jnius.c` is Cython's own generated module boilerplate) |
| Introspection ergonomics: docstrings from Javadoc, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | Only bare `dir()` listing (method names visible, `__doc__` is `None` for every bound method) — same situation as jep | no docstring-generation code found in `jnius/*.pxi`/`reflect.py` |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in pyjnius source |
| Per-class conversion caching with generation-based invalidation | Yes | Not applicable the same way — no equivalent per-call matching cost was found to cache (see numpy-scalar-dispatch note above, which is a coverage gap, not a caching opportunity) | `jnius_conversion.pxi` |

### Features pyjnius has that jpy lacks

Confirmed working empirically, not just found in source.

| Feature | pyjnius | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterator`/`Map.Entry` | Real `__getitem__`/`__setitem__`/`__len__`/`__contains__`/`__iter__`, backed by the actual Java collection | `reflect.py`'s `protocol_map`, applied automatically inside `autoclass()` to every class whose hierarchy includes one of these interfaces; verified an `ArrayList`/`HashMap` support `len()`, indexing, iteration, and `in` directly |
| `Comparable`/`Iterable` duck-typing | `__lt__`/`__gt__`/`__eq__`/etc. delegate to `compareTo`/`equals`; `__iter__` delegates to `iterator()` | same `protocol_map`; verified `Integer(3) < Integer(5)` works directly |
| `AutoCloseable`/`Closeable` → Python context manager protocol | `__enter__`/`__exit__` delegate to `close()` | same `protocol_map` (jpype has this too, `jpype/_jio.py` — noted here since pyjnius has it as well) |
| Functional-interface duck typing (pass a Python `lambda`/callable directly as a Java SAM interface arg) | Supported | `jnius_conversion.pxi`'s functional-interface detection (~line 415-451); verified `DeepBench.invokeCallback(lambda x: x + 1, 5)` works directly, no proxy class needed |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | `PythonJavaClass` subclass + `@java_method('<jni-signature>')`, own test file (`test_proxy.py`) | present and works for the common case (verified `int`-arg callback) — see the reproduced defect below for one input shape it doesn't handle |

### A reproduced defect in pyjnius's proxy implementation

Unlike jpy's proxy gap (a construction-time failure — see the jpy table
above) and jep's proxy support (no defect found), pyjnius's general
proxy mechanism crashes on one specific input: a Python-implemented Java
interface method receiving a genuinely **null** `Object` argument
(`DeepBench.invokeObjectCallbackWithNull` — `jpype.benchmark.DeepBench`'s
`ObjectCallback` methods exist specifically to cover this case, per
`jp_proxy.cpp`'s own history) segfaults the JVM with a native `SIGSEGV`
in `jni_GetObjectClass`. Reproduced independently three times against a
fresh build in a disposable venv (checked for stale-build-state first,
per this repo's CLAUDE.md, before treating it as real). pyjnius's
proxy-argument-marshalling code calls `GetObjectClass`/`IsSameObject` on
the argument without a null check.

The non-crashing case (a real, non-null `Object` argument) also doesn't
round-trip correctly: `invokeObjectCallback` returns `None` instead of
the object the Python callback handed back. See
`project/benchmark/RESULTS.md`'s pyjnius section and
`project/benchmark/pyjnius/proxy.py`; that script deliberately never
calls the null-argument variant.

### Where pyjnius's behavior differs on its own tradeoffs

| Behavior | jpype | pyjnius |
|---|---|---|
| numpy scalar dispatch for `int`/`long` parameters | Resolves all numpy scalar types (`int32`/`int64`/`float32`/`float64`) correctly and unambiguously | Same underlying gap as jpy (no fallback for non-exact-Python-type numeric args on `int`/`long` params), with a different failure mode: `Math.max(np.int32(3), 5)` raises `"No static methods called max in java/lang/Math matching your arguments... available: [...]"` rather than jpy's "ambiguous Java method call" |
| Scalar/boxed/string/proxy call overhead generally | See `RESULTS.md` | Highest of the four libraries on every boxed/string/proxy row measured, in some cases by 5-10x (`new Integer`: 7403ns vs. 840-1257ns elsewhere; `new String`+`.toString()`: 22349ns vs. 927-2512ns; proxy: 39412ns vs. 2240-2655ns) — not attributed to a specific mechanism at the source level here |

### Not compared here (excluded, no pyjnius equivalent to port to)

Same exclusion as jpy/jep: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
exercise jpype's own internals, not portable behavior.

### pyjnius suite-size context

pyjnius: 37 Python test files, 160 tests, `~/devel/pyjnius/tests/`.
Between jpy's 21 files and jep's 34-file/247-test suite in file count,
but fewer total tests than jep's. The collection-protocol/Comparable/
functional-interface/general-proxy support above means pyjnius has
something to port against for a comparably large slice of jpype's suite
as jep does — but `test_classhints.py`/`test_hints.py`/
`test_customizer.py`, `test_pickle.py`/`test_serial.py`, the
introspection-ergonomics files, and (uniquely among the three) any
multi-dimensional/buffer array test remain gaps for pyjnius.

## Future: jpype's reverse-embedding direction (`origin/reverse`)

`org.jpype.MainInterpreter` (`~/devel/jpype` on `origin/reverse`,
`native/jpype_module/src/main/java/org/jpype/MainInterpreter.java`) is a
Java-side singleton that locates/probes/launches an embedded CPython
interpreter from Java code, with no Python process involved in starting
anything — the same shape as jep's "Java hosts Python" niche. Demonstrated
by `native/jpype_module/src/test/java/runner/HelloWorldMain.java`, a
pure `public static void main(String[] args)` with no Python involvement
in bootstrapping:

```java
public static void main(String[] args) {
    MainInterpreter.getInstance().start(new String[0]);
    Script context = new Script(MainInterpreter.getInstance());
    context.exec("msg = 'Hello World from Python'");
    PyObject msg = context.eval("msg");
    ...
}
```

`origin/reverse` exposes three separate embedding layers:

1. **JSR-223** (`org.jpype.script.JPypeScriptEngine`) — implements
   `javax.script.AbstractScriptEngine`/`Invocable`, the standard scripting
   API every JVM already has a pluggable-scripting-language story for
   (`ScriptEngineManager`) — not jpype-specific.
2. **Context** (`org.jpype.Script`, built on `MainInterpreter`) — its own
   javadoc: "a scope of variables in the Python interpreter... we can
   consider these to be modules housed in Java space." Each `Script`
   instance owns its own globals/locals `PyDict`, exposes `eval()`/
   `exec()`/`importModule()`, and multiple `Script` instances can coexist
   against one shared interpreter — the same pattern other embedding
   systems call a Context/session object (e.g. GraalVM's `Context`).
3. **`python.lang`** — a 54-file typed Java class library mirroring
   Python's builtin type system: `PyObject`, `PyDict`, `PyList`,
   `PySet`/`PyFrozenSet`, `PyInt`/`PyFloat`/`PyComplex`,
   `PyString`/`PyBytes`/`PyByteArray`, `PyGenerator`/`PyCoroutine`/
   `PyAwaitable`, `PyCallable`, iterators for all of it. Its
   `package-info.java` states the design intent: implement Java
   collection interfaces where they don't conflict with Python
   semantics, tight return types, loose parameter types, fall back to
   `eval()` only when a wrapper can't express something.

**How `python.lang` gets populated is a fourth proxy model, built on top
of the other two.** jpype has three ways to make a Python object answer
to a Java interface:

1. **Bare-callable SAM duck typing** — a plain Python `lambda`/callable
   passed directly where a Java functional interface argument is
   expected, no proxy class or registration.
2. **`@JImplements`** — a Python class declares which Java interface(s)
   it implements, once, ahead of time (`jpype/_jproxy.py`).
3. **`JProxy(interfaces, dict=... | inst=...)`** — hand a dict of
   callables or an object instance to `JProxy`'s constructor directly;
   `@JImplements`'s predecessor, still supported (`_jproxy.py:186-234`).
4. **Automatic, structural, no user call at all** —
   `JPConversionPython` (`native/common/jp_classhints.cpp:1908-1992`), a
   conversion rule `JPPybaseType::findJavaConversionImpl`
   (`jp_pybasetype.cpp:34-48`) tries for `java.lang.Object` and, by
   inheritance, every interface type that falls through to it. Any time
   a Python value needs to become a Java-typed value — argument, return,
   field, not just an explicit proxy site — `matches()` calls
   `PyJP_probe(st, Py_TYPE(object))` (`native/python/pyjp_probe.cpp`),
   which reads the Python type's own C-level protocol slots (`tp_call`,
   `tp_as_buffer`, `tp_as_sequence`, `tp_as_mapping`, `tp_as_number`,
   `__enter__`/`__index__`) plus `collections.abc` subclass checks to
   derive which `python.lang` interfaces that type structurally
   satisfies. If a probed interface matches the target, `convert()`
   (`jp_classhints.cpp:1972-1991`) constructs a `JProxy` on the spot —
   `_jpype._JProxy`, the same class backing models 2 and 3, wrapping the
   value with the method table the probe resolved. Model 4 is models
   2/3's own machinery, invoked automatically by structural type-probing
   instead of an explicit decorator or constructor call.

jpy's typed wrappers (`PyModule`, `PyDictWrapper`, `PyListWrapper` —
three classes total) must be constructed explicitly by the caller around
a generic `PyObject`, with no probe-driven automatic selection; its own
general-proxy support (`PyObject.createProxy()`) didn't produce a usable
object in this checkout regardless. jep's Python-to-Java dispatcher,
`PyObject_As_jobject`
(`~/devel/jep/src/main/c/Jep/convert_p2j.c:1050-1116`), has one
automatic, declared-type-driven case: `PyCallable_Check(pyobject) &&
isFunctionalInterfaceType(env, expectedType)` triggers
`PyCallable_as_functional_interface` (lines 1091-1097), converting any
Python callable into any SAM-shaped target interface automatically, on
both argument and return paths. Everything else in that function is a
fixed, hardcoded C-level `if`/`else` chain (`PyLong_Check`/`PyDict_Check`/
`PyUnicode_Check`/buffer/numpy) mapping to a closed, compiled-in set of
concrete Java types, with `jep.python.PyObject` as the catch-all —
adding a new target interface to jep's version means patching and
recompiling its C source, the same extensibility gap as the
`WrapperService`/`.pyspi` comparison below, applying here to
proxy-selection specifically.

**All four models, side by side:**

| Model | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| 1. Bare lambda/callable → SAM argument (forward: Python passes a callable where Java wants a functional interface) | Yes | No — only explicit proxy objects, no implicit lambda conversion | Yes (`pyjtype.c`'s `functionalInterface`) | Yes (`jnius_conversion.pxi`) |
| 1b. Same idea, declared-type-driven on the reverse side (Java expects a functional interface back from Python, no proxy call) | Yes (subsumed into model 4) | N/A — no reverse direction | Yes — `PyCallable_as_functional_interface`, `convert_p2j.c:1091-1097` | N/A — no reverse direction |
| 2/3. Explicit proxy (decorator and/or manual dict/inst construction) | Two forms: `@JImplements` (typed, modern) and `JProxy(dict=...\|inst=...)` (manual, older, still supported) | One form, `PyObject.createProxy()` — did not produce a usable object in this checkout | One form, `jep.jproxy()` — no defect found | One form, `PythonJavaClass` + `@java_method(...)` subclassing — works for the common case, with the null-`Object`-argument segfault and return-value bug noted above |
| 4. Automatic, structural, no proxy call — Java declares the type it wants and gets a live proxy for free | Yes — `JPConversionPython`/`PyJP_probe`, probes Python's own C-level protocol slots, extensible to `WrapperService`-registered interfaces | No — three hand-written wrapper classes, constructed explicitly, no probing | No — `PyObject_As_jobject` is a fixed C-level type chain to a closed set of concrete Java types (plus the model-1b functional-interface case); a new target interface means patching and recompiling jep's C source | N/A — no Java-hosts-Python direction exists |

jpype is the only one of the four with a structural, extensible,
no-call-site-changes automatic path (model 4); the other three require
the Python side to either be plain-callable (model 1) or opt into a
proxy at construction time (models 2/3). jep independently arrived at
the same idea as model 4, scoped to callables-as-functional-interfaces
rather than generalized to arbitrary multi-method interfaces via
protocol introspection. jpy and pyjnius have no reverse direction to
compare model 4 against (jpy's exists but wasn't usable in this
checkout; pyjnius's doesn't exist).

jep has no JSR-223 `ScriptEngine` implementation (checked
`~/devel/jep/src/main/java/jep/` — no `javax.script` reference), and its
public embedding surface is essentially one class (`Jep`, with
`eval`/`exec`/`getValue`/`set`) plus its `pyj*` wrapper types, not three
separated layers.

**A user-extensible SPI, beyond `python.lang`'s builtin coverage.**
`org.jpype.WrapperService` (discovered via `java.util.ServiceLoader`,
JPMS-compatible via `provides ... with` in `module-info.java`) lets any
Java library expose any Python class as a typed Java interface by
registering a provider and dropping a declarative resource file per
class (`.pyspi`: a `key: value` header naming the Python
module/class/target Java interface, a `---` separator, then a Python
source blob binding a `METHODS = {...}` dict) — no editing of jpype's
own source. Example, `collections.deque.pyspi`:

```
kind: class
module: collections
class: deque
interface: python.collections.PyDeque
---
METHODS = {
    ".addFirst": lambda x, v: x.appendleft(v),
    ".removeFirst": lambda x: x.popleft(),
    ".size": len,
    ...
}
```

— mapping Python's `deque` onto a Java interface using
`java.util.Deque`'s own method names (`addFirst`/`removeFirst`), so Java
code gets a collection backed transparently by the real Python object.
jpype ships five built-in providers this way (27 `.pyspi` files,
475/475 tests passing): `python.io` (the `io`/`_io` hierarchy —
`BytesIO`/`StringIO`/`FileIO`/`BufferedReader`/`Writer`/`TextIOWrapper`/
etc.), `python.collections` (`ChainMap`/`Counter`/`OrderedDict`/
`defaultdict`/`deque`), `python.datetime` (`date`/`datetime`/
`timedelta`), `python.decimal` (`Decimal`), `python.pathlib`
(`PosixPath`/`WindowsPath`).

None of jpy/jep/pyjnius have an equivalent. jep's collection support
(`pyjlist.c`/`pyjmap.c`/etc.) and pyjnius's `protocol_map`
(`reflect.py`) are both real and both verified working above, but both
are fixed in each library's own source — adding support for a new
Python stdlib or third-party class (say, exposing `numpy.ndarray` as a
typed Java interface) means patching jep's or pyjnius's C/Cython source
and rebuilding the extension. jpy has no collection-protocol support to
compare against, let alone an extension mechanism for one.

**Neither side of the bridge needs awareness of the other.**
`JClassHints.registerClassImplementation(classname, proto)`
(`jpype/_jcustomizer.py:222-231`, behind `@JImplementationFor`) keys
purely on a string class name — no marker interface, no annotation, no
jpype dependency on the target's classpath, and no requirement that the
class exists yet at registration time (`_applyCustomizerPost` handles
customizing a class that's already loaded). `WrapperService`/`.pyspi`
has the same property from the other side: a Python module is declared
as satisfying a Java interface by name, with the module itself needing
no jpype awareness. Consequence: a closed-source, never-published Java
library or Python module can be customized to feel native, with the
customization living in a third location the end user writes, while the
library or module being customized stays unaware anything is bridging
into it. jep and jpy have no equivalent gate — the only way to get
comparable ergonomics for a private class there is patching and
recompiling their own C source, not an option for someone else's
internal library.

`jpype/_jcustomizer.py`'s `JImplementationFor(javaClassName)`/
`JConversion(cls, ...)` (a string-named target Java class, a decorator
registering a prototype whose methods get copied onto or converted to
that class's wrapper, applied retroactively even to an already-loaded
class) is the forward-direction version of the same pattern
`WrapperService`/`.pyspi` implements in reverse: string-named target
Python module/class, a declarative method binding to a named Java
interface, discovered and replayed at startup instead of hardcoded.
Every Python-side customizer referenced elsewhere in this doc
(`_JCharArray` on `byte[]`/`char[]`, the `toPython()` conventions on
`java.io` streams) is built on the forward version of this mechanism.

**Status**: `origin/reverse` is 190 commits ahead of `review` (the main
branch) and not yet merged — subinterpreters, cross-interpreter GC, an
`InterpreterPipe`, `toPython()` conversions for
`Instant`/`Path`/`File`/`BigDecimal`/dates, coverage raised to 90-100% on
many modules per its own plan docs — but not yet current `review`
behavior, and not re-verified with the same empirical rigor (actually
running it, checking edge cases) applied to jpy/jep/pyjnius elsewhere in
this document.

jpy also has a Java-hosts-Python entry point, independent of jep's:
`org.jpy.PyLib.startPython()`/`stopPython()`/`isPythonRunning()` lets a
pure Java application embed and control a Python interpreter directly,
demonstrated by a JUnit test with no Python bootstrap
(`src/test/java/org/jpy/EmbeddableTestJunit.java` ->
`EmbeddableTest`'s `PyLibControl` inner class, `~/devel/jpy`). So: jep's
architecture is natively Java-hosts-Python (its primary direction); jpy
has it as a secondary capability alongside its usual Python-hosts-Java
mode; pyjnius has neither (checked its Java sources specifically — only
test fixtures and the `PythonJavaClass` proxy-callback machinery, no
embeddable launcher). Once `origin/reverse` merges, jpype would cover
both embedding directions — the ground jep and jpy each independently
cover on the Java-hosts-Python side, plus jpype's existing
Python-hosts-Java surface — while pyjnius remains the only one of the
four with just one direction.

### Launching embedded Python from Java: three discovery models

Before any of the API surface above can run, something has to find the
right `python`/`libpython` on disk and load it into the JVM process.
pyjnius doesn't have this problem (no Java-hosts-Python direction).
jpype, jpy, and jep each answer it differently.

**jpy: static, ahead-of-time, no runtime discovery.** `PyLibConfig`'s
static initializer
(`~/devel/jpy/src/main/java/org/jpy/PyLibConfig.java:53-71`) only reads a
`jpyconfig.properties` file — from the classpath, `-Djpy.config=<path>`,
or the current working directory — and copies its keys into `System`
properties. That file is written once, ahead of time, by a separate
Python-side step (`jpyutil.write_config()`, run manually during setup).
`PyLib.loadLib()` reads `jpy.jpyLib`/`jpy.pythonLib` out of that config
via `getProperty(key, mustHave=true)` (`PyLib.java:521-546`) and calls
`System.load()` directly — no search, no fallback. If the config is
missing or stale (Python reinstalled, venv moved, wheel rebuilt), the
failure is immediate: `RuntimeException("missing configuration property
'jpy.jpyLib'")` (`PyLibConfig.java:120-122`). The discovery problem is
pushed onto the user/build system, once, before Java runs.

**jep: runtime discovery, by re-deriving Python's own search path in
Java.** `MainInterpreter.initialize()` first tries the conventional
`System.loadLibrary("jep")` (whatever `-Djava.library.path` already
points at); only on `UnsatisfiedLinkError` does it fall back to
`LibraryLocator.findJepLibrary()`
(`~/devel/jep/src/main/java/jep/MainInterpreter.java:124-135`). That
locator walks `PYTHONPATH` (`searchPythonPath`), then reimplements
CPython's own `site.py`-`getsitepackages()` layout against
`PYTHONHOME`/`VIRTUAL_ENV` (`searchSitePackages` — `lib`/`lib64`/`Lib`,
`site-packages`, `site-python`, versioned `pythonX.Y/site-packages`),
then user-site locations per PEP 370 across all three OS conventions
(`searchUserSitePackages` — `~/.local/lib/pythonX.Y/site-packages`,
Windows `%APPDATA%/Python`, macOS `~/Library/Python/X.Y`) —
`LibraryLocator.java:100-227`. A narrow self-healing step: if
`libjep`'s `System.load()` fails because a specific `libpython*.so`
isn't found, it regex-parses the missing library name out of the
`UnsatisfiedLinkError` message and searches `PYTHONHOME` for it
(`findPythonLibrary`, `LibraryLocator.java:253-292`). All of this is a
Java-side mirror of Python's layout logic rather than a query against a
live Python process — the class's own doc comment: "this is just a
mirror of what Python is doing, if there are changes to Python it may
require changes here" (`LibraryLocator.java:39-46`). No caching (the
full walk re-runs on every failed-`loadLibrary` startup), and no
install/self-heal step if nothing is found — it returns `false` and the
original `UnsatisfiedLinkError` propagates.

**jpype: runs a Python subprocess to ask the interpreter directly,
caches the answer, and can self-heal via `pip`.**
`Launcher.resolveLibraries()`
(`native/jpype_module/src/main/java/org/jpype/Launcher.java:417-456`)
resolves which `python` executable to target (system property →
`PYTHONHOME` env var → first `python3` on `PATH`,
`getExecutable`/`checkPath`, lines 190-253), then launches that
executable and runs a bundled probe script inside it
(`loadProbeResource`/`executeProbe`, lines 167-314): the probe reports
its own `sys.executable`, library paths, and JPype install location as
`Properties`, sourced from the live interpreter rather than
reconstructed from directory-layout conventions. That result is cached
on disk (`~/.jpype/jpype.properties` or the Windows `AppData`
equivalent) keyed by a hash of the executable path
(`saveCache`/`loadFromCache`, lines 255-264, 336-415), with a staleness
check on load that verifies the cached library paths still exist before
trusting the cache — specifically because a `pip install --upgrade` can
move the native module to a new wheel-cache path while leaving the
interpreter itself untouched (comment, `Launcher.java:397-402`). If the
probe fails outright and `jpype.install=true`, `runPipInstall()` (lines
462-504) looks for a local matching wheel first, falls back to a network
`pip install JPype1>=<version> --only-binary` otherwise, then re-probes.

**Summary**: jpy's discovery is entirely ahead-of-time (a file generated
once, that must be regenerated if the environment moves); jep's is
runtime discovery via simulating Python's own path logic in Java, which
tracks CPython's layout only as well as the simulation stays in sync
with it; jpype queries the live interpreter, caches the answer with a
validity check, and can repair a missing install rather than fail.
jep's approach is real discovery, not absent — the difference is that
jpype's is the only one of the three that both queries Python directly
and validates/self-heals its cache.

## Further out (speculative): J2NI, and what "JPype2" could mean

Speculative — a separate project (`~/javafx2/j2ni`, its own repo, own
`pom.xml`), not a jpype branch, not present in jpype's own git history
(checked every local and remote branch). Its own README states,
verbatim: `**DRAFT**` — "most piece are drafted but getting all the
definitions consistent and tested will take a while." None of this is
jpype's committed roadmap; it's included because it changes what "jpype
ahead of jpy/jep/pyjnius" would mean if it lands.

**What it is**: a proposed replacement for JNI itself, built on Java
22+'s Foreign Function & Memory API (Project Panama) — a rework of the
layer every one of jpy/jep/pyjnius, and jpype's own current codebase,
sits on top of. Concretely:

- **Metadata-driven dispatch** (signature hashing, pre-cooked method
  handles) instead of JNI's string-based method/field lookup.
- **Query-View-Pull** bulk data transfer — native code queries a "view"
  (size, element type, writability) then pulls/pushes in bursts —
  instead of JNI's pin-a-raw-pointer model.
- **Process-agnostic identifiers** (`int64_t` handles, not raw
  object-header pointers), so the same protocol works whether native and
  JVM code share a process, share memory, or are separate, possibly
  remote processes. `j2ni-remote` is a bit-packed wire protocol for this
  (`FIXME.md` shows header-layout tuning —
  `(size:11)(routing:16)(ack:1)(op:12)(checksum:24)`, with routing,
  opcodes, and checksums) — an object broker over a wire, not an
  in-process trampoline.
- `FIXME.md` mentions J2NI's own native-to-Java export surface "grows
  via the SPI extension pattern" — the same extensibility approach as
  `WrapperService`/`.pyspi` above, in a different project by the same
  author.

Everything else in this "Future" section — the reverse bridge, three
embedding layers, the SPI, subinterpreters — is jpype getting more
capable within the JNI-based architecture jpy/jep/pyjnius are also built
on. J2NI is a different kind of change: if jpype's native layer is ever
rebuilt on it, the comparison stops being about feature count and
becomes about memory-safety and cross-process/remote capability that
isn't a row on a feature-matrix table, since none of jpy/jep/pyjnius (or
jpype's own `review` branch, today) have anything comparable. Whether it
ships as part of jpype is unknown from here; this section is a marker of
direction, not a claim about outcome.
