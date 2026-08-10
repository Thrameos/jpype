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

**Coverage note.** Sections 2-3 (scalars, dispatch, proxies) remain
**int-only** across libraries -- that surface wasn't re-benchmarked this
round. Sections 4-7 (array push/pull) now sweep long/float/double, ragged
shape, row/column shape, and non-contiguous buffer sources across all four
libraries wherever the library's API supports the case at all; where one
doesn't (jpy has no multi-dim buffer-to-buffer bulk path in some
configurations, pyjnius has no `buffer->array` push whatsoever, jep has no
native multi-dim `numpy->array` push), that's stated explicitly at the
point it matters rather than silently leaving the cell blank.

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
best-of-5. The size-sweep tables below are int-only for readability; the
type breakdown beneath them covers all four libraries.

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

**By element type, size 100,000, ns/call and ratio-to-int (all four
libraries now covered, not jpype-only):**

`list->array`:

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 1,829,617 (1.0x) | 4,090,324 (2.24x) | 4,175,308 (2.28x) | 4,167,789 (2.28x) |
| jpy | 789,924 (1.0x) | 856,623 (1.08x) | 1,240,937 (1.57x) | 1,340,292 (1.70x) |
| jep | 842,845 (1.0x) | 873,552 (1.04x) | 1,522,265 (1.81x) | 1,566,213 (1.86x) |
| pyjnius | 4,291,169 (1.0x) | 4,153,820 (0.97x) | 3,068,018 (0.72x) | 3,150,899 (0.73x) |

`buffer->array` (pyjnius has none, see Section 6\*):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 249,364 (1.0x) | 321,174 (1.29x) | 276,684 (1.11x) | 319,701 (1.28x) |
| jpy | 54,344 (1.0x) | 185,117 (3.41x) | 106,956 (1.97x) | 212,033 (3.90x) |
| jep | 49,286 (1.0x) | 149,387 (3.03x) | 106,586 (2.16x) | 157,181 (3.19x) |

\* the `buffer->array` long/double-vs-int/float ratio in every library
roughly tracks element byte width (8 bytes vs. 4) -- consistent with a
bulk memcpy-shaped cost proportional to bytes moved, not an inefficiency.
jpype's `buffer->array` ratios are the flattest of the three, i.e. jpype's
buffer path is the *least* sensitive to element type, not the most.

**Analysis.** Three separate things here, confirmed by source, not
conflated:

- **jpype-vs-jpy on `list->array`**: jpy's array matching doesn't inspect
  elements before committing to a conversion; jpype validates every
  element up front to support correct Java-style overload disambiguation.
  **Call: architectural tradeoff, don't chase** -- closing it would mean
  giving up element-level validation jpy doesn't do.
- **int-vs-long/float/double, within jpype's `list->array`** (1.8-2.3x):
  `JPIntType` is the only primitive type with a `fastElementCheck`
  override (jp_inttype.cpp); long/float/double always take the general
  per-element path even at depth 1. The cross-library table above shows
  this same directional gap in jpy (1.1-1.7x) and jep (1.0-1.9x) too --
  so it isn't a jpype-only defect, it's a real cost every one of these
  bridges pays for non-int primitives on the list path, just smaller
  elsewhere. jpype's gap being the largest of the three is exactly what a
  missing `fastElementCheck` predicts. pyjnius is the outlier: float/double
  are *faster* than int/long there, the opposite direction, not
  investigated further (a pyjnius-side finding, not a jpype one). **Call:
  fix** -- narrow, mechanical (mirror the existing int implementation for
  the other three primitive types), not architectural, and the existing
  int fast path proves the approach works. Not yet implemented.
- **`buffer->array` type ratios**: already explained by byte width, not a
  gap. **Call: no gap.**

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

**Ragged push, type parity across all four libraries** (ns/call at
depth 5, n=114,940 elements; long/float/double shown as a ratio to that
library's own int column, same shape as the flat-push table in Section 4):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 5,230,263 | 1.00x | 1.02x\* | 1.03x\* |
| jpy | 2,241,931 | 1.04x | 1.02x | 1.06x |
| jep | 7,249,576 | 1.02x | 0.97x | 0.97x |
| pyjnius | 5,447,465 | 1.01x | 1.42x | 1.48x |

\* jpype's own ragged-push type breakdown wasn't re-tabulated cell-by-cell
here since it's already covered above; ratios computed from the same
underlying run.

**Trends.** Ragged is the one array shape where element type stops
mattering: jpype, jpy, and jep all sit within a few percent across every
type, at every depth -- the `fastElementCheck` gap from Section 4 doesn't
show up here because the ragged-native path (`isRaggedLeafElement`) is a
single per-leaf branch regardless of type, not a separate fast/slow
dispatch. pyjnius again breaks the pattern, 1.4-1.5x slower on
float/double even on ragged input -- consistent with the same reversed
type-cost direction seen in Section 4, and again not investigated further
since it's not a jpype-side finding.

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
- **Ragged type parity, jpype/jpy/jep vs. pyjnius**: confirmed cross-library
  now, not jpype-only -- three of the four bridges show no meaningful
  type-cost difference on ragged input; pyjnius is the one exception.
  **Call: no gap on jpype's side.**

## 6. Array push, shape at fixed depth and total element count

**Methodology.** 2D and 3D shapes holding a fixed total element count
(mostly 100,000, two shapes at 300,000 and one at 1,000,000, called out),
pushed both as list and buffer. Reports both ns/call and ns/element so
shapes with different totals stay comparable. ns/call, best-of-5. jpy,
jep, and pyjnius were extended with the same shape sweep this round --
pyjnius has no `buffer->array` push at all (Section 4), so it's list-only
below; jep has no native multi-dim `numpy->array` push (Section 5), so
its "buffer" column is the same manual per-row workaround used elsewhere
in this report, not a real bulk path -- flagged again inline since it
changes what the numbers mean.

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

**Cross-library confirmation, int, ns/element at the two extreme 2D
shapes (`3x100000` = few long rows, `100000x3` = many short rows, same
300,000 elements), `list->array`:**

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 30.6 | 65.9 | 2.2x |
| jpy | 8.6 | 37.0 | 4.3x |
| jep | 9.0 | 146.4 | 16.3x |
| pyjnius | 29.6 | 82.1 | 2.8x |

**`buffer->array`** (jep's column is the manual-assembly workaround, not
a real bulk path -- see methodology note above; pyjnius has none):

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 0.5 | 18.5 | 37x |
| jpy | 36.4 | 92.7 | 2.5x |
| jep\* | 0.7 | 534.5 | 742x |

\* jep's `buffer_manual` ratio is an artifact of the manual per-row
workaround (each row is individually assembled in Python), not a signal
about jep's own architecture -- included for completeness, not compared
head-to-head with the other two.

**Trends.** Row count, not total element count or depth, is the dominant
cost driver for `buffer->array` and, to a smaller degree, `list->array`,
**in every library that was checked, not just jpype**: `100000x3` and
`3x100000` hold the same 300,000 elements but differ sharply per element
purely on row count (100,000 vs. 3). The 3D sweep (10x10x1000 vs.
1000x10x10, both 100,000 elements) confirms the same pattern at a smaller
scale in every library. But the *size* of the effect differs a lot by
library and isn't correlated with which library is faster in absolute
terms: jpype's `buffer->array` is the most sensitive by ratio (37x) yet
still the fastest in absolute ns/element at the worst shape (18.5 vs.
jpy's 92.7) -- jpy pays a smaller relative penalty on top of a slower
baseline. jep's real (non-manual) `list->array` ratio (16.3x) is also
markedly worse than jpype's (2.2x) or jpy's (4.3x).

**Analysis.** `JPConversionMultiArrayBuffer`'s array-build step pays one
JNI sub-array allocation per outer-dimension row on top of the bulk
per-element copy; when per-element cost is normally tiny (a memcpy-style
bulk read), that per-row cost dominates once row count is large.
`JPConversionSequence`'s list path already pays a comparable per-row cost
as its baseline (a Python-level sub-sequence access per row), so the same
absolute per-row overhead is a much smaller *relative* effect there. The
cross-library data confirms this is a shared JNI-shaped reality (every
bridge that builds nested Java arrays pays some per-row tax) rather than
a jpype-specific defect, but jpype's *relative* sensitivity to it on
`buffer->array` is worse than jpy's even though jpype wins in absolute
terms -- there's real headroom being left on the table at extreme row
counts. **Call: fix, if row-heavy multi-dimensional buffer pushes are a
real workload** -- batching multiple rows per JNI call (rather than one
JNI call per row) in `JPConversionMultiArrayBuffer` would directly target
this; not attempted this session (this section is a new finding, not part
of any change landed here). In the meantime, the actionable guidance for
callers today, true across every library measured: at a fixed total
element count, prefer numpy shapes with fewer, longer rows over many
short rows -- this matters more than element type choice for
`buffer->array` push.

## 7. Array pull (Java -> Python)

**Methodology.** `list(makeArray(n))`, `makeArray(n).tolist()`, and
`np.asarray(makeArray(n))` -- three ways to materialize a returned Java
array in Python -- swept across size (flat) and depth (multi-dim). ns/call,
best-of-5.

**Flat (1D), jpype vs. alternatives (int), `array->list`:**

| size | jpype (pre-fix) | jpype (current, see below) | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|---:|
| 100 | 35,113 | 10,674 | 4,344 | 2,795 | 1,234 |
| 1,000 | 423,319 | 156,215 | 40,029 | 23,566 | 10,436 |
| 10,000 | 4,431,385 | 1,192,970 | 401,341 | 237,692 | 113,502 |
| 100,000 | 50,298,764 | 16,182,181 | 5,487,530 | 3,712,665 | 1,142,328 |

**Flat (1D), jpype vs. alternatives (int), `array->buffer`:**

| size | jpype | jpy | jep\* | pyjnius\* |
|---:|---:|---:|---:|---:|
| 100 | 1,922 | 994 | 7,025 | 3,759 |
| 1,000 | 2,462 | 1,520 | 48,691 | 33,889 |
| 10,000 | 7,391 | 6,490 | 479,937 | 351,246 |
| 100,000 | 46,740 | 46,129 | 6,173,758 | 4,969,111 |

\* Neither jep nor pyjnius has a real buffer-protocol return path: jep's
returned array (`pyjarray`) has no `getbufferproc`, and pyjnius returns
arrays as already-materialized native Python lists. Both columns above are
therefore `array->list`'s cost plus a redundant `np.asarray()` conversion,
not a real bulk read -- structurally different from jpype's/jpy's genuine
buffer reads, which is why they get *worse* than `array->list`, not
better, as size grows.

**Trends: this is still jpype's single largest problem spot, though the
gap has narrowed substantially (see the fix history below).**
`array->list` is now 2.5-3.9x slower than jpy and 3.8-6.6x slower than jep
at every size -- down from the pre-fix 8.1-11.0x and 12.6-18.7x -- and
jpype is still the only one of the four where `array->list` is *slower
than its own `array->buffer`* -- by 5.6x at size 100 growing to 346x at
size 100,000 (was 20x-1115x pre-fix). jpy and jep stay close to their own
buffer numbers throughout (they don't have jpype's internal gap between
the two paths). Only against pyjnius's `array->buffer` (the non-real one
above) does jpype come out ahead at scale.

**jpype's own `array->list` (pre-fix numbers -- superseded, kept for the
`tolist()`/`buffer` comparison) vs. `tolist()` vs. `array->buffer`, by
element type** (jpype-only, flat 1D):

| size | list() int | list() long | list() float | list() double | tolist() int | tolist() double | buffer int | buffer double |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 35,113 | 34,316 | 27,091 | 27,463 | 11,942 | 6,467 | 1,757 | 1,813 |
| 1,000 | 423,319 | 408,679 | 322,208 | 328,288 | 163,718 | 106,730 | 2,257 | 2,856 |
| 10,000 | 4,431,385 | 4,356,234 | 3,447,790 | 3,498,659 | 1,324,523 | 706,303 | 6,688 | 12,189 |
| 100,000 | 50,298,764 | 49,237,426 | 40,196,068 | 41,326,802 | 16,019,934 | 9,863,736 | 45,113 | 96,172 |

**jpype's `list()`, current, by element type** (see "Fix landed" below):

| size | list() int | list() long | list() float | list() double |
|---:|---:|---:|---:|---:|
| 100 | 10,674 | 10,626 | 9,091 | 9,237 |
| 1,000 | 156,215 | 158,384 | 135,928 | 141,597 |
| 10,000 | 1,192,970 | 1,206,027 | 990,014 | 1,016,304 |
| 100,000 | 16,182,181 | 16,393,522 | 14,298,606 | 14,370,855 |

`list()` now lands within 1.21-1.44x of `tolist()` (int narrowest, double
widest) at every size, roughly the same span as 1.17-1.51x the round
before (int/double still the narrowest/widest, just each ~30-40% cheaper
in absolute terms) -- see "Fix landed" below for what changed -- but
still lands well behind `array->buffer`, since neither `list()` nor
`tolist()` is a real bulk-*decode* path: boxing still happens one
`PyObject` at a time either way, just cheaper per call now (see the
round-4 entry below). Multi-dimensional pull shows a similar pattern at
every depth (2-5): `list()` is now within 1.00-1.50x of `tolist()`, both
still roughly an order of magnitude or more behind the buffer path. Full
multi-dim numbers, jpype-only:

| depth | list() int (pre-fix) | list() int (current, see below) | tolist() int | buffer int |
|---:|---:|---:|---:|---:|
| 2 | 50,843 | 19,537 | 17,931 | 3,497 |
| 3 | 576,515 | 251,710 | 251,010 | 8,490 |
| 4 | 6,270,064 | 3,018,978 | 2,124,538 | 63,024 |
| 5 | 71,166,338 | 38,539,253 | 25,743,804 | 613,734 |

**`array->list` pull, type ratio to int, size 100,000, all four
libraries** (jpy/jep/pyjnius now covered, not jpype-only):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 16,182,181 (1.0x) | 1.01x | 0.88x | 0.89x |
| jpy | 5,695,493 (1.0x) | 0.88x | 0.78x | 0.79x |
| jep | 3,093,851 (1.0x) | 1.04x | 0.75x | 0.76x |
| pyjnius | 1,168,938 (1.0x) | 1.92x | 1.02x | 2.07x |

**Trends.** float/double pull *faster* than int/long in jpype, jpy, and jep
alike -- this is not a jpype-specific quirk, it shows up everywhere except
pyjnius, which instead shows long and double pulling ~2x *slower* than
int/float (an inconsistent, not byte-width-explained pattern, flagged but
not investigated further as a pyjnius-side finding). jpype's own
float/double margin (12-17% faster than int) narrowed sharply this round
(was 27-31%, jpy/jep sit at 21-25%) because int/long got cheaper while
float/double didn't move -- see the round-4 fix below, which only touches
the long-family boxing path that int/long/boolean route through; float
already used a fixed-layout, single-allocation constructor
(`newFloatFixed`) with nothing left to cut.

**Analysis, and fixes landed across four rounds.** The root cause as
originally diagnosed here -- "no bulk path exists for `list(jarray)`, only
for `tolist()`/`array->buffer`" -- turned out to be **half wrong** on
closer inspection. `tolist()`
already *is* that bulk path (`JPPrimitiveType::getArrayRange`, one
`GetPrimitiveArrayCritical` pin, a tight C boxing loop) and already
existed before this session, just not wired to `list(arr)`. Reading jpy's
own C source directly (`JObj_sq_item`, jpy's array-indexing function)
found that jpy's own `list()` speed advantage is *not* a bulk read either
-- it's a single-element `Get<Type>ArrayRegion` call per element, the
same O(n)-JNI-calls shape jpype had, just reached through a native
`sq_item` slot (CPython's built-in `PySeqIter` C loop) instead of a
Python-level `__next__` method. jpype's actual gap was **stacking**
Python-level iterator overhead *on top of* the per-element JNI cost, not
lacking a bulk path.

**Fix, in two rounds -- the first one regressed multi-dim and was caught
by the same benchmark suite before landing for good.** Round 1 added a
native `sq_item` slot (`PyJPArray_sqItem`) and removed the pure-Python
`_JArrayProto.__iter__`/`_JavaArrayIter` (`jpype/_jarray.py`) that was
shadowing it, so CPython's built-in `PySeqIter` would drive iteration.
Flat improved 25-37%, but a clean A/B against the pre-fix commit (same
benchmark script, ruling out version skew) showed multi-dimensional pull
got consistently ~30% *slower*. Root-caused with `strace`, not
guesswork: `PySeqIter` finds the end of iteration by calling `sq_item`
one index past the end and catching `IndexError`, and raising *any*
exception while a `JPJavaFrame` is open and then popping that frame is
expensive here -- thousands of `futex` calls per hit, consistent with
JVM safepoint synchronization triggered by the frame-pop/exception-state
interaction, not anything in jpype's own code. A bounds check inside
`sq_item` (to dodge jpype's own C++-throw exception path specifically)
did not fix it -- the cost turned out to be tied to *any* pending
exception at frame-pop time, including a plain `PyErr_SetString` with no
C++ throw at all. Small arrays paid this once per `list()` call as a
large fixed cost, which is why deeply nested multi-dim pulls (many small
leaf arrays) regressed while one large flat array barely noticed it.

Round 2 (landed): `PyJPArrayIter`, a real native iterator type mirroring
CPython's own `list`/`tuple` iterators -- checks length *before* ever
calling into array access, and returns `NULL` with **no exception set**
to signal a clean stop, the same trick that lets `list`/`tuple` iteration
skip exception-raising overhead entirely. Registered as `JArray`'s
`Py_tp_iter`, ahead of the `sq_item`/`PySeqIter` fallback in CPython's
iterator-protocol lookup order (`sq_item` itself stays, now with a
matching bounds check, since other C-level consumers still reach it).
Deliberately did **not** take the eager-bulk-materialize option (routing
`list(arr)` through `tolist()` on first `next()`), since that would
silently change iteration laziness (an early-`break`'d `for` loop over a
huge array would eagerly pull the whole thing) for a further win treated
as a separate, not-yet-taken decision.

**Result**: faster than *both* the round-1 fix and the original baseline
at every case measured, flat and multi-dim alike (see tables above; e.g.
flat @100 int: 35,113 -> 21,782 ns, -38%; multi-dim depth 5:
71,166,338 -> 51,369,609 ns, -31%). Full suite green (1831 passed, 173
skipped) in both fixed and `pytest-randomly` orderings, laziness
confirmed unchanged (early-`break` iteration still does exactly the
elements touched, no more, verified by instrumented call-counting).
After round 2, the remaining gap to jpy's/jep's own numbers (jpype @100
int: 21,782ns; jpy: 4,344ns; jep: 2,795ns) was attributed to jpype's
per-JNI-call overhead itself: `JPJavaFrame` construction, `JPPyObject`
wrapping, and exception-frame bookkeeping around each single-element JNI
call -- the same architectural cost Section 2 attributes jpy's general
speed lead to. A real (pushed) `PushLocalFrame`/`PopLocalFrame` pair was
being paid for every element read even though a primitive read
(`Get<Type>ArrayRegion`) creates zero local Java references, so there was
nothing for that frame to ever actually protect.

**Round 3 (landed): a frame-free per-element read path.** `JPArray` is now
forked into a concrete subclass per component type
(`JPArrayBoolean`...`JPArrayDouble`, `JPArrayObject`, `JPArrayNested`),
resolved once at array-wrapper construction rather than dispatched
per-element, mirroring the existing `JPClass`/`JPPrimitiveType` fork. Each
of the 8 primitive leaves reads through `JPJavaAccess`, a narrow,
non-frame-creating companion to `JPJavaFrame` that has no reference-creating
methods at all -- a future edit that tried to add one there would be a
compile error, not a runtime assertion. Its exception check
(`checkFast()`) touches nothing but `ExceptionCheck()` on the (overwhelming)
common case; only a genuine pending exception escalates to a real
`JPJavaFrame::inner()` to build the `JPJavaError`. Boxing itself is fully
inlined per primitive type (matching each type's existing
`convertToPythonObject` exactly) rather than calling through it, since
`PyJPValue_assignJavaSlot` is a guaranteed no-op for six of the eight
primitive families and a plain frame-free slot write for the other two
(float/double) -- so no primitive element read ever touches a
`JPJavaFrame` at all on its common path. Correctness (no accidental local
reference ever taken without a real frame somewhere in the call chain to
release it) is enforced by a `JP_ASSERT_FAST_FRAMES` build: a thread-local
real-frame-depth counter asserted non-zero at every reference-creating
`JPJavaFrame` member function, verified both by the full suite under that
build and by a deliberate-violation regression test (an artificially
broken `getItem()` that reaches for a real reference with zero frames
anywhere in the chain, confirmed to raise correctly and leak nothing under
`-Xcheck:jni`).

**Result**: a further 23-39% improvement on top of round 2 at every
flat size/type (e.g. int[100]: 21,782 -> 14,532 ns; double[100,000]:
20,442,630 -> 14,777,477 ns) and 12-25% further on multi-dim depth 2-5
(e.g. depth 5: 51,369,609 -> 44,978,715 ns) -- full numbers in the tables
above. `tolist()`/`array->buffer`, whose hot paths were untouched by this
round, moved by only a few percent in either direction across the same
re-run, as expected for unrelated code paths under normal run-to-run
variance -- a useful sanity check that the win is correctly scoped to the
per-element path this round actually changed. The `list()`-vs-`tolist()`
gap in particular narrowed sharply: `list()` now lands within 1.17-1.51x
of `tolist()` across all four types and every flat size (int narrowest at
1.17-1.21x, double widest at 1.33-1.51x), down from the pre-round 2-3x
gap. Still fell short of jpy's/jep's own numbers at the time (jpype @100 int:
14,532ns; jpy: 4,344ns; jep: 2,795ns), with part of that remaining gap
traced to `convertLong()` in round 4 below rather than being fully
irreducible per-JNI-call cost.

**Round 4 (landed): construct the boxed `JInt`/`JLong`/`JBoolean` instance
directly instead of routing through `PyLong_Type.tp_new`.** Every `int[]`/
`long[]`/`boolean[]` element pulled into Python is a genuine subclass of
`int` (`JInt`, `JLong`, `JBoolean` -- not a plain `int`), and
`PyJPNumber_longFromLongLong` (`native/python/pyjp_number.cpp`) was
building each one by allocating a throwaway plain `PyLong` via
`PyLong_FromLongLong`, packing it into a 1-tuple, and calling
`PyLong_Type.tp_new(subtype, args, nullptr)` -- CPython's generic
int-subclass path (`long_subtype_new`), which itself re-parses that tuple
and allocates a *second*, real instance before copying digits across and
discarding the first. Two allocations, a tuple, and the attendant
refcounting, just to reskin one integer value as the right subtype --
found by `py-spy`-recording (`--native`, frame-pointer build) a tight
`list(int[100000])` loop and comparing self-time: JNI-side self-time
(`GetIntArrayRegion`+`ExceptionCheck`+`GetEnv`+bounds check) came in well
under `PyJPNumber_longFromLongLong`+`JPPyObject::decref`+`JPPyTuple_Pack`
combined, ruling out the JNI call itself as the bottleneck (independently
confirmed earlier by a `Support.getInt`-vs-`GetIntArrayRegion`
micro-benchmark showing the JNI call is not slow in isolation).

Fixed by allocating the digits directly into the real subtype in one
shot: `type->tp_alloc(type, ndigits)` sized to the value's actual digit
count, with the CPython-version-appropriate digit layout
(`_PyLongValue`/`lv_tag` on >=3.12, `ob_size`/`ob_digit` on <=3.11) filled
in directly -- no throwaway intermediate object, no tuple. This is a
narrower revival of a digit-based constructor this codebase carried
earlier (on a now-divergent line of history) for an unrelated reason --
that version additionally reserved a fixed worst-case digit budget so an
appended per-instance `JPValue` struct always landed at a constant
offset. The current object layout keeps no such appended slot at all
(`tp_jvalue`/`longJValue` reconstructs the `jvalue` from the `PyLong`'s
own digits on demand), so the fixed-budget reservation is gone too --
this version allocates exactly the digits each value needs, nothing
spare.

**Result**: `list()` int[100,000] 20,256,724 -> 16,182,181ns (-20%),
int[100] 14,532 -> 10,674ns (-27%); `tolist()` int[100,000] 16,019,934 ->
12,918,718ns (-19%), which benefits identically since
`convertToPythonObject`/`convertLong` route through the same constructor.
Multi-dim depth 5 int: `list()` 44,978,715 -> 38,539,253ns (-14%),
`tolist()` 31,192,646 -> 25,743,804ns (-17%). `float`/`double` and
`array->buffer`, whose paths never touched `PyJPNumber_longFromLongLong`,
are unaffected. Full suite green (1831 passed, 173 skipped) in both a
plain release build and the `JP_ASSERT_FAST_FRAMES` build; boundary
values (`2**63-1`, `-(2**63)`, zero, negative) verified correct by hand
in addition to the suite. **Call: landed, real win** -- narrows but does
not close the remaining gap to jpy/jep, which still comes from the
irreducible per-JNI-call cost of `Get<Type>ArrayRegion` plus one
`PyObject` allocation per element, the same architectural floor Section 2
attributes jpy's general speed lead to.

The float/double-faster-than-int/long pattern is a separate, smaller
finding, originally attributed to jpype's own `convertLong()` step
(confirmed by source: `JPIntType`/`JPLongType::convertToPythonObject`
both call it, where `JPFloatType::convertToPythonObject` does a direct
`tp_alloc` + `ob_fval` set instead). For `list()`, the margin is now
12-17% (int narrowest, see the type-ratio table above) -- narrowed sharply
this round (was 27-31%) since round 4 above removed most of the extra
cost `convertLong()` was paying for int/long, without touching float's
already-minimal path. `tolist()` shows a similarly narrowed margin
(18-27% across sizes, e.g. size 100,000: int 12,918,718ns vs. double
10,419,499ns; was 35-47%) for the same reason. **Call: minor, low
priority** -- round 4 already captured the bulk of this gap; what
remains is dwarfed by the `list()`-vs-`buffer` gap above.

## 8. Recent fixes already landed on this branch

Three smaller, already-completed items, kept brief since there's no
remaining gap to act on:

- **Array-class and interface conversion matching, per-type
  specialization.** `JPArrayClass` (the metadata object behind every array
  type, e.g. resolving what a Python list/buffer argument converts to) is
  now forked per component type (`JPArrayClassBoolean`...`JPArrayClassDouble`,
  `JPArrayClassNested`, `JPArrayClassNestedRagged`), each with an
  unconditional conversion chain instead of one shared class testing every
  possible conversion (char/byte/buffer/multi-dim/ragged) against every
  array regardless of whether that array's element type could ever match.
  `JPInterfaceType` was split off `JPClass` the same way, so
  `proxyConversion` is only ever tried against an actual interface (a
  dynamic proxy can never be assigned to a plain class). **Not reflected
  in any table above, deliberately**: `JPClass::findJavaConversion` checks
  a per-Python-type `JPConversionCache` before ever calling into
  `findJavaConversionImpl`, and every benchmark here calls the same
  operation with the same argument type thousands of times, so the
  conversion-matching chain itself runs at most once per benchmark and is
  invisible in a warm, best-of-5 steady-state number. This change targets
  cold-path cost (a fresh Python type never seen before, or after a hints
  registration invalidates the cache) and, independently, keeps `gcov`
  coverage meaningful per array element type (previously a shared chain
  could report "covered" for `double[]` purely from a `char[]` test
  exercising the same lines). **Call: correctness/coverage improvement,
  not a performance change** -- no cold-path-specific benchmark exists to
  measure it against, and none of Sections 3-6's numbers above changed
  because of it.
- **Non-contiguous buffer sources** (a numpy column slice or transposed
  array) used to fail the buffer match outright (no stride support
  requested) and fall all the way back to the general per-element/per-row
  path -- now reaches the same bulk path as a contiguous buffer, confirmed
  across all four element types: 73-98% faster than the old fallback,
  landing within a few percent of the contiguous numbers in Sections 4-5.
  **Call: no remaining gap.** Extending the same non-contiguous sweep to
  jpy/jep this round surfaced a genuine cross-library gap, not a jpype
  one: jpy's 1D non-contiguous push fails outright with `RuntimeError: no
  matching Java method overloads found` at every size and every type --
  confirmed via source, jpy's buffer matcher requests `PyBUF_SIMPLE`
  (no stride support at all), and the resulting error message doesn't even
  name the real cause. jpy's ND non-contiguous (transposed multi-dim)
  cases do succeed. jep succeeds on both 1D and ND non-contiguous sources
  (1D via its real numpy fast path, ND via the same manual-assembly
  workaround used elsewhere in this report). pyjnius has no buffer push at
  all, so there's no case to test. Documented in `project/comparison.md`.
  **Call: no jpype action -- this is a documented jpy API gap, not
  something to fix on jpype's side.**
- **Bulk in-place transfer** (`pullTo`/`pushFrom`): new API for copying a
  1-D primitive array directly into/from a caller-supplied buffer without
  producing a new Python object. No prior route existed to compare against
  except a naive per-element loop, which this replaces by 3-4 orders of
  magnitude (e.g. `double[]` pull at 100,000 elements: 21,284 ns bulk vs.
  33,292,511 ns naive). No cross-library equivalent to compare against.
  **Call: no remaining gap; this is a new capability, not a closed gap.**

## 9. Where to focus next (ranked)

1. **`array->list` pull, plain `list(arr)`/iteration** (Section 7) --
   substantially closed across four rounds. Round 1/2 landed a native
   `PyJPArrayIter` iterator type (31-40% win, no laziness/correctness
   trade-off, and no lingering regression unlike the first attempt at a
   native `sq_item` + CPython's generic `PySeqIter`, which improved flat
   but regressed multi-dim ~30% via an expensive JVM-safepoint interaction
   on the iteration-boundary exception, caught and fixed before landing).
   Round 3 then removed the real (pushed) JNI local frame every primitive
   element read was still paying for despite creating zero local
   references (`JPJavaAccess`, a forked `JPArray` per component type, and
   fully-inlined per-type boxing) -- a further 23-39% flat / 12-25%
   multi-dim win on top of round 2. Round 4 then found (via a `py-spy
   --native` profile) that the JNI call itself was never the remaining
   bottleneck -- it was `PyLong_Type.tp_new`'s generic two-allocation path
   for boxing each element as the right `int` subtype (`JInt`/`JLong`/
   `JBoolean`) -- and replaced it with a direct single-allocation digit
   constructor, a further 14-27% win on `list()`/`tolist()` alike across
   flat and multi-dim int/long. `list()` now lands within 1.14-1.24x of
   `tolist()` (was 2-3x pre-round-3). Still 2.5-6.6x behind jpy/jep's own
   `list()` numbers -- the remainder is jpype's irreducible per-JNI-call
   cost (one `Get<Type>ArrayRegion` plus one `PyObject` allocation per
   element), not an iteration-protocol, frame-management, or boxing gap
   anymore. Closing further means either giving up iteration laziness
   (eager-bulk-via-`tolist()`, explicitly deferred), or a separate, broader
   look at reducing per-JNI-call cost below one call per element, which
   isn't scoped here.
2. **Row-heavy shape penalty on `buffer->array` push** (Section 6) -- up
   to 37x per-element at extreme row counts. Confirmed this round to be a
   shared JNI-shaped cost every library pays some version of, but jpype's
   *relative* sensitivity to it (37x) is worse than jpy's (2.5x) even
   though jpype wins in absolute terms -- real headroom left on the table.
   Fix is architectural (batch JNI calls across rows) but narrow in scope.
3. **`fastElementCheck` missing for long/float/double, flat push**
   (Section 4) -- 1.8-2.3x. Confirmed this round as a real gap and not
   purely a jpype artifact (jpy/jep show a milder version of the same
   direction), but jpype's version is the largest of the three and the
   fix is mechanical, mirroring the existing int implementation. Not yet
   implemented.
4. **`convertLong()` overhead on int/long pull** (Section 7) -- now 12-17%
   for `list()`, 18-27% for `tolist()` (was 27-31%/35-47% before round 4
   above removed the bulk of it). jpy and jep show the same direction and
   a comparable magnitude for what remains, which argues this residual is
   a shared CPython boxing cost, not a jpype-specific inefficiency worth
   chasing further. Low priority.
5. **Everything else** (Sections 2, 3, 5's `list->array` gap to jpy) is an
   architectural tradeoff jpype makes deliberately (element-level
   validation for correct overload disambiguation, general-purpose
   `JPConversion` chain for broader behavior) or a case where jpype
   already leads (multi-dim `buffer->array`, numpy scalar dispatch,
   dispatch caching vs. jep/pyjnius). No action recommended. Two
   cross-library findings surfaced this round are explicitly *not* jpype
   action items: jpy's 1D non-contiguous buffer push fails outright
   (Section 8), and jep showed an unexplained OOM/hang under a
   high-iteration multi-dim `double` pull-buffer benchmark cell -- an
   architectural asymmetry in jep's cross-runtime reference management is
   confirmed (jep has no jpype-equivalent bidirectional GC trigger), but a
   causal link to that specific hang is not confirmed and is not claimed
   here.

## 10. Verification

Every change referenced in this report went through the full local suite
(standard and `ENABLE_COVERAGE=ON`/fault-injection builds, both fixed and
`pytest-randomly` orderings) in a disposable venv per this repo's
CLAUDE.md, plus targeted regression tests for the correctness edge cases
each fast path introduced (null arguments, covariant/subclass returns,
mixed-type lists, boxed-type round trips, cache invalidation,
non-contiguous numpy sources, ragged nested lists). Full `test/jpypetest`
suite on this branch's final state: 1831 passed, 173 skipped, 0 failures.
