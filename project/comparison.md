# jpype vs. jpy vs. jep vs. pyjnius: feature comparison

Scoped to the question "if we ported jpype's test suite to jpy/jep/pyjnius,
how much of it would even have something to run against" (see the
testbench-porting discussion this doc grew out of). Everything below is
grounded in reading each library's own source, not inferred from behavior
or docs: jpy's C source (`~/devel/jpy/src/main/c`) and 21-file Python test
suite, jep's C source (`~/devel/jep/src/main/c`) and 34-file/247-test Python
test suite, and pyjnius's Cython source (`~/devel/pyjnius/jnius/*.pxi`,
`reflect.py`) and 37-file/160-test Python test suite. See
`project/benchmark/RESULTS.md` for the jpype/jpy/jep/pyjnius speed
comparison.

jpy is architecturally a thin C extension with essentially no Python-side
wrapper layer. That's the source of most rows in its table below: jpype
spends effort on Python-idiomatic ergonomics and correctness breadth that
jpy's design never took on. jep sits in between: it embeds Python inside
the JVM (the reverse of jpype/jpy's architecture) and, unlike jpy, does
give Java collections real Python protocol support (`pyjlist.c`,
`pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`) and both functional-interface
duck typing and general multi-method proxy support (`pyjtype.c`'s
`functionalInterface` detection, `jep/Proxy.java` + `java_access/Proxy.c`)
— closer to jpype in scope than jpy is, though still narrower. pyjnius is
closer still in *scope* (it has real collection-protocol, `Comparable`,
functional-interface, and general-proxy support too, all confirmed working
this session -- see below), but its multi-dimensional/buffer array support
is the narrowest of the three, and its general-proxy implementation has a
real, reproduced crash bug jpy/jep don't have.

## Feature matrix: all four libraries

A single-glance summary of the pairwise sections below, which have the
full evidence/citations for every row here. "Yes"/"No" means fully
working and verified (empirically this session, where practical) or
confirmed absent from source; anything more nuanced gets a footnote.

### Language / object-model integration

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Collection protocols (`List`/`Map`/`Iterator` as native Python `list`/`dict`/iterator) | Yes | No | Yes (`pyjlist.c`/`pyjmap.c`/etc.) | Yes (`protocol_map`) |
| `Comparable`/`Iterable` duck-typing (`<`, `for x in`, `hash()`) | Yes | No | Yes | Yes (`protocol_map`) |
| `AutoCloseable` → Python context manager (`with obj:`) | Yes (`jpype/_jio.py`) | No | Yes (`pyjautocloseable.c`) | Yes (`protocol_map`) |
| Functional-interface duck typing (bare `lambda`/callable as a Java SAM arg, no proxy class needed) | Yes | No (explicit proxy object only) | Yes (`pyjtype.c`'s `functionalInterface`) | Yes (`jnius_conversion.pxi`) |
| General proxy (Python object implementing an arbitrary Java interface) | Yes, multithread-safe | Broken in this checkout\* | Yes | Yes, but with a reproduced crash bug\*\* |

\* jpy's `PyObject.createProxy()` exists but didn't produce a usable
object from Python in this checkout (see jpy section below).

\*\* pyjnius's proxy mechanism itself works for the common case, but a
Python-implemented interface method receiving a **null** `Object`
argument reliably segfaults the JVM (`jni_GetObjectClass` on a null
jobject), and even the non-null case silently returns `None` instead of
the real object -- see the dedicated bug section below. This is a defect
found in this checkout, not a design gap like jpy's row above.

### Conversion / arrays

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Class hints / custom conversions (`@JConversion`) | Yes (54 tests) | No | No | No |
| Buffer-protocol (numpy) array push, flat 1D | Yes, bulk path, value-correct across ~14 source formats (see dtype matrix below); as of this round, the dtype coercion itself runs in Java (`Support.fillFlatFromBuffer`) via a single JNI call, matching or beating jpy/jep at 100,000 elements on `long`/`float`/`double` (see `RESULTS.md` Section 4) -- previously the argument-conversion route (as opposed to the explicit `pushFrom` API) never took this fast path at all | Yes, via a separate argument-matching fast path, not `jpy.array()` itself -- **but with no dtype check: silently bit-reinterprets same-width dtype mismatches, see dtype matrix below** | Yes, genuine bulk path, but a closed 8-dtype allowlist (no `float16`) | **No, rejected unconditionally** (`"Expecting a python list/tuple"`) |
| Buffer-protocol (numpy) array push, multi-dimensional (`int[][]`+) | Yes, bulk path (this session's follow-on) | No (`TypeError`, falls back to slower per-element path) | No (`TypeError: Error matching ndarray.dtype...`) | **No** (same blanket rejection as 1D) |
| Per-class `findJavaConversion` caching w/ invalidation | Yes (this session) | N/A -- already unconditionally cheap per call | N/A -- short-circuits on arity before any per-arg work | Not applicable the same way; no equivalent caching opportunity found |

### numpy scalar argument dispatch (`Math.max(int,int)`, i.e. an overloaded method)

| numpy scalar type | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| `numpy.int32` | OK | FAILS ("ambiguous Java method call") | OK | FAILS ("no matching method") |
| `numpy.int64` | OK | FAILS (same) | OK | FAILS (same) |
| `numpy.float32` | OK | FAILS (same) | FAILS ("cannot be interpreted as an integer") | FAILS (same) |
| `numpy.float64` | OK | OK (genuine Python `float` subclass) | OK | OK (same reason) |

All four verified empirically this session (not assumed from source
alone). jpype is the only one that's fully correct across every numpy
scalar type. jep is notably *better* than jpy/pyjnius here for integers
(`int32`/`int64` both resolve correctly against `Math.max`'s overloads --
`jep_numpy.c` has an explicit numpy-scalar path jpy/pyjnius lack) but has
its *own*, different gap at `float32` specifically. jpy and pyjnius share
essentially the same failure shape (only a genuine Python `int`/`float`
passes their fast dispatch, no numpy-aware fallback for `int`/`long`
params) but fail with different, differently-worded errors.

### Buffer dtype conversion: the full matrix, and a serious jpy correctness bug

Requested explicitly this session ("check all the conversion types, not
just float8 -- jpype has a very large number"). Tested every combination
empirically, source-grounded where behavior needed explaining, not
assumed from a single example.

**jpype's real fast-path matrix is large and value-correct.** Its buffer
dispatcher (`getConverter()`, `jp_convert.cpp`) recognizes 14 source
buffer format codes -- `?`/`c`/`b` (bool/int8), `B` (uint8), `h`/`H`
(int16/uint16), `i`/`l`/`I`/`L` (int32/uint32), `q`/`Q` (int64/uint64),
`f`/`d` (float32/float64), `n`/`N` (native ssize_t/size_t) -- **and
`e`, IEEE 754 half-precision (`float16`), via a dedicated hand-written
bit-level decoder** (`Half<Convert<float>::toX>`, manually unpacks sign/
exponent/fraction including subnormals, then reuses the same
`Convert<float>::toX` machinery every other type shares) -- not a
fallback, a genuine bulk fast path, contrary to what the previous
revision of this doc assumed without checking. Tested all 12 realistic
numpy dtypes (`bool`/`int8`/`uint8`/`int16`/`uint16`/`int32`/`uint32`/
`int64`/`uint64`/`float16`/`float32`/`float64`) against all 8 Java
primitive array types directly (`JArray(JType)(arr)`): **94 of 96
combinations succeed with genuinely converted values** (confirmed
`float32([1.0,2.0,3.0]) -> int[]` gives `[1, 2, 3]`, not bit garbage).
The only 2 failures are `boolean`/`float*` → `char`, both sensible
rejections (Java has no implicit boolean-to-char or float-to-char
narrowing either). `float8_e4m3` (via `ml_dtypes`, since neither Java
nor numpy has a native 8-bit float) is the one case with no recognized
format code at all -- it still succeeds for `float`/`double` targets, but
via the general per-element fallback (`ml_dtypes` scalars support
Python's `__float__` protocol), not the buffer fast path, since `float8`
isn't in the 14-format list above.

**jpy's buffer path has no dtype/format check at all -- confirmed to
silently corrupt data for same-width dtype mismatches.** Its argument
buffer handler (`jpy_jtype.c` ~line 2026) does a raw `memcpy(arrayItems,
pyBuffer->buf, itemCount*itemSize)` after checking only that the byte
length matches -- it never inspects `pyBuffer->format`. Verified: passing
`numpy.float32([1.0, 2.0, 3.0])` where `DeepBench.sumIntArray(int[])`
expects an `int[]` returns `3217031168`, not an error and not `6` --
that's the raw IEEE-754 bit patterns of `1.0f`/`2.0f`/`3.0f`
reinterpreted as `int32` and summed (`1065353216 + 1073741824 +
1077936128 = 3217031168`, confirmed by hand). This is a **silent
correctness bug**, not a mere gap: it happens whenever the source dtype
and the target's byte width match but the dtype itself doesn't
(`float32`↔`int32`, `float64`↔`int64`) -- a plausible real mistake (wrong
dtype passed to a Java-typed API) produces a plausible-looking wrong
number with no error at all. Different-width mismatches are safely
caught (`uint8`/`float16` → `int[]` both correctly raise "no matching
Java method overloads found", since the byte-length check alone catches
those) -- it's specifically the same-width, wrong-dtype case that's
dangerous. Arguably worse than pyjnius's crash bug below: a crash is at
least loud.

**jep's numpy fast path is a safe, closed allowlist** -- confirmed
`float32` → `int[]` and `float16` → `int[]` both cleanly fail
(`"Error matching ndarray.dtype to Java primitive type"`), consistent
with `convert_pyndarray_jprimitivearray`'s exact-match check against
exactly 8 `NPY_*` constants (no `NPY_FLOAT16` in the list at all, so
`float16` is a hard, permanent gap for jep, not just untested). No
reinterpretation risk, since dtype identity is checked before any data
is touched.

**pyjnius**: rejects all numpy input unconditionally regardless of
dtype (established earlier) -- trivially safe, trivially incapable.

### Non-contiguous buffer sources: another jpy gap, confirmed by the extended benchmark suite

A numpy column slice or a transposed array is a fully valid
buffer-protocol object -- it just can't offer a C-contiguous view.
`project/benchmark/{jpype,jpy,jep,pyjnius}/array_noncontig.py` measures
what happens when one is pushed as a Java array argument, at both flat
(1D) and multi-dimensional depths, across all four primitive types.

**jpy fails outright on the 1D case, for every type and every size**,
confirmed by actually running it (not just predicted from source):
`DeepBench.sumIntArray(non_contiguous_column)` raises `RuntimeError: no
matching Java method overloads found`. Root cause (`jpy_jtype.c`,
`JType_ConvertPyArgToJObjectArg`): jpy's flat-array buffer-argument path
requests the source buffer with `flags = PyBUF_SIMPLE` -- no
`PyBUF_ND`/`PyBUF_STRIDES` -- so numpy's own `bf_getbuffer` refuses the
request against a non-contiguous array. What actually surfaces to the
caller, though, isn't a buffer error at all: jpy's overload matcher
appears to treat the failed buffer request as "argument doesn't match
any candidate," so the exception reads like an overload-resolution
problem with the *Java method signature*, not a contiguity problem with
the *input array*. That's a materially worse failure mode than a direct
`BufferError` would have been -- anyone hitting this has no clue from
the error text alone that transposing or slicing their array is the fix.
jpy's own multi-dimensional buffer argument matching is unaffected (an
`int[][]`-or-deeper target never enters this buffer branch at all,
confirmed both by source and by this run -- every transposed
multi-dimensional case pushed correctly, same cost as the contiguous
case).

jpype handles a non-contiguous source correctly at every depth, and as of
this round the flat (1D) case reaches the same single-JNI-call bulk path
as a contiguous source instead of a slower fallback -- `int[100000]`
non-contiguous column slice: 80,715ns, essentially tied with jep's own
non-contiguous number below, and *cheaper* than jpype's own contiguous
push used to be pre-fix (249,364ns). See `project/benchmark/RESULTS.md`
Section 4/8 for the fix and full numbers.

**jep handles the flat (1D) case correctly, through its real numpy fast
path** -- confirmed by actually running `project/benchmark/jep/array_noncontig.py`
against this branch's harness this round (not just cited from a prior
session): a non-contiguous column slice pushes successfully at every size
and type, landing close to jep's own contiguous `buffer->array` numbers
(`int[100000]`: 75,448ns non-contiguous vs. ~51,150ns contiguous,
`array_flat.py`, this round) -- jep's numpy fast path does not require
contiguity the way jpy's flat buffer-argument path does.
**jep has no automatic multi-dimensional numpy push at all** (established
earlier in this doc), so the ND/transposed case has no automatic path to
test contiguity-handling on either -- the benchmark instead measures the
same manual per-row assembly workaround used for jep's ordinary
multi-dimensional `buffer->array` push (see the "Features jpype has that
jep does not" table above), fed a transposed (non-contiguous) source per
row; this completed successfully at every depth (e.g. `double[][][][][]`
(10^5): 21.5M ns), consistent with jep's contiguous manual-assembly
numbers at the same depth -- expected, since each per-row leaf call is
itself a small, independently-contiguous 1D slice by construction, not a
genuinely non-contiguous bulk read.

pyjnius has no `buffer->array` push at any size or depth, contiguous or
not (established earlier in this doc) -- there is no non-contiguous case
to test for the same reason there's no contiguous one; its
`array_noncontig.py` is an intentional stub, not a benchmark.

### Fast bulk-transfer path coverage: counting genuine paths, not just presence of *a* path

The sections above establish each path individually; laid out as one
matrix, the real story is how *few* of these eight push/pull paths jpy,
jep, and pyjnius actually have as genuine bulk operations, versus how
many of them turn out to be a slower fallback dressed up to look like
one. "Genuine" here means what Sections 3-5 above verified empirically,
not what a library's API merely accepts without erroring -- a call that
succeeds via a per-element/per-row walk is a correctness pass, not a
bulk-path pass, and is marked accordingly.

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

Legend: **Y** = a real, verified bulk path. **N** = no bulk path --
rejected outright, falls back to a fully general per-element walk with
no dedicated fast-path code, or (jep's/pyjnius's `array->buffer` rows)
returns real data but by paying `array->list`'s per-element cost plus a
redundant wrapping step, landing *worse* than the library's own list-pull
number rather than faster (Section 3's `array->buffer` result). **⚠** = a
path exists and executes, but isn't bulk in the sense the others are:
jep's `list->array` rows are incomplete at multi-dim depth (its own
`float`/`double` sweeps didn't finish -- see the OOM finding below, so Y
above for jep's flat `list->array` row is provisional on `int`/`long`
only), and jep's transposed-ND `buffer->array` push is a manual,
user-written per-row Python walk, not an automatic argument-conversion
path the way every other Y in this table is (Section 4's own text: "not
a like-for-like comparison to jpype's single-JNI-call path"). \* jpy's
flat contiguous `buffer->array` push is real and bulk, but see the
"serious jpy correctness bug" above -- it's the one Y in this table that
should be read with a caveat, not the one to imitate.

Framed this way: jpype has all eight, jpy has five of eight (with one of
those five silently wrong on dtype mismatch), and jep and pyjnius each
have two of eight, genuinely bulk, no caveats. The "Other" table below
adds the non-buffer feature gaps on top of this -- this table is scoped
specifically to bulk numeric array transfer, the one category where raw
path *count*, not just per-path speed, is itself the finding.

### Other

| Feature | jpype | jpy | jep | pyjnius |
|---|---|---|---|---|
| Pickling / `copyreg` support | Yes (9 tests) | No | No | No |
| Caller-sensitive JDK method handling | Yes (20 tests) | No | No | No |
| Javadoc-derived docstrings / Jedi / typing-stub generation | Yes (~37 tests) | No | No (bare `dir()` only) | No (bare `dir()` only, confirmed `__doc__ is None`) |
| Rich JVM-finder / startup-options API | Yes | Different, narrower (`jpy.create_jvm`) | N/A -- embeds Python *inside* the JVM, reverse architecture | Auto-starts on first `autoclass()` call from a preset classpath (`jnius_config`) -- simplest of the four, but least configurable |
| Late class loading (add a jar/path to the classpath *after* the JVM is already running) | Yes -- `addClassPath()` live-injects into `org.jpype.JPypeContext`'s custom classloader via JNI, a mechanism built specifically for this | No -- `jvm_classpath` is only settable as an argument to `init_jvm()`, no post-startup API found | No -- architecture mismatch, not just a missing feature: jep doesn't start its own JVM, it's embedded inside one already launched via `java -classpath ...`; `JepConfig.setClassLoader()` supplies a pre-built `ClassLoader` to a new sub-interpreter at construction time, not a live "add this jar now" call | No, and more deliberately than jpy/jep: `add_classpath()` calls `check_vm_running()` first and raises `ValueError` if the JVM has already started -- functionally identical to `set_classpath()`, both pre-startup only |

### Test suite size

| | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| test files | 90 | 21 | 34 | 37 |
| tests | 1,884 | 151 | 247 | 160 |

Collection-protocol/Comparable/functional-interface/general-proxy support
means jep and pyjnius both have *something* to port a meaningfully larger
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
| Multi-dimensional numpy array push (`int[][]` etc. from an ndarray) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | reproduced this session via `bench_deep_jep.py`-style harness (jpy hits its own equivalent failure) |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jpy source or test suite |
| Introspection ergonomics: docstrings from Javadoc, `repr()`, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | None | no analogous test files or source in jpy |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference in jpy source |
| JVM lifecycle ergonomics (`jvmfinder`, startup options, `test_startup.py`/`test_opts.py`, 40 tests) | Rich, discoverable JVM-finding + startup-option API | Different, narrower launch API (`jpy.create_jvm`) — not a subset, a different shape | jpy has no equivalent finder/options surface |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | N/A — jpy's matching is already unconditionally cheap per call (see below), so it never needed a cache | `jpy_jtype.c`/`jpy_jmethod.c` read this session |

### Where jpy is faster *because* it does less (not a jpype gap — a jpy tradeoff)

These aren't "jpype missing a feature" rows — they're places jpy is
faster because it skips work jpype does on purpose. Recorded here for
completeness since they came up in the same source dive.

| Behavior | jpype | jpy |
|---|---|---|
| Array-argument matching | Validates every element up front (needed for correct Java-style overload disambiguation) | Does not inspect elements before committing to a conversion — confirmed by reading `jpy_jtype.c`/`jpy_jmethod.c` |
| Scalar/dispatch/proxy matching | More abstraction layers, broader general-purpose machinery (implicit numeric widening, the full `JPConversion` chain) | Leaner architecturally, but *not* found to be cutting correctness corners here — see `RESULTS.md` |

### Not compared here (excluded, no jpy equivalent to port to)

Fault-injection (`test_fault.py`, 88 tests) and coverage-instrumentation
tests (`test_coverage.py`, `test_javacoverage.py`, 50 tests) test jpype's
own internal error paths, not a portable behavior — excluded per the
original ask, not because jpy lacks the feature.

### jpy suite-size context

jpype: 1,884 tests across 90 files (~21k lines), `test/jpypetest/`.
jpy: 151 tests across 21 Python test files, `~/devel/jpy/src/test/python/`.

Of jpype's suite, roughly 250+ tests exercise features with literally no
jpy counterpart (the first table above) — those can't be "ported," only
noted as gaps. The remainder (conversion, arrays, strings, exceptions,
fields/properties, overloads/varargs, reflect, jclass/jpackage/imports,
numeric/boxing, buffers, inherit, hash, synchronized) is the realistic
portable subset if this comparison is ever turned into an actual ported
test run.

### Five jpy misfeatures: things that look like features but aren't

Recorded separately from the tables above because these aren't gaps or
tradeoffs — they're jpy behaviors that present as a capability while
actually being broken or unstable, confirmed by reading jpy's own source.

**"Restarting the interpreter" is faked by a flag that skips `Py_Finalize`
entirely.** `PyLib.stopPython()`'s own javadoc
(`~/devel/jpy/src/main/java/org/jpy/PyLib.java:243-259`) states plainly
that stopping the interpreter again after a restart "currently causes a
fatal error in the Java Runtime Environment" and links jpy's own
[issue #70](https://github.com/bcdev/jpy/issues/70) — there is no working
restart. `STOP_IS_NO_OP` (`PyLib.java:57`,
`Boolean.getBoolean("jpy.stopIsNoOp") || ON_WINDOWS`) makes `stopPython()`
skip `Py_Finalize` altogether when set, so "stop" becomes a no-op that
leaves the interpreter fully alive — no module teardown, no `sys.modules`
clear, nothing reclaimed. jpy's own `setup.py:226-232` (`test_maven`) sets
`-Djpy.stopIsNoOp=true` for every Maven test run specifically because
"multiple start/stop cycles in the same JVM crash CPython." The one test
that would exercise a real stop/start/stop cycle,
`LifeCycleTest.testCanStartAndStopWithoutException`, self-skips under
exactly that flag (`Assume.assumeFalse(..., "jpy.stopIsNoOp")`) —
i.e. it never runs under the config the suite always uses. The API shape
(call `stop`, call `start` again) still typechecks; the semantics ("the
interpreter restarted") are false. jpype doesn't claim this either, and
doesn't need the equivalent of a no-op flag to avoid the crash: it exposes
real PEP 684 subinterpreters (`Py_NewInterpreterFromConfig`/
`Py_EndInterpreter`, `native/common/jp_bridge.cpp:454-560`) as
independently disposable instances (`org.jpype.SubInterpreter`) instead of
pretending the single root interpreter can be torn down and revived.

**Boxed-type selection for a generic `Object`/`Number` argument is
magnitude-dependent, not type-stable.** `JType_CreateJavaNumberFromPythonInt`
(`~/devel/jpy/src/main/c/jpy_jtype.c:542-563`) picks `Byte`/`Short`/
`Integer`/`Long` by testing whether the Python int's value survives a
narrowing cast (`b`/`s`/`i` vs. `j`), not from anything about the
argument's Python type or the target parameter's declared type. The same
Python `int` literal boxes to a different Java runtime class depending
purely on its magnitude at that call: `foo(5)` boxes as `Byte`,
`foo(5000)` as `Short`, `foo(5_000_000)` as `Integer`,
`foo(5_000_000_000)` as `Long`. Since Java-side overload resolution and
`instanceof` both dispatch on the boxed object's runtime class, this means
`someMethod(x)`'s effective behavior on the Java side can change based on
how large `x` happens to be at a given call, for arguments that are all
equally plain Python `int`s — a "works until the value grows past the
next boundary" trap, not a documented or predictable conversion contract.
jpype's equivalent path, `JPConversionBoxLong::convert`
(`native/common/jp_classhints.cpp:1662-1697`), boxes a plain Python `int`
to `java.lang.Long` unconditionally — fixed and size-independent. The one
place jpype *does* vary the box class is for numpy scalar types
(`numpy.int32` → `Integer`, `numpy.int16` → `Short`, etc., same function,
just below), and that's driven by the input's actual dtype identity, not
its magnitude — a type-preserving choice, not a magnitude heuristic, and
not the same kind of trap.

**Every Java exception collapses into a single, generic Python
`RuntimeError`, with no real cause-chaining.** `JPy_HandleJavaException`
(`~/devel/jpy/src/main/c/jpy_module.c:1244-1420`) is jpy's only
Java-to-Python exception path, and it always ends in
`PyErr_Format(PyExc_RuntimeError, ...)` (lines 1398/1411) — regardless of
whether the underlying Java exception was a `NullPointerException`, an
`IllegalArgumentException`, or an application-defined checked exception.
`getCause()` is walked (line 1394) only to splice `"caused by "` text into
that one flat message (lines 1259-1284); there is no `__cause__`, no
`__context__`, and no distinct Python exception object per cause — just
string concatenation. The whole stack-trace walk that produces a useful
message only runs at all when `JPy_VerboseExceptions` is set (line 1255);
otherwise the message is bare `error.toString()`. Net effect: Python code
calling into Java through jpy can never write `except SomeSpecificException`
— every failure looks identical (`RuntimeError`) to the caller, and how
much detail even ends up in the message text depends on a debug flag.
jpype's `JException` (`jpype/_jexception.py`, `@JImplementationFor
("java.lang.Throwable", base=True)`) instead maps each real Java exception
class onto its own Python exception type, mirroring the actual Java
`Throwable` hierarchy — `except java.lang.NullPointerException` and
`except java.lang.IllegalArgumentException` are genuinely distinguishable,
and `getCause()`/`getMessage()`/`printStackTrace()` stay available as real
methods on the exception object rather than being pre-flattened into a
string.

**Threads that call from Python into Java are attached to the JVM as
non-daemon, and never detached.** `JPy_GetJNIEnv` (`jpy_module.c:267-298`)
is jpy's one auto-attach path: on `JNI_EDETACHED` it calls plain
`AttachCurrentThread` (line 281) — not `AttachCurrentThreadAsDaemon` —
whenever a Python thread first calls into Java. There is no
`DetachCurrentThread` call anywhere in jpy's C source (confirmed by grep
across `src/main/c/*.c`) or its Java source. Two compounding problems from
one omission: (1) every Python thread that ever calls a Java method
leaks a JVM-side thread registration for its entire life, with no jpy API
to release it, and (2) because the attach is non-daemon, `DestroyJavaVM`
(JNI-mandated to block until all non-daemon threads exit) will hang
waiting on any such thread that's still alive but idle — a Python thread
that made one Java call ten minutes ago and is now just sitting in a
`time.sleep()` can still block JVM shutdown indefinitely, for a reason
completely invisible from the call site that triggered the attach. jpype
auto-attaches too, but deliberately as a daemon and with the leak named
in its own public API: `JPContext::getEnv()`
(`native/common/jp_context.cpp:870-894`) attaches via
`AttachCurrentThreadAsDaemon` specifically "so that the newly attached
thread does not deadlock the shutdown" (its own comment, line 885-886),
and `java.lang.Thread.isAttached()`/`.attach()`/`.attachAsDaemon()`/
`.detach()` (`jpype/_jthread.py:22-84`) are exposed precisely so
long-running threads can detach and avoid the leak jpy has unconditionally
and silently.

**"Restarting"'s JVM-side counterpart has the identical asymmetry as the
interpreter-restart misfeature above, just on the other embedding
direction.** `JPy_destroy_jvm` (`jpy_module.c:510-521`) calls
`DestroyJavaVM()` with no `isRunning()`/shutting-down guard anywhere in
jpy's proxy-invocation path — nothing checks whether a Java daemon thread
is mid-callback into a Python-implemented proxy before or after the call.
jpy's only shutdown-race guard at all is `Py_IsFinalizing()`
(`org_jpy_PyLib.c:57-86`), checked at the top of every native entry point
for the *opposite* direction (a Java thread calling into Python while
Python is finalizing) — and even that guard's own comment admits it
"doesn't completely prevent the race condition (TOCTOU), but... mitigates
the risk significantly." So jpy has a partial, self-acknowledged-racy
guard for one direction and nothing at all for the other. jpype's model
(`[[jvm_shutdown_daemon_thread_safety]]`) relies on `DestroyJavaVM`'s own
JNI-mandated block on non-daemon threads for the general case, documented
explicitly in `jp_context.cpp:97-101` ("VM_Exit parks all remaining
daemon threads at the final safepoint; nothing executes Java code after
DestroyJavaVM returns"), plus one targeted, non-racy check
(`jp_proxy.cpp:161`, `context->isRunning() || ...is_shutting_down`) for
the one gap that guarantee doesn't cover — a daemon-thread proxy callback
still parked when shutdown completes underneath it.

**Credit where it's due, so this section doesn't read as "jpy is bad at
everything":** two things jpy actually gets right are properties of its
Java-side/threading engineering, not its C-side conversion logic (where
all three misfeatures above live) — and jpype already has equivalent, or
architecturally stronger, coverage of both. Not gaps jpy fills that jpype
lacks; independent-design parity, worth recording as such rather than
silently, since the previous revision of this doc almost filed them as
open gaps before checking.

| Concern | jpy's approach | jpype's approach | Evidence |
|---|---|---|---|
| GIL acquisition at native entry points, called from arbitrary (Java) threads | `PyGILState_Ensure`/`Release` around every JNI entry point, ~35 call sites; explicitly rejects a Python call made mid-interpreter-shutdown rather than racing it | Same primitive, same discipline: `PyGILState_Ensure`/`Release` around Python calls from Java threads, including explicit handling of the reentrant case (`PyGILState_LOCKED` → release is a documented no-op) and `PyGILState_Check()` used specifically because it stays reliable across subinterpreters, not just the main interpreter | jpy: `~/devel/jpy/src/main/c/jni/org_jpy_PyLib.c:65-86` and ~35 call sites (e.g. 284, 407, 501, 881). jpype: `native/common/jp_bridge.cpp:280-390,601-624`, `native/python/jp_pythontypes.cpp:407-476` |
| Preventing a native pointer from being reclaimed/reused while a JNI call using it is still in flight | Reads the native pointer off the *live* wrapper object, then relies on `Reference.reachabilityFence(this)` (`PyObject.java:33-40`) as an explicit, manually-placed guard against the JIT deciding the wrapper is dead early and letting GC collect it mid-call | Never reads the pointer off a live object at cleanup time at all: `org.jpype.ref.NativeReference` is a `PhantomReference` that copies the native handle onto *itself* at construction (`hostReference` field); a `PhantomReference` is only enqueued once the JVM has already proven the referent unreachable, so there's no live-object race to fence against in the first place | jpy: `~/devel/jpy/src/main/java/org/jpy/PyObject.java:33-40`. jpype: `native/jpype_module/src/main/java/org/jpype/ref/NativeReference.java:62-108` |

Same outcome (no use-after-free of the native handle, no unsafe call
during shutdown), reached two different ways: jpy patches a hazard its
design creates; jpype's design doesn't create that hazard shape to begin
with. Neither row is a jpy misfeature — they're included so the "five
misfeatures" above don't get read as "jpy's engineering is uniformly
worse," which the C-side evidence alone would wrongly imply. (The thread-
attachment misfeature above is the one place this section's "credit"
doesn't extend to jpy's threading engineering generally — GIL discipline
and the reachability fence are sound; the never-detach non-daemon attach
is a separate, real defect in the same subsystem.)

## jpype vs. jep

### Features jpype has that jep does not

| Feature | jpype | jep | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | no hints/customizer machinery found in `jep/src/main/c` |
| Multi-dimensional numpy array *push* (Python ndarray → `int[][]` etc. as a method argument) | Bulk buffer path, see `caching-multidim-push` branch | Not supported — raises `TypeError: Error matching ndarray.dtype to Java primitive type` for any ndim > 1 | reproduced this session; `jep_numpy.c`'s only `PyArray_NDIM` use (line 399) is on the opposite direction (Java array → Python ndarray return value), confirming the push path genuinely has no multi-dim handling, not just an untested one |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | not present in jep source or test suite |
| Javadoc-derived docstrings, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_module.py`, `test_module2.py` | Only bare `dir()` listing method names (`test_dir.py`) — no docstrings, no stub generation | `pyjobject`/`pyjtype` expose method names via `dir()` but no docstring text sourced from Javadoc |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in jep source |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | Not applicable the same way — jep's overload resolution already short-circuits on arity before doing any per-argument type work (see below) | `pyjmultimethod.c:118-155`, `pyjmethod.c:284`, read this session |

### jep, past the feature table: the same threading/lifecycle lens applied

The table above and the earlier feature matrices came from a
features/speed angle. Applying the same misfeature-hunting lens used on
jpy (thread attachment, GIL discipline, sub-interpreter shutdown,
exception fidelity, boxed-type selection, JVM-destroy-vs-daemon-thread
guarding, native-object lifetime safety) to jep's own source
(`~/devel/jep/src/main/c/Jep/*.c`, `src/main/java/jep/**/*.java`) turns
up a mixed picture -- three real gaps, several places jep's engineering is
at least as careful as jpype's, and one architectural choice that's
neither library's approach.

**Java-to-Python exceptions collapse into one generic type,
`JepException` — the same misfeature class as jpy's `RuntimeError`
collapse.** `process_py_exception` (`src/main/c/Jep/jep_exceptions.c:42-175`)
is jep's only Python-to-Java exception path, and every Python exception
— regardless of its real type — becomes a `JepException(String, long)`
built from `"ExcType: message"` string concatenation (lines 148-163),
not a distinct Java exception class per Python exception type. `catch
SomeSpecificPythonException` is as impossible through jep as it is
through jpy. One partial mitigation jpy lacks: if the Python exception
is itself wrapping a Java exception that crossed into Python and back
out (a `PyJObject`-backed exception), jep does preserve that as a real
`Throwable` cause via a second constructor, `JepException(String,
Throwable)` (lines 162-167) — genuine cause-chaining, but only for that
round-trip case, not for exceptions that originate natively in Python.
jpype's per-class `JException` mapping (`jpype/_jexception.py`) still
has no counterpart in jep for either case.

**No shutdown guard on the proxy-invocation path — matches jpy's gap,
not jpype's protection.** `jep.python.InvocationHandler.invoke()`
(`src/main/java/jep/python/InvocationHandler.java:132-141`) calls
straight into native code with no liveness check first, and the native
side, `Java_jep_python_InvocationHandler_invoke`
(`src/main/c/Jep/python/invocationhandler.c`), has no
`Py_IsFinalizing()`/interpreter-liveness check anywhere in the file. A
Java thread mid-callback into a Python-implemented proxy (`jep.jproxy()`)
when the interpreter is closing has nothing stopping it — no equivalent
of jpype's `jp_proxy.cpp:161` `isRunning()`/`is_shutting_down` guard, and
not even jpy's TOCTOU-racy `Py_IsFinalizing()` check on the *other*
direction. This is the one place jep is flatly behind both.

**No smuggler guard: jep has nothing checking that a `PyObject` is being
touched by the interpreter that actually created it.** jpype's own guard
for this (`JPClass::convertToPythonObject`, `native/common/jp_class.cpp:379-403`)
exists precisely because own-GIL subinterpreters have separate
allocators/arenas — handing one interpreter's `PyObject*` back into
another's Python code is memory corruption, not just a wrong answer, so
`proxy->m_Context != context` is checked explicitly and raises a clean
`RuntimeError` ("Python object crossed into a different interpreter than
the one that created it (smuggled proxy)") rather than touching the
pointer. jep has the identical hazard — `jep.python.PyObject`'s own
javadoc names the constraint outright: *"This class is not thread safe
and PyObjects can only be used on the Thread where they were created.
When an Interpreter instance is closed all PyObjects from that instance
will be invalid"* (`src/main/java/jep/python/PyObject.java:36-38`) — but
that's a documented caller contract, not an enforced one. Tracing the
actual call path: `PyObject.tstate()` calls
`MemoryManager.getThreadState()` → `getThreadLocalJep()`
(`src/main/java/jep/python/MemoryManager.java:124-134`), which looks up
*whatever* `Jep` instance is bound to the *calling* thread via a plain
`ThreadLocal<Jep>` and throws only if none is bound at all
("`Invalid thread access.`") — it never checks that this specific
`PyObject`'s originating interpreter is the one now resolved. That
`tstate` (the calling thread's interpreter) is then passed straight into
native code together with the object's raw pointer
(`Java_jep_python_PyObject_getAttr` and five sibling functions,
`src/main/c/Jep/python/jep_object.c:36-278`, each doing
`jepThread = (JepThread*) tstate; PyEval_AcquireThread(jepThread->tstate); ...`
against `pyobj` with no ownership check at all). No thread-crossing is
even required to hit this: one thread opens interpreter A, creates a
proxy/`PyObject`, stores it in an ordinary `java.util.List` (plain Java
state, outside any interpreter's scope), and returns; the same thread
later opens interpreter B and something reads the stashed object back
off the list and touches it — `getThreadLocalJep()` resolves *B* (the
one now bound to this thread), against *A*'s raw pointer. The hazard is
general: any Java-side storage of a `PyObject`/proxy outside the
interpreter call that produced it is a potential smuggling site, not
just a deliberately cross-thread one.

Tried to reproduce this live, same-thread, two ways, rather than leave
it as a source-only claim:

1. `SharedInterpreter` A creates an object, is never closed (a realistic
   forget-to-close leak), and a second `SharedInterpreter` B opens on the
   same thread — blocked outright: opening B while A is still open throws
   `JepException: "Unsafe reuse of thread main for another Python
   Interpreter. Please close() the previous Interpreter to ensure
   stability"` (a real, if incidental, guard — not the principled
   ownership check jpype has, but it does close this specific door).
2. Closed A first (with a second, unrelated `SharedInterpreter` kept
   alive on another thread so `MemoryManager.closeInterpreter`'s
   `interpreterSet.size() == 1` branch — which synchronously disposes
   every tracked pointer when true — does *not* fire), then opened B on
   the freed thread and touched A's stashed object. No crash: it returned
   the correct-looking `<class '__main__.Foo'>`. Root cause of the
   non-crash, checked afterward: `SharedInterpreter` instances aren't
   separate own-GIL subinterpreters at the CPython level at all — its own
   javadoc says instances "share all imported modules" and only keep
   *globals* distinct, so there's no real separate interpreter/arena
   boundary for a `SharedInterpreter`-only reproduction to actually
   violate. A genuine own-GIL `SubInterpreter`-vs-`SubInterpreter` version
   of this same experiment wasn't reproduced this session (each plain
   `new SubInterpreter()` gets its own private `MemoryManager`, so its
   objects always resolve back through their own dedicated `ThreadLocal`
   regardless of what else is active on the thread — the cross-manager
   mixup this guard-gap would need requires either `attach()` sharing,
   which is explicitly documented as safe to share objects across, or
   some less obvious path not yet found).

Net honest result: the structural gap is real and confirmed by
source — there is no `proxy->m_Context != context`-style identity check
anywhere in jep, unlike jpype's explicit one. But jep's *other*,
incidental protections (the same-thread-reuse rejection, and synchronous
pointer disposal when an interpreter's own manager is solely used) close
off the two concrete repro paths tried this session — a live crash was
not produced. That's a weaker finding than "reproduced," but the
underlying point stands regardless of whether the specific repro
succeeds: the safety here comes from a patchwork of situational checks
jep happens to have for other reasons, not from a principled per-object
ownership check the way jpype's is — so *any* Java-side stash of a
`PyObject`/proxy outside its producing interpreter's own scope remains
an unguarded hazard in general, even where the two paths tried this
session didn't happen to trigger it.

**Why repro attempt 2 came back clean, stated plainly so it isn't
mistaken for "jep is safe here": `SharedInterpreter` isn't a real
subinterpreter at all.** Checked directly: opening one does
`globals = PyDict_New(); PyDict_SetItemString(globals, "__builtins__",
...)` (`src/main/c/Jep/pyembed.c:766-768`) — a fresh Python dict, not
`Py_NewInterpreter`/`Py_NewInterpreterFromConfig` anywhere in that path.
Every `SharedInterpreter` instance runs in the *same* single underlying
CPython interpreter, one GIL, one `sys.modules`, and only gets its own
`globals` dict — architecturally the same thing as jpype's `Script`
(`org.jpype.Script`, "a scope of variables in the Python interpreter...
housed in Java space"), not the same thing as `jep.SubInterpreter` or
jpype's own `SubInterpreter` (both real, `Py_NewInterpreter`-backed
isolation). `Script`'s reason for existing is exactly this: a Python
*module* is itself nothing more than a `dict` of globals bound to a
name — `sys.modules['foo'].__dict__` — and a single interpreter already
supports as many of those as you want, no isolation machinery required.
`Script` is jpype giving that existing, cheap, arbitrarily-repeatable
concept (one interpreter, N independent globals dicts, i.e. N modules)
a first-class Java-side handle, rather than inventing a new isolation
primitive for something the interpreter already does for free.
`SharedInterpreter` is jep doing the same thing, just not naming it that
way. So repro attempt 2's clean result doesn't show jep's missing
ownership check is harmless — it shows that test picked the one jep
class where there was never a real interpreter boundary to smuggle
across in the first place. `jep.SubInterpreter` — genuinely own-GIL
isolated, confirmed earlier in this section — goes through the identical
unchecked `tstate()`/`getThreadLocalJep()` path and was *not* the one
tested; a `SubInterpreter`-vs-`SubInterpreter` repro is the one that
would actually exercise this gap for real, and remains untried.
Separately worth a features-table row on its own merits regardless of
the smuggling question: jep gives this shared-globals-one-interpreter
pattern its own class with its own name in a family that otherwise means
"isolated interpreter" (`SubInterpreter`/`SharedInterpreter` share the
`Interpreter` API), where jpype keeps the always-cheap, never-isolated
version (`Script`) visibly separate from the opt-in real-isolation one
(`SubInterpreter`) rather than naming them as siblings — a real
usability/naming difference, not just an implementation-detail one.

**Credit due — three places jep's engineering matches or exceeds jpy's,
worth recording so the two gaps above don't read as "jep is jpy with a
different name":**

| Concern | jep's approach | Verdict | Evidence |
|---|---|---|---|
| Thread attachment for calls crossing into Java | `AttachCurrentThreadAsDaemon`, with a comment explicitly reasoning through why: "there are no hooks to detach the thread later[, so] daemon is the only way to let the process exit normally" | Correct and *deliberate* — jep's authors clearly understood the exact hazard jpy's plain-`AttachCurrentThread`-without-detach bug creates, and designed around it up front rather than patching it after the fact | `src/main/c/Jep/pyembed.c:817-834` |
| Boxed numeric type selection for a generic Java target | `pylong_as_jobject` dispatches on the *declared/expected* Java type via `IsAssignableFrom` checks (Long → Integer → Byte → Short → BigInteger fallback on overflow), never on the Python value's magnitude | Type-stable, matching jpype's fixed-boxing approach, not jpy's magnitude-dependent bug | `src/main/c/Jep/convert_p2j.c:317-368` |
| Sub-interpreter shutdown | `pyembed_thread_close` calls a genuine `Py_EndInterpreter(jepThread->tstate)` when closing a non-main interpreter thread — no `stopIsNoOp`-style flag anywhere in the source | Real, not faked — jep's multi-`Jep`-instance isolation claim holds up architecturally, the same honest shape as jpype's `SubInterpreter.close()` | `src/main/c/Jep/pyembed.c:794-805` |

**A third, distinct object-lifetime model — not jpy's hazard, not
jpype's phantom-reference safety net, its own tradeoff.**
`jep.python.PyObject` (Java) has no `PhantomReference`/`Cleaner`/
`reachabilityFence` anywhere — cleanup is exclusively manual, via
explicit `close()`; its own javadoc states a `PyObject` becomes invalid
once its owning interpreter closes. This sidesteps jpy's live-pointer
race entirely (there's no GC-triggered decref racing a JNI call, because
there's no GC-triggered decref at all), but trades it for a pure
manual-lifetime contract: forget to call `close()`, and the native
object simply leaks until its whole sub-interpreter tears down. Neither
a bug fixed with a fence (jpy) nor a bug that structurally can't happen
(jpype) — a third, stricter-but-leak-prone design, worth naming as its
own category rather than forcing it into "matches jpy" or "matches
jpype."

**GIL discipline uses a different, coherent mechanism — no bug found, but
less defensive than jpy's.** jep acquires/releases via
`PyEval_AcquireThread(jepThread->tstate)`/`PyEval_ReleaseThread` against
a per-`JepThread`-cached `PyThreadState` (12+ call sites in
`pyembed.c`), not the `PyGILState_*` TLS API jpy and jpype both use — a
architectural choice consistent with jep predating PEP 684's cleaner
sub-interpreter story. No shutdown-race guard comparable to jpy's
`Py_IsFinalizing()` check (racy as it is) was found anywhere in this
path — not confirmed as a live bug, just less defended than either
jpy's checked-but-racy approach or jpype's structural one.

**Adversarial concurrency, checked against jep's own test suite rather
than assumed: it holds up, but for a narrower claim than jpype/jpy's.**
Built jep's existing native lib (`build/lib.linux-x86_64-cpython-312`,
already exports the `Java_jep_*` JNI symbols — the same `.so` serves both
embedding directions in this jep version) and ran its two adversarial
multithreading tests directly rather than reading them and assuming:
`jep.test.synchronization.TestCrossLangSync` (16 Python-sub-interpreter
threads + 16 Java threads, all hammering one shared lock/`AtomicInteger`
via `obj.synchronized()`) and `jep.test.TestSharedModulesThreads` (16
threads concurrently creating `SubInterpreter`s and importing the same
shared module). Both exited 0, clean, this session. But neither is the
same claim `GilConcurrencyParityNGTest`
(`native/jpype_module/src/test/java/org/jpype/GilConcurrencyParityNGTest.java`)
or jpy's `MultiThreadedEvalTestFixture` test: **jep has no way to construct
the scenario those two check** — N uncoordinated threads mutating *one
shared interpreter's globals* with no explicit lock, relying entirely on
the automatic per-call GIL guard for correctness. `SharedInterpreter`'s
own javadoc states it plainly: "each `SharedInterpreter` still maintains
distinct global variables" even though modules are shared, and mixing
`Interpreter` instances on the same thread at the same time is
unsupported outright (`SharedInterpreter.java:34-44`). `MainInterpreter`
looks like it could be that shared interpreter but isn't one in the
usable sense — its own javadoc frames it purely as GIL-deadlock-avoidance
bootstrap machinery ("the main Python interpreter that all
sub-interpreters will be created from... used to avoid potential
deadlocks", `MainInterpreter.java`), not something application code
executes against directly. So jep's concurrency safety comes from
architecturally not sharing mutable interpreter state across threads,
rather than from an automatic guard proven safe under shared mutable
state the way jpype's now is (`[[jvm_shutdown_daemon_thread_safety]]`-
adjacent territory, same session that produced the `PyCallable.call()`→
`invoker()` fix). Narrower guarantee, but a real one, and its own tests
for it pass.

**From the benchmark side, not the source dive — two more real, empirical
findings folded in here rather than left only in `RESULTS.md`:**

- **A reproducible `OutOfMemoryError` in jep's own multi-dimensional
  array benchmark**, `array_multidim.py`, partway through its combined
  `int`/`long`/`float`/`double` sweep, even at `-Xmx3g` — raising the
  heap to 4GB then 6GB only postpones it. It's specific to that one
  long-running combined-sweep process; the narrower single-scenario jep
  scripts (`array_ragged.py`, `array_noncontig.py`, `array_shape.py`)
  complete cleanly at the same depths/sizes under the stock default
  heap, pointing at an allocation-rate-vs-GC-throughput problem rather
  than a fixed working-set size (`RESULTS.md` Section 10). Flagged as
  real and reproducible, not root-caused at the source level in this
  pass.
- **jep has no genuine `array->buffer` (Java array → numpy) return path
  at any depth** — its numbers are `array->list`'s per-element cost plus
  a redundant `np.asarray()` wrap, not a bulk buffer read, which is why
  jep lands *worse* than its own list-pull number instead of better:
  **~180x slower than jpype/jpy at `long[100000]`** (10.7M ns vs.
  111-116K ns, `RESULTS.md` Section 3). Same architectural gap as
  pyjnius; see the fast-bulk-path-coverage table earlier in this doc for
  the full accounting across all eight push/pull paths.

### Features jep has that jpy lacks (closer to jpype here)

Noted because it changes the porting-effort picture from the jpy table
above — these jpype-suite categories that were "no counterpart, don't
bother" for jpy actually have something to port to for jep.

| Feature | jep | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterable` | Real `__getitem__`/`__setitem__`/slicing/iteration, backed by the actual Java collection | `pyjlist.c`, `pyjmap.c`, `pyjcollection.c`, `pyjiterable.c`, `pyjiterator.c` |
| `Comparable`/hashing duck-typing | `tp_richcompare` delegates to `Comparable.compareTo`, `tp_hash` to `hashCode` | `pyjobject.c:155-181`, `pyjobject.c:310-312` |
| Functional-interface duck typing (pass a Python callable directly as a Java SAM interface arg) | Detected via `isInterface` + single-abstract-method check | `pyjtype.c:259-335` (`functionalInterface`) |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | Full `InvocationHandler`-style proxy, own test file | `jep/Proxy.java`, `java_access/Proxy.c`, `test_jproxy.py` |
| `synchronized` block support | `pyjmonitor.c`, `test_synchronized.py` | jpy has no equivalent test or source |

### Where jep is faster *because* it does less (not a jpype gap — a jep tradeoff)

Verified this session by reading `pyjmultimethod.c`/`pyjmethod.c`, not
inferred from timing. jep's overload resolution filters candidates by
**parameter count only** (O(1), no type inspection); the expensive
per-argument type-compatibility check (`PyJMethod_CheckArguments`) only
runs when two or more candidates share the same arity. For a method with a
single overload (the common case, and the case for every array/scalar
benchmark in `RESULTS.md`), jep converts arguments directly against the one
known parameter type in a single pass. jpype always does two passes — a
full `matches()` scoring scan, then a separate `convert()` — even with no
overload ambiguity, because its architecture doesn't special-case "only one
candidate." This is a genuine skip, not a leaner-but-equivalent
implementation: it's also *why* jep falls badly behind on the 16-overload
dispatch benchmark in `RESULTS.md` (all candidates share arity there, so
the shortcut can't fire and the expensive check runs repeatedly).

### Not compared here (excluded, no jep equivalent to port to)

Same exclusion as jpy: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
test jpype's own internals, not portable behavior.

### jep suite-size context

jep: 34 Python test files, 247 tests, `~/devel/jep/src/test/python/`.
Larger than jpy's suite, still under a fifth of jpype's 1,884. The
collection-protocol, Comparable, functional-interface, and proxy support
above mean a meaningfully larger fraction of jpype's suite has *something*
to port against for jep than for jpy — but `test_classhints.py`/
`test_hints.py`/`test_customizer.py`, `test_pickle.py`/`test_serial.py`,
and the introspection-ergonomics files remain gaps for jep too.

## jpype vs. pyjnius

### Features jpype has that pyjnius does not

| Feature | jpype | pyjnius | Evidence |
|---|---|---|---|
| Class hints / custom conversions (`@JConversion`, `JConversionCustomizer`) | Full registration system (54 tests) | No equivalent subsystem | grep of `jnius/*.pxi`/`reflect.py` for hints/customizer/register-conversion machinery: nothing |
| Multi-dimensional / buffer-protocol array push (`int[]`/`int[][]` etc. from an ndarray) | Bulk buffer path (`caching-multidim-push` branch), any dimension | Not supported at **any** dimension, including flat 1D — narrower than both jpy and jep, which at least accept a 1D buffer object | reproduced this session: `DeepBench.sumIntArray(numpy.arange(...))` raises `JavaException('Expecting a python list/tuple, got array(...)')` unconditionally, for any ndim |
| Pickling / `copyreg` support for Java objects | `test_pickle.py`, `test_serial.py` (9 tests) | No equivalent | no `__reduce__`/pickle-registration logic in pyjnius's own source (the one `pickle` hit in the built `jnius.c` is Cython's own generated module boilerplate, not a feature) |
| Introspection ergonomics: docstrings from Javadoc, Jedi/IDE completion, module/typing-stub generation | `test_docstring.py`, `test_jedi.py`, `test_repr.py`, `test_module.py`, `test_module2.py` (~37 tests) | Only bare `dir()` listing (method names visible, `__doc__` is `None` for every bound method, confirmed this session) — same situation as jep | no docstring-generation code found in `jnius/*.pxi`/`reflect.py` |
| Caller-sensitive JDK method handling | `test_caller_sensitive.py` (20 tests) | Not handled as a distinct case | no reference found in pyjnius source |
| Per-class conversion caching with generation-based invalidation | Yes (this session's `caching` branch) | Not applicable the same way — no equivalent per-call matching-cost problem was found to cache in the first place (see numpy-scalar-dispatch note below, which is a coverage gap, not a caching opportunity) | `jnius_conversion.pxi` read this session |

### Features pyjnius has that jpy lacks (closer to jpype here, comparable to jep)

Confirmed working empirically this session, not just found in source:

| Feature | pyjnius | Evidence |
|---|---|---|
| Python collection protocols on `java.util.List`/`Map`/`Collection`/`Iterator`/`Map.Entry` | Real `__getitem__`/`__setitem__`/`__len__`/`__contains__`/`__iter__`, backed by the actual Java collection | `reflect.py`'s `protocol_map`, applied automatically inside `autoclass()` to every class whose hierarchy includes one of these interfaces; verified an `ArrayList`/`HashMap` support `len()`, indexing, iteration, and `in` directly |
| `Comparable`/`Iterable` duck-typing | `__lt__`/`__gt__`/`__eq__`/etc. delegate to `compareTo`/`equals`; `__iter__` delegates to `iterator()` | same `protocol_map`; verified `Integer(3) < Integer(5)` works directly |
| `AutoCloseable`/`Closeable` → Python context manager protocol | `__enter__`/`__exit__` delegate to `close()` | same `protocol_map` (jpype has this too, `jpype/_jio.py` -- not a jpype gap, just noting pyjnius has it as well, unlike the jpy/jep tables above which didn't need to call it out) |
| Functional-interface duck typing (pass a Python `lambda`/callable directly as a Java SAM interface arg) | Supported | `jnius_conversion.pxi`'s functional-interface detection (~line 415-451); verified `DeepBench.invokeCallback(lambda x: x + 1, 5)` works directly, no proxy class needed |
| General multi-method proxy (Python object implementing an arbitrary Java interface) | `PythonJavaClass` subclass + `@java_method('<jni-signature>')`, own test file (`test_proxy.py`) | present and works for the common case (verified `int`-arg callback) -- **but see the bug below**, unlike jep's proxy support, which has no equivalent defect found |

### A real, reproduced defect in pyjnius's proxy implementation (not a feature gap — a bug)

Unlike jpy's proxy gap (a construction-time failure, "no matching Java
method overloads found" — see the jpy table above) and jep's proxy support
(no defect found), pyjnius's general proxy mechanism has a genuine crash
bug: a Python-implemented Java interface method receiving a genuinely
**null** `Object` argument (`DeepBench.invokeObjectCallbackWithNull` —
`jpype.benchmark.DeepBench`'s `ObjectCallback` methods exist specifically
to cover this case, per `jp_proxy.cpp`'s own history) reliably segfaults
the JVM with a native `SIGSEGV` in `jni_GetObjectClass`. Reproduced
independently three times against a fresh build in a disposable venv
(ruled out as stale-build-state first, per this repo's CLAUDE.md, before
treating it as real). pyjnius's proxy-argument-marshalling code calls
`GetObjectClass`/`IsSameObject` on the argument without a null check first
— undefined behavior over JNI, not a usage error on this session's part.

Even the non-crashing case (a real, non-null `Object` argument) doesn't
work correctly: `invokeObjectCallback` silently returns `None` instead of
the object the Python callback handed back — a separate, non-fatal
correctness bug in the return-value path. See
`project/benchmark/RESULTS.md`'s pyjnius section and
`project/benchmark/pyjnius/proxy.py` for the full detail; that script
deliberately never calls the null-argument variant.

### Where pyjnius is faster/slower *because* of its own tradeoffs

| Behavior | jpype | pyjnius |
|---|---|---|
| numpy scalar dispatch for `int`/`long` parameters | Resolves all numpy scalar types (`int32`/`int64`/`float32`/`float64`) correctly and unambiguously | Same underlying gap as jpy (no fallback for non-exact-Python-type numeric args on `int`/`long` params), but a cleaner failure mode: `Math.max(np.int32(3), 5)` raises `"No static methods called max in java/lang/Math matching your arguments... available: [...]"` — a plain "no match," not jpy's "ambiguous Java method call" |
| Scalar/boxed/string/proxy call overhead generally | See `RESULTS.md` | Slowest of the four libraries on every boxed/string/proxy row measured this session, sometimes by 5-10x (`new Integer`: 7403ns vs. 840-1257ns elsewhere; `new String`+`.toString()`: 22349ns vs. 927-2512ns; proxy: 39412ns vs. 2240-2655ns) — flagged, not attributed to a specific mechanism at the source level the way the jpy/jep rows above were |

### Not compared here (excluded, no pyjnius equivalent to port to)

Same exclusion as jpy/jep: fault-injection (`test_fault.py`) and
coverage-instrumentation tests (`test_coverage.py`, `test_javacoverage.py`)
test jpype's own internals, not portable behavior.

### pyjnius suite-size context

pyjnius: 37 Python test files, 160 tests, `~/devel/pyjnius/tests/`. Between
jpy's 21 files and jep's 34-file/247-test suite in file count, but fewer
total tests than jep's. The collection-protocol/Comparable/functional-
interface/general-proxy support above means pyjnius has *something* to
port against for a comparably large slice of jpype's suite as jep does —
but `test_classhints.py`/`test_hints.py`/`test_customizer.py`,
`test_pickle.py`/`test_serial.py`, the introspection-ergonomics files, and
(uniquely among the three) *any* multi-dimensional/buffer array test
remain gaps for pyjnius.

## Future: jpype's reverse-embedding direction (`origin/reverse`)

**A wrong assumption got made in this doc's first pass, and it's worth
recording plainly, because it's the kind of mistake an AI assistant
reasoning about this codebase keeps being tempted to make again:** the
initial write-up claimed jpype's architecture is permanently
Python-hosts-the-JVM, and that jep's "embeds Python inside the JVM"
niche — a pure Java application (`java -cp your_app.jar
com.your.Main`) bringing up an embedded Python interpreter itself,
Java as the host process, not Python — was therefore something no
amount of jpype development could ever close. That claim was made
without checking the `reverse` branch first, and it's false.

`org.jpype.MainInterpreter` (`~/devel/jpype` on `origin/reverse`,
`native/jpype_module/src/main/java/org/jpype/MainInterpreter.java`) is
exactly that entry point — a Java-side singleton, "main entry point for
interacting with the Python interpreter" per its own javadoc, that
locates/probes/launches an embedded CPython interpreter from Java code,
no Python process involved in starting anything. Demonstrated concretely
by `native/jpype_module/src/test/java/runner/HelloWorldMain.java`, a
pure `public static void main(String[] args)` with zero Python
involvement in bootstrapping:

```java
public static void main(String[] args) {
    MainInterpreter.getInstance().start(new String[0]);
    Script context = new Script(MainInterpreter.getInstance());
    context.exec("msg = 'Hello World from Python'");
    PyObject msg = context.eval("msg");
    ...
}
```

It goes further than parity with jep: `origin/reverse` actually exposes
**three separate, purpose-built embedding layers**, not one API wearing
different hats. Each was checked individually against its own source,
not assumed from the others:

1. **JSR-223** (`org.jpype.script.JPypeScriptEngine`) — implements
   `javax.script.AbstractScriptEngine`/`Invocable`, the standard scripting
   API every JVM already has a pluggable-scripting-language story for
   (`ScriptEngineManager`). Generic, not jpype-specific — any tooling that
   already knows JSR-223 gets Python for free.
2. **Context** (`org.jpype.Script`, built on `MainInterpreter`) — its own
   javadoc calls it "a scope of variables in the Python interpreter... we
   can consider these to be modules housed in Java space." Each `Script`
   instance owns its own globals/locals `PyDict`, exposes `eval()`/
   `exec()`/`importModule()`, and multiple `Script` instances can coexist
   against one shared interpreter -- the same pattern other embedding
   systems call a Context/session object (e.g. GraalVM's `Context`), not
   just a thin exec/eval wrapper.
3. **`python.lang`** — a genuinely large (54 files) typed Java class
   library mirroring Python's own builtin type system directly:
   `PyObject`, `PyDict`, `PyList`, `PySet`/`PyFrozenSet`,
   `PyInt`/`PyFloat`/`PyComplex`, `PyString`/`PyBytes`/`PyByteArray`,
   `PyGenerator`/`PyCoroutine`/`PyAwaitable`, `PyCallable`, iterators for
   all of it, etc. Its `package-info.java` states the design intent
   explicitly: implement Java collection interfaces where they don't
   conflict with Python semantics, tight return types, loose parameter
   types, fall back to `eval()` only when a wrapper can't express
   something. Meant for direct, idiomatic object-level Java code -- no
   string-based `exec`/`eval` required for most uses.

**How `python.lang` actually gets populated is itself a fourth, distinct
proxy model, and it's built on top of the other two rather than being a
separate implementation.** jpype has three ways to make a Python object
answer to a Java interface, and it's worth being precise about which is
which, since all three ultimately construct the same underlying object:

1. **Bare-callable SAM duck typing** — a plain Python `lambda`/callable
   passed directly where a Java functional interface argument is
   expected, no proxy class or registration involved.
2. **`@JImplements`** — the modern, type-checked decorator: a Python
   class declares which Java interface(s) it implements, once, ahead of
   time (`jpype/_jproxy.py`).
3. **`JProxy(interfaces, dict=... | inst=...)`** — the older, manual
   form: hand a dict of callables or an object instance straight to
   `JProxy`'s constructor, still supported, `@JImplements`'s
   predecessor (`_jproxy.py:186-234`).
4. **Automatic, structural, no user call at all** — `JPConversionPython`
   (`native/common/jp_classhints.cpp:1908-1992`), one of the conversion
   rules `JPPybaseType::findJavaConversionImpl` (`jp_pybasetype.cpp:34-48`)
   tries for `java.lang.Object` and, by inheritance, every interface
   type that falls through to it. Whenever *any* Python value needs to
   become *any* Java-typed value — an argument, a return, a field, not
   just an explicit user-declared proxy site — `matches()` calls
   `PyJP_probe(st, Py_TYPE(object))` (`native/python/pyjp_probe.cpp`),
   which interrogates the Python type's own CPython C-level protocol
   slots directly (`tp_call`, `tp_as_buffer`, `tp_as_sequence`,
   `tp_as_mapping`, `tp_as_number`, `__enter__`/`__index__`) plus
   `collections.abc` subclass checks, to derive which `python.lang`
   interfaces that type structurally satisfies — genuinely structural
   introspection of Python's own protocol machinery, not a fixed table
   of hardcoded per-type branches. If one of the probed interfaces
   matches the declared target, `convert()`
   (`jp_classhints.cpp:1972-1991`) **dynamically constructs a `JProxy`
   on the spot** — it literally instantiates `_jpype._JProxy`
   (`PyJPProxy_Type->tp_new(...)`), the exact same class backing models
   2 and 3 above, wrapping the value with the method table the probe
   resolved — and returns the resulting Java-side proxy object. So
   model 4 isn't a fourth *implementation*; it's models 2/3's own
   machinery, invoked automatically by structural type-probing instead
   of an explicit decorator or constructor call. This is the real
   engine behind `python.lang` looking like 54 concretely-typed classes
   without 54 hand-written JNI wrapper implementations: `_jbridge.py`'s
   `_concrete`/`_protocol`/`_methods` tables (already described above)
   supply the per-type method dictionaries; `PyJP_probe` + `JPConversionPython`
   supply the automatic, no-call-site-changes-needed dispatch that picks
   the right one and proxies it into place.

Neither jpy nor jep has anything at this level. jpy's typed wrappers
(`PyModule`, `PyDictWrapper`, `PyListWrapper` — three classes total) must
be constructed explicitly by the caller around a generic `PyObject`; there
is no probe-driven automatic selection, and its own general-proxy support
(`PyObject.createProxy()`) didn't produce a usable object in this
checkout regardless. jep comes closer on one narrow slice: its own
Python-to-Java dispatcher, `PyObject_As_jobject`
(`~/devel/jep/src/main/c/Jep/convert_p2j.c:1050-1116`), does have one
genuinely automatic, declared-type-driven case — `PyCallable_Check(pyobject)
&& isFunctionalInterfaceType(env, expectedType)` triggers
`PyCallable_as_functional_interface` (lines 1091-1097), converting any
Python callable into any SAM-shaped target interface automatically, on
both argument *and* return paths, no explicit proxy construction needed.
That's real, and closer to jpype's model 1 generalized to return
positions than the feature matrix's "Yes, argument-only" framing gave it
credit for. But everything else in that same function is a fixed,
hardcoded C-level `if`/`else` chain (`PyLong_Check`/`PyDict_Check`/
`PyUnicode_Check`/buffer/numpy) mapping to a **closed, compiled-in** set
of concrete Java types, with the single generic `jep.python.PyObject` as
the catch-all for anything else — no equivalent of probing a Python
type's own protocol slots to decide it structurally satisfies some
*arbitrary* multi-method interface (jpype's `python.lang.PyMapping`,
`PyIterable`, or a third-party `WrapperService`-registered one). Adding
a new target interface to jep's version means patching and recompiling
its C source — the same extensibility gap already established in the
`WrapperService`/`.pyspi` section below, showing up again here in the
proxy-selection mechanism specifically, not just the type-registration
surface.

**All four models, side by side, across all four libraries:**

| Model | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| 1. Bare lambda/callable → SAM argument (forward: Python passes a callable where Java wants a functional interface) | Yes | **No** — only explicit proxy objects, no implicit lambda conversion | Yes (`pyjtype.c`'s `functionalInterface`) | Yes (`jnius_conversion.pxi`) |
| 1b. Same idea, declared-type-driven on the **reverse** side (Java expects a functional interface back from Python, no proxy call) | Yes (subsumed into model 4 below) | N/A — no reverse direction | **Yes** — `PyCallable_as_functional_interface`, `convert_p2j.c:1091-1097` | N/A — no reverse direction |
| 2/3. Explicit proxy (decorator and/or manual dict/inst construction) | Two distinct forms: `@JImplements` (typed, modern) and `JProxy(dict=...\|inst=...)` (manual, older, still supported) | One form, `PyObject.createProxy()` — confirmed **broken in this checkout**, didn't produce a usable object | One form, `jep.jproxy()` — real, working, no defect found | One form, `PythonJavaClass` + `@java_method(...)` subclassing — works for the common case, but has the reproduced null-`Object`-argument segfault and a silent wrong-return-value bug |
| 4. Automatic, structural, no proxy call at all — Java just declares the type it wants (any multi-method interface, not just functional ones) and gets a live proxy for free | **Yes** — `JPConversionPython`/`PyJP_probe`, probes Python's own C-level protocol slots, extensible to arbitrary `WrapperService`-registered interfaces | **No** — three hand-written wrapper classes (`PyModule`/`PyDictWrapper`/`PyListWrapper`), must be constructed explicitly by the caller, no probing | **No** — `PyObject_As_jobject` is a fixed, hardcoded C-level type chain to a closed set of concrete Java types (plus the model-1b functional-interface case above); adding a new target interface means patching and recompiling jep's C source | N/A — no Java-hosts-Python direction exists at all |

The shape that falls out: jpype is the only one of the four with a
structural, extensible, no-call-site-changes automatic path (model 4) —
the other three all require the Python side to either be plain-callable
(model 1) or to explicitly opt into a proxy at construction time (models
2/3), and none of them can say "Java declared it wants a `Mapping`-shaped
thing back, and any Python object that happens to support
`__getitem__`/`__setitem__` just satisfies that automatically." jep is
the interesting middle case: it independently arrived at the *same idea*
model 4 embodies, but scoped narrowly to callables-as-functional-
interfaces rather than generalized to arbitrary multi-method interfaces
via real protocol introspection. jpy and pyjnius don't have a reverse
direction to compare model 4 against at all (jpy's exists but is
unused/broken for this purpose; pyjnius's doesn't exist).

For scale: jep has **no JSR-223 `ScriptEngine` implementation at all**
(checked `~/devel/jep/src/main/java/jep/` -- no `javax.script` reference
anywhere), and its public embedding surface is essentially one class
(`Jep`, with `eval`/`exec`/`getValue`/`set`) plus its `pyj*` wrapper
types -- not three separated, purpose-built layers. So once merged,
jpype's Java-hosts-Python story isn't just parity with jep's -- it's a
richer set of entry points than jep itself offers for that same
direction.

**A fourth thing, not checked until pointed at directly, that widens
this further: a real, user-extensible SPI.** `python.lang`'s type
hierarchy above covers Python's *builtins*. Beyond that,
`org.jpype.WrapperService` (discovered via standard
`java.util.ServiceLoader`, JPMS-compatible via `provides ... with` in
`module-info.java`) is a genuine third-party plugin point: any Java
library can expose *any* Python class as a typed Java interface by
registering a provider and dropping one small declarative resource file
per class (`.pyspi`: a `key: value` header naming the Python
module/class/target Java interface, a `---` separator, then a Python
source blob binding a `METHODS = {...}` dict) -- **no editing of
jpype's own source required.** Concretely, e.g.
`collections.deque.pyspi`:

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

-- mapping Python's `deque` onto a Java interface using *`java.util.Deque`'s
own method names* (`addFirst`/`removeFirst`), so Java code gets a
collection that feels native, backed transparently by the real Python
object. jpype ships five built-in providers this way already (not
hypothetical -- 27 `.pyspi` files, 475/475 tests passing): `python.io`
(the full `io`/`_io`
hierarchy -- `BytesIO`/`StringIO`/`FileIO`/`BufferedReader`/`Writer`/
`TextIOWrapper`/etc.), `python.collections` (`ChainMap`/`Counter`/
`OrderedDict`/`defaultdict`/`deque`), `python.datetime` (`date`/
`datetime`/`timedelta`), `python.decimal` (`Decimal`), `python.pathlib`
(`PosixPath`/`WindowsPath`).

None of jpy/jep/pyjnius have anything resembling this. jep's collection
support (`pyjlist.c`/`pyjmap.c`/etc.) and pyjnius's `protocol_map`
(`reflect.py`) are both real and both confirmed working earlier in this
doc -- but both are fixed, hardcoded in each library's own source. A
third party cannot add jep or pyjnius support for a new Python stdlib or
third-party class (say, exposing `numpy.ndarray` as a proper typed Java
interface) without patching jep's or pyjnius's own C/Cython source and
rebuilding the extension. jpype's SPI turns that into a one-file,
no-recompile, ordinary-Java-service-provider addition. jpy has no
collection-protocol support at all to compare against (established
earlier in this doc), let alone an extension mechanism for one.

**The sharper point underneath "no editing of jpype's own source
required": neither side of the bridge needs any awareness of the other
at all.** `JClassHints.registerClassImplementation(classname, proto)`
(`jpype/_jcustomizer.py:222-231`, the machinery behind
`@JImplementationFor`) keys purely on a *string* class name -- it
requires no marker interface, no annotation, no jpype dependency on the
target's classpath, not even that the class exists yet at registration
time (the retroactive path, `_applyCustomizerPost`, exists precisely for
"customize a class that's already loaded"). `WrapperService`/`.pyspi`
has the identical property from the other side: a Python module gets
declared as satisfying a Java interface by name, with the Python module
itself needing no jpype awareness either. The practical consequence: a
private, closed-source, never-published Java library -- or Python
module -- can be customized to feel completely native, with the
customization living entirely in a third location (glue code the end
user writes themselves), while the library or module being customized
stays exactly what it always was, unaware anything is bridging into it.
jep and jpy have no equivalent gate at all, so the only way to get
comparable ergonomics for a private class there is patching and
recompiling jep's or jpy's own C source -- not a real option for someone
else's internal library, which in practice means private code using jep
or jpy is permanently stuck with whichever generic, un-customized
wrapper each library ships.

**The key point this adds up to: the reverse direction wasn't just given
a feature set, it was given parity of *extensibility* with the forward
direction jpype already had -- deliberately, not as an accidental
byproduct.** jpype's forward direction (Python customizing how a Java
class looks to Python) has always had exactly this shape:
`jpype/_jcustomizer.py`'s `JImplementationFor(javaClassName)`/
`JConversion(cls, ...)` -- a string-named target Java class, a decorator
registering a prototype whose methods get copied onto (or converted to)
that class's wrapper, applied retroactively even if the class is already
loaded. Every Python-side customizer referenced throughout this document
(`_JCharArray` on `byte[]`/`char[]`, the `toPython()` conventions on
`java.io` streams) is built on exactly this mechanism. `WrapperService`/
`.pyspi` is the same idea, same shape, opposite direction: string-named
target Python module/class, a declarative method binding to a named
Java interface, discovered and replayed at startup instead of hardcoded.
Not a new kind of extensibility invented for the reverse bridge -- the
existing forward-direction pattern, mirrored, so that neither direction
is stuck with a closed, hardcoded set of supported types while the other
gets a real plugin point. That symmetry is the thing jep's and pyjnius's
architectures don't have on *either* side, let alone both.

**Status, to avoid overclaiming the other direction**: `origin/reverse`
is 190 commits ahead of `review` (the main branch) and not yet merged —
substantial, apparently mature work (subinterpreters, cross-interpreter
GC, an `InterpreterPipe`, `toPython()` conversions for
`Instant`/`Path`/`File`/`BigDecimal`/dates, coverage raised to 90-100% on
many modules per its own plan docs), but "future," not current `review`
behavior, and not re-verified with the same empirical rigor (actually
running it, checking edge cases) applied to jpy/jep/pyjnius throughout
the rest of this document.

**Correction to the correction**: jep is not the *only* other library
with a native Java-hosts-Python story -- jpy has its own, independent
of jep's, and this doc almost repeated the exact mistake above a second
time by not checking jpy for it too. `org.jpy.PyLib.startPython()`/
`stopPython()`/`isPythonRunning()` (jpy's own core native-bridge class)
let a pure Java application embed and control a Python interpreter
directly, demonstrated by a real JUnit test with zero Python bootstrap
(`src/test/java/org/jpy/EmbeddableTestJunit.java` ->
`EmbeddableTest`'s `PyLibControl` inner class, `~/devel/jpy`). So the
accurate three-way picture is: jep's architecture *is* Java-hosts-Python
(its native, primary direction); jpy has it as a secondary, less-
documented capability alongside its usual Python-hosts-Java mode;
pyjnius has neither -- checked its Java sources specifically for this and
found only test fixtures and the `PythonJavaClass` proxy-callback
machinery, nothing resembling an embeddable launcher. Once `origin/reverse`
merges, jpype covers *both* embedding directions natively too -- the
ground jep and jpy each independently stake out on the Java-hosts-Python
side, plus jpype's own dominant Python-hosts-Java strength (everything
above), while pyjnius remains the only one of the four with just one
direction.

**The general lesson, stated for whoever (human or AI) reads this doc
next**: "library X's architecture makes Y permanently impossible" is a
claim that must be checked against that library's own branches/plans
before being written down, not inferred from "that's not what the
`review` branch does today." This doc got that wrong once already.

### Launching embedded Python from Java: three different discovery models

The other half of "Java hosts Python" that hasn't been compared yet:
before any of the API surface above can run, something has to find the
right `python`/`libpython` on disk and load it into the JVM process.
pyjnius doesn't have this problem at all (no Java-hosts-Python direction
to compare, established earlier). jpype, jpy, and jep each answer it
completely differently.

**jpy: fully static, ahead-of-time, no runtime discovery at all.**
`PyLibConfig`'s static initializer (`~/devel/jpy/src/main/java/org/jpy/PyLibConfig.java:53-71`)
only ever reads a `jpyconfig.properties` file — from the classpath, from
`-Djpy.config=<path>`, or from the current working directory — and
copies its keys into `System` properties. That file isn't generated at
run time; it's written once, ahead of time, by a separate Python-side
step (`jpyutil.write_config()`, run manually from Python during setup).
`PyLib.loadLib()` then reads `jpy.jpyLib`/`jpy.pythonLib` straight out of
that config via `getProperty(key, mustHave=true)`
(`PyLib.java:521-546`) and calls `System.load()` directly — no search,
no fallback. If the config is missing or stale (Python reinstalled, venv
moved, wheel rebuilt), the failure is immediate and unhelpful:
`RuntimeException("missing configuration property 'jpy.jpyLib'")`
(`PyLibConfig.java:120-122`). The entire discovery problem is pushed
onto the user/build system, once, before Java ever runs.

**jep: real runtime discovery, but by re-deriving Python's own search
path in Java rather than asking Python directly.** `MainInterpreter.initialize()`
first tries the conventional `System.loadLibrary("jep")` (i.e. whatever
`-Djava.library.path` already points at); only on `UnsatisfiedLinkError`
does it fall back to `LibraryLocator.findJepLibrary()`
(`~/devel/jep/src/main/java/jep/MainInterpreter.java:124-135`). That
locator is a genuine search, not a stub: it walks `PYTHONPATH`
(`searchPythonPath`), then reimplements CPython's own
`site.py`-`getsitepackages()` layout against `PYTHONHOME`/`VIRTUAL_ENV`
(`searchSitePackages` — `lib`/`lib64`/`Lib`, `site-packages`,
`site-python`, versioned `pythonX.Y/site-packages`), then user-site
locations per PEP 370 across all three OS conventions
(`searchUserSitePackages` — `~/.local/lib/pythonX.Y/site-packages`,
Windows `%APPDATA%/Python`, macOS `~/Library/Python/X.Y`) —
`LibraryLocator.java:100-227`. It even has a narrow self-healing step:
if `libjep`'s own `System.load()` fails because the *specific*
`libpython*.so` it needs isn't found, it regex-parses the exact missing
library name out of the `UnsatisfiedLinkError` message and searches
`PYTHONHOME` for it by that exact name (`findPythonLibrary`,
`LibraryLocator.java:253-292`). But every bit of this is a Java-side
*mirror* of Python's layout logic, not a query against a live Python
process — the class's own doc comment admits the risk directly: "this is
just a mirror of what Python is doing, if there are changes to Python it
may require changes here" (`LibraryLocator.java:39-46`). No caching (the
full walk re-runs every failed-`loadLibrary` startup), and no install/
self-heal step if nothing is found — it just returns `false` and the
original `UnsatisfiedLinkError` propagates.

**jpype: runs a real Python subprocess to ask the interpreter directly,
caches the answer, and can self-heal via `pip` if nothing is found.**
`Launcher.resolveLibraries()` (`native/jpype_module/src/main/java/org/jpype/Launcher.java:417-456`)
resolves which `python` executable to target (system property →
`PYTHONHOME` env var → first `python3` on `PATH`,
`getExecutable`/`checkPath`, lines 190-253), then — unlike jep's static
path-mirroring — actually launches that executable and runs a bundled
"detective probe" script inside it (`loadProbeResource`/`executeProbe`,
lines 167-314): the probe reports back its own real `sys.executable`,
library paths, and JPype install location as `Properties`, authoritative
because it comes from the live interpreter itself, not reconstructed
from directory-layout conventions that can drift. That result is cached
on disk (`~/.jpype/jpype.properties` or the Windows `AppData`
equivalent) keyed by a hash of the executable path
(`saveCache`/`loadFromCache`, lines 255-264, 336-415) — with an explicit
staleness check on load, verifying the cached library paths still exist
on disk before trusting the cache, specifically because a `pip install
--upgrade` can move the native module to a new wheel-cache path while
leaving the interpreter itself untouched (comment,
`Launcher.java:397-402`). And if the probe fails outright and
`jpype.install=true`, `runPipInstall()` (lines 462-504) attempts a real
self-heal: look for a local matching wheel first (offline-friendly),
fall back to a network `pip install JPype1>=<version> --only-binary`
otherwise, then re-probe.

**The shape of the difference**: jpy pushes discovery entirely outside
the running system (a one-time, ahead-of-time file you must remember to
regenerate); jep does real runtime discovery but by simulating Python's
own path logic in Java, which can only ever be as correct as that
simulation stays in sync with CPython's actual behavior; jpype asks the
live interpreter what's true, caches the authoritative answer, validates
that cache before trusting it, and can repair a missing install rather
than just fail. Three genuinely different engineering answers to the
same problem, not one library having "a feature" the others lack outright
— jep's is real discovery, not absent — but jpype's is the only one of
the three that is both authoritative (asks Python, doesn't guess) and
resilient (cached-with-validation, self-healing) at the same time.

## Further out (speculative): J2NI, and what "JPype2" could mean

**Flagged explicitly as speculative** -- this is a separate project
(`~/javafx2/j2ni`, its own repo, own `pom.xml`), not a jpype branch, not
found anywhere in jpype's own git history (checked every local and
remote branch), and its own README says, verbatim: `**DRAFT**` --
"most piece are drafted but getting all the definitions consistent and
tested will take a while." Nothing below should be read as jpype's
committed roadmap. It's included because it changes what "jpype ahead of
jpy/jep/pyjnius" would eventually *mean*, if it lands.

**What it is**: a proposed replacement for JNI itself, built on Java
22+'s Foreign Function & Memory API (Project Panama) -- not a jpype
feature, a rework of the layer every one of jpy/jep/pyjnius, *and
jpype's own current codebase*, sits on top of. Concretely:

- **Metadata-driven dispatch** (signature hashing, pre-cooked method
  handles) instead of JNI's string-based method/field lookup.
- **Query-View-Pull** bulk data transfer -- native code queries a "view"
  (size, element type, writability) then pulls/pushes in bursts --
  instead of JNI's pin-a-raw-pointer model.
- **Process-agnostic identifiers** (`int64_t` handles, not raw
  object-header pointers), explicitly so the same protocol works
  whether native and JVM code share a process, share memory, or are
  **separate, possibly remote processes**. `j2ni-remote` is a real,
  bit-packed wire protocol for this (`FIXME.md` shows actual header-
  layout tuning -- `(size:11)(routing:16)(ack:1)(op:12)(checksum:24)`,
  with routing, opcodes, and checksums) -- not an in-process trampoline
  at all. That's the part that makes the earlier "full CORBA" framing
  of jpype's forward/reverse customizer symmetry more literal than it
  first looked: an actual object broker over a wire, not just the same
  *shape* of idea applied in-process.
- Worth a passing note, not a pattern to lean on too hard: `FIXME.md`
  mentions J2NI's own native-to-Java export surface "grows via the SPI
  extension pattern" -- the same extensibility instinct as
  `WrapperService`/`.pyspi` above, showing up independently in a
  different project by evidently the same author.

**The point, stated the way it was raised**: everything else in this
"Future" section -- the reverse bridge, three embedding layers, the SPI,
subinterpreters -- is jpype getting *more capable within the JNI-based
architecture jpy/jep/pyjnius are also built on*. J2NI is not that kind of
thing. If jpype's native layer is ever rebuilt on it, the comparison
stops being "jpype has more features than the other three" and becomes
"the other three are built on a 30-year-old ABI jpype no longer is" --
memory-safety and cross-process/remote capability that isn't a row you
can add to a feature-matrix table, because none of jpy/jep/pyjnius (or
jpype's own `review` branch, today) have anything like it to compare
against. That's a different kind of "ahead" than everything documented
above it in this file -- a foundation-level jump, not an incremental one.
Whether it ever ships as part of jpype is genuinely unknown from here;
treat this section as a marker of direction, not a claim about outcome.
