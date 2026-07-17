# Fix `getattr()`-retrieved callables failing on the reverse bridge

## Status (2026-07-17): DONE

Root cause found and fixed. It was **not** about `getattr()` vs `eval()`
retrieval, and **not** about `PyCallable.call()`'s default-method dispatch
vs `context.call()` - both halves of that axis were red herrings from the
original narrowing. The actual bug: a Java `null` passed as a fixed-arity
**object-typed** reverse-bridge argument (not a boxed primitive) did not
convert to real Python `None` when the native argument-marshaling code took
the "cast" fast path - it stayed a live-but-null-backed Java-object wrapper
instead. Confirmed by isolating a 5th combination not in the original
2x2 table: `context.call(identity, args, null)` (null kwargs, no `getattr`
or `PyCallable.call()` involved at all) reproduced the exact same failure,
proving retrieval method and invocation style were both irrelevant.

**Root cause (confirmed via instrumented tracing, not just reasoning):**
`getArgs()`/`getKwArgs()` in `native/common/jp_proxy.cpp` (marshal Java
arguments into a Python argument tuple/dict for a reverse-bridge call, e.g.
`Backend.call(PyCallable, PyTuple, PyDict kwargs)` implemented by Python)
called `JPClass::convertToPythonObject(frame, val, cast)` with
`cast = (type != java_lang_String)` - i.e. `cast=true` for every argument
except `String`, regardless of whether the argument was actually `null`.
`JPClass::convertToPythonObject`'s `value.l == nullptr -> return None` fast
path only runs when `cast == false` (see the comment there: this is
deliberate for `JObject(None, someClass)` - Java needs to keep the type
information on an explicit typed null, so it intentionally does *not*
collapse to Python's untyped `None` singleton in that scenario). Passing
`cast=true` for a null argument skips that fast path entirely and falls
through to the `isProxy()` branch, which (for `null`) resolves a null
`JPProxy*`/wraps a null-backed generic Java-object proxy instead of
`None`. When Python's `_call(x, v, k)` (`jpype/_jbridge.py`) later does
`if k is None: ... else: return x(*v, **k)`, `k is None` is false for this
wrapper, so it takes the `**k` branch - which calls `k.keys()`
(`_jcollection.py`'s generic `Map.keys()` shim), which calls Java's
`Map.keySet()` on the null-backed wrapper's underlying (null) `jobject`,
hitting `JPClass::invoke`'s `obj == nullptr` guard
(`native/common/jp_class.cpp:202`, raises `PyValueError: method called on
null object`).

**Isolation traceback confirming this (from
`PyRun_SimpleString("traceback.print_stack()")` at the crash site):**
```
File ".../jpype/_jbridge.py", line 191, in _call
  return x(*v, **k)
File ".../jpype/_jcollection.py", line 222, in keys
  return list(self.keySet())
```

**Fix** (`native/common/jp_proxy.cpp`, `getArgs`/`getKwArgs`): only pass
`cast=true` when the argument is actually non-null -
`bool cast = obj != nullptr && type != java_lang_String;`. A null
argument now always takes `convertToPythonObject`'s `cast=false` path,
which converts it to real Python `None` regardless of declared parameter
type - correct for reverse-bridge argument slots (there is no "keep the
Java type on this null" use case for an *argument* the way there is for
`JObject(None, cls)`, which is a distinct, deliberately-preserved forward-
bridge feature and was **not** touched - `JPClass::convertToPythonObject`
itself is unchanged; the fix is entirely in the two call sites that decide
`cast` for reverse-bridge arguments).

**One existing test encoded the bug as expected behavior and needed
correcting, not preserving:** `test_bridge.py::TestPyBuiltInSuite::
test_eval_expression` asserted `self.PyBuiltIn.eval(expr, globals, None)`
must raise `TypeError`, reasoning "the underlying Python implementation
expects a mapping." Real Python's `eval(expr, globals, None)` is valid
(locals defaults to globals) - confirmed directly
(`python3.10 -c "eval('sum([1,2,3])', {}, None)"` returns `6`, no error).
The old test only passed because the pre-fix bug prevented `None` from
ever reaching Python's real `eval()` as `None`. Updated the test to assert
the call succeeds and returns the correct result, matching real Python
semantics now that the boundary is fixed.

**A first attempt at this fix was too broad and had to be narrowed.**
Initially moved the `value.l == nullptr -> None` check in
`JPClass::convertToPythonObject` unconditionally above the `if (!cast)`
guard, on the reasoning that a null value has no runtime class to look up
either way so `cast` shouldn't gate the null check. This broke
`JObject(None, JString)`'s intentional typed-null behavior
(`test_jstring.py::testNullString`/`testNullCompare`/`testNullAdd`,
`test_comparable.py::testComparableNull`, and 6 more - 10 failures total)
because that function is *also* the shared path for the deliberate
`JObject(None, cls)` forward-bridge feature, which specifically wants
`cast=true` callers to keep a typed null rather than collapsing to `None`.
Reverted that, and instead fixed only the two reverse-bridge call sites
(`getArgs`/`getKwArgs`) that had no legitimate reason to request the
typed-null behavior for a `null` argument in the first place - `cast=true`
there was only ever meant to skip the `findClassForObject` runtime-type
lookup for a known-exact declared type, not to request typed-null
preservation. **Lesson: `cast` is an overloaded flag with two logically
separate meanings (skip runtime class resolution vs. preserve typed-null
identity) baked into one bool across every caller of
`convertToPythonObject`; a fix motivated by one meaning can silently
break every other caller relying on the other meaning. Grep all callers
and re-run the full test suite (not just the isolation tests) before
trusting a "small" semantic change to a widely-shared conversion
function.**

Tests: 4 new isolation tests + 1 confirming test in
`native/jpype_module/src/test/java/python/lang/PyBuiltInNGTest.java`
(`testGetattrRetrievalDefaultCallInvocation`,
`testEvalRetrievalContextCallInvocation`,
`testGetattrRetrievalContextCallInvocation`,
`testEvalRetrievalDefaultCallInvocation`,
`testContextCallWithNullKwargsInvocation`), all passing post-fix. Full
suites clean: Java NGTest 648/648 (14 skipped, pre-existing/unrelated),
Python `pytest` 1752/1752 (172 skipped), no regressions.

## Reproduction (original, 2026-07-11)

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

This reproduction happened to always go through `PyCallable.call()`'s
default method, which happened to always pass `null` for kwargs - so it
looked like a `getattr()`/`.call()` issue until the isolation step (this
session) tried the missing 4th combination and a bare null-kwargs case,
which is what actually narrowed it to the real cause above.

## Out of scope (unchanged from original scoping)

- No investigation into whether other `python.lang.*` reverse-bridge
  interfaces (beyond `PyDict`-typed kwargs) have other null-handling
  gaps - only the kwargs-null case was reproduced and fixed. The fix
  itself is general (any null object-typed reverse-bridge argument now
  gets real `None`), so other cases are expected to be fixed too, but
  weren't individually re-verified beyond the full test suite passing.
