# Bulk Java-array -> caller-provided Python buffer copy (ArrayRegion-style)

## Status (2026-07-11): DONE. Branch `array-region-copy`.

Implemented and tested. `JArray.copyInto(dest)` (classic side only, per the
scoping below) bulk-copies a Java primitive array into any Python object
supporting the buffer protocol. Benchmarked against the existing best
alternative (`dest[:] = np.asarray(ja)`, zero-copy export + numpy bulk
assign): consistent **1.4x-2.6x** speedup across sizes 10 to 1,000,000
elements (bigger relative win at small/medium sizes, where per-call
overhead — the extra Python object, the pin/release pair — dominates;
narrows toward ~1.4x at 1M elements where raw memcpy throughput dominates
and the two paths converge). Not in the 33x-1164x class the IO-buffering
work found (see `[[jpype_io_buffering_status]]`) — that win came from
removing *per-element* bridge crossings, whereas here both the old and new
paths were already bulk operations; this feature only trims fixed
per-call overhead, not algorithmic complexity.

**One deliberate simplification vs. the original design sketch below**: no
dtype conversion. This is a same-dtype bulk copy only (destination
`itemsize` must exactly equal the source Java type's item size) — no
`getConverter`/`pack` round trip. That collapses the "per-type templated
function" the design sketch anticipated into one plain, non-template
helper (`copyArrayToBuffer` in `jp_primitive_accessor.h`) since there's no
per-type value conversion left to do, only a raw `memcpy` per element. If
implicit dtype casting is ever wanted, that's a separate follow-on, not
scoped here.

Implementation:
- `JPArray::copyInto` (`native/common/jp_array.cpp`) — dispatches between
  two tiers exactly as designed: fast path (`m_Step == 1` source, `PyBuffer_IsContiguous(&view, 'C')`
  destination) reuses the existing `copyElements` virtual directly, no new
  per-type code. General path (stepped source and/or non-contiguous/N-D
  destination) calls the new `copyArrayToBuffer` free function
  (`jp_primitive_accessor.h`), which pins the Java array via
  `GetPrimitiveArrayCritical` and walks the destination `Py_buffer`'s
  shape/strides via the existing `getBufferPtr`/odometer-increment pattern
  from `convertMultiArray`, reversed.
- Turned out array *slicing* (non-unit `m_Step`) needed the general path
  too, not just non-contiguous destinations — the original design sketch's
  "no general dtype conversion" framing undersold this; scope grew slightly
  from "N-D/non-contiguous destination only" to "stepped source or
  non-contiguous/N-D destination," which is why `m_Slice` is no longer
  rejected up front.
- Python entry point: `PyJPArray_copyInto` (`native/python/pyjp_array.cpp`),
  `arr.copyInto(dest)`.
- Tests: `test/jpypetest/test_array.py`, 9 new cases (`testCopyInto*`) —
  contiguous, stepped source, non-contiguous destination, N-D contiguous
  destination, stepped destination, multiple primitive types (int/bool/
  byte), size-mismatch (`ValueError`), dtype-mismatch (`TypeError`),
  non-primitive array (`TypeError`). Full suite: 1747 passed, 172 skipped,
  no regressions.

## Original scoping (below, kept for context — see Status above for what actually shipped)

Raised as a question during the numpy buffer-protocol discussion
(`plan/archive/NumpyBufferBench.md`): JNI has a "you provide the
destination, JVM copies into it" family (`Get<Type>ArrayRegion`/
`Set<Type>ArrayRegion`, and `GetPrimitiveArrayCritical` used as a pinned
source/destination) that is a fundamentally different shape from Python's
buffer protocol, which is "I (the exporter) provide a live pointer, you
accept it." jpype's *exported* Java-array-as-Python-buffer path
(`PyJPArray_getBuffer`, `native/python/pyjp_array.cpp:442-473`) is entirely
the accept-a-live-pointer shape. There is no operation for bulk-copying a
Java array's contents into a Python buffer the *caller* already owns (e.g.
a preallocated numpy array, an existing `bytearray`) without first
exporting/pinning the Java array itself as a Python buffer object.

## Existing precedent (both already in production — read these before writing anything new)

Two directions already exist, and between them they contain every piece
this feature needs. Nothing here is a new algorithm; this is an assembly
job.

**1. `convertMultiArray`** (`native/common/include/jp_primitive_accessor.h:88-160`)
— Python buffer (arbitrary shape/strides) as *source* -> **newly
constructed** Java array as destination. Walks the source `Py_buffer` via
`buffer.getBufferPtr(indices)` element-by-element, writing into a
`GetPrimitiveArrayCritical`-pinned pointer over a freshly allocated Java
array, and commits/re-acquires per innermost-dimension run
(`ReleasePrimitiveArrayCritical(..., JNI_COMMIT)`). This is the template
for the *general N-dimensional / non-contiguous destination* stride-walk
loop — reverse the roles (pin the Java array as source, walk the
destination `Py_buffer`'s strides instead of the source's).

**2. `copyElements` + `JPArrayView`'s jagged-array constructor**
(`native/common/jp_bytetype.cpp:279-284` and sibling `jp_<type>type.cpp`
files; called from `native/common/jp_array.cpp:209-215`) — Java array as
*source* -> **caller-owned contiguous memory** as destination, using the
plain `Get<Type>ArrayRegion` bulk call (`frame.GetByteArrayRegion((jbyteArray)
a, start, len, b)`), no pinning/critical-section at all. This is already
used today to flatten a jagged multi-dimensional Java array into one
`Py_buffer`-exportable block of memory, one bulk `ArrayRegion` call per
row (`m_Memory` in `JPArrayView`, allocated with `new char[sz]` at
`jp_array.cpp:203`). **This is the closer precedent for the common case**:
it already does "Java array -> caller-provided/owned flat memory" via the
region-call family this doc originally proposed choosing between — the
only gap is that `JPArrayView` always allocates its *own* destination
memory (`m_Owned = true`) rather than accepting a `Py_buffer` the Python
caller already owns.

Both are virtual dispatch through `JPPrimitiveType` (`getView`/
`releaseView`/`copyElements`/`newMultiArray` at
`native/common/include/jp_primitivetype.h:38-46`), so the per-type
boilerplate pattern (one `.cpp` per primitive type: `jp_booleantype.cpp`,
`jp_bytetype.cpp`, `jp_chartype.cpp`, `jp_doubletype.cpp`,
`jp_floattype.cpp`, `jp_inttype.cpp`, `jp_longtype.cpp`,
`jp_shorttype.cpp`) is already established for whatever new virtual we
add.

## Goal

A bulk copy operation: given a Java array and a target Python object that
already exports the buffer protocol (e.g. a preallocated numpy array, a
`bytearray`, an existing `PyMemoryView`'s backing buffer), copy the Java
array's contents directly into that buffer's memory, honoring the
*destination* buffer's shape/strides — no intermediate `PyMemoryView`
export/pin of the Java array itself, no intermediate full-array Python
object materialization.

## Design decision (resolved — two-tier, not either/or)

The original doc framed `Get/Set<Type>ArrayRegion` vs
`GetPrimitiveArrayCritical` as a single either/or choice. Having found
precedent #2 above already live and tested, the right shape is **two
tiers sharing one virtual dispatch**, because the two JNI families are
each already the natural fit for a different destination shape:

- **Fast path — destination is 1-D and C-contiguous** (the common numpy
  case: `np.empty(n, dtype=...)`, a plain `bytearray`, a fresh
  `PyMemoryView`): one `Get<Type>ArrayRegion` call, direct Java-array ->
  destination-buffer memory, no pin, no per-element loop, no critical
  section. This is *exactly* `copyElements`'s existing signature
  (`jp_bytetype.cpp:279`) reused verbatim — detect the contiguous case
  from `Py_buffer.strides == nullptr` or `PyBuffer_IsContiguous(&view,
  'C')`, then call `componentType->copyElements(frame, source, 0, len,
  view.buf, 0)` directly. No new per-type code needed for this tier.
- **General path — destination is N-D or non-contiguous** (a transposed
  or strided numpy view, a sliced buffer): mirror `convertMultiArray`'s
  stride-walk exactly, but reversed — `GetPrimitiveArrayCritical`-pin the
  Java array as the *source* pointer, walk the *destination* `Py_buffer`'s
  `shape`/`strides` via the same indices-vector traversal
  (`jp_primitive_accessor.h:124-158`), releasing the source pin once at
  the end with `JNI_ABORT` (read-only access to the Java side — no
  `JNI_COMMIT` needed since nothing is written back to the Java array).

Both tiers are needed: the fast path matters because most real numpy
interop is a plain contiguous array and a per-element critical-section
walk would be needlessly slow for the dominant case (this is the same
"per-element bridge crossing" cost class flagged as the byte-read
bottleneck in the IO buffering benchmark, see `[[jpype_io_buffering_status]]`
memory — a bulk region call must be preferred whenever the shape allows
it). The general path must exist too, or the feature silently fails to be
a *real* generalization of "copy into a buffer I own" and degrades back to
"copy into a contiguous buffer I own," which is not the stated goal
(non-contiguous destination handling is explicitly the point — see
Testing below).

No new JNI Critical Section hazards beyond what `convertMultiArray`
already carries (no GC-triggering or blocking calls while pinned) — same
constraint, already respected by the existing code being mirrored.

## Concrete plan

1. **New virtual on `JPPrimitiveType`** (`jp_primitivetype.h:44`, sibling to
   `copyElements`): something like
   `virtual void copyToBuffer(JPJavaFrame&, jarray source, jsize start, jsize len, JPPyBuffer& dest) = 0;`
   implemented once as a shared template (not per-type virtual body) —
   `copyElements` is already virtual per-type because the JNI
   `Get<Type>ArrayRegion` call is type-specific, but the *dispatch logic*
   between fast/general path does not need to be: write one templated
   free function (sibling to `convertMultiArray` in
   `jp_primitive_accessor.h`), parameterized on `type_t` the same way,
   that:
   - checks destination contiguity and calls `copyElements` for the fast
     path,
   - otherwise pins the source via `GetPrimitiveArrayCritical` and walks
     `dest`'s shape/strides for the general path,
   then each `JPXxxType::copyToBuffer` override is a one-line call into
   that template, exactly mirroring how `newMultiArray` overrides
   (`jp_bytetype.cpp:291-298`) are one-line calls into
   `convertMultiArray`.
2. **Entry point** — add to the classic-side `JArray`
   (`native/common/jp_array.h`) a method
   `void copyInto(JPJavaFrame& frame, PyObject* dest);` that validates
   `dest` supports the buffer protocol (`PyObject_CheckBuffer`), builds a
   `JPPyBuffer(dest, PyBUF_WRITABLE | PyBUF_FULL_RO)`, checks
   itemsize/format compatibility the same way `setArrayRange` does
   (`jp_bytetype.cpp:194`, `getConverter(view.format, itemsize, code)`
   as in `convertMultiArray`), and dispatches to
   `componentType->copyToBuffer(...)`. Wire it to Python as a new method
   on the classic array wrapper: `PyJPArray_copyInto`
   (`native/python/pyjp_array.cpp`, sibling to `PyJPArray_getItem` at
   line 422) exposed as `arr.copyInto(dest)` — placement chosen because
   this is squarely a classic-side (Python calling into a Java array)
   feature; the reverse-bridge (`python.lang`) direction has no existing
   Java-side array wrapper type to hang this off (checked: no
   `python.lang.PyArray`/similar exists — only `PyByteArray`,
   `PyBytes`, `PyMemoryView`), and the driving use case (numpy interop,
   `plan/archive/NumpyBufferBench.md`) is classic-side only. Do not
   invent a reverse-bridge entry point speculatively; add one only if a
   concrete reverse-bridge need shows up later.
3. **Reuse `getConverter`/dtype-mismatch handling** exactly as
   `convertMultiArray` does — do not reinvent format-string matching.

## Testing

- Extend `plan/archive/NumpyBufferBench.md`'s test bench (or add a sibling
  `PyArrayCopyIntoNGTest`/benchmark): allocate a numpy array up front
  (`np.empty(shape, dtype=...)`), copy a same-shaped Java array into it via
  `copyInto`, confirm values match — both:
  - a contiguous destination (exercises the fast path — assert only a
    single `Get<Type>ArrayRegion`-equivalent call happens, not a
    per-element loop; a call-count spy or a debug counter is enough),
  - a non-contiguous destination (transpose or step-sliced numpy array
    view) to prove the general path's destination-stride handling
    actually works, not just the contiguous case.
- Confirm no intermediate full-array Python object or `PyMemoryView`
  export of the Java array is created during the copy (this is the actual
  point of the feature — avoid the pin-and-hold-a-live-pointer path
  entirely for a one-shot bulk copy). A reference-count/leak check on the
  Java array wrapper before/after `copyInto` is sufficient evidence.
- Dtype-mismatch case: destination buffer format incompatible with the
  Java array's component type should raise the same `TypeError` shape as
  `convertMultiArray`'s `getConverter(...) == nullptr` path.
- Size-mismatch case: destination buffer too small/large for the source
  array length should raise `ValueError`, matching `setArrayRange`'s
  existing `"mismatched size"` check (`jp_bytetype.cpp:188`).

## Open questions

- Should this also support the reverse (Python buffer -> existing Java
  array, i.e. "SetArrayRegion-style" bulk write into a Java array the
  caller already has, as opposed to `convertMultiArray`'s
  construct-a-new-array path, and as opposed to `setArrayRange`'s
  existing sequence-assignment path which only supports 1-D)? Not yet
  scoped; flag as a likely follow-on once this lands, not part of this
  doc's initial goal.
- `JPArrayView`'s jagged-array constructor (`jp_array.cpp:161-229`)
  could arguably be rewritten to call the new shared
  fast/general-path template instead of its own hand-rolled per-row
  `copyElements` loop, now that a more general version exists — worth
  revisiting once the new code is written and tested, but not required
  for this feature to land (don't refactor working, tested code as a
  drive-by; separate follow-on if desired).
