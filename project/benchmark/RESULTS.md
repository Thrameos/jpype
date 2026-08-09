# JPype performance report, 2026-08-09

## 1. Scope and methodology

This report compares jpype's call/conversion performance against three
alternative Python-Java bridges (jpy, jep, pyjnius) and, where no
alternative exists for a given operation, against jpype's own prior
behavior. Each section below covers one topic area: methodology specific
to that area, a table built to show trends and problem spots at a glance,
and an analysis that ends in an explicit call -- **fix**, **architectural
tradeoff (don't chase)**, or **no gap**. Section 9 consolidates those calls
into a single ranked list.

**Common methodology.** ns/call, best-of-5 trials, standard
(non-instrumented) build, run inside a disposable venv per this repo's
CLAUDE.md. Iteration count scales down as per-call cost grows
(`n = max(20, 5_000_000 // total_elements)`, `n // 10` warmup), so the
largest array sizes (~1,000,000 elements) run at only 20-50 timed
iterations -- called out explicitly wherever it affects confidence in a
specific number. jep runs on Python 3.10 (this checkout's only working
native build) and embeds Python inside the JVM, the reverse of
jpype/jpy/pyjnius's architecture, so its numbers carry extra uncertainty.
Every number in every table comes from an actual recorded run of the
corresponding script in `project/benchmark/{jpype,jpy,jep,pyjnius}/*.py`
-- none are hand-transcribed or extrapolated. See
`project/benchmark/README.md` to reproduce.

**Coverage note.** The library comparison is **int-only** throughout --
jpy/jep/pyjnius were never re-benchmarked across long/float/double. Where
a section also breaks jpype down by element type (arrays), that type
breakdown is jpype-only; say so again inline at that point rather than
letting it get lost.

## 2. Scalars, boxing, strings, object identity

**Methodology.** Single-call round trips: a static method call
(`Math.max`, `Math.sqrt`), a boxed-object construction
(`new Integer`/`new Double`), a `String` round trip, and passing/returning
a plain `Object` reference. ns/call, best-of-5.

| operation | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| `Math.max(int,int)` | 750 | 353 | 1299 | 1276 |
| `new Integer(int)` | 840 | 479 | 1257 | 7403 |
| `Math.sqrt(double)` | 690 | 393 | 645 | 497 |
| `new Double(double)` | 918 | 506 | 1378 | 6765 |
| `new String` + `.toString()` | 1022 | 927 | 2512 | 22349 |
| `Object` identity (arg + return) | 995 | 730 | 1971 | 3763 |

**Trends.** jpy is fastest on every row, by 1.6-2.1x on the numeric rows
and up to ~9x on boxed-`Integer`. jpype beats jep on every row except
`Math.sqrt` (7% slower, likely noise) and beats pyjnius everywhere except
`Math.sqrt`. pyjnius is the clear problem spot: 6-22x slower than jpype on
every boxed/string row.

**Analysis.** Confirmed by reading jpy's C source
(`jpy_jtype.c`/`jpy_jmethod.c`), not inferred from timing: jpy's matchers
here aren't cutting correctness corners, they're architecturally leaner
-- fewer abstraction layers, no general-purpose `JPConversion` chain to
walk. jpype pays a real, deliberate cost for breadth (implicit numeric
widening, functional-interface duck typing, hint-based custom conversions)
that jpy's narrower binding doesn't support. **Call: architectural
tradeoff, don't chase.** pyjnius's gap was not source-level investigated
(flagging it, not attributing it) and is a pyjnius problem, not a jpype
one -- no action on jpype's side.

## 3. Method dispatch and proxy callbacks

**Methodology.** A 16-overload static method called two ways
(monomorphic: same argument type every call; polymorphic: alternating
types, forcing re-resolution), and a Java-calls-Python proxy callback with
an established binding, `int` and `Object` argument variants. ns/call,
best-of-5.

| operation | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| dispatch, overload x16, monomorphic | 686 | 370 | 4454 | 3858 |
| dispatch, overload x16, polymorphic | 925 | 456 | 4558 | 4000 |
| proxy callback, `int` arg | 2655 | N/A\* | 2240 | 39412\*\* |

\* jpy's `PyObject.createProxy()` didn't produce a usable object in this
checkout -- a jpy-side issue, not a benchmark gap.

\*\* pyjnius: only the `int`-arg case is measured. The `Object`-arg case
reliably crashes the JVM with a `SIGSEGV` in `jni_GetObjectClass` on a
null argument (reproduced independently three times against a fresh
build). Even the non-crashing (non-null) case is wrong:
`invokeObjectCallback` silently returns `None` instead of the Python
callback's actual return value. Both are pyjnius bugs, not benchmark
issues.

**Trends.** jpy is 1.9-2.5x faster on dispatch; jpype beats jep and
pyjnius on dispatch by 4-6x. Proxy: jep is 16% faster than jpype (likely
its reversed embedding direction -- Java-calls-Python is jep's native
direction, not a re-crossing), pyjnius is ~15x slower and has two
correctness bugs on top of the speed gap.

**Analysis.** jpype's dispatch lead over jep/pyjnius comes from a
single-slot overload cache plus the `findJavaConversion` cache (Section
2's work); each of jep's/pyjnius's per-call candidate scans re-does work
jpype now caches. The gap to jpy on dispatch is the same architectural
tradeoff as Section 2. **Call: no gap vs. jep/pyjnius; architectural
tradeoff vs. jpy, don't chase.** The pyjnius `Object`-arg crash and silent
wrong-return bug are real findings worth filing upstream against pyjnius,
but are not jpype action items.

**numpy scalar arguments** (not a speed comparison, a coverage one): jpype
correctly dispatches `Math.max` for all four numpy scalar types
(`int32`/`int64`/`float32`/`float64`). jpy and pyjnius both fail on
`np.int32`/`np.int64`/`np.float32` (jpy: "ambiguous ... too many matching
overloads"; pyjnius: "No static methods called max ... matching your
arguments") because their per-type fast dispatch has no fallback for
`int`/`long` parameters, unlike their `float`/`double` matchers. **Call:
no gap -- jpype is already ahead here.**

## 4. Array push, flat (1D)

**Methodology.** `sumIntArray(source)` and its long/float/double
counterparts, sweeping size (100/1k/10k/100k) and two source kinds: a
plain Python list, and a `Py_buffer`-backed object (numpy). ns/call,
best-of-5. The jpype-vs-alternatives columns are int-only; the type
breakdown beneath is jpype-only.

**list->array push, jpype vs. alternatives (int):**

| size | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 100 | 3,018 | 1,313 | 1,939 | 3,068 |
| 1,000 | 19,695 | 8,294 | 9,574 | 25,932 |
| 10,000 | 194,631 | 78,342 | 86,629 | 268,303 |
| 100,000 | 1,829,617 | 774,569 | 840,301 | 4,520,089 |

**buffer->array push, jpype vs. alternatives (int; pyjnius has no
`buffer->array` at all -- see Section 6):**

| size | jpype | jpy | jep |
|---:|---:|---:|---:|
| 100 | 1,336 | 540 | 882 |
| 1,000 | 3,088 | 951 | 1,331 |
| 10,000 | 21,763 | 5,927 | 6,506 |
| 100,000 | 249,364 | 48,833 | 49,781 |

**Trends.** jpy is 2.3-3.9x faster than jpype on `list->array`, 4.5-5.7x
faster on `buffer->array`, and the gap widens with size on both. jep sits
between jpype and jpy. pyjnius is 1.0-2.5x slower than jpype on
`list->array` (worse at large size) and has no buffer path to compare.

**jpype by element type** (`list->array`, jpype-only):

| size | int | long | float | double |
|---:|---:|---:|---:|---:|
| 100 | 3,018 | 5,353 | 5,383 | 5,398 |
| 1,000 | 19,695 | 42,318 | 44,426 | 44,367 |
| 10,000 | 194,631 | 407,522 | 427,915 | 431,812 |
| 100,000 | 1,829,617 | 4,090,324 | 4,175,308 | 4,167,789 |

**Analysis.** Two separate gaps, confirmed by source, not conflated:

- **jpype-vs-jpy**: jpy's array matching doesn't inspect elements before
  committing to a conversion; jpype validates every element up front to
  support correct Java-style overload disambiguation. **Call:
  architectural tradeoff, don't chase** -- closing it would mean giving up
  element-level validation jpy doesn't do.
- **int-vs-long/float/double, within jpype** (1.8-2.3x): `JPIntType` is
  the only primitive type with a `fastElementCheck` override
  (jp_inttype.cpp); long/float/double always take the general per-element
  path even at depth 1. **Call: fix** -- this is a narrow, mechanical gap
  (mirror the existing int implementation for the other three primitive
  types), not an architectural one, and the existing int fast path proves
  the approach works.

## 5. Array push, multi-dimensional (depth 2-5, rectangular and ragged)

**Methodology.** `sum{2,3,4,5}D{Type}Array(nested_list)`, 10\*\*depth
elements, uniform 10-wide shape (rectangular) and a second sweep with
irregular sibling lengths at every level (ragged, fixed seed for
reproducibility). ns/call, best-of-5.

**list->array push, jpype vs. alternatives (int, rectangular):**

| depth | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 2 | 5,473 | 2,226 | 5,914 | 4,867 |
| 3 | 44,685 | 18,304 | 54,016 | 41,419 |
| 4 | 432,827 | 179,039 | 536,362 | 418,743 |
| 5 | 4,178,878 | 1,832,619 | 5,343,343 | 4,563,869 |

**buffer->array push, jpype vs. alternatives (int; jep has no automatic
multi-dim buffer path -- see below):**

| depth | jpype | jpy | jep\* |
|---:|---:|---:|---:|
| 2 | 2,204 | 5,392 | 19,344 |
| 3 | 7,199 | 53,558 | 190,041 |
| 4 | 69,180 | 542,709 | 1,896,885 |
| 5 | 567,554 | 5,452,297 | 18,973,229 |

\* jep raises `TypeError` for numpy input to a multi-dim argument; the jep
column is a manual per-row workaround, slower than jep's own
`list->array` at this row size.

**Trends.** `list->array`: jpype now beats jep at every depth and is
1.0-1.3x ahead of pyjnius, but stays a flat ~2.3-2.5x behind jpy regardless
of depth. `buffer->array` flips the pattern entirely: jpype is *faster*
than both jpy and jep at every depth, and the gap widens with depth in
jpype's favor (2.4x ahead of jpy at depth 2, 9.6x ahead at depth 5) --
jpype is the only one of the three with a real bulk multi-dimensional
buffer path.

**jpype by element type** (`list->array`, rectangular, jpype-only -- all
four types within 1-8% of each other at every depth):

| depth | int | long | float | double |
|---:|---:|---:|---:|---:|
| 2 | 5,384 | 5,370 | 5,219 | 5,462 |
| 3 | 44,355 | 44,337 | 44,589 | 46,491 |
| 4 | 433,175 | 432,453 | 429,016 | 450,266 |
| 5 | 4,228,182 | 4,199,778 | 4,148,553 | 4,536,156 |

**jpype, ragged vs. rectangular** (int, jpype-only; ragged `n` is the
tree's actual element count, not exactly 10\*\*depth):

| depth | rectangular | ragged (n) |
|---:|---:|---:|
| 2 | 5,384 | 3,625 (n=53) |
| 3 | 44,355 | 51,693 (n=1,034) |
| 4 | 433,175 | 372,779 (n=8,073) |
| 5 | 4,228,182 | 5,230,263 (n=114,940) |

**Analysis.**

- **`list->array` vs. jpy, ~2.3-2.5x flat**: same element-validation
  tradeoff as Section 4. **Call: architectural tradeoff, don't chase.**
  Worth noting for the record: this used to be a *depth-growing* 7-22x gap
  before a ragged-native single-pass conversion replaced a redundant
  per-level verify+copy re-match; what's left now is a roughly constant
  per-element cost, not a scaling problem.
- **`list->array` vs. jep/pyjnius**: no gap, jpype already wins. **Call:
  no gap.**
- **`buffer->array` vs. jpy/jep**: jpype already wins, gap widens with
  depth in jpype's favor. **Call: no gap -- this is jpype's strongest
  array result.**
- **Type parity within jpype at depth >= 2**: confirmed at 1-8%, no
  action needed. **Call: no gap.**
- **Ragged vs. rectangular, per element, within jpype**: normalized for
  actual element count, ragged costs essentially the same as rectangular
  at every depth. **Call: no gap** -- the per-node marker overhead ragged
  input pays isn't a meaningfully large cost.

## 6. Array push, shape at fixed depth and total element count

**Methodology.** jpype-only -- no cross-library shape sweep exists for
jpy/jep/pyjnius, so this section has no comparison column; it's included
because it's an internal jpype finding with a direct, actionable
implication. 2D and 3D shapes holding a fixed total element count (mostly
100,000, two shapes at 300,000 and one at 1,000,000, called out), pushed
both as list and buffer. Reports both ns/call and ns/element so shapes
with different totals stay comparable. ns/call, best-of-5.

**buffer->array, 2D, ns/element by shape and type:**

| shape (rows x cols) | int | long | float | double |
|---|---:|---:|---:|---:|
| 3 x 100,000 | 0.5 | 3.0\* | 1.2 | 1.7 |
| 10 x 10,000 | 0.7 | 7.6 | 1.3 | 1.6 |
| 100 x 1,000 | 0.6 | 8.1 | 1.2 | 1.6 |
| 1,000 x 100 | 1.2 | 3.8 | 1.6 | 2.1 |
| 1,000 x 1,000 | 1.3 | 5.5 | 1.5 | 3.2 |
| 10,000 x 10 | 5.2 | 7.6 | 5.8 | 5.9 |
| **100,000 x 3** | **18.5** | 19.1 | 19.9 | 18.0 |

\* Noisy: a 9-trial/200-iteration spot-check of this cell gave 1.7
ns/elem (522,478 ns/call), not the 3.0 ns/elem (914,750 ns/call) shown --
direction held, magnitude didn't. This is the only cell independently
re-checked; treat other outliers in the smallest-iteration-count rows with
the same skepticism.

**list->array, 2D, ns/element by shape and type (for comparison):**

| shape (rows x cols) | int | long | float | double |
|---|---:|---:|---:|---:|
| 3 x 100,000 | 30.6 | 31.4 | 31.2 | 33.1 |
| 10 x 10,000 | 30.6 | 30.7 | 33.4 | 31.8 |
| 100 x 1,000 | 30.8 | 30.9 | 32.5 | 33.5 |
| 1,000 x 100 | 31.7 | 32.0 | 31.4 | 37.6 |
| 1,000 x 1,000 | 31.7 | 37.4 | 32.0 | 33.5 |
| 10,000 x 10 | 41.4 | 41.6 | 40.1 | 44.6 |
| **100,000 x 3** | **65.9** | 70.5 | 66.7 | 72.3 |

**Trends.** Row count, not total element count or depth, is the dominant
cost driver for `buffer->array`: `100000x3` and `3x100000` hold the same
300,000 elements but differ by ~37x per element (18.5 vs. 0.5 ns/elem,
int) purely on row count (100,000 vs. 3). `list->array` shows the same
direction but a much smaller effect (~2.1x, 65.9 vs. 30.6 ns/elem) at the
same two shapes. The 3D sweep (10x10x1000 vs. 1000x10x10, both 100,000
elements) confirms the same pattern at a smaller scale (buffer: 0.5 vs.
5.6 ns/elem, ~11x; list: 30.7 vs. 41.9 ns/elem, ~1.4x).

**Analysis.** `JPConversionMultiArrayBuffer`'s array-build step pays one
JNI sub-array allocation per outer-dimension row on top of the bulk
per-element copy; when per-element cost is normally tiny (a memcpy-style
bulk read), that per-row cost dominates once row count is large.
`JPConversionSequence`'s list path already pays a comparable per-row cost
as its baseline (a Python-level sub-sequence access per row), so the same
absolute per-row overhead is a much smaller *relative* effect there.
**Call: fix, if row-heavy multi-dimensional buffer pushes are a real
workload** -- batching multiple rows per JNI call (rather than one
JNI call per row) in `JPConversionMultiArrayBuffer` would directly target
this; not attempted this session (this section is a new finding, not part
of any change landed here). In the meantime, the actionable guidance for
callers today: at a fixed total element count, prefer numpy shapes with
fewer, longer rows over many short rows -- this matters more than element
type choice for `buffer->array` push.

## 7. Array pull (Java -> Python)

**Methodology.** `list(makeArray(n))`, `makeArray(n).tolist()`, and
`np.asarray(makeArray(n))` -- three ways to materialize a returned Java
array in Python -- swept across size (flat) and depth (multi-dim). ns/call,
best-of-5.

**Flat (1D), jpype vs. alternatives (int), `array->list`:**

| size | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 100 | 35,113 | 4,344 | 2,795 | 1,234 |
| 1,000 | 423,319 | 40,029 | 23,566 | 10,436 |
| 10,000 | 4,431,385 | 401,341 | 237,692 | 113,502 |
| 100,000 | 50,298,764 | 5,487,530 | 3,712,665 | 1,142,328 |

**Flat (1D), jpype vs. alternatives (int), `array->buffer`:**

| size | jpype | jpy | jep\* | pyjnius\* |
|---:|---:|---:|---:|---:|
| 100 | 1,757 | 994 | 7,025 | 3,759 |
| 1,000 | 2,257 | 1,520 | 48,691 | 33,889 |
| 10,000 | 6,688 | 6,490 | 479,937 | 351,246 |
| 100,000 | 45,113 | 46,129 | 6,173,758 | 4,969,111 |

\* Neither jep nor pyjnius has a real buffer-protocol return path: jep's
returned array (`pyjarray`) has no `getbufferproc`, and pyjnius returns
arrays as already-materialized native Python lists. Both columns above are
therefore `array->list`'s cost plus a redundant `np.asarray()` conversion,
not a real bulk read -- structurally different from jpype's/jpy's genuine
buffer reads, which is why they get *worse* than `array->list`, not
better, as size grows.

**Trends: this is jpype's single largest problem spot.** `array->list` is
10-45x slower than jpy and jep at every size, and jpype is the only one of
the four where `array->list` is *slower than its own `array->buffer`* --
by 20x at size 100 growing to over 1000x at size 100,000. jpy and jep stay
close to their own buffer numbers throughout (they don't have jpype's
internal gap between the two paths). Only against pyjnius's `array->buffer`
(the non-real one above) does jpype come out ahead at scale.

**jpype's own `array->list` vs. `tolist()` vs. `array->buffer`, by
element type** (jpype-only, flat 1D):

| size | list() int | list() long | list() float | list() double | tolist() int | tolist() double | buffer int | buffer double |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 35,113 | 34,316 | 27,091 | 27,463 | 11,942 | 6,467 | 1,757 | 1,813 |
| 1,000 | 423,319 | 408,679 | 322,208 | 328,288 | 163,718 | 106,730 | 2,257 | 2,856 |
| 10,000 | 4,431,385 | 4,356,234 | 3,447,790 | 3,498,659 | 1,324,523 | 706,303 | 6,688 | 12,189 |
| 100,000 | 50,298,764 | 49,237,426 | 40,196,068 | 41,326,802 | 16,019,934 | 9,863,736 | 45,113 | 96,172 |

`tolist()` recovers 2-3x over `list()` at every size (one JNI critical
section per leaf array instead of per element) but still lands 100-350x
behind `array->buffer`. Multi-dimensional pull shows the identical
pattern at every depth (2-5): `list()` 15-100x behind `array->buffer`,
`tolist()` recovering roughly half that gap, both still an order of
magnitude behind the buffer path. Full multi-dim numbers, jpype-only:

| depth | list() int | tolist() int | buffer int |
|---:|---:|---:|---:|
| 2 | 50,843 | 21,249 | 3,272 |
| 3 | 576,515 | 282,514 | 8,449 |
| 4 | 6,270,064 | 2,463,360 | 56,710 |
| 5 | 71,166,338 | 28,799,636 | 579,287 |

Type pattern within jpype pull: float/double are consistently 15-25%
*faster* than int/long on `list()`/`tolist()` (not shown per-column above
for space; see the flat table's int-vs-double columns), the opposite
direction from the push-side int advantage in Section 4.

**Analysis.** `list(jarray)` goes through `_JavaArrayIter`/
`JPClass::getArrayItem`: one `JPPyObject` allocation and one JNI
element-fetch call *per element*, with no bulk path at all, unlike
`array->buffer`'s buffer-protocol bulk read. **Call: fix -- highest
priority in this report.** This is jpype's largest measured gap against
every other library and against its own buffer path, it's a genuinely new
finding (pull had no benchmark coverage before this suite existed, so it
wasn't previously visible), and the fix shape is well understood by
analogy to work already done elsewhere in this report: a primitive-type-
specific bulk `GetIntArrayRegion`-style read directly into a freshly
allocated Python `list`, mirroring what `array->buffer` and the
`pullTo`/`tolist()` work (Section 8) already do for their respective
paths. Not attempted this session.

The float/double-faster-than-int/long pattern within jpype (15-25%,
`list()`/`tolist()` only) is a separate, smaller finding: confirmed by
reading source, `JPIntType`/`JPLongType::convertToPythonObject` both call
an extra `convertLong()` step that `JPFloatType::convertToPythonObject`
skips (direct `tp_alloc` + `ob_fval` set). It does not appear in
`array->buffer`, which never boxes individual elements. **Call: minor,
low priority** -- the absolute cost here is dwarfed by the `list()`-vs-
`buffer` gap above; not worth separate investment ahead of the bulk-read
fix.

## 8. Recent fixes already landed on this branch

Two smaller, already-completed items, kept brief since there's no
remaining gap to act on:

- **Non-contiguous buffer sources** (a numpy column slice or transposed
  array) used to fail the buffer match outright (no stride support
  requested) and fall all the way back to the general per-element/per-row
  path -- now reaches the same bulk path as a contiguous buffer, confirmed
  across all four element types: 73-98% faster than the old fallback,
  landing within a few percent of the contiguous numbers in Sections 4-5.
  **Call: no remaining gap.**
- **Bulk in-place transfer** (`pullTo`/`pushFrom`): new API for copying a
  1-D primitive array directly into/from a caller-supplied buffer without
  producing a new Python object. No prior route existed to compare against
  except a naive per-element loop, which this replaces by 3-4 orders of
  magnitude (e.g. `double[]` pull at 100,000 elements: 21,284 ns bulk vs.
  33,292,511 ns naive). No cross-library equivalent to compare against.
  **Call: no remaining gap; this is a new capability, not a closed gap.**

## 9. Where to focus next (ranked)

1. **`array->list` pull** (Section 7) -- 10-45x behind jpy/jep, and 20x
   to 1000x+ behind jpype's own `array->buffer` depending on size. Largest
   gap in this report, well-understood fix shape, not yet attempted.
2. **Row-heavy shape penalty on `buffer->array` push** (Section 6) -- up
   to 37x per-element at extreme row counts; fix is architectural
   (batch JNI calls across rows) but narrow in scope.
3. **`fastElementCheck` missing for long/float/double, flat push**
   (Section 4) -- 1.8-2.3x, mechanical fix mirroring the existing int
   implementation.
4. **`convertLong()` overhead on int/long pull** (Section 7) -- 15-25%,
   low priority relative to #1 above; revisit after the bulk-read fix
   lands, since that fix may subsume this path entirely.
5. **Everything else** (Sections 2, 3, 5's `list->array` gap to jpy) is an
   architectural tradeoff jpype makes deliberately (element-level
   validation for correct overload disambiguation, general-purpose
   `JPConversion` chain for broader behavior) or a case where jpype
   already leads (multi-dim `buffer->array`, numpy scalar dispatch,
   dispatch caching vs. jep/pyjnius). No action recommended.

## 10. Verification

Every change referenced in this report went through the full local suite
(standard and `ENABLE_COVERAGE=ON`/fault-injection builds, both fixed and
`pytest-randomly` orderings) in a disposable venv per this repo's
CLAUDE.md, plus targeted regression tests for the correctness edge cases
each fast path introduced (null arguments, covariant/subclass returns,
mixed-type lists, boxed-type round trips, cache invalidation,
non-contiguous numpy sources, ragged nested lists). Full `test/jpypetest`
suite on this branch's final state: 1831 passed, 173 skipped, 0 failures.
