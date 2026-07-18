# InterpreterPipe: a safe, backpressured stream between two subinterpreters

## Status (2026-07-18): scoped, not started. Depends on
`plan/Smuggler.md` steps 2-4 (Java-side whitelist + primitive fast path)
landing first - this plan is a *consumer* of that conversion primitive,
not a separate one. Design captured here so it isn't lost; do not start
before Smuggler steps 2-4 are implemented, since this plan's `put()`
needs their whitelist gate to stay safe.

## Why this needs to be an official mechanism, not left to users

Own-GIL subinterpreter isolation (`plan/MultiPhaseInit.md`, COMPLETE) is
real now, which means users running multiple independent subinterpreters
will inevitably want to move a stream of values between them - a worker
pool pattern feeding results back to a coordinator, a producer/consumer
split across two isolated interpreters, etc. (`SubInterpreterBuilder
.asSupplier()` was built exactly for this kind of multi-subinterpreter
worker setup - [[jpype_subinterpreter_builder_status]].)

If JPype doesn't provide a safe way to do this, users will build their
own - and a **pure-Python** attempt (a `queue.Queue`/`threading.Lock`
shared across a smuggled reference, or any hand-rolled polling loop) can
**definitely deadlock**: CPython's own synchronization primitives have no
concept of releasing a *different* interpreter's GIL while blocked, so
two own-GIL interpreters coordinating through a pure-Python primitive can
easily reach a state where each is blocked waiting on the other while
each is still holding its own GIL - exactly the AB-BA shape
`plan/Smuggler.md`'s whole design exists to avoid, just recreated one
layer up by a user who has no way to know it's unsafe. Providing a real,
safe mechanism closes that gap before users go looking for their own.

## The key insight: this is nearly free given the existing forward-bridge convention

Checked directly (2026-07-18, during the design conversation this plan
came out of): `JPPyCallRelease` - release the GIL before a JNI call into
Java, reacquire after - is already the **universal, unconditional**
convention for every forward-bridge call site in this codebase
(`jp_class.cpp`, `jp_inttype.cpp`, `jp_booleantype.cpp`,
`jp_typemanager.cpp`, `jp_proxy.cpp`, and more - every primitive-type
conversion path already does this). This means a thread calling into a
**plain Java object** from Python already holds no Python GIL for the
duration of that call, for free, with zero new native code - that's just
how every existing Python→Java call already behaves.

Consequence: a `java.util.concurrent`-backed bounded queue, exposed to
Python on both interpreters as an ordinary Java object (not a proxy - a
real ordinary Java instance, callable via the normal forward bridge), gets
correct backpressure and thread-safe hand-off entirely from Java's own
concurrency primitives. A thread blocked in `queue.put()` (queue full) or
`queue.take()` (queue empty) is *already* not holding any interpreter's
GIL, because it's inside an ordinary Java call. Unlike `plan/Smuggler.md`'s
proxy-mismatch case - which is discovered *mid-call*, already deep inside
a JNI trampoline attached to some interpreter's GIL, with no clean "about
to call Java" boundary to hang a release on, hence needing a dedicated
thread pool - this is a deliberately-designed call from Python code that
already knows it's calling Java. **No thread pool, no `Future`, no new
native GIL-choreography code should be needed for the pipe mechanism
itself.**

## Design

1. **A plain Java class** (tentatively `org.jpype.io.InterpreterPipe` -
   naming TBD, could also live under `org.jpype.proxy` alongside the
   transport registry it depends on) wrapping a bounded
   `java.util.concurrent.ArrayBlockingQueue<byte[]>` (or similar -
   `BlockingQueue` interface, concrete choice TBD). Payload is bytes, not
   arbitrary objects - see point 1a below.

1a. **Presented to Python as a duck-typed byte stream, not raw
   `put()`/`take()`.** Decided 2026-07-18: this follows the exact
   pattern `python.io` already uses for its reverse-bridge adapters
   (`PyIOInputStream`/`PyIOOutputStream`, see
   [[jpype_io_buffering_status]]) - a Python-facing wrapper object that
   looks and behaves like a native Python stream (readable/writable,
   `read()`/`write()`/`readline()`-shaped, whatever subset of the
   `io` duck-type contract makes sense for a pipe specifically), backed
   by the plain Java `BlockingQueue` object underneath. Resolves what
   was an open question below (python.io-style wrapper - yes, byte-
   stream shaped, not a generic-object queue). Narrowing the payload to
   bytes also simplifies the whitelist-gate question (see point 3): a
   byte pipe only ever needs to move `byte[]`/`bytes`, which is already
   the built-in primitive fast path, so this pipe likely doesn't need to
   wait on Smuggler step 5 (the general pluggable transport registry) at
   all - `String`/boxed-primitive-level conversion already covers it.

2. **Shared across interpreters the ordinary way**: a single Java
   instance handed to Python code running in two (or more) different
   subinterpreters - e.g. via a shared static, or passed through
   whatever mechanism the user already uses to bootstrap cross-
   interpreter Java references. This is exactly the "smuggler
   reproduction shape" from `plan/Smuggler.md`, but deliberately and
   safely so here, because of point 3 below.

3. **Payload conversion reuses `plan/Smuggler.md`'s transport primitive,
   not a separate one.** `put()` must gate its argument through the same
   whitelist (`org.jpype.proxy.TransportRegistry` or wherever Smuggler
   steps 2-4 land it) before enqueueing - **never a raw, unconverted
   Python object reference (a proxy)**. If `put()` accepted an
   unconverted proxy, the queue would just be a new, convenient way to
   reproduce the original smuggling bug (a Java-side handle with no
   interpreter tag, later dequeued and touched by a different
   interpreter's GIL with no protection). This is the one correctness
   requirement that doesn't come for free from the forward-bridge
   insight above - it has to be built in explicitly.

4. **Ordinary conversion on both ends, no new dance needed.** On the
   producer side, `put(value)`'s argument conversion (Python→Java) is
   the same conversion Smuggler steps 2-4 already do for primitives,
   already running under the producer's own GIL as part of the ordinary
   call. On the consumer side, `take()`'s return value conversion
   (Java→Python) is the same ordinary conversion, already running under
   the consumer's own GIL as part of the ordinary call return. Nothing
   sitting *inside* the queue is ever a raw PyObject* - by construction,
   every item was already converted to an interpreter-agnostic Java
   value before it was enqueued.

## Open questions - not yet designed

- Exact class placement/name for both the underlying `BlockingQueue`
  wrapper and the Python-facing duck-typed stream adapter (point 1a) -
  likely mirrors `python.io`'s existing split between a plain Java
  stream class and its customizer-registered Python-facing wrapper, see
  [[jpype_io_buffering_status]].
- Which `io` duck-type surface to implement - raw
  `read(size)`/`write(bytes)`/`close()` only, or the fuller
  `readline()`/iteration-protocol surface `python.io`'s existing
  adapters already support? Bytes-only payload (point 1a) means this
  doesn't need Smuggler step 5 (the general transport registry) at all -
  `byte[]`/`bytes` is squarely inside the built-in primitive fast path,
  so this plan is unblocked as soon as Smuggler steps 2-4 land, no
  further dependency.
- Bounded capacity: fixed default, configurable per-pipe-instance
  constructor argument, or both?
- Close/termination semantics: an explicit `close()` that unblocks any
  waiter with an exception (like `python.io`'s stream-closed handling),
  a poison-pill sentinel value, or both? What happens to a consumer
  blocked in `take()` if the *producer's* interpreter closes
  (`SubInterpreter.close()`) without an explicit pipe close?
- Multi-producer/multi-consumer support, or strictly single-producer/
  single-consumer for v1?
- Exception propagation shape: if `put()`'s conversion gate rejects a
  value (not on the whitelist), does that throw synchronously on the
  producer's own thread (most likely, no cross-interpreter round trip
  needed since the rejection is a pure Java-side check per Smuggler step
  2), or does it need any special handling given it's now inside this
  pipe's own `put()` rather than a smuggled proxy call?

## Non-goals

- Not a general message bus or pub/sub system - a simple bounded
  producer/consumer primitive between two (or a small fixed set of)
  interpreters in the same JVM process.
- Not for cross-process communication - stays inside
  `plan/MultiPhaseInit.md`'s "single shared JVM" scope, same boundary
  `plan/Smuggler.md`'s non-goals already draw.
- Not attempting to make this safe for payloads outside Smuggler's
  transport whitelist - if a type isn't safe to migrate one-shot via
  Smuggler, it isn't safe to stream through this pipe either; this plan
  doesn't relitigate that boundary.
