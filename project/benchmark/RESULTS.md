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

**Push methodology note.** Sections 4-6's push benchmarks call a Java
method on the converted array so the JIT can't dead-code-eliminate the
conversion; earlier editions of this report used `DeepBench.sum{Type}Array`
for that purpose, whose own O(elements) summation loop runs *inside* the
timed call. That loop's cost turned out to be real and sharply
type-dependent -- measured directly, with an already-converted Java array
(no conversion in the call at all), at n=100,000: `sumIntArray` 8,414ns,
`sumLongArray` 23,771ns, `sumFloatArray`/`sumDoubleArray` ~68,000ns each
(int/long's accumulation into a `long` auto-vectorizes; float/double's
sequential-dependency accumulation into a `double` does not). Against
`list->array`'s push cost (hundreds of thousands to millions of ns at this
size) that's a small fraction and doesn't change any conclusion; against
`buffer->array`'s (tens of thousands of ns) it was a large fraction and
did -- it was the actual explanation for the "float costs ~2x int despite
identical byte width" pattern an earlier edition attributed to it, in one
place, without recognizing the same confound applied everywhere else `sum`
was used for push timing. Every push number in Sections 4-6 now instead
calls a `DeepBench.void{Type}Array`/`void{2,3,4,5}D{Type}Array` method that
touches nothing and returns nothing (measured directly, same setup: 360-450ns
for all four types, effectively pure per-call overhead) -- library-vs-library
comparisons at a fixed type were already valid either way (`sum`'s cost is
identical Java bytecode regardless of which binding invokes it, so it
cancels in a same-type difference), but by-type ratios inside a single
library were not, and are corrected here.

**Sample size.** The iteration-count formula above means the largest push/pull
benchmarks in this report run at n=20-50 raw calls (best-of-5 trials each).
Combined with the push-methodology note above, treat any by-type ratio
in Sections 4-6 as directional at the last significant figure, not as a
precise measurement -- the qualitative pattern (which type is cheaper, by
roughly how much) is the reliable part.

## 2. Scalars, boxing, strings, object identity

**Methodology.** Single-call round trips: a static method call
(`Math.max`, `Math.sqrt`), a boxed-object construction
(`new Integer`/`new Double`), a `String` round trip, and passing/returning
a plain `Object` reference. ns/call, best-of-5.

| operation | jpype | jpy | jep | pyjnius |
|---|---:|---:|---:|---:|
| `Math.max(int,int)` | 672 | 353 | 1299 | 1276 |
| `new Integer(int)` | 811 | 479 | 1257 | 7403 |
| `Math.sqrt(double)` | 706 | 393 | 645 | 497 |
| `new Double(double)` | 933 | 506 | 1378 | 6765 |
| `new String` + `.toString()` | 1022 | 927 | 2512 | 22349 |
| `Object` identity (arg + return) | 995 | 730 | 1971 | 3763 |

jpype's `Math.max`/`new Integer` rows were re-run for this edition (not
byte-identical to the previous one) since scalar `int` returns go
through the same `convertToPythonObject` this pass's redundant-allocation
fix touched (Section 7.1); `Math.sqrt`/`new Double` were re-run too, for
comparison, since `double` wasn't touched by that fix. `Math.max` moved
from 750 to 672ns (-10%) and `new Integer` from 840 to 811ns (-3%), both
in the expected direction; `Math.sqrt`/`new Double` moved by a similar
few percent in *both* directions across the two rows, which is the more
likely explanation for all four deltas here -- at a ~750ns call, a
single-allocation removal is a plausible few-percent effect, but it's not
distinguishable from ordinary run-to-run noise at n=20-50 (Section 1) in
either the touched or untouched rows. Not re-measured with a
controlled, larger-n microbenchmark the way Section 7.1's redundant
allocation was; treat this row's specific deltas as unconfirmed.

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

**Methodology.** `voidIntArray(source)` and its long/float/double
counterparts (see Section 1's push methodology note), sweeping size
(100/1k/10k/100k) and two source kinds: a plain Python list, and a
`Py_buffer`-backed object (numpy). ns/call, best-of-5. The size-sweep
tables below are int-only for readability; the type breakdown beneath
them covers all four libraries.

### 4.1 `list->array` push (method argument)

| size | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 100 | 2,983 | 1,186 | 2,492 | 3,017 |
| 1,000 | 20,762 | 8,142 | 14,645 | 26,054 |
| 10,000 | 201,954 | 76,972 | 136,955 | 275,144 |
| 100,000 | 1,905,281 | 791,520 | 1,328,300 | 4,653,195 |

### 4.2 `buffer->array` push (method argument)

Argument conversion for a numpy array against a flat primitive parameter
(e.g. `int[]`) hands the source buffer directly to a single Java call
(`Support.fillFlatFromBuffer`), which performs any dtype coercion
(signed/unsigned int 1/2/4/8 bytes, float32/float64, float16) and takes
an explicit stride parameter rather than requiring a C-contiguous
source, so a sliced/strided numpy column takes the same code path as a
fully contiguous array -- architecturally, not necessarily at the same
*cost*: Section 4.4 measures a non-contiguous 1D source at this same
size running 2.6-3.9x slower than the contiguous number above, which
looked at first like a regression but bisects out as benchmark noise
(see 4.4) -- non-contiguous pushes through this path are consistently
noisier run-to-run than contiguous ones, not consistently costlier. A
negative-stride source (`arr[::-1]`) falls back to a per-element path.

pyjnius has no `buffer->array` push at all (see 4.4).

| size | jpype | jpy | jep |
|---:|---:|---:|---:|
| 100 | 1,206 | 436 | 836 |
| 1,000 | 1,571 | 768 | 1,174 |
| 10,000 | 5,876 | 4,765 | 5,915 |
| 100,000 | 54,480 | 37,371 | 45,630 |

**Interpretation.** jpype trails jpy by 10-46% and jep by 3-19% across
all four types at 100,000 elements (int shows the widest gap to both;
double is closest, and edges ahead of jep specifically, 92,204 vs
95,957ns -- see 4.5 for the full by-type table). At small sizes,
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
| `int[100000]` method-argument push (4.2, for comparison) | 54,480 |

**Interpretation.** Slice assignment is cheaper than the method-argument
push at the same size because it skips both the fresh-array allocation
and the critical-section pinning the argument-push path still pays for.

### 4.4 Non-contiguous sources

A numpy column slice or transposed array (non-unit stride) reaches the
same bulk path as a contiguous buffer via an explicit `strideBytes`
parameter, rather than requiring a C-contiguous source.

| library | `int[100000]`, non-contiguous column slice |
|---|---:|
| jpype | 213,581 (best), 345,015 (median) |
| jep | 60,201 |
| jpy | fails (`RuntimeError: no matching Java method overloads found`) |
| pyjnius | N/A (no buffer push at all) |

**Interpretation.** jep's non-contiguous push came down with the push
methodology fix (Section 1), as expected. jpype's number looks like a
2.6x regression against the previous edition's 80,715ns, but it isn't
one: bisecting this exact benchmark (via disposable venvs built from
`git worktree` checkouts, per this repo's CLAUDE.md isolation guidance)
against both the commit immediately before this session's changes and
the commit immediately before the `strideBytes` bulk-JNI push path was
even added reproduces the same 170,000-215,000ns (best) /
270,000-345,000ns (median) range at every point checked -- including
commits where none of this session's code changes are present. No
commit in this session's history, or in the one that added the bulk
path, ever measures near 80,715ns; three repeated runs at current HEAD
land in the same range (185,163-211,733ns best). The 80,715ns figure in
the previous edition was a single unrepresentative low sample, not a
baseline that regressed -- the wide best-vs-median spread this row
shows (a 30-60% gap) is itself the signature of a noisy measurement,
consistent with that explanation. Nothing in this session's C++ changes
(the `fastElementCheck`/`PyList_CheckExact` list-push work, or the
`convertLong` signature change) touches this benchmark's code path at
all -- it exercises `tryFastBufferPush`/`fillFlatIntoArray`, an
unrelated buffer-handoff mechanism untouched by either fix. jpy's
buffer matcher requests `PyBUF_SIMPLE` (no stride support at all) and
fails outright on any non-contiguous 1D source; its ND non-contiguous
(transposed multi-dim) cases do succeed, at the same cost as the
contiguous case. jep succeeds on both 1D (real numpy fast path) and ND
non-contiguous sources (manual per-row assembly, e.g.
`double[][][][][]` (10^5): 24,222,693ns). Full capability matrix in
`project/comparison.md`.

### 4.5 By element type, size 100,000

Two different `list->array` inputs are reported separately here, since
they measure genuinely different things: a Python list whose element
type already matches the target array (a float list for
`float[]`/`double[]`), and a plain Python **int** list pushed into a
`float[]`/`double[]` target -- an ordinary, idiomatic thing for a caller
to write (`javaMethod([1, 2, 3])` against a `double[]` parameter) that an
earlier edition of this report conflated with the matched-type case.

`list->array`, matched element type:

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 1,905,281 (1.0x) | 1,994,072 (1.05x) | 1,736,646 (0.91x) | 1,766,987 (0.93x) |
| jpy | 791,520 (1.0x) | 848,558 (1.07x) | 731,562 (0.92x) | 805,252 (1.02x) |
| jep | 1,328,300 (1.0x) | 1,301,621 (0.98x) | 1,006,701 (0.76x) | 952,065 (0.72x) |
| pyjnius | 4,653,195 (1.0x) | 4,228,499 (0.91x) | 3,021,846 (0.65x) | 3,484,234 (0.75x) |

`list->array`, **widening from a Python int list** (float/double only --
int/long have no widening case, a plain int list already *is* their
matched-type input):

| library | float | double | float vs. matched-type float | double vs. matched-type double |
|---|---:|---:|---:|---:|
| jpype | 4,301,929 | 4,317,766 | 2.48x | 2.44x |
| jpy | 1,174,941 | 1,235,077 | 1.61x | 1.53x |
| jep | 2,726,655 | 2,740,411 | 2.71x | 2.88x |
| pyjnius | 2,939,610 | 4,273,681 | 0.97x | 1.23x |

`buffer->array` (pyjnius has none; numpy's own dtype already fixes the
element type, so there's no separate widening case here -- pushing an
`int32` numpy array into a `double[]` parameter is a dtype-coercion
question numpy itself resolves via casting rules, not something this
report's `buffer->array` benchmarks exercise):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 54,480 (1.0x) | 97,818 (1.80x) | 42,919 (0.79x) | 92,204 (1.69x) |
| jpy | 37,371 (1.0x) | 81,999 (2.19x) | 37,906 (1.01x) | 83,511 (2.23x) |
| jep | 45,630 (1.0x) | 91,056 (2.00x) | 41,630 (0.91x) | 95,957 (2.10x) |

**Interpretation.**

- **`list->array`, jpype vs. jpy**: jpy's array matching doesn't inspect
  elements before committing to a conversion; jpype validates every
  element up front to support correct Java-style overload
  disambiguation -- an architectural tradeoff, not a gap to close.
- **`list->array`, matched-type int vs. long/float/double, within
  jpype**: all four types land within 0.91-1.05x of each other.
  `setArrayRange` (the loop that does the actual per-element conversion)
  has a `PyList_CheckExact` fast loop -- `PyList_GET_ITEM` plus a direct
  `PyLong`/`PyFloat` read, skipping the generic sequence-protocol
  dispatch -- in all four of `jp_inttype.cpp`/`jp_longtype.cpp`/
  `jp_floattype.cpp`/`jp_doubletype.cpp`. `fastElementCheck` overrides
  (used only for overload-resolution quality, not the conversion itself)
  exist on all four primitive types for the same reason, but were not
  what closed this gap. This parity is real, but it is specifically a
  matched-Python-type result -- it says the fast loop works, not that
  `list->array` is type-insensitive in general (see the widening table).
- **`list->array`, widening from int, within jpype (2.4-2.5x)**: this is
  the case an earlier edition of this report measured under the
  "float/long/double" columns without separating it from matched-type
  input, and attributed to the missing `fastElementCheck`/fast-loop
  overrides that were added in that pass. That attribution doesn't
  survive this edition's split: the fast loop's own `PyFloat_CheckExact`
  check never matches a `PyLong` element, so an int list falls straight
  to the general per-element path regardless of the fast-loop fix --
  which is exactly why the widening numbers here are close to what that
  earlier edition originally reported (2.28x for float, 2.28x for
  double) while the matched-type numbers moved to near-parity. The
  earlier 2.3x figure wasn't measurement noise; it was, and remains, the
  real cost of this specific (common) input pattern. Closing it would
  need the fast loop to also accept `PyLong` elements for a float/double
  target (valid Java widening) -- not attempted here.
- **`list->array` type parity, cross-library, matched-type**: all four
  libraries show float/double at or below int/long's cost (0.65-1.07x),
  not above it -- jpy and jep track jpype's near-parity result;
  pyjnius's float/double lead is the widest of the four, consistent
  across every library here rather than an outlier.
- **`list->array`, widening from int, cross-library**: jpy (1.5-1.6x)
  and jep (2.7-2.9x) show the same directional penalty as jpype: none of
  the three libraries' float/double fast paths accept a Python int
  element without falling back. pyjnius is again the exception (0.97x
  and 1.23x) -- it doesn't show a widening penalty at all, consistent
  with its float/double lead in the matched-type table; whatever pyjnius
  does for float/double array construction isn't type-checking the
  Python element the way the other three libraries' fast paths do.
- **`buffer->array` type ratios**: now that Java-side per-type summation
  cost is out of the timed call (see Section 1's push methodology note --
  an earlier edition of this report measured `sum{Type}Array` here and
  attributed float's inflated cost to that method's own non-vectorizable
  accumulation loop, without realizing the same confound touched every
  other push table in Sections 4-6 too), the numbers track element byte
  width the way a bulk buffer copy should: long/double (8 bytes) cost
  1.7-2.2x int/float (4 bytes) across all three libraries, and int/float
  -- both 4 bytes -- land within 1-21% of each other (float is
  consistently the *cheaper* of the two, jpype's widest such gap at 21%).
  This is the cleanest physical story in the whole by-type comparison,
  and it only emerged once the confound was removed. Unlike
  `list->array`, this table has no separate widening case to conflate
  with it (see the table note above).

## 5. Array push, multi-dimensional (depth 2-5, rectangular and ragged)

**Methodology.** `void{2,3,4,5}D{Type}Array(nested_list)` (see Section
1's push methodology note), 10\*\*depth elements, uniform 10-wide shape
(rectangular) and a second sweep with
irregular sibling lengths at every level (ragged, fixed seed for
reproducibility). ns/call, best-of-5.

**`list->array` push (int, rectangular):**

| depth | jpype | jpy | jep | pyjnius |
|---:|---:|---:|---:|---:|
| 2 | 5,393 | 2,009 | 7,154 | 4,866 |
| 3 | 45,925 | 17,306 | 65,772 | 42,665 |
| 4 | 438,572 | 168,455 | 641,714 | 432,587 |
| 5 | 4,282,564 | 1,747,970 | 6,495,953 | 5,133,107 |

**`buffer->array` push (int; jep has no automatic multi-dim buffer
path):**

| depth | jpype | jpy | jep\* |
|---:|---:|---:|---:|
| 2 | 1,916 | 5,332 | 20,395 |
| 3 | 6,382 | 54,050 | 199,846 |
| 4 | 68,381 | 548,059 | 2,013,800 |
| 5 | 589,825 | 5,409,156 | 20,274,290 |

\* jep raises `TypeError` for numpy input to a multi-dim argument; the
jep column is a manual per-row workaround, slower than jep's own
`list->array` at this row size.

**jpype by element type** (`list->array`, rectangular -- all four types
within 2-10% of each other at every depth):

| depth | int | long | float | double |
|---:|---:|---:|---:|---:|
| 2 | 5,393 | 5,284 | 5,484 | 5,515 |
| 3 | 45,925 | 45,498 | 48,045 | 49,414 |
| 4 | 438,572 | 438,923 | 445,475 | 454,450 |
| 5 | 4,282,564 | 4,356,063 | 4,460,878 | 4,705,742 |

**jpype, ragged vs. rectangular** (int; ragged `n` is the tree's actual
element count, not exactly 10\*\*depth):

| depth | rectangular | ragged (n) |
|---:|---:|---:|
| 2 | 5,393 | 3,429 (n=53) |
| 3 | 45,925 | 48,899 (n=1,034) |
| 4 | 438,572 | 374,519 (n=8,073) |
| 5 | 4,282,564 | 5,269,898 (n=114,940) |

**Ragged push, type parity across all four libraries** (depth 5,
n=114,940 elements; long/float/double shown as a ratio to that library's
own int column):

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 5,269,898 | 1.04x | 1.08x | 1.08x |
| jpy | 2,032,404 | 1.07x | 1.01x | 1.05x |
| jep | 7,897,254 | 1.03x | 0.95x | 0.95x |
| pyjnius | 6,371,139 | 1.06x | 1.38x | 1.40x |

**Interpretation.**

- **`list->array` vs. jpy** (~2.4-2.7x, roughly constant with depth):
  same element-validation tradeoff as Section 4.
- **`list->array` vs. jep/pyjnius**: jpype already wins at every depth.
- **`buffer->array` vs. jpy/jep**: jpype is *faster* at every depth, and
  the gap widens with depth in jpype's favor (2.8x ahead of jpy at depth
  2, 9.2x ahead at depth 5) -- jpype is the only one of the three with a
  real bulk multi-dimensional buffer path.
- **Type parity within jpype, depth >= 2**: all four types within 2-10%
  of each other at every depth, matching the flat-push matched-type
  parity in Section 4.5 -- this sweep's nested lists use a genuine
  Python `float` at float/double leaves (`leaf = float if label in
  ('float', 'double') else int`, `array_multidim.py`), not an int being
  widened, so it's the same fast-path scenario Section 4.5 measures, not
  the widening one.
- **Ragged vs. rectangular, within jpype**: normalized for actual
  element count, ragged costs essentially the same as rectangular at
  every depth -- the ragged-native path (`isRaggedLeafElement`) is a
  single per-leaf branch regardless of type, the same shape as the flat
  push's `PyList_CheckExact` fast loop (Section 4.5).
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
| 3 x 100,000 | 0.51 | 4.52 | 0.47 | 1.03 |
| 10 x 10,000 | 0.65 | 1.40 | 0.55 | 0.89 |
| 100 x 1,000 | 0.59 | 1.56 | 0.60 | 1.08 |
| 1,000 x 100 | 0.88 | 1.70 | 0.92 | 1.37 |
| 1,000 x 1,000 | 0.53 | 1.21 | 0.58 | 1.13 |
| 10,000 x 10 | 4.82 | 5.46 | 4.97 | 5.19 |
| **100,000 x 3** | **15.52** | 18.90 | 17.96 | 21.41 |

**`list->array`, 2D, ns/element by shape and type (for comparison):**

| shape (rows x cols) | int | long | float | double |
|---|---:|---:|---:|---:|
| 3 x 100,000 | 32.76 | 34.59 | 31.66 | 35.16 |
| 10 x 10,000 | 31.89 | 32.33 | 31.43 | 31.53 |
| 100 x 1,000 | 31.92 | 31.52 | 31.11 | 31.38 |
| 1,000 x 100 | 32.25 | 32.46 | 32.06 | 32.79 |
| 1,000 x 1,000 | 32.87 | 37.55 | 31.46 | 33.38 |
| 10,000 x 10 | 41.42 | 42.08 | 41.08 | 43.63 |
| **100,000 x 3** | **67.93** | 74.04 | 67.83 | 72.70 |

**Cross-library confirmation, int, ns/element at the two extreme 2D
shapes (`3x100000` = few long rows, `100000x3` = many short rows, same
300,000 elements):**

**`list->array`:**

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 32.76 | 67.93 | 2.1x |
| jpy | 8.01 | 35.56 | 4.4x |
| jep | 13.61 | 163.63 | 12.0x |
| pyjnius | 30.55 | 83.37 | 2.7x |

**`buffer->array`** (jep's column is the manual-assembly workaround, not
a real bulk path; pyjnius has none):

| library | 3x100000 | 100000x3 | ratio |
|---|---:|---:|---:|
| jpype | 0.51 | 15.52 | 30.4x |
| jpy | 37.43 | 94.74 | 2.5x |
| jep\* | 0.70 | 608.25 | 869x |

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
by ratio (30x) yet still the fastest in absolute ns/element at the worst
shape (15.5 vs. jpy's 94.7) -- jpy pays a smaller relative penalty on top
of a slower baseline that barely moved when the push-methodology fix
(Section 1) landed, meaning jpy's 2D buffer numbers were already
conversion-dominated, not summation-dominated, unlike jpype's smallest
shapes. jep's real (non-manual) `list->array` ratio (12.0x) is markedly
worse than jpype's (2.1x) or jpy's (4.4x).

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
| 100 | 9,753 | 4,344 | 2,795 | 1,234 |
| 1,000 | 84,508 | 40,029 | 23,566 | 10,436 |
| 10,000 | 843,648 | 401,341 | 237,692 | 113,502 |
| 100,000 | 9,906,480 | 5,487,530 | 3,712,665 | 1,142,328 |

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

**Interpretation.** `array->list` is 1.8-2.3x slower than jpy and
2.7-3.6x slower than jep at every size. jpype is the only one of the
four where `array->list` is *slower than its own `array->buffer`* -- by
5.1x at size 100 growing to 212x at size 100,000. jpy and jep stay close
to their own buffer numbers throughout (they don't have jpype's internal
gap between the two paths). Only against pyjnius's `array->buffer` (the
non-real one above) does jpype come out ahead at scale.

pyjnius's own `array->list` is the fastest number in this whole table,
and the gap to jpype widens with size (1.234 vs jpype's 9,753ns at size
100, to 1,142,328 vs jpype's 9,906,480ns at 100,000 -- an 8.7x gap at
scale) despite pyjnius losing almost every other benchmark in this
report. Checked directly rather than assumed: `make{Type}Array`'s
return value is confirmed a fresh, fully-populated Python `list` inside
the timed call every time (verified via `type()`/`len()`/`sum()` on a
direct call, not a lazy or reused object), so this isn't a
materialize-outside-the-timed-region artifact. The real mechanism is
that pyjnius's Cython JNI-array-to-list conversion builds one plain
`PyLong`/`PyFloat` per element straight from the read buffer, while
jpype's own per-element boxing (`convertToPythonObject`) builds a
jpype-specific numeric subtype instance carrying Java-value metadata on
top (the slot that makes `isinstance(x, jpype.JInt)` and similar work) --
a genuinely heavier object than a plain `int`/`float`, on top of the same
per-element JNI-call overhead described below. int/long/short/byte used
to also pay for a second, wholly redundant allocation beyond that (a
throwaway plain `PyLong` built and then immediately unpacked straight
back out, just to hand its value to the real wrapper's constructor) --
removed in this pass (`convertLong` now takes the native value directly;
`jp_inttype.cpp`/`jp_longtype.cpp`/`jp_shorttype.cpp`/`jp_bytetype.cpp`),
which is why `array->list`/`tolist()` below are noticeably cheaper than
before for those two types. The remaining gap against pyjnius is the
wrapper-subtype construction itself, an architectural cost, not further
redundant work.

`list()` and `tolist()` both box one `PyObject` per element (one
`Get<Type>ArrayRegion` JNI call plus one boxed-object allocation each);
`array->buffer` avoids per-element boxing entirely via a direct buffer
handoff, which is why it is one to two orders of magnitude cheaper than
either. The remaining gap between jpype's `list()`/`tolist()` and
jpy's/jep's own numbers (jpype @100 int: 9,753ns; jpy: 4,344ns; jep:
2,795ns) is jpype's per-JNI-call overhead itself (`JPJavaFrame`
construction, `JPPyObject` wrapping, exception-frame bookkeeping around
each single-element JNI call) -- the same architectural cost Section 2
attributes jpy's general speed lead to.

### 7.2 jpype's own `list()` vs. `tolist()` vs. `array->buffer`, by element type

| size | list() int | list() long | list() float | list() double | tolist() int | tolist() long | tolist() float | tolist() double |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 100 | 9,753 | 10,139 | 10,287 | 10,411 | 7,759 | 6,916 | 7,696 | 7,658 |
| 1,000 | 84,508 | 88,827 | 90,874 | 92,289 | 63,279 | 55,696 | 63,153 | 63,093 |
| 10,000 | 843,648 | 871,045 | 895,955 | 916,370 | 611,811 | 543,112 | 620,346 | 620,752 |
| 100,000 | 9,906,480 | 10,312,611 | 10,568,598 | 10,816,051 | 7,560,779 | 7,205,120 | 7,714,589 | 7,669,728 |

**Interpretation.** `list()` lands within 1.03-1.60x of `tolist()`
across sizes and types (int narrowest at small sizes, long widest) --
neither is a real bulk-*decode* path, boxing happens one `PyObject` at a
time either way, `list()` simply pays additional Python-iterator-protocol
overhead on top. Both remain well behind `array->buffer` (7.1) for the
same reason.

**Multi-dimensional** (jpype-only, int):

| depth | list() | tolist() | buffer |
|---:|---:|---:|---:|
| 2 | 17,921 | 17,392 | 3,558 |
| 3 | 176,029 | 171,559 | 8,980 |
| 4 | 1,848,088 | 1,783,278 | 62,000 |
| 5 | 20,961,567 | 18,375,404 | 650,395 |

`list()` stands within 1.03-1.14x of `tolist()` at every depth, both
roughly an order of magnitude or more behind the buffer path -- the
same pattern as the flat case.

### 7.3 `array->list` pull, type ratio to int, size 100,000

| library | int | long | float | double |
|---|---:|---:|---:|---:|
| jpype | 9,906,480 (1.0x) | 1.04x | 1.07x | 1.09x |
| jpy | 5,695,493 (1.0x) | 0.88x | 0.78x | 0.79x |
| jep | 3,093,851 (1.0x) | 1.04x | 0.75x | 0.76x |
| pyjnius | 1,168,938 (1.0x) | 1.92x | 1.02x | 2.07x |

**Interpretation.** float/double pull *faster* than int/long in jpy and
jep; within jpype the two are now close to parity (int fastest, double
1.09x behind it) rather than the reverse. That reversal is a direct
consequence of the `convertToPythonObject` fix described in 7.1: int/
long previously paid for a redundant throwaway `PyLong` allocation that
float/double's own conversion path never had, which made int/long look
artificially expensive relative to float/double in the same table in the
previous edition of this report. With that allocation removed, int/long
land at or slightly below float/double's own cost -- consistent with
`JPIntType`/`JPLongType`'s wrapper construction being no heavier than
`JPFloatType`/`JPDoubleType`'s direct `tp_alloc` + field set once the
redundant step is gone. jpy's and jep's own float/double-faster-than-
int/long pattern is unrelated to this fix (neither library's code
changed) -- a genuine property of their own conversion paths, not
something this report can attribute further. pyjnius instead shows long
and double pulling ~2x *slower* than int/float, an inconsistent pattern
not investigated further.

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
| `Math.max(int,int)` | 672 | 353 | 1299 | 1276 | **183** | 1250 |
| `new Integer(int)` | 811 | 479 | 1257 | 7403 | 229 | 281 |
| `Math.sqrt(double)` | 706 | 393 | 645 | 497 | **117** | 874 |
| `new Double(double)` | 933 | 506 | 1378 | 6765 | 726 | 758 |
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
4,511,797 ns vs. jpype 1,905,281, jpy 791,520, jep 1,328,300 (Section
4.1) -- GraalPy is 2.4-5.7x slower here, in the same ballpark as
pyjnius). The other two rows are not in the same ballpark as anything
else in this report:

- **`buffer->array` (manual)**: 250-300x slower per element than
  GraalPy's own `list->array` (e.g. int @100,000: 1,270.2 ms vs. 4.51
  ms). jpype's real `buffer->array` fast path at the same size is
  54,480ns = 0.054 ms (Section 4.2) -- GraalPy's manual emulation is
  roughly 23,000x slower.
- **`array->buffer` pull**: 69.7x slower than GraalPy's own `array->list`
  at the same size (328.3 ms vs. 4.71 ms) -- the opposite ranking from
  jpype/jpy, where `array->buffer` is the fast path and beats
  `array->list` by 5.1-212x for jpype and 4.4-119x for jpy, growing with
  size in both cases (Section 7.1). This matches the jep/pyjnius
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
buffer path, far smaller in relative terms than jpype's own 30.4x
`buffer->array` penalty at the same shape extreme (Section 6), because
GraalPy's baseline per-element cost is already dominated by
polyglot-crossing overhead. The `list->array` `MemoryError`s are the
standout result, but not a capacity finding: int and long both fail at
`100000x3`, while float and double -- the *same* byte widths (4 and 8
bytes respectively) as int and long -- complete the identical row
without incident. If this were genuinely too much live data for a 3GB
heap, byte width would predict the split; it doesn't, so the failure
tracks something else about how int/long values get built and churned
at this shape (a GC-throughput/allocation-rate problem -- the collector
falling behind, "GC overhead limit exceeded" in spirit -- rather than
the live set not fitting). That makes it tuning-sensitive in a way a
real capability gap isn't: a larger heap or different GC settings might
clear it, unlike the missing `buffer->array` push path itself, which no
heap size fixes. Flagged as an open question, not run down further here
-- something no other library in this report comes close to triggering
at the same shape and heap budget, regardless of its ultimate cause.

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
it, plus a second, GC-throughput-shaped failure (8.4: int/long fail with
`MemoryError` on ordinary row-heavy shapes at a capped 3GB heap while
same-byte-width float/double don't, pointing at an allocation-rate
problem rather than a hard capacity limit -- tuning-sensitive, and not
weighted the same as the missing `buffer->array` path above, which no
amount of heap or GC tuning fixes) that none of jpype/jpy/jep/pyjnius
exhibit at the same budget. The one place GraalPy is unambiguously ahead
structurally, not just faster, is the callback/proxy direction (8.5).
Net read for anyone weighing GraalVM's polyglot model as an architecture
to follow: viable, even excellent, for microscript/glue-code call
patterns; not viable as-is for a scientific-orchestration substitute,
where jpype (and jpy) remain the only two bridges in this report with
real bulk buffer-transfer paths in both directions.
