# Debug: `PyGenerator`-typed cast return crashes the JVM

## Status (2026-07-18): ROOT-CAUSED AND FIXED. Two independent bugs, both fixed.

Found while chasing coverage for `python.lang`'s protocol interfaces
(`plan/Coverage.md`). `mvn -o verify -Dpython.executable=python3.10`:
775/775 (five new tests, `ProtocolInterfaceCoverageNGTest`), 0 failures,
14 skips (same baseline). `PyGenerator` casting, `.iter()`, `.iterator()`,
and iteration via `next()` are now all safe to test normally - the ban in
the old "Impact on plan/Coverage.md" section below no longer applies.

## Background: how the protocol interfaces actually get exercised

`python.lang.PyAbstractSet`/`PyContainer`/`PyIterable`/`PyGenerator`/
`PyCollection`/`PySized` are "protocol" interfaces with default methods,
advertised in their own Javadoc as duck-typed - any Python object can
satisfy them structurally, not just JPype's own concrete wrapper types
(`PySet`, `PyList`, etc.).

The concrete wrapper types (`PySet`, `PyDict`, ...) all *shadow* these
protocol defaults with their own redeclarations (see `PySet.contains`/
`.size` reverting to abstract, `PyDict.keySet`/`.values`/`.entrySet`
building their own `PyDictKeySet`/etc. instead of `PyMapping`'s), so
calling `context.set(...)`/`context.dict(...)` and using the result never
actually reaches the protocol interface's own default body - this is
*why* `PyAbstractSet` et al. showed flat 0% coverage even though the
concrete types built on top of them are heavily tested.

The real, intended way to reach them is the native structural-duck-typing
probe (`native/python/pyjp_probe.cpp`'s `interrogate()`/`PyJP_probe`,
exposed as `_jpype.pyobject(target_type, python_object)`): given an
arbitrary Python object that isn't one of JPype's own concrete types,
`interrogate()` checks CPython type slots and `collections.abc` ABC
membership (`is_set`, `is_container`, `is_iterable`, `is_generator`,
`is_collection`, ...) and builds a Java dynamic proxy implementing
whichever protocol interfaces match, via `st->protocol_pipeline[15]`
(populated in `pyjp_module.cpp` from `st->protocolDict`, keyed by name:
`"callable"`, `"buffer"`, `"sequence"`, `"mapping"`, `"iterable"`,
`"iter"`, `"generator"`, `"coroutine"`, `"awaitable"`, `"abstract_set"`,
`"collection"`, `"container"`, `"index"`, `"number"`, `"combinable"` -
note there is **no** `"mutable_set"` or `"sized"` entry, see below).

Confirmed empirically (throwaway diagnostic test, not committed) that
casting genuinely custom Python classes this way really does reach the
protocol defaults and moved real coverage:

```java
context.exec(
  "import _jpype, jpype\n" +
  "class MySet:\n" +
  "    def __contains__(self, x): ...\n" +
  "    def __iter__(self): ...\n" +
  "    def __len__(self): ...\n" +
  "import collections.abc\n" +
  "collections.abc.Set.register(MySet)\n" +
  "_t = _jpype.pyobject(jpype.JClass('python.lang.PyAbstractSet'), MySet([...]))\n"
);
PyAbstractSet<?> obj = (PyAbstractSet<?>) context.eval("_t");
obj.contains(...); obj.size(); obj.isEmpty();   // all worked, 0/25 -> 17/25
```

Same pattern worked cleanly for `PyCollection` (exercises the otherwise
believed-dead `PySized` too, since `PyCollection` extends it without
overriding `size()`/`isEmpty()`) and `PyIterable`/`PyContainer`.

**`PySized` is dead as standalone probe-reachable surface but not
deletable** - confirmed via `grep`: zero implementors, and neither it nor
`PyMutableSet` appears in the `protocol_pipeline` name list above, so the
structural probe can never select either directly (`PySized` is only
reachable *indirectly* through `PyCollection`, as noted; `PyMutableSet`
has no path at all - nothing in the probed interface set ever extends
it).

**Correction (2026-07-18): `PyMutableSet` is NOT actually a dead-code
deletion candidate**, despite being unreachable via the structural probe.
`jpype/_jbridge.py` loads it eagerly at bootstrap regardless -
`JClass("python.lang.PyMutableSet")` - and registers it in
`_jpype._protocol["mutable_set"]` and `_jpype._methods[_PyMutableSet]`.
Verified by actually deleting `PyMutableSet.java`: `mvn -o verify`
immediately broke every single `PyTestHarness`-based test at
`setUpClass` with `PyBuiltIn.getContext()` NPE, since `_jbridge.py`'s
bootstrap sequence throws before it finishes populating the builtin
context. Reverted via `git checkout`, suite back to 775/775. So this
interface is dead *as reachable API surface* (no real or duck-typed
object can ever be proxied as one) but not dead *as a referenced class* -
deleting the Java file requires also editing `_jbridge.py`'s bootstrap,
which is a different, riskier change than a plain dead-code removal and
out of scope here. Left in place.

## The crash: `PyGenerator`

Same cast pattern, but for a **real Python generator object**
(`def f(): yield 1`, not a hand-registered class) cast to
`python.lang.PyGenerator`:

```java
context.exec(
  "import _jpype, jpype\n" +
  "def mygen():\n" +
  "    yield 1\n" +
  "_b = _jpype.pyobject(jpype.JClass('python.lang.PyGenerator'), mygen())\n"
);
PyGenerator<?> gen = (PyGenerator<?>) context.eval("_b");   // <-- SIGSEGV here
```

**Minimal, deterministic repro** (reproduced 3 times across slightly
different variants while isolating it):
- The `_jpype.pyobject(...)` cast call itself, inside `context.exec(...)`,
  **succeeds** - confirmed by adding a `print(...)` immediately after it
  in the same script, which reliably prints before returning to Java.
- The crash is in `context.eval("_b")` - i.e. converting the
  already-successfully-cast Python-side proxy value back across the
  bridge into a Java object typed as `PyGenerator`. Never reached any
  Java-side method call on the result (`.iterator()` etc.) - the crash
  happens before that's even possible.
- `PyCollection`, `PyAbstractSet`, `PyIterable`, `PyContainer` casts +
  their `context.eval(...)` retrieval all work fine with the exact same
  script shape - this is specific to `PyGenerator`.
- No `hs_err_pid*.log` was produced for this crash (consistent with the
  earlier `plan/ExecCrashDebug.md` finding that WSL intercepts core dumps
  before a local hs_err file gets written); only `Segmentation fault
  (core dumped)` on stderr and Surefire's "forked VM terminated" wrapper.

## Root cause: two independent bugs, both needed to be fixed

Confirmed via Java-side diagnostics (`System.err.println` in
`ProxyType`'s constructor, no gdb needed - the crash site turned out to
be pure Java/JVM-level infinite recursion, not a native memory bug) plus
one native one-liner. Reproduced with a scratch `DiagTest.java` (not
committed - see `ProtocolInterfaceCoverageNGTest` for the permanent
version) that isolated the crash down to `PyGenerator.iterator()`
specifically (`.iter()` alone was always safe; the earlier belief that
`context.eval("_b")` itself crashed was wrong - eval always succeeded,
the crash only ever needed a `.iterator()` call, which the original
repro's follow-up `print(...)` never happened to trigger).

### Bug 1: `PyIterable` and `PyIter`/`PyGenerator` are genuinely unrelated
Java interfaces that both declare `iter()`/`iterator()`

`native/python/pyjp_probe.cpp`'s `interrogate()` independently flags
`is_iterable`, `is_iterator`, and `is_generator` from `collections.abc`
subclass checks. Since `Generator` subclasses `Iterator` subclasses
`Iterable`, a real Python generator sets all three, so the structural
probe attached **both** `PyIterable` and `PyGenerator` (via `PyIter`) to
the same dynamic proxy. `PyIter`/`PyGenerator` do NOT Java-extend
`PyIterable` (a deliberate API split - generators have a different
method surface), so from `org.jpype.proxy.ProxyType`'s point of view
these are "genuinely unrelated interfaces" sharing a method name -
exactly the diamond case its `isBetter()` tie-break was written for. But
that tie-break unconditionally prefers *any* default over a plain
abstract method, which is wrong here: `PyGenerator.iter()` is
deliberately left abstract (same pattern as `PyDict.get()`) to force
native dispatch, and `PyIterable.iter()`'s ordinary default
(`builtin().iter(this)`) won the tie-break instead, diverting dispatch
away from the intended native path.

**Fix** (`native/python/pyjp_probe.cpp`, `interrogate()`): suppress
`is_iterable` whenever `is_iterator` already holds, so the two
interfaces are never combined on the same proxy - mirrors the real
`collections.abc` subsumption (every `Iterator`/`Generator` already *is*
an `Iterable`) instead of relying on a generic, unrelated-interface
tie-break to sort out a collision that shouldn't exist in the first
place.

### Bug 2 (the actual crash mechanism): `PyGenerator.iterator()`'s own
default recurses into itself forever

Independent of bug 1 - confirmed by fixing bug 1 alone and reproducing
the crash again unchanged. `PyGenerator.iterator()`'s default was:
```java
default Iterator<T> iterator() { return iter().iterator(); }
```
For a real generator, `iter()` (Python's `__iter__`) returns the
generator itself - the same underlying Python object, structurally
satisfying the exact same interface set. So the returned `PyIter<T>` is,
at the Java dispatch level, *also* a `PyGenerator`-typed proxy, and
calling `.iterator()` on it dispatches straight back into this same
default. Every cycle crosses back into native code (proxy construction,
GIL acquisition) rather than staying in pure Java stack frames, so it
exhausts the native/C stack and SIGSEGVs well before the JVM's own
`StackOverflowError` guard would ever catch a pure-Java infinite
recursion - explaining why no `hs_err_pid*.log`/clean Java exception was
ever produced.

**Fix** (`python/lang/PyGenerator.java`): construct the wrapper directly
instead of re-dispatching, matching `PyIter.iterator()`'s own default
(`new PyIterator<>(this)`):
```java
default Iterator<T> iterator() { return new PyIterator<>(iter()); }
```

Both fixes are required together: bug 1 alone left the crash unchanged
(verified); bug 1's fix is still worth keeping independently since it
also restores `PyGenerator.iter()`'s intended native dispatch (bug 1 was
silently routing it through `PyIterable`'s default instead).

## Verification

`mvn -o verify -Dpython.executable=python3.10`: 775/775 (up from 770;
five new tests in `python.lang.ProtocolInterfaceCoverageNGTest`), 0
failures, 14 skips (same pre-existing baseline). New test
`testGeneratorCastAndIteration` casts a real two-value generator,
iterates it fully via the Java `Iterator` interface, and asserts the
values come back in order - this is the regression test for the crash.

## Is there any remaining path to hit this?

No known one. `PyGenerator.iterator()`'s fix is unconditional - it no
longer self-recurses regardless of what interface combination a future
probe change might produce. Bug 1's fix closes the specific
`PyIterable`/`PyIter` collision at its source (rather than only papering
over `PyGenerator`'s symptom), so any other duck-typed object satisfying
`is_iterator` (plain iterators, not just generators) also gets the
collision-free interface set.

## Impact on `plan/Coverage.md`

Resolved - `PyGenerator` is no longer unsafe to test.
`PyAbstractSet`/`PyContainer`/`PyIterable`/`PyCollection`/`PyGenerator`
are all now covered by committed tests in
`python.lang.ProtocolInterfaceCoverageNGTest`.
