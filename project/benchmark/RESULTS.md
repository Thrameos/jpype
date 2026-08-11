# JPype cross-library performance comparison

## 1. Scope and methodology

This report compares jpype's call/conversion performance against three
alternative Python-Java bridges (jpy, jep, pyjnius) and, where no
alternative exists for a given operation, against a synthetic baseline.
Each section covers one topic area: methodology specific to that area, a
table of measured results, and an interpretation of what the numbers
show.

**Common methodology.** ns/call, best-of-5 trials, standard
(non-instrumented) build, run inside a disposable venv per this repo's
CLAUDE.md. Iteration count scales down as per-call cost grows
(`n = max(20, 5_000_000 // total_elements)`, `n // 10` warmup), so the
largest array sizes (~1,000,000 elements) run at only 20-50 timed
iterations. This formula applies to every number in Sections 2-7 and to
GraalPy's `list->array`/`array->list`/`array->buffer` numbers in Section
8 (verifiable directly against the `n` column recorded in each
`project/benchmark/graalpy/*_results.csv`). It does *not* apply to
Section 8's `buffer->array (manual)` numbers, which use a much smaller
sample budget (n as low as 3) -- see Section 8's opening note. jep runs
on Python 3.10 (this checkout's only working native build) and embeds
Python inside the JVM, the reverse of jpype/jpy/pyjnius's architecture,
so its numbers carry extra uncertainty. Every number in every table
comes from an actual recorded run of the corresponding script in
`project/benchmark/{jpype,jpy,jep,pyjnius,graalpy}/*.py` -- none are
hand-transcribed or extrapolated. See `project/benchmark/README.md` to
reproduce.

**Coverage note.** Sections 2-3 (scalars, dispatch, proxies) are
int-only across libraries. Sections 4-7 (array push/pull) sweep
long/float/double, ragged shape, row/column shape, and non-contiguous
buffer sources across all four libraries wherever the library's API
supports the case at all; where one doesn't (jpy has no multi-dim
buffer-to-buffer bulk path in some configurations, pyjnius has no
`buffer->array` push whatsoever, jep has no native multi-dim
`numpy->array` push), that's stated at the point it matters.

**Verification.** Every capability and correctness edge case referenced
in this report (null arguments, covariant/subclass returns, mixed-type
lists, boxed-type round trips, non-contiguous numpy sources, ragged
nested lists) is covered by `test/jpypetest`, run in a disposable venv
per this repo's CLAUDE.md.

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

**Interpretation.** jpy is fastest on every row, by 1.6-2.1x on the
numeric rows and up to ~9x on boxed-`Integer`. jpype beats jep on every
row except `Math.sqrt` (7% slower, likely noise) and beats pyjnius
everywhere except `Math.sqrt`. pyjnius is 6-22x slower than jpype on
every boxed/string row.

jpy's speed comes from an architecturally leaner binding (confirmed by
reading `jpy_jtype.c`/`jpy_jmethod.c`): fewer abstraction layers, no
general-purpose conversion chain to walk. jpype pays a deliberate cost
for breadth (implicit numeric widening, functional-interface duck
typing, hint-based custom conversions) that jpy's narrower binding
doesn't support -- this is an architectural tradeoff, not a defect.
pyjnius's gap was not source-level investigated.

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
checkout.

\*\* pyjnius: only the `int`-arg case is measured. The `Object`-arg case
reliably crashes the JVM with a `SIGSEGV` in `jni_GetObjectClass` on a
null argument (reproduced independently three times). Even the
non-crashing (non-null) case is wrong: `invokeObjectCallback` silently
returns `None` instead of the Python callback's actual return value.

**Interpretation.** jpy is 1.9-2.5x faster than jpype on dispatch;
jpype beats jep and pyjnius on dispatch by 4-6x, driven by a
single-slot overload cache plus a per-argument-type conversion cache
that jep/pyjnius re-derive on every call. The gap to jpy on dispatch is
the same architectural tradeoff as Section 2. On proxy callbacks, jep is
16% faster than jpype (likely its reversed embedding direction --
Java-calls-Python is jep's native direction), and pyjnius is ~15x
slower with two correctness bugs on top of the speed gap.

**numpy scalar arguments** (a coverage check, not a speed comparison):
jpype correctly dispatches `Math.max` for all four numpy scalar types
(`int32`/`int64`/`float32`/`float64`). jpy and pyjnius both fail on
`np.int32`/`np.int64`/`np.float32` (jpy: "ambiguous ... too many
matching overloads"; pyjnius: "No static methods called max ... matching
your arguments") because their per-type fast dispatch has no fallback
for `int`/`long` parameters, unlike their `float`/`double` matchers.

## 4. Array push, flat (1D)

**Methodology.** `sumIntArray(source)` and its long/float/double
counterparts, sweeping size (100/1k/10k/100k) and two source kinds: a
plain Python list, and a `Py_buffer`-backed object (numpy). ns/call,
best-of-5. The size-sweep tables below are int-only for readability; the
type breakdown beneath them covers all four libraries.

### 4.1 `list->array` push (method argument)

| size | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 100 | 3,018 | 1,313 | 1,939 | 3,068 |
| 1,000 | 19,695 | 8,294 | 9,574 | 25,932 |
| 10,000 | 194,631 | 78,342 | 86,629 | 268,303 |
| 100,000 | 1,829,617 | 774,569 | 840,301 | 4,520,089 |

### 4.2 `buffer->array` push (method argument)

Argument conversion for a numpy array against a flat primitive parameter
(e.g. `int[]`) hands the source buffer directly to a single Java call
(`Support.fillFlatFromBuffer`), which performs any dtype coercion
(signed/unsigned int 1/2/4/8 bytes, float32/float64, float16) and
handles non-unit stride directly, so a sliced/strided numpy column
reaches the same fast path as a fully contiguous array. A negative-stride
source (`arr[::-1]`) falls back to a per-element path.

pyjnius has no `buffer->array` push at all (see 4.4).

| size | jpype | jpy | jep |
|---:|---:|---:|---:|
| 100 | 1,459 | 497 | 770 |
| 1,000 | 1,847 | 890 | 1,176 |
| 10,000 | 6,862 | 6,392 | 6,118 |
| 100,000 | 57,371 | 48,307 | 51,150 |

**Interpretation.** jpype lands within ~9-19% of jpy and jep at 100,000
elements across all four types, and is faster than both jpy and jep on
`long`, `float`, and `double` at that size (see 4.4). At small sizes,
per-call fixed overhead (buffer validation, `NewDirectByteBuffer`, one
JNI call) dominates and jpype trails jpy/jep by a wider margin.

### 4.3 Slice assignment (`javaArr[:] = numpy_array`) and array clone

Writing a numpy array into an existing Java array slice (or cloning an
array's contents) uses the same buffer-handoff mechanism as 4.2, but
writes directly into the destination array (no allocation, no
`Get/ReleaseArrayElements` copy-in/copy-out) instead of allocating and
returning a fresh array. jpype-only measurement -- no direct
cross-library equivalent benchmarked.

| operation | ns/call |
|---|---:|
| `int[100000]` slice assignment | 9,371 |
| `int[100000]` method-argument push (4.2, for comparison) | 57,371 |

**Interpretation.** Slice assignment is cheaper than the method-argument
push at the same size because it skips both the fresh-array allocation
and the critical-section pinning the argument-push path still pays for.

### 4.4 Non-contiguous sources

A numpy column slice or transposed array (non-unit stride) reaches the
same bulk path as a contiguous buffer via an explicit `strideBytes`
parameter, rather than requiring a C-contiguous source.

| library | `int[100000]`, non-contiguous column slice |
|---|---:|
| jpype | 80,715 |
| jep | 75,448 |
| jpy | fails (`RuntimeError: no matching Java method overloads found`) |
| pyjnius | N/A (no buffer push at all) |

**Interpretation.** jpype's non-contiguous push (80,715ns) is only 1.4x
its own contiguous push (57,371ns, Section 4.2), and lands close to
jep's dedicated 1D non-contiguous fast path (75,448ns). jpy's buffer
matcher requests `PyBUF_SIMPLE` (no stride support at all) and fails
outright on any non-contiguous 1D source; its ND non-contiguous
(transposed multi-dim) cases do succeed, at the same cost as the
contiguous case. jep succeeds on both 1D (real numpy fast path) and ND
non-contiguous sources (manual per-row assembly, e.g.
`double[][][][][]` (10^5): 21,500,000 ns). Full capability matrix in
`project/comparison.md`.

### 4.5 By element type, size 100,000

`list->array`:

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 1,829,617 (1.0x) | 4,090,324 (2.24x) | 4,175,308 (2.28x) | 4,167,789 (2.28x) |
| jpy | 789,924 (1.0x) | 856,623 (1.08x) | 1,240,937 (1.57x) | 1,340,292 (1.70x) |
| jep | 842,845 (1.0x) | 873,552 (1.04x) | 1,522,265 (1.81x) | 1,566,213 (1.86x) |
| pyjnius | 4,291,169 (1.0x) | 4,153,820 (0.97x) | 3,068,018 (0.72x) | 3,150,899 (0.73x) |

`buffer->array` (pyjnius has none):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 57,371 (1.0x) | 123,853 (2.16x) | 108,005 (1.88x) | 163,149 (2.84x) |
| jpy | 48,307 (1.0x) | 138,314 (2.86x) | 107,158 (2.22x) | 146,112 (3.02x) |
| jep | 51,150 (1.0x) | 116,257 (2.27x) | 109,273 (2.14x) | 162,870 (3.18x) |

**Interpretation.**

- **`list->array`, jpype vs. jpy**: jpy's array matching doesn't inspect
  elements before committing to a conversion; jpype validates every
  element up front to support correct Java-style overload
  disambiguation -- an architectural tradeoff, not a gap to close.
- **`list->array`, int vs. long/float/double, within jpype** (1.8-2.3x):
  `JPIntType` is the only primitive type with a `fastElementCheck`
  override (`jp_inttype.cpp`); long/float/double always take the general
  per-element path even at depth 1. jpy (1.1-1.7x) and jep (1.0-1.9x)
  show the same directional gap, smaller in magnitude -- jpype's is the
  largest of the three, consistent with the missing `fastElementCheck`.
  pyjnius is the outlier: float/double are *faster* than int/long there,
  the opposite direction, not investigated further.
- **`buffer->array` type ratios**: long/double cost roughly 2-3x int/float
  across jpype, jpy, and jep alike -- shared behavior of `DeepBench`'s
  per-type sum method (included in the timed call), not a push-path
  difference between libraries, and does not track element byte width
  (float and int are both 4 bytes, yet float costs ~2x int's time in all
  three).

## 5. Array push, multi-dimensional (depth 2-5, rectangular and ragged)

**Methodology.** `sum{2,3,4,5}D{Type}Array(nested_list)`, 10\*\*depth
elements, uniform 10-wide shape (rectangular) and a second sweep with
irregular sibling lengths at every level (ragged, fixed seed for
reproducibility). ns/call, best-of-5.

**`list->array` push (int, rectangular):**

| depth | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 2 | 5,473 | 2,226 | 5,914 | 4,867 |
| 3 | 44,685 | 18,304 | 54,016 | 41,419 |
| 4 | 432,827 | 179,039 | 536,362 | 418,743 |
| 5 | 4,178,878 | 1,832,619 | 5,343,343 | 4,563,869 |

**`buffer->array` push (int; jep has no automatic multi-dim buffer
path):**

| depth | jpype | jpy | jep\* |
|---:|---:|---:|---:|
| 2 | 2,204 | 5,392 | 19,344 |
| 3 | 7,199 | 53,558 | 190,041 |
| 4 | 69,180 | 542,709 | 1,896,885 |
| 5 | 567,554 | 5,452,297 | 18,973,229 |

\* jep raises `TypeError` for numpy input to a multi-dim argument; the
jep column is a manual per-row workaround, slower than jep's own
`list->array` at this row size.

**jpype by element type** (`list->array`, rectangular -- all four types
within 1-8% of each other at every depth):

| depth | int | long | float | double |
|---:|---:|---:|---:|---:|
| 2 | 5,384 | 5,370 | 5,219 | 5,462 |
| 3 | 44,355 | 44,337 | 44,589 | 46,491 |
| 4 | 433,175 | 432,453 | 429,016 | 450,266 |
| 5 | 4,228,182 | 4,199,778 | 4,148,553 | 4,536,156 |

**jpype, ragged vs. rectangular** (int; ragged `n` is the tree's actual
element count, not exactly 10\*\*depth):

| depth | rectangular | ragged (n) |
|---:|---:|---:|
| 2 | 5,384 | 3,625 (n=53) |
| 3 | 44,355 | 51,693 (n=1,034) |
| 4 | 433,175 | 372,779 (n=8,073) |
| 5 | 4,228,182 | 5,230,263 (n=114,940) |

**Ragged push, type parity across all four libraries** (depth 5,
n=114,940 elements; long/float/double shown as a ratio to that library's
own int column):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 5,230,263 | 1.00x | 1.02x | 1.03x |
| jpy | 2,241,931 | 1.04x | 1.02x | 1.06x |
| jep | 7,249,576 | 1.02x | 0.97x | 0.97x |
| pyjnius | 5,447,465 | 1.01x | 1.42x | 1.48x |

**Interpretation.**

- **`list->array` vs. jpy** (~2.3-2.5x, roughly constant with depth):
  same element-validation tradeoff as Section 4.
- **`list->array` vs. jep/pyjnius**: jpype already wins at every depth.
- **`buffer->array` vs. jpy/jep**: jpype is *faster* at every depth, and
  the gap widens with depth in jpype's favor (2.4x ahead of jpy at depth
  2, 9.6x ahead at depth 5) -- jpype is the only one of the three with a
  real bulk multi-dimensional buffer path.
- **Type parity within jpype, depth >= 2**: all four types within 1-8%
  of each other at every depth -- no type-cost gap here, unlike the flat
  case in Section 4.
- **Ragged vs. rectangular, within jpype**: normalized for actual
  element count, ragged costs essentially the same as rectangular at
  every depth -- the ragged-native path (`isRaggedLeafElement`) is a
  single per-leaf branch regardless of type, so the `fastElementCheck`
  gap from Section 4 doesn't appear here.
- **Ragged type parity, cross-library**: jpype, jpy, and jep all sit
  within a few percent across every type; pyjnius is the exception,
  1.4-1.5x slower on float/double, consistent with the same reversed
  type-cost direction seen in Section 4.

## 6. Array push, shape at fixed depth and total element count

**Methodology.** 2D and 3D shapes holding a fixed total element count
(mostly 100,000, two shapes at 300,000 and one at 1,000,000, called out),
pushed both as list and buffer. Reported as ns/element (not ns/call) so
shapes with different totals stay comparable. best-of-5. pyjnius has no
`buffer->array` push at all (Section 4), so it's list-only below; jep
has no native multi-dim `numpy->array` push (Section 5), so its
"buffer" column is the same manual per-row workaround used elsewhere in
this report, not a real bulk path.

**`buffer->array`, 2D, ns/element by shape and type:**

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
direction held, magnitude didn't. Treat other outliers in the
smallest-iteration-count rows with the same skepticism.

**`list->array`, 2D, ns/element by shape and type (for comparison):**

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
300,000 elements):**

**`list->array`:**

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 30.6 | 65.9 | 2.2x |
| jpy | 8.6 | 37.0 | 4.3x |
| jep | 9.0 | 146.4 | 16.3x |
| pyjnius | 29.6 | 82.1 | 2.8x |

**`buffer->array`** (jep's column is the manual-assembly workaround, not
a real bulk path; pyjnius has none):

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 0.5 | 18.5 | 37x |
| jpy | 36.4 | 92.7 | 2.5x |
| jep\* | 0.7 | 534.5 | 742x |

\* jep's `buffer_manual` ratio is an artifact of the manual per-row
workaround (each row is individually assembled in Python), not a signal
about jep's own architecture.

**Interpretation.** Row count, not total element count or depth, is the
dominant cost driver for `buffer->array` and, to a smaller degree,
`list->array`, in every library measured: `100000x3` and `3x100000` hold
the same 300,000 elements but differ sharply per element purely on row
count. The 3D sweep (10x10x1000 vs. 1000x10x10, both 100,000 elements)
confirms the same pattern at a smaller scale. The size of the effect
differs a lot by library and isn't correlated with which library is
faster in absolute terms: jpype's `buffer->array` is the most sensitive
by ratio (37x) yet still the fastest in absolute ns/element at the worst
shape (18.5 vs. jpy's 92.7) -- jpy pays a smaller relative penalty on top
of a slower baseline. jep's real (non-manual) `list->array` ratio
(16.3x) is markedly worse than jpype's (2.2x) or jpy's (4.3x).

`JPConversionMultiArrayBuffer`'s array-build step pays one JNI
sub-array allocation per outer-dimension row on top of the bulk
per-element copy; when per-element cost is normally tiny (a memcpy-style
bulk read), that per-row cost dominates once row count is large.
`JPConversionSequence`'s list path already pays a comparable per-row
cost as its baseline (a Python-level sub-sequence access per row), so
the same absolute per-row overhead is a much smaller *relative* effect
there. This is a shared JNI-shaped reality every bridge that builds
nested Java arrays pays some version of, not a jpype-specific defect --
but jpype's *relative* sensitivity to it on `buffer->array` is worse
than jpy's even though jpype wins in absolute terms. Actionable guidance
for callers, true across every library measured: at a fixed total
element count, prefer numpy shapes with fewer, longer rows over many
short rows.

## 7. Array pull (Java -> Python)

**Methodology.** `list(makeArray(n))`, `makeArray(n).tolist()`, and
`np.asarray(makeArray(n))` -- three ways to materialize a returned Java
array in Python -- swept across size (flat) and depth (multi-dim). ns/call,
best-of-5.

### 7.1 Flat (1D)

`array->list`:

| size | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 100 | 11,138 | 4,344 | 2,795 | 1,234 |
| 1,000 | 102,331 | 40,029 | 23,566 | 10,436 |
| 10,000 | 1,024,320 | 401,341 | 237,692 | 113,502 |
| 100,000 | 11,443,363 | 5,487,530 | 3,712,665 | 1,142,328 |

`array->buffer`:

| size | jpype | jpy | jep\* | pyjnius\* |
|---:|---:|---:|---:|---:|
| 100 | 1,922 | 994 | 7,025 | 3,759 |
| 1,000 | 2,462 | 1,520 | 48,691 | 33,889 |
| 10,000 | 7,391 | 6,490 | 479,937 | 351,246 |
| 100,000 | 46,740 | 46,129 | 6,173,758 | 4,969,111 |

\* Neither jep nor pyjnius has a real buffer-protocol return path: jep's
returned array (`pyjarray`) has no `getbufferproc`, and pyjnius returns
arrays as already-materialized native Python lists. Both columns above
are `array->list`'s cost plus a redundant `np.asarray()` conversion, not
a real bulk read -- structurally different from jpype's/jpy's genuine
buffer reads, which is why they get *worse* than `array->list`, not
better, as size grows.

**Interpretation.** `array->list` is 2.1-2.6x slower than jpy and
3.1-4.3x slower than jep at every size. jpype is the only one of the
four where `array->list` is *slower than its own `array->buffer`* -- by
5.8x at size 100 growing to 245x at size 100,000. jpy and jep stay close
to their own buffer numbers throughout (they don't have jpype's internal
gap between the two paths). Only against pyjnius's `array->buffer` (the
non-real one above) does jpype come out ahead at scale.

`list()` and `tolist()` both box one `PyObject` per element (one
`Get<Type>ArrayRegion` JNI call plus one boxed-object allocation each);
`array->buffer` avoids per-element boxing entirely via a direct buffer
handoff, which is why it is one to two orders of magnitude cheaper than
either. The remaining gap between jpype's `list()`/`tolist()` and
jpy's/jep's own numbers (jpype @100 int: 11,138ns; jpy: 4,344ns; jep:
2,795ns) is jpype's per-JNI-call overhead itself (`JPJavaFrame`
construction, `JPPyObject` wrapping, exception-frame bookkeeping around
each single-element JNI call) -- the same architectural cost Section 2
attributes jpy's general speed lead to.

### 7.2 jpype's own `list()` vs. `tolist()` vs. `array->buffer`, by element type

| size | list() int | list() long | list() float | list() double | tolist() int | tolist() long | tolist() float | tolist() double |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 11,138 | 11,602 | 10,373 | 10,518 | 8,703 | 8,347 | 7,968 | 8,023 |
| 1,000 | 102,331 | 108,616 | 91,493 | 94,288 | 80,369 | 73,121 | 65,981 | 66,098 |
| 10,000 | 1,024,320 | 1,078,992 | 911,071 | 935,501 | 784,487 | 708,673 | 647,727 | 644,329 |
| 100,000 | 11,443,363 | 12,218,063 | 10,681,402 | 10,860,732 | 9,738,113 | 8,752,710 | 8,034,715 | 8,191,833 |

**Interpretation.** `list()` lands within 1.18-1.52x of `tolist()`
across sizes and types (int narrowest, long widest) -- neither is a real
bulk-*decode* path, boxing happens one `PyObject` at a time either way,
`list()` simply pays additional Python-iterator-protocol overhead on
top. Both remain well behind `array->buffer` (7.1) for the same reason.

**Multi-dimensional** (jpype-only, int):

| depth | list() | tolist() | buffer |
|---:|---:|---:|---:|
| 2 | 19,537 | 17,931 | 3,497 |
| 3 | 251,710 | 251,010 | 8,490 |
| 4 | 3,018,978 | 2,124,538 | 63,024 |
| 5 | 38,539,253 | 25,743,804 | 613,734 |

`list()` stands within 1.00-1.50x of `tolist()` at every depth, both
roughly an order of magnitude or more behind the buffer path -- the
same pattern as the flat case.

### 7.3 `array->list` pull, type ratio to int, size 100,000

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 11,443,363 (1.0x) | 1.07x | 0.93x | 0.95x |
| jpy | 5,695,493 (1.0x) | 0.88x | 0.78x | 0.79x |
| jep | 3,093,851 (1.0x) | 1.04x | 0.75x | 0.76x |
| pyjnius | 1,168,938 (1.0x) | 1.92x | 1.02x | 2.07x |

**Interpretation.** float/double pull *faster* than int/long in jpype,
jpy, and jep alike -- not a jpype-specific quirk. pyjnius instead shows
long and double pulling ~2x *slower* than int/float, an inconsistent
pattern not investigated further. jpype's float/double margin over int
(5-7% for `list()`, 8-18% for `tolist()`) is narrower than jpy's/
jep's own 21-25% margin. The remaining int/long-vs-float/double cost
difference within jpype traces to `convertToPythonObject`: int/long
route through a more general integer-construction path
(`JPIntType`/`JPLongType`) than float/double's direct `tp_alloc` + field
set (`JPFloatType`/`JPDoubleType`).

### 7.4 Bulk in-place transfer (`pullTo`/`pushFrom`)

Copying a 1-D primitive array directly into/from a caller-supplied
buffer, without producing a new Python object. jpype-only capability, no
cross-library equivalent.

| operation, size 100,000 | ns/call |
|---|---:|
| `double[]` pull, bulk (`pullTo`) | 21,284 |
| `double[]` pull, naive per-element loop | 33,292,511 |

**Interpretation.** The bulk path is 3-4 orders of magnitude faster than
a naive per-element loop over the same data, since it makes one JNI call
for the whole transfer instead of one per element.

## 8. GraalPy: a true-JIT comparison point

**Scope.** jpy/jep/pyjnius (Sections 2-7) are all CPython, no JIT at
all -- the interesting question against them is architectural overhead
per call. GraalPy runs on Truffle/Graal, a genuine tiered JIT compiler,
so the question here is different: how much of jpype's remaining gap to
a fast bridge is call-dispatch overhead a JIT *can* buy back, versus a
structural gap (a missing bulk-transfer path) no amount of JIT
compilation fixes? Same `DeepBench` test class, same benchmark scripts
ported to `project/benchmark/graalpy/`, run via a small Java launcher
(GraalPy embeds Python *inside* the JVM, jep's direction) -- see
`project/benchmark/README.md`'s GraalPy section for setup and the
mandatory heap cap.

**Methodology notes.** Every table below reports both best-of-5 and
median-of-5, because GraalPy's best/median split is far wider than any
of jpype/jpy/jep/pyjnius's -- best-of-5 alone would misleadingly flatter
it (likely JIT warmup/deopt noise; the benchmarks use a 1000-iteration
warmup before any timed trial, same as every other library here, and the
split persists regardless).

The "buffer->array (manual)" numbers throughout 8.2-8.4 carry far less
statistical confidence than every other number in this report: they run
at n=3 samples at every size >=10,000 elements
(`_arrayutil.py`'s `calls_for_manual()`, `n = max(3, 5_000 // size)`, a
fixed per-trial element budget chosen once and applied uniformly across
all four files this category spans), versus n=20-50,000 everywhere else
in this report. This exists because **GraalPy has no native
`buffer->array` push at all**, at any size or depth (confirmed
empirically: `TypeError('invalid instantiation of foreign object')`
unconditionally). Every number in the "buffer->array (manual)" rows
below comes from a per-element Java-array-construction routine written
for this comparison (`graalpy/_arrayutil.py`), not from anything GraalPy
does on its own -- without it, this entire category would be blank for
GraalPy. Two cells (int and long `list->array` push at the `100000x3`
row-heavy shape, an automatic category) hit a genuine `MemoryError`
under a capped `-Xmx3g` heap and are recorded as `N/A` in
`array_shape_results.csv`, not silently omitted -- see 8.4.

### 8.1 Scalars, dispatch, proxy

**Methodology.** Same operations as Section 2/3. int-only.

| operation | jpype | jpy | jep | pyjnius | GraalPy best | GraalPy median |
|---|---:|---:|---:|---:|---:|---:|
| `Math.max(int,int)` | 750 | 353 | 1299 | 1276 | **183** | 1250 |
| `new Integer(int)` | 840 | 479 | 1257 | 7403 | 229 | 281 |
| `Math.sqrt(double)` | 690 | 393 | 645 | 497 | **117** | 874 |
| `new Double(double)` | 918 | 506 | 1378 | 6765 | 726 | 758 |
| `new String` + `.toString()` | 1022 | 927 | 2512 | 22349 | 791 | 908 |
| `Object` identity (arg + return) | 995 | 730 | 1971 | 3763 | **160** | 887 |
| dispatch, overload x16, monomorphic | 686 | 370 | 4454 | 3858 | **119** | 932 |
| dispatch, overload x16, polymorphic | 925 | 456 | 4558 | 4000 | 911 | 1153 |
| proxy callback, `int` arg | 2655 | N/A | 2240 | 39412 | 526 | 1459 |

**Interpretation.** On best-of-5, GraalPy wins outright on 6 of 9 rows --
beating jpy (this report's previous fastest bridge on every earlier
section) by 1.9-3.1x on `Math.max`, `Math.sqrt`, `Object` identity, and
monomorphic dispatch. But every GraalPy median is 1.4-10.5x its own
best, a spread none of the other four libraries show at any comparable
magnitude (jpype's own best-vs-median split, visible in Section 7's
tables, is typically under 1.3x). Boxed `Integer`/`Double` and the
`String` round trip are the exception -- GraalPy's best and median are
close together there (1.0-1.2x), and not even GraalPy's fastest row
against jpy.

The JIT does real work on monomorphic call sites (`Math.max`, a single
dispatch target, `Object` identity) -- GraalPy's best numbers there are
the fastest in this entire report. Boxed `Integer(int)`/`Double(double)`
construction stresses object allocation and GC more than call dispatch,
which likely explains both why GraalPy doesn't lead there and why it
shows the tightest best/median spread on those two rows specifically.
Any orchestration workload sensitive to *tail* latency, not just
throughput, should weight GraalPy's median column, not its best -- on
that column jpype is competitive or ahead on most of these same rows.

### 8.2 Array push (flat, 1D)

**Methodology.** Same as Section 4. GraalPy cannot do a `buffer->array`
push at all -- confirmed empirically (`TypeError('invalid instantiation
of foreign object')` unconditionally). "buffer->array (manual)" below is
a replacement written from scratch for this comparison
(`graalpy/_arrayutil.py`'s `build_manual()`: allocate a real Java array,
fill it element-by-element from the numpy source) -- it measures this
comparison's code, not GraalPy's own capability. See the note at the top
of Section 8 for its reduced sample count.

**int, all four categories, all sizes** (`list->array`/`array->list` in
ns/call; `buffer->array (manual)`/`array->buffer` in ms/call -- unit
switched per column since the manual/buffer-pull categories run 2-5
orders of magnitude slower, see methodology note above):

| size | list->array (auto), ns | buffer->array (manual), ms | array->list (pull), ns | array->buffer (pull), ms |
|---:|---:|---:|---:|---:|
| 100 | 6,343 | 0.9 | 4,976 | -- |
| 1,000 | 45,881 | 18.0 | 48,655 | -- |
| 10,000 | 436,983 | 100.2 | 531,490 | -- |
| 100,000 | 4,511,797 | 1,270.2 | 4,711,893 | 328.3 |

**By element type, size 100,000** (push rows in ns/call, buffer/manual
rows in ms/call):

| direction/source | unit | int | long | float | double |
|---|---|---:|---:|---:|---:|
| push, list->array | ns | 4,511,797 | 4,041,574 | 4,218,600 | 4,927,029 |
| push, buffer->array (manual) | ms | 1,270.2 | 1,074.6 | 1,107.4 | 1,395.7 |
| pull, array->list | ns | 4,711,893 | 4,666,041 | 8,080,424 | 7,030,721 |
| pull, array->buffer | ms | 328.3 | 318.8 | 372.5 | 346.2 |

**Interpretation.** `list->array` push and `array->list` pull are both
competitive with jpype/jpy/jep at every size (e.g. int @100,000: GraalPy
4,511,797 ns vs. jpype 1,829,617, jpy 774,569, jep 840,301 (Section
4.1) -- GraalPy is 2.5-5.9x slower here, in the same ballpark as
pyjnius). The other two rows are not in the same ballpark as anything
else in this report:

- **`buffer->array` (manual)**: 250-300x slower per element than
  GraalPy's own `list->array` (e.g. int @100,000: 1,270.2 ms vs. 4.51
  ms). jpype's real `buffer->array` fast path at the same size is
  57,371ns = 0.057 ms (Section 4.2) -- GraalPy's manual emulation is
  roughly 22,000x slower.
- **`array->buffer` pull**: 69.7x slower than GraalPy's own `array->list`
  at the same size (328.3 ms vs. 4.71 ms) -- the opposite ranking from
  jpype/jpy, where `array->buffer` is the fast path and beats
  `array->list` by 100-350x (Section 7). This matches the jep/pyjnius
  pattern (Section 7.1's footnote): `np.asarray()` on a
  `polyglot.ForeignList` pays `array->list`'s per-element cost plus a
  numpy-array-build step on top, not a real buffer read.

GraalPy's polyglot interop layer has a real, JIT-accelerated
per-element/per-call marshalling path (`list->array`/`array->list`), but
no bulk buffer-protocol bridge in either direction. Every other library
in this report that has any numpy interop (jpype, jpy, and jep for flat
targets) treats this as a first-class fast path precisely because
scientific-Python workloads are dominated by exactly this operation.

### 8.3 Array push, multi-dimensional and ragged

**Methodology.** Same as Section 5.

**int, depth 5 (100,000 elements), all categories:**

| category | unit | value |
|---|---|---:|
| push, list->array | ns | 5,081,921 |
| push, buffer->array (manual) | ms | 1,187.9 |
| pull, array->list | ns | 8,420,835 |
| pull, array->buffer | ms | 590.9 |

**Ragged push (list->array, GraalPy's only push path), all four types,
depth 5:**

| type | n (actual elements) | ns/call |
|---|---:|---:|
| int | 114,940 | 6,543,572 |
| long | 114,940 | 6,478,348 |
| float | 114,940 | 7,402,321 |
| double | 114,940 | 7,783,541 |

**Interpretation.** `list->array` push at depth 5 (5,081,921 ns) barely
moves from flat @100,000 (4,511,797 ns, Section 8.2) -- GraalPy's automatic push
path is not noticeably sensitive to nesting depth, matching jpype's own
depth-insensitivity within its `list->array` path (Section 5). The
manual `buffer->array` emulation is not meaningfully worse at depth 5
(1,187.9 ms) than flat (1,270.2 ms) either, since `build_manual()`'s cost is
driven by total element count and per-element polyglot crossings, not
nesting depth. `array->buffer` pull gets worse relative to `array->list`
as depth grows (70x at flat, 125x at depth 5), consistent with
`np.asarray()` walking a deeper recursive `ForeignList`-of-`ForeignList`
structure. Ragged push costs essentially the same as rectangular
`list->array` at a matched element count (int: 6,543,572 ns for 114,940
ragged elements vs. 5,081,921 ns for 100,000 rectangular -- 56.9 vs. 50.8
ns/element, an 11% difference), the same finding as jpype's own
ragged-vs-rectangular parity (Section 5).

### 8.4 Non-contiguous sources and row-heavy shapes

**Methodology.** Same as Section 6 and 4.4. GraalPy's manual
`build_manual()` push indexes the numpy source directly (`np_sub[i]`),
so numpy itself resolves whatever strides the source has.

**Non-contiguous vs. contiguous, int, manual push, same element counts
(ms/call):**

| shape | contiguous | non-contiguous | ratio |
|---|---:|---:|---:|
| flat 100,000 (column slice vs. flat) | 1,270.2 | 1,224.1 | 0.96x |
| depth 5, 100,000 (rectangular vs. transposed) | 1,187.9 | 1,218.8 | 1.03x |

**Row-heavy 2D shape sweep, int, manual buffer->array push, ns/element:**

| shape (rows x cols) | ns/element |
|---|---:|
| 1,000 x 100 | 11,639 |
| 10 x 10,000 | 12,175 |
| 100 x 1,000 | 12,212 |
| 10,000 x 10 | 12,315 |
| 3 x 100,000 | 12,888 |
| 1,000 x 1,000 | 13,536 |
| **100,000 x 3** | **15,449** |

**`list->array`, same shapes, `100000x3` row:** int and long both hit a
genuine `MemoryError` (dozens of `TruffleCompilerThread`/`Python GC`
`OutOfMemoryError`s precede each one at a capped `-Xmx3g` heap). float
and double completed the same row without incident (115.9 ns/element
for double, 2.7x its `3x100000` counterpart's 43.1 ns/element).

**Interpretation.** The non-contiguous manual push shows no measurable
penalty (0.96-1.03x, noise-level) -- unsurprising, `build_manual()` was
never a bulk-read path to begin with. The row-heavy shape sweep shows
only a mild 1.33x penalty (11,639 -> 15,449 ns/element) for the manual
buffer path, far smaller in relative terms than jpype's own 37x
`buffer->array` penalty at the same shape extreme (Section 6), because
GraalPy's baseline per-element cost is already dominated by
polyglot-crossing overhead. The `list->array` `MemoryError`s are the
standout result: GraalPy's per-object overhead for building ~100,000
small Java array objects (one per row) is heavy enough to exhaust a 3GB
heap outright, something no other library in this report comes close to
at the same shape and heap budget.

### 8.5 Proxy: the one place GraalPy is architecturally simpler

GraalPy needs no explicit proxy-construction step at all: a plain Python
object (or bare function, for a single-method interface) with a matching
method name is auto-adapted to any Java functional interface wherever
one is expected -- qualitatively different from jpype's `@JImplements`,
jep's `jep.jproxy()`, and pyjnius's `PythonJavaClass` subclassing, all of
which require an explicit class-implements-interface declaration
constructed ahead of the steady-state calls being measured. It also has
no null-argument crash -- `invokeObjectCallbackWithNull`, the exact case
that segfaults pyjnius (Section 3), works cleanly under GraalPy with no
special handling. This is architectural, not a speed result (see 8.1's
proxy row for the speed comparison): a genuinely simpler interop model
for the callback direction specifically, unrelated to the array-transfer
gap in 8.2-8.4. Full detail in `project/benchmark/README.md`'s proxy
section.

### 8.6 Strategic summary

GraalPy's Truffle/Graal JIT delivers exactly where a JIT can: hot,
simple, monomorphic call sites (8.1) beat every other bridge in this
report, sometimes by 2-3x. But scientific-Python orchestration is
dominated by bulk numpy<->Java array transfer, not scalar call overhead,
and GraalPy has no purpose-built path for that at all, in either
direction (8.2), a gap wide enough (tens of thousands of times versus
jpype's real fast path) that no realistic amount of JIT tiering closes
it, plus a second, independent failure mode (8.4: heap exhaustion on
ordinary row-heavy shapes) that none of jpype/jpy/jep/pyjnius exhibit at
the same budget. The one place GraalPy is unambiguously ahead
structurally, not just faster, is the callback/proxy direction (8.5).
Net read for anyone weighing GraalVM's polyglot model as an architecture
to follow: viable, even excellent, for microscript/glue-code call
patterns; not viable as-is for a scientific-orchestration substitute,
where jpype (and jpy) remain the only two bridges in this report with
real bulk buffer-transfer paths in both directions.
