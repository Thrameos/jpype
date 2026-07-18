# Smuggler: detect and safely reject/convert cross-interpreter Proxy calls

## Status (2026-07-18): step 1 DONE (committed `c6e243a7`, 2026-07-11).
Steps 2-4 (whitelist + thread-pool exchange + primitive fast path) are
fully designed below and **ready to implement next session** - not yet
started. Steps 5-6 (general Python-side transport registry) are designed
but deliberately deferred (see "Scope decision" below) - do not implement
those without a concrete new caller/use case motivating them.
`JPClass::convertToPythonObject`
(`native/common/jp_class.cpp`) already detects a proxy's owning-context
mismatch and raises `RuntimeError` instead of corrupting memory, verified
against a real Python 3.12 own-GIL subinterpreter
(`SubInterpreterNGTest.testSmuggledProxyAcrossInterpretersThrows`). The
exception-unpack edge case (a smuggled proxy inside a Java exception
object being converted inside exception-handling itself) is also fixed,
wrapped in a local try/catch (`JPypeException::convertJavaToPython`,
`native/common/jp_exception.cpp`). Follow-on to
[[jpype_multiphase_init_status]]/`plan/MultiPhaseInit.md` (COMPLETE) -
own-GIL subinterpreter isolation is now real, which is precisely what
makes this bug reachable/dangerous rather than theoretical.

The design below replaced the original "jump the locks" sketch (see
"Superseded original sketch" at the bottom) after a design session -
the load-bearing change is that the exchange never holds two
interpreters' GILs on one thread at any point, by moving the actual
cross-interpreter work onto a dedicated thread pool rather than trying to
sequence acquire/release on the calling thread itself.

**Scope decision (2026-07-18): build steps 2-4 next session, defer step
5.** Steps 2-4 (Java whitelist, thread-pool exchange, primitive fast path
for str/int/float/etc.) are in scope for the *next* session to implement -
this is a bounded, fully-designed task, and it directly serves an
already-shipped feature: `SubInterpreterBuilder.asSupplier()`
([[jpype_subinterpreter_builder_status]]) was explicitly built for a
worker-pool pattern of independent own-GIL subinterpreters, and worker
pools routinely need to hand simple values back and forth between
workers - that's exactly what steps 2-4 close, a real non-speculative
gap. Step 5 (the general pluggable Python-side transport registry for
arbitrary user-registered types) is explicitly **deferred, not
abandoned** - written into this doc as a scoped design so it isn't lost,
but there's no concrete caller needing to move more than primitives yet,
so don't build it until one shows up. Step 6 (serializer-failure handling)
only applies once step 5 exists, so it's deferred alongside it.

## The bug

A `PyJPProxy` (a Java-side dynamic proxy backing a Python callable/object,
`native/common/include/jp_proxy.h`) is created inside one interpreter and
carries a `JPContext* m_Context` plus `PyJPProxy* m_Instance` (the actual
Python target object) that both belong to the interpreter that registered
it. If that Java proxy reference is then invoked from a *different*
interpreter (a second subinterpreter, or main, running under its own GIL
now that per-interpreter-GIL isolation is real per
`plan/MultiPhaseInit.md`), `hostInvoke`/`getArgs`
(`native/common/jp_proxy.cpp`) will touch interpreter A's `PyObject`s and
allocator arena while holding interpreter B's GIL. This is memory
corruption, not just a wrong-answer bug — cross-interpreter object access
without holding the owning interpreter's GIL is exactly the class of
unsafety `Py_MOD_PER_INTERPRETER_GIL_SUPPORTED` isolation exists to
prevent, and JPype's Java-side proxy layer currently has no guard against
it at all.

**Reproduction shape** ("smuggler" name comes from this): register a
`JProxy`-backed Java object in interpreter A, hand the *Java* reference
(not the Python object - the whole point is the Java side has no
interpreter tag on it once it's just a `jobject`) across to interpreter B
(e.g. via a shared static, a JNI global, or just Java code running with
access to both), then call a proxy method on it while executing inside
interpreter B. Today: silent corruption or crash. Desired: detected and
handled.

## Desired behavior (finalized design, 2026-07-18)

1. **Detect** (done, step 1): at the point a Java value is being converted
   into a Python object and turns out to be a proxy owned by a different
   interpreter than the one currently attached
   (`JPClass::convertToPythonObject`), that's a smuggled proxy.

2. **Fast-fail whitelist check - Java-side, no thread hop.** The request
   for a transported exchange starts in Java carrying just the object
   handle and its Java-facing type (`Class<?>`/`JPClass`) - not the Python
   object itself, and no GIL needs to be touched to make this check. A new
   Java-side registry (default entries: `String`, `Integer`, `Long`,
   `Double`, `Float`, `Boolean`, and other simple boxed-primitive types;
   user-extensible via a public register call) is checked synchronously.
   **Not on the list → immediate exception, the existing step-1
   `RuntimeError` path, no thread pool or Python involvement at all.**
   This is deliberately cheap: the common "genuinely not transportable"
   case costs one registry lookup, nothing more.

3. **On the list → dispatch to a dedicated thread pool, never nesting
   GILs.** The detecting thread releases its own current GIL attachment
   (it's mid-`hostInvoke`, already attached to *some* interpreter) and
   submits the exchange as a task to a pool dedicated to these exchanges,
   then blocks on the returned `Future` without holding any interpreter's
   GIL. The pool thread does a clean `JPPyCallAcquire` against the
   *owning* interpreter - never nested under the calling thread's lock -
   so no thread ever holds two interpreters' GILs at once and no AB-BA
   deadlock path exists, regardless of how many concurrent smuggled calls
   are in flight in either direction.

4. **Primitive fast path** (str/int/float/etc. - the built-in whitelist
   entries): skip Python entirely. The pool thread, attached to the owning
   interpreter, extracts the raw value (`PyUnicode_AsUTF8`,
   `PyFloat_AsDouble`, ...) into a plain Java value, hands it across the
   `Future`. The originally-detecting thread reacquires its own
   interpreter's GIL and builds a fresh Python object from that plain
   Java value via the ordinary (already-existing) Java→Python conversion
   path. No serialization format is involved anywhere for these types -
   they have stable, context-free representations already.

5. **DEFERRED - not next session's scope.** General whitelisted types
   (anything a user registers beyond the built-ins) → transport layer
   implemented in Python. The native/Java
   layers stay ignorant of format: they ferry an opaque byte payload and
   nothing more. A Python-side registry (type → serialize/deserialize
   pair) decides how - `pickle`, something custom, whatever the
   registrant wants - the only contract is that a type's own
   deserializer must be able to read whatever its own serializer wrote.
   The pool thread (attached to the owning interpreter) calls the
   registered serializer to get bytes; the calling thread (reacquiring
   its own interpreter's GIL after the `Future` resolves) calls that same
   type's registered deserializer on those bytes to build the fresh local
   object.

6. **DEFERRED alongside step 5** (only applies once step 5 exists).
   Serialization failure is a distinct case from "not whitelisted." If
   a registered serializer itself raises while running on the pool thread
   (a bug in the handler, not a policy rejection), that exception is
   caught right there, on the owning interpreter, and its message is
   extracted as a plain string - reusing the exact same primitive fast
   path from step 4 (never the exception object itself, which would just
   re-trigger the same smuggling problem recursively). That message
   string crosses the `Future` normally; the calling thread then raises a
   new, distinct exception built from that message (not the original
   "not transportable" exception from step 2) - so a broken handler
   surfaces as a clearly different failure mode than a deliberate
   whitelist rejection.

## Open questions - to resolve during next session's implementation (steps 2-4)

- Java whitelist registration call signature: keyed by `Class<?>`
  directly (a `Set<Class<?>>`/`Map<Class<?>, ...>` on a new registry
  class, e.g. `org.jpype.proxy.TransportRegistry` or similar under
  `org.jpype.proxy` alongside `ProxyType`/`ProxyFactory`), with a public
  static `register(Class<?>)` for user additions. Built-in seed list
  confirmed against the existing `JPBoxedType` family
  (`native/common/include/jp_context.h:130-138`, the complete set jpype
  already treats as boxed-primitive round-trippable): `Boolean`, `Byte`,
  `Character`, `Short`, `Integer`, `Long`, `Float`, `Double`, plus
  `String` separately (not itself a `JPBoxedType` - handled by
  `JPStringType`/`_java_lang_String`). `Void` exists in the family too
  but is meaningless as a transportable value - exclude it from the
  built-in seed list.
- Thread pool sizing/lifecycle: dedicated `ExecutorService` scoped to
  the process, sized how (a small fixed/cached pool is almost certainly
  right - this should be a rare path, not a throughput one), shut down
  when (interpreter close? process exit? does it need to survive one
  `SubInterpreter.close()` if other interpreters are still alive - almost
  certainly yes, so likely process-lifetime, not tied to any one
  `Interpreter` instance), and whether it needs its own thread
  name/priority to stay distinguishable from `NativeReferenceQueue`'s
  worker in thread dumps.
- Where exactly the native→Java and Java→native call surfaces live - new
  JNI entry points needed for "acquire owning interpreter, extract
  primitive value, return it" and the reverse "acquire calling
  interpreter, build Python object from plain Java value", callable from
  a pool-thread task. Likely lands in `jp_proxy.cpp` (detection site is
  already there) plus a new native source file for the exchange itself,
  given the scope.
- Whether the calling thread should release *its own* GIL while blocked
  on the `Future` (not just needing the owning interpreter's) so other
  Python threads on the calling interpreter can keep running during the
  round trip - worth doing given `JPPyCallRelease` already exists for
  exactly this shape of "release for a blocking wait, reacquire after."
- Performance cost, now dominated by the thread-pool round trip itself
  (context switch + queue latency) rather than the GIL dance - needs a
  real benchmark once built (extend `SubInterpreterNGTest`'s existing
  cross-interpreter test scaffolding rather than building new harness
  from scratch).

## Open questions - deferred alongside step 5 (not next session)

- Python transport registry's serialize/deserialize pair signature
  (plain functions? a small interface/ABC?), how a user associates a
  Python type with its handler pair, and whether the *Java* whitelist
  entry needs to already know which Python type it maps to or that's
  resolved dynamically off the actual object at serialize time.

## Non-goals

- Not attempting to make arbitrary Python objects transparently shareable
  across interpreters - that's what `plan/MultiPhaseInit.md`'s "Known
  non-goal" section already draws the line at (single shared JVM, not
  full multi-JVM-per-process), and this plan doesn't relitigate that.
- Not persisting migrated objects' identity across calls - every exchange
  produces a throwaway clone, not a rehomed/shared proxy. A registered
  type opting into transport is implicitly promising it's safe to
  duplicate this way; the framework does not attempt to reconcile
  divergent mutable state between the original and any clones.

## Superseded original sketch (2026-07-11, kept for the record)

The original idea was to "jump the locks" directly on the calling
thread - temporarily acquire the owning interpreter's GIL while still
holding (or having just released) the calling interpreter's, perform the
conversion inline, then switch back - with the convertible-type set
limited to an open-ended "strings and other context-free-safe
primitives." This was replaced because sequencing two interpreters' GILs
on one thread reintroduces exactly the deadlock hazard the whole design
exists to avoid (two threads doing the mirror-image sequence would AB-BA
deadlock), and because a Java-side whitelist plus a Python-side pluggable
transport layer is strictly more general than a hardcoded "strings only"
list while still being just as cheap to reject on the common path.
