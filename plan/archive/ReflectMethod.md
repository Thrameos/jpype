# java.lang.reflect.Method -> single-signature Python callable

## Status (2026-07-17): DONE

Implemented on branch `reverse`: `TypeManager.methodFromReflect(Method)`
(Java, cached per-`Method`), `JPTypeManager::methodFromReflect` (native
JNI bridge), `_jpype._reflectMethod` (module-level entry point wrapping
the resulting `JPMethodDispatch*` via `PyJPMethod_create`), and a
`toPython()` customizer on `java.lang.reflect.Method`
(`jpype/_jreflect.py`) following the same pattern as the `java.io`
customizers in `jpype/_jio.py`. Instance methods take the instance as
their first call argument (`f(instance, *args)`) since there's no
attribute to bind `self` to — this works for free because
`JPMethod::matches()` already treats argument 0 as the instance whenever
the method itself is non-static, regardless of the `callInstance` flag
passed to `invoke()`; no new C++ dispatch logic was needed, confirming
the design section below. Tests: `test/jpypetest/test_jreflect.py` (5/5
pass; full suite 1752 passed/172 skipped, no regressions).

**Runtime fast path tried and reverted.** Prototyped a genuine
skip-the-search fast path in `JPMethodDispatch` (an `m_Single` flag
short-circuiting `findOverload()`'s cache lookup and search/ambiguity
loop straight to a single `matches()` call, plus a `trackHash` flag to
skip `JPMethodMatch`'s per-argument hash arithmetic). Benchmarked in
isolation via `_matches()` (dispatch resolution only, no JNI invoke): the
skip saved on the order of 1-5ns/call out of ~250-850ns total even
against a 13-overload method (`StringBuilder.append`), because JNI frame
setup and argument marshaling dominate the per-call cost, not the
overload search - jpype methods rarely have enough overloads for the
search itself to matter. Reverted the `m_Single`/`trackHash` machinery as
not worth the added complexity in `jp_methoddispatch.{h,cpp}`/
`jp_match.h`/`jp_classhints.cpp`; kept the `raiseNoMatch()` extraction
(deduplicated the "no matching overload" error-message block, which
existed twice verbatim) since that's a clean win independent of the
performance question. `methodFromReflect()`'s value is the single-
signature *binding* itself (skips ambiguity entirely, useful for e.g. a
`java.lang.reflect.Method` obtained dynamically where the call site
can't otherwise pin an overload) - not a measurable per-call speedup.

## Status (2026-07-11): scoped, not started

Follow-on from prior-art survey discussion (see `jpype_gold_standard_mission`
memory). jpype's normal method binding always goes through
`JPMethodDispatch` (`native/common/jp_methoddispatch.cpp`) — the
"overload-dispatch monster" that holds *all* same-named overloads
(`m_Overloads`) and does a runtime match search (`findOverload`) across all
of them on every call. There is currently no way to get a Python callable
bound to exactly one already-known Java method/signature (e.g. one obtained
from `java.lang.reflect.Method`, via `Class.getDeclaredMethod(...)` or a
dynamic lookup) that only type-checks against that single signature,
skipping the overload search entirely.

## Design (confirmed feasible against existing code, no new dispatch logic needed)

The existing single-overload construction path already does exactly this
per-overload, it's just always called with the *full* candidate list for a
class. In `TypeManager.java`:

- `createMethodDispatch(ClassDescriptor desc, String key, LinkedList<Method> candidates)`
  (`TypeManager.java:754`) filters `candidates` down to the ones matching
  `key`, resolves overload precedence (`MethodResolution.sortMethods`),
  builds one native `JPMethod` per candidate via `typeFactory.defineMethod`
  (`:775`, wraps `Java_org_jpype_manager_TypeFactoryNative_defineMethod`,
  `jp_typefactory.cpp:434`), then bundles them into one
  `JPMethodDispatch` via `typeFactory.defineMethodDispatch` (`:784`, wraps
  `Java_org_jpype_manager_TypeFactoryNative_defineMethodDispatch`,
  `jp_typefactory.cpp:127`).
- `PyJPMethod_create(frame, JPMethodDispatch*, instance)`
  (`native/python/pyjp_method.cpp:419`) wraps *any* `JPMethodDispatch*` as
  a Python callable, regardless of how many overloads it holds.

So: calling `createMethodDispatch` (or a copy of its core logic) with a
**singleton list containing exactly one `java.lang.reflect.Method`** already
produces a `JPMethodDispatch` with one overload — `JPMethod::matches()`
(`jp_method.h:47`) then only ever checks that one signature, and
`PyJPMethod_create` wraps it exactly like any other bound method. No new
C++ dispatch/matching code, no new `PyJPMethod` variant — this is a
one-element-list call into machinery that already exists and is already
exercised by every normal method bind.

## What's actually new

- A small Java-facing entry point, e.g.
  `TypeManager.methodFromReflect(ClassDescriptor desc, java.lang.reflect.Method m)`
  (or equivalent placement — check `plan/Naming.md` conventions), that:
  1. Resolves/loads the `ClassDescriptor` for `m.getDeclaringClass()` (must
     already be (or become) known to jpype's type system — should already
     work for any class jpype has touched, since class registration happens
     automatically).
  2. Calls the existing single-overload-dispatch construction path with
     `candidates = List.of(m)`.
  3. Wraps the resulting `JPMethodDispatch*` the same way normal bound
     methods are exposed to Python (needs to trace exactly how `desc.methods`/
     `desc.methodDispatch` results normally reach a Python-visible attribute,
     e.g. via `_jclass.py`'s customizer/attribute-binding path, vs. this
     needing to hand back a raw callable object directly without being
     attached to a class attribute — likely the latter, since the whole
     point is a standalone callable, not a class member).
- Python-facing surface: something like
  `jpype.methodFromReflect(reflectMethodObj) -> Callable`, taking an actual
  `java.lang.reflect.Method` Java object (already accessible from Python
  today via normal reflection: `cls.class_.getDeclaredMethod(...)`) and
  returning a plain callable bound to that one signature. Exact placement
  (`jpype/_jmethod.py`? a new top-level function in `_core.py`?) TBD.

## Open questions

- Does the returned callable need `self`/instance binding the way
  `PyJPMethod_get` (`pyjp_method.cpp:76`) does for normal bound methods
  (i.e. `obj.method` binds `self` via descriptor `__get__`), or is it
  always used in an explicit `(instance, *args)` call form since it's not
  attached as a class attribute? Leaning toward requiring the instance as
  the first call argument for instance methods, matching
  `JPMethodDispatch::invoke(frame, args, instance)`'s existing `instance`
  flag, rather than inventing a separate binding protocol.
- Should this go through `ClassDescriptor`/`TypeManager`'s normal caching
  (so calling it twice for the same `Method` reuses the same native
  `JPMethodDispatch`), or is a fresh one-off construction per call
  acceptable given it's expected to be used to build a long-lived callable
  once and reuse it many times, not called in a hot loop itself?
- Confirm `MethodResolution.sortMethods` behaves sanely (or is skippable)
  for a singleton list — it should be a no-op precedence resolution with
  only one candidate, but verify at implementation time rather than assume.

## Testing

- Given a class with an overloaded method (e.g. `Arrays.asList` or a test
  fixture with 2+ overloads), obtain one specific
  `java.lang.reflect.Method` via `getDeclaredMethod` with an explicit
  parameter-type list, build a callable from it, and confirm: (a) it
  invokes correctly with matching args, (b) it does NOT silently accept
  args that would only match a *different* overload of the same name
  (proving it's not silently falling back to full dispatch).
- Static and instance method cases.
- Confirm repeated calls perform comparably to a normal bound-method call
  (no per-call overload search overhead) — this is the actual point of the
  feature, so a basic before/after timing sanity check (not a strict
  benchmark) is worth including.
