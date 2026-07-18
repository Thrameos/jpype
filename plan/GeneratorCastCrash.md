# Debug: `PyGenerator`-typed cast return crashes the JVM

## Status (2026-07-18): REPRODUCED, MINIMAL REPRO IN HAND, NOT ROOT-CAUSED

Found while chasing coverage for `python.lang`'s protocol interfaces
(`plan/Coverage.md`). Not yet fixed - this doc exists so a future session
doesn't have to re-derive the reproduction steps.

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

**`PyMutableSet` and `PySized` really are dead** by contrast - confirmed
via `grep`: zero implementors anywhere in the codebase, and neither
appears in the `protocol_pipeline` name list above, so the structural
probe can never select them directly either (`PySized` is only reachable
*indirectly* through `PyCollection`, as noted; `PyMutableSet` has no path
at all - nothing in the probed interface set ever extends it). Confirmed
user-clarified reasoning: these defaults exist so *some* implementor -
built-in or a probed duck-typed object - gets them for free; `PySized` has
one (`PyCollection`), `PyMutableSet` has none. `PyMutableSet` is the
actual dead-code deletion candidate, not the other five.

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

## Hypothesis (not verified)

`PyGenerator.iterator()` is the interface's only default method; `iter()`
is abstract (native-dispatched). Suspect something in `ProxyType`'s
per-method descriptor construction (`native org.jpype.proxy.ProxyType.
getDefaultHandle`, see `[[plan/Coverage.md]]`'s note on `MethodHandles`-
based default invocation) mishandles `PyGenerator`'s particular mix of one
abstract + one generic-typed default method during the value's return
conversion, as opposed to construction-time dispatch (which never crashed
in isolation for the other four interfaces). Not confirmed - this is a
guess for where to start, not a diagnosis.

## Next steps for whoever picks this up

1. Use the instrumentation-over-gdb method from `plan/ExecCrashDebug.md`
   (`fprintf(stderr, ...)` checkpoints compiled into the real binary,
   rebuilt via `make -f project/dev.mk`, reproduced via `mvn -o test
   -Dtest=<scratch class>` - not gdb-wrapping Surefire, previously proven
   unreliable) - same playbook, different crash site.
2. Disassemble against the `_jpype.so` binary + whatever `hs_err_pid*.log`
   (if any) appears this time, same as the original crash investigation.
3. The minimal repro above is the starting reproducer - no need to
   rediscover it.

## Impact on `plan/Coverage.md`

`PyGenerator` itself cannot be safely tested until this is root-caused -
casting one crashes the JVM outright, unlike a normal test failure. Do
**not** add a `PyGenerator` cast test to the suite until this is fixed.
`PyAbstractSet`/`PyContainer`/`PyIterable`/`PyCollection`/`PySized` are
all safe and were exercised via the pattern above (not yet committed as
real tests as of this doc - only diagnostic/throwaway code was run).
