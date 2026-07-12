# Fix `getattr()`-retrieved callables failing on the reverse bridge

## Status (2026-07-11): NOT STARTED - reproduced, root cause narrowed but not confirmed

Found while writing a Java-drives-Python benchmark (`benchmark/reverse/`,
`JpypeBench.java`/see [[jpy_vs_jpype_benchmark]]) that exercises jpype's
reverse bridge (`org.jpype.MainInterpreter` / `Script` / `python.lang.*`)
more heavily than any existing test does. The benchmark had to route around
this bug rather than hit it, which means the workaround is now silently
baked into `JpypeBench.java` and nothing regresses if the real bug reappears
or gets worse.

## Reproduction

```java
MainInterpreter.getInstance().start(new String[0]);
Script context = new Script(MainInterpreter.getInstance());
context.importModule("bench_reverse");           // a plain .py file, def identity(x): return x

PyObject mod = context.eval("bench_reverse");
PyCallable identity = (PyCallable) context.getattr(mod, "identity");
identity.call(42);                                 // <-- throws here
```

Throws:

```
python.exceptions.PyValueError: method called on null object
	at org.jpype.proxy.ProxyInstance.hostInvoke(Native Method)
	at org.jpype.proxy.ProxyInstance.invoke(ProxyInstance.java:92)
	at python.lang.PyCallable.call(PyCallable.java:98)
```

Two independently-working alternatives, either one avoids it:

```java
// (A) retrieve via eval() instead of getattr(), same target function
PyCallable identity = (PyCallable) context.eval("bench_reverse.identity");
identity.call(42);                                 // works

// (B) still retrieve via eval(), but invoke via context.call(...)
//     instead of the PyCallable's own default call() method
PyCallable identity = (PyCallable) context.eval("bench_reverse.identity");
context.call(identity, context.tuple(42), context.dict());  // works
```

`JpypeBench.java` currently uses the combination of (A) retrieval +
`context.call(...)` invocation (belt and suspenders) - it has **not** been
confirmed which single change actually fixes it, because both were changed
at the same time while chasing the working benchmark. See "First step"
below - this needs to be isolated before any fix is written.

## What's already been narrowed down (not yet confirmed)

- `PyCallable.call(Object... args)`'s default implementation
  (`native/jpype_module/src/main/java/python/lang/PyCallable.java:96-98`) is
  `return builtin().backend.call(this, builtin().tuple(args), null);` - i.e.
  it ultimately calls the exact same `Backend.call(PyCallable, PyTuple,
  PyDict)` that `context.call(...)` calls directly
  (`PyBuiltIn.java:231`, `return backend.call(obj, args, kwargs);`). If (A)
  alone (eval retrieval + the callable's own `.call()`) turns out to work,
  that rules out the invocation path entirely and confirms the bug is
  specific to `getattr()`'s retrieval.
- The Python-side implementation of the `"getattr"` backend primitive is
  trivial and looks correct: `jpype/_jbridge.py:299`,
  `"getattr": lambda x,s: getattr(x, str(s))`. It returns the real Python
  function object (`bench_reverse.identity`) with no wrapping of its own -
  the same object `eval("bench_reverse.identity")` would resolve to.
- Both `getattr` and `eval` cross their return value back into Java through
  the same generic path in `Java_org_jpype_proxy_ProxyInstance_hostInvoke`
  (`native/common/jp_proxy.cpp:128-238`): call the Python callable, then
  `returnClass->findJavaConversion(returnMatch)` +
  `returnMatch.convert()` to box the result as a live `JPProxy` matching the
  Java-declared return type (`PyObject` for both `getattr` and `eval`,
  per their `Backend` interface signatures). Since this conversion step is
  shared code, a bug specific to `getattr` (if retrieval really is the
  culprit, pending the isolation step above) is more likely to be in
  something `getattr` does differently upstream of that shared conversion -
  e.g. how `obj` (the module, itself a proxy from an earlier `eval`) gets
  unwrapped back to a raw `PyObject*` before `getattr(x, str(s))` runs on
  it - than in the conversion itself.
- `python.lang.PyBuiltIn.getattr(PyObject obj, CharSequence key)`
  (`native/jpype_module/src/main/java/python/lang/PyBuiltIn.java:375-378`)
  just forwards to `backend.getattr(obj, key)` - no Java-side logic to
  suspect there either.

None of the above has been read closely enough yet to say *which specific
line* is wrong - it narrows the search to "something about how `obj` is
passed as an argument into the `getattr` host call, or something about the
particular runtime type of the value `getattr` returns (a `function`,
specifically, vs. whatever `eval` most commonly returns in existing tests)",
not a confirmed root cause.

## Why existing tests didn't catch this

`PyBuiltInNGTest.testGetattr()` (`native/jpype_module/src/test/java/python/lang/PyBuiltInNGTest.java:159-164`)
only asserts `upperFunc instanceof PyCallable` - it never actually *calls*
the retrieved callable. That appears to be the only existing test that
retrieves a callable via `getattr()` at all, and it stops one assertion
short of exercising this bug. (Its target, `"hello".upper`, is also a bound
method on a builtin type, not a plain module-level function - worth testing
both shapes once this is isolated, in case they hit different code paths.)

## First step (before any fix)

Isolate which axis is actually broken - four combinations, only one
(`getattr` + `.call()`) is confirmed broken and only one (`eval` +
`context.call()`) is confirmed working so far:

| retrieval | invocation | status |
|---|---|---|
| `getattr(mod, "identity")` | `identity.call(42)` | confirmed **broken** |
| `eval("mod.identity")` | `context.call(identity, ...)` | confirmed **working** |
| `getattr(mod, "identity")` | `context.call(identity, ...)` | **not yet tried** |
| `eval("mod.identity")` | `identity.call(42)` | **not yet tried** |

Write these four as a single throwaway Java test (or extend
`PyBuiltInNGTest`) before touching any production code. Whichever two rows
share the same "broken" verdict tells you whether to look at `getattr`'s
retrieval path or `PyCallable.call()`'s default-method dispatch.

## Fix scope (once isolated)

- If retrieval is the culprit: instrument or trace
  `Java_org_jpype_proxy_ProxyInstance_hostInvoke` for the `"getattr"` call
  specifically (`JP_TRACE` calls are already present throughout
  `jp_proxy.cpp` - build with tracing enabled rather than adding new prints)
  and compare against an `eval` call side by side, looking for where the
  returned function's `JPProxy` ends up with a null/zero host reference.
- If invocation is the culprit: same tracing approach, but on
  `PyCallable.call()`'s path specifically - check whether `this` (the
  callable being invoked) round-trips correctly as an *argument* to the
  `backend.call` host call (i.e. Java proxy -> native `PyObject*`
  extraction for an argument, as opposed to for a return value - these are
  different code paths and either could be the one with the bug).
- Either way, add a regression test next to `PyBuiltInNGTest.testCall`
  (`native/jpype_module/src/test/java/python/lang/PyBuiltInNGTest.java:38-49`)
  that actually invokes a `getattr()`-retrieved plain module-level function,
  not just a builtin bound method - that's the exact shape this plan's
  reproduction uses and the exact shape `testGetattr` currently stops short
  of covering.
- Update `benchmark/reverse/JpypeBench.java` to use whichever retrieval +
  invocation combination is idiomatic/fixed, removing the current
  belt-and-suspenders workaround, once the real fix lands - so the
  benchmark exercises the normal path again instead of permanently dodging
  the bug it found.

## Out of scope

- No investigation yet into whether this also affects `getattr()` on
  non-module objects (e.g. an attribute on a plain Python class instance)
  or only module attribute lookups specifically - the reproduction above
  only tests the module case because that's what the benchmark needed.
- Not chasing whether `PyDict`/`PyList`/other `python.lang.*` proxy types
  retrieved via `getattr()` have the same problem - only `PyCallable` has
  been reproduced as broken so far.
