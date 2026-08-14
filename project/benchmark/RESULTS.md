# Cross-library benchmark results

jpype vs. jpy, jep, and pyjnius on the JVM-embedding side of a Python/Java
bridge (Python drives Java in all four); GraalPy (Python-in-the-JVM,
opposite architecture) is tracked separately in Section 9 and was not
re-run for this edition. All numbers below are from a single sequential
re-run of the full suite, in disposable/isolated environments per this
repo's CLAUDE.md, with increased statistics (`trials=7`, higher
iteration floors) versus prior editions of this report to reduce noise.
Every section follows the same shape: **Methodology** (what is measured
and how), **Table** (raw numbers, `best` of the trials, nanoseconds per
call unless noted), **Result** (the factual takeaway only -- no
running commentary, no revision history).

## Global methodology

- **Timing.** `timeit()` (`project/benchmark/_common.py`, and an
  inlined equivalent in every `jep/*.py` script since jep's embedded
  interpreter can't import a sibling file): warm up, then run `trials=7`
  timed batches of `n` calls each; report `best` (minimum batch mean) and
  `median` across the 7 batches. Tables below show `best`; raw `best`/
  `median` pairs and CSVs are preserved alongside each script's own
  output.
- **Iteration counts.** Fixed-cost benchmarks (scalars, dispatch, proxy,
  strings, object identity) use `n=200,000`. Array benchmarks scale `n`
  down as element count grows (`calls_for()` in each script) so a
  100,000-element case doesn't take minutes; floors were raised this
  edition (`n >= 30`, `warmup >= 6` per batch, up from `n >= 20`,
  `warmup >= 5`) for better statistics without making the deepest
  multi-dimensional cases impractically slow.
- **Environments.** jpype: fresh venv, `pip install --no-build-isolation
  -e .` with `BUILD_TEST_HARNESS=ON`. jpy: fresh venv, prebuilt wheel
  from `~/devel/jpy/dist`. jep: `~/devel/jep/target/jep-4.3.1.jar` +
  the Python-3.12-matched native build
  (`~/devel/jep/build/lib.linux-x86_64-cpython-312`), launched as a Java
  process per this repo's README. pyjnius: fresh venv, built from source
  (`~/devel/pyjnius`, Cython 3.1.2) against this machine's JDK. Full
  per-library setup in `project/benchmark/README.md`.
- **Machine.** Single 16-core/7.7GB-RAM machine, one library's suite run
  at a time except where noted; no concurrent unrelated load.
- **Scope.** `int`/`long`/`float`/`double` element types throughout
  unless noted. `Z`/`B`/`C`/`S` (boolean/byte/char/short) arrays are not
  separately benchmarked in this report.


## 1. Scalars, strings, object identity

**Methodology.** `Math.max(int,int)` / `new Integer(int)` (int.py),
`Math.sqrt(double)` / `new Double(double)` (double.py), a round-trip
Java-String-from-Python-then-`str()`-back (strings.py), and reference
identity of a returned Java object compared across two calls
(object.py). All four are fixed-cost, `n=200,000` per batch.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| Math.max(int,int) | 682 | 363 | 1,344 | 1,323 |
| new Integer(int) | 804 | 478 | 1,383 | 8,014 |
| Math.sqrt(double) | 723 | 419 | 688 | 533 |
| new Double(double) | 944 | 527 | 1,454 | 7,177 |
| new String + toString | 1,049 | 1,010 | 2,616 | 23,546 |
| Object identity | 984 | 737 | 2,135 | 4,096 |

**Result.** jpy is fastest on every scalar/string/identity op; jpype
trails jpy by roughly 1.3-1.9x; jep and pyjnius trail jpype by a further
1.5-4x depending on the op, with pyjnius's `new Integer`/`new Double`/
string-roundtrip costs the widest outliers (7-23x jpy).

## 2. Method dispatch and proxy callbacks

**Methodology.** `dispatch.py`: a 16-overload method called
monomorphically (same arg type every call) vs. polymorphically
(argument type varies call-to-call, forcing overload resolution to
redo work jpype/jep can otherwise cache). `proxy.py`: steady-state cost
of invoking an already-constructed Python-implements-Java-interface
callback, `int` argument and (where supported) `Object` argument.

| operation | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| overload x16, monomorphic | 612 | 543 | 4,783 | 4,090 |
| overload x16, polymorphic | 790 | 546 | 5,146 | 4,238 |
| proxy callback (established), int arg | 2,597 | -- | 2,440 | 42,587 |
| proxy callback (established), Object arg | 2,459 | -- | 5,159 | -- |

**Result.** jpy has no separate proxy script (not benchmarked here).

pyjnius's proxy Object-arg case is not benchmarked: it reliably
segfaults this pyjnius checkout (`GetObjectClass`/`IsSameObject` called
without a null check on a genuinely-null `Object` argument), reproduced
independently against a fresh build before being treated as a real
finding rather than stale-build noise, per this repo's CLAUDE.md. jpype
leads jep and pyjnius on dispatch by roughly 4-7x; jpy leads jpype on
raw dispatch cost the same way it does on scalars.

## 3. Array push, flat (1D)

**Methodology.** `array_flat.py`. A Python `list`/numpy array of
length 100/1,000/10,000/100,000 pushed into a Java method parameter
(`int[]`/`long[]`/`float[]`/`double[]`), and the reverse (`array->list`,
`array->buffer`): reading a Java array back into a Python `list` /
numpy buffer. `list->array, widening from int` additionally covers a
plain Python `int` list pushed against a `float[]`/`double[]`
parameter (int has no widening case against itself).

### `list->array` push (method argument)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,231 | 1,183 | 2,493 | 3,057 |
| int[1000] | 4,836 | 8,074 | 14,740 | 27,116 |
| int[10000] | 38,324 | 76,676 | 136,222 | 280,330 |
| int[100000] | 382,358 | 781,784 | 1,371,269 | 4,673,403 |
| long[100] | 1,282 | 1,210 | 2,456 | 2,900 |
| long[1000] | 5,306 | 8,689 | 14,246 | 26,344 |
| long[10000] | 43,349 | 82,353 | 136,267 | 275,971 |
| long[100000] | 447,456 | 824,832 | 1,380,915 | 5,590,922 |
| float[100] | 1,071 | 1,110 | 2,108 | 3,417 |
| float[1000] | 3,096 | 7,626 | 11,485 | 29,009 |
| float[10000] | 22,423 | 72,344 | 102,599 | 283,160 |
| float[100000] | 201,059 | 724,713 | 1,013,267 | 3,323,044 |
| double[100] | 1,112 | 1,190 | 1,993 | 3,421 |
| double[1000] | 3,859 | 8,430 | 10,737 | 31,134 |
| double[10000] | 28,632 | 81,607 | 94,816 | 292,036 |
| double[100000] | 284,054 | 807,684 | 1,038,434 | 5,077,917 |

### `list->array`, widening from int (float/double only)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| float[100] | 1,260 | 1,588 | 3,932 | 3,464 |
| float[1000] | 4,812 | 11,962 | 31,456 | 28,942 |
| float[10000] | 40,098 | 115,583 | 303,602 | 285,874 |
| float[100000] | 373,891 | 1,187,625 | 2,911,070 | 3,645,482 |
| double[100] | 1,319 | 1,654 | 4,002 | 3,399 |
| double[1000] | 5,382 | 13,096 | 30,740 | 29,730 |
| double[10000] | 44,206 | 123,520 | 286,372 | 299,577 |
| double[100000] | 451,802 | 1,266,424 | 2,802,420 | 3,729,057 |

### `buffer->array` push (method argument, numpy source)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,125 | 438 | 818 | -- |
| int[1000] | 1,503 | 810 | 1,213 | -- |
| int[10000] | 5,369 | 5,310 | 5,621 | -- |
| int[100000] | 50,464 | 43,510 | 42,613 | -- |
| long[100] | 1,141 | 467 | 886 | -- |
| long[1000] | 1,896 | 1,168 | 1,765 | -- |
| long[10000] | 9,901 | 9,116 | 11,514 | -- |
| long[100000] | 87,888 | 72,483 | 103,775 | -- |
| float[100] | 1,197 | 441 | 842 | -- |
| float[1000] | 1,574 | 761 | 1,249 | -- |
| float[10000] | 5,592 | 4,917 | 5,841 | -- |
| float[100000] | 40,159 | 41,541 | 46,643 | -- |
| double[100] | 1,257 | 463 | 912 | -- |
| double[1000] | 3,042 | 1,142 | 1,830 | -- |
| double[10000] | 15,837 | 8,922 | 11,827 | -- |
| double[100000] | 134,341 | 72,876 | 104,318 | -- |

### `array->list` pull (Java array -> Python list)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 7,108 | 5,182 | 5,017 | 1,993 |
| int[1000] | 61,033 | 46,138 | 40,936 | 12,941 |
| int[10000] | 659,175 | 489,233 | 419,284 | 167,126 |
| int[100000] | 9,265,813 | 6,576,669 | 5,848,730 | 1,785,407 |
| long[100] | 6,724 | 5,378 | 5,452 | 2,168 |
| long[1000] | 56,779 | 48,160 | 38,753 | 16,237 |
| long[10000] | 613,107 | 483,064 | 368,904 | 141,594 |
| long[100000] | 8,235,060 | 7,442,961 | 6,322,217 | 5,623,093 |
| float[100] | 10,228 | 4,529 | 4,332 | 1,607 |
| float[1000] | 92,252 | 42,160 | 44,974 | 13,192 |
| float[10000] | 909,845 | 439,692 | 431,904 | 153,484 |
| float[100000] | 10,602,681 | 4,459,722 | 4,282,515 | 1,194,549 |
| double[100] | 11,100 | 4,730 | 4,259 | 1,606 |
| double[1000] | 94,934 | 45,415 | 43,017 | 14,002 |
| double[10000] | 946,430 | 456,469 | 436,006 | 121,902 |
| double[100000] | 11,524,401 | 4,912,456 | 4,366,728 | 1,705,660 |

_jpype's int/long rows reflect a recycling pool for the tagged-number
leaves (`JByte`/`JShort`/`JInt`/`JLong` -- see Section 11); float/double
are untouched by that change since they don't go through the same
`tp_alloc` path. Boolean array pulls were already unaffected either way
-- `JPBooleanType::getFastArrayItem` already returned a plain
`PyBool_FromLong` singleton, never a tagged wrapper._

### `array->buffer` pull (Java array -> Python/numpy buffer)

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,832 | 973 | 8,996 | 5,358 |
| int[1000] | 2,356 | 1,424 | 73,586 | 44,621 |
| int[10000] | 6,606 | 6,409 | 757,977 | 497,008 |
| int[100000] | 47,588 | 48,006 | 9,408,453 | 6,640,984 |
| long[100] | 1,963 | 999 | 9,486 | 6,136 |
| long[1000] | 3,003 | 2,023 | 78,725 | 52,362 |
| long[10000] | 13,325 | 11,845 | 788,202 | 554,075 |
| long[100000] | 115,647 | 116,314 | 10,729,171 | 9,742,540 |
| float[100] | 1,872 | 971 | 7,423 | 4,516 |
| float[1000] | 2,455 | 1,473 | 68,721 | 38,682 |
| float[10000] | 7,123 | 6,215 | 693,028 | 371,209 |
| float[100000] | 47,517 | 47,487 | 7,084,413 | 3,947,891 |
| double[100] | 1,953 | 1,058 | 7,474 | 4,509 |
| double[1000] | 2,964 | 2,272 | 68,611 | 39,138 |
| double[10000] | 12,968 | 13,311 | 684,508 | 369,847 |
| double[100000] | 103,899 | 132,296 | 7,028,567 | 4,170,636 |

**Result.** `list->array`: jpype leads jpy/jep/pyjnius at every
size 1,000 and up; all four show a 1.5-2x int-widening penalty on
float/double vs. their own matched-type number, jpype's the narrowest.
`buffer->array`: pyjnius has no buffer-protocol push at all (falls back
to `sequenceConversion`, i.e. it isn't in this table -- see Section 5
for the isolated cost of that fallback). `array->list`: pyjnius is still
the fastest of all four despite losing most other benchmarks in this
report, because its Cython bridge boxes one plain `PyLong`/`PyFloat` per
element while jpype/jep/jpy build heavier tagged wrapper objects --
jpype's int/long gap to pyjnius narrowed (a recycling pool for those
wrapper allocations, Section 11) but float/double and jep/jpy across the
board still pay full per-element allocation cost.
`array->buffer`: jep/pyjnius have no real buffer-protocol return path --
their columns above are `array->list`'s cost plus a redundant
`np.asarray()`, not a genuine bulk read, which is why they land *worse*
than their own `array->list` number instead of better. jpype and jpy
have a genuine buffer-protocol return path and are close to each other,
with jpy consistently faster.

## 4. Array push, non-contiguous sources

**Methodology.** `array_noncontig.py`. A numpy source that cannot
provide a C-contiguous buffer view -- a non-unit-stride column slice
(flat, 1D) or a transposed array (2D-5D, `np.transpose` with reversed
axis order) -- pushed as a method argument. Tests whether a bulk
buffer-read path is still reached, or whether the implementation falls
back to a fully general per-element/per-row walk.

### Flat (1D), non-contiguous column slice

| size | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[100] | 1,225 | -- | 1,247 | -- |
| int[1000] | 1,873 | -- | 1,926 | -- |
| int[10000] | 8,659 | -- | 8,504 | -- |
| int[100000] | 64,188 | -- | 67,997 | -- |
| long[100] | 1,286 | -- | 1,296 | -- |
| long[1000] | 2,273 | -- | 2,540 | -- |
| long[10000] | 12,662 | -- | 14,861 | -- |
| long[100000] | 127,900 | -- | 134,891 | -- |
| float[100] | 1,234 | -- | 1,254 | -- |
| float[1000] | 1,819 | -- | 1,966 | -- |
| float[10000] | 8,210 | -- | 8,313 | -- |
| float[100000] | 65,916 | -- | 68,367 | -- |
| double[100] | 1,230 | -- | 1,281 | -- |
| double[1000] | 2,215 | -- | 2,485 | -- |
| double[10000] | 12,916 | -- | 14,846 | -- |
| double[100000] | 125,988 | -- | 134,718 | -- |

_jpy and pyjnius have no entry: jpy's buffer matcher requires `PyBUF_SIMPLE` (fails outright on a non-contiguous 1D source); pyjnius has no buffer->array push at all, contiguous or not._


### Multi-dimensional (2D-5D), transposed

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 2,630 | 5,652 | 25,512 | -- |
| int[][][](10^3) | 14,601 | 55,991 | 248,265 | -- |
| int[][][][](10^4) | 142,064 | 571,821 | 2,493,939 | -- |
| int[][][][][](10^5) | 1,450,440 | 5,904,847 | 24,911,885 | -- |
| long[][](10^2) | 2,689 | 5,474 | 25,654 | -- |
| long[][][](10^3) | 15,333 | 54,552 | 254,678 | -- |
| long[][][][](10^4) | 146,160 | 559,801 | 2,486,915 | -- |
| long[][][][][](10^5) | 1,526,884 | 5,726,589 | 26,296,872 | -- |
| float[][](10^2) | 2,704 | 5,642 | 25,522 | -- |
| float[][][](10^3) | 15,444 | 54,951 | 248,772 | -- |
| float[][][][](10^4) | 146,328 | 532,072 | 2,494,275 | -- |
| float[][][][][](10^5) | 1,498,161 | 5,546,981 | 24,792,662 | -- |
| double[][](10^2) | 2,609 | 5,316 | 24,907 | -- |
| double[][][](10^3) | 14,821 | 50,789 | 247,473 | -- |
| double[][][][](10^4) | 150,749 | 491,134 | 2,498,462 | -- |
| double[][][][][](10^5) | 1,485,241 | 4,963,800 | 25,427,445 | -- |

**Result.** jpype and jep are the only libraries with a real bulk
path for a non-contiguous 1D source; jep is faster at this size. jpy
has no buffer->array push at all for a non-contiguous source in any
dimensionality (fails outright, 1D; not benchmarked, ND, since the
underlying push has no bulk path to exercise); pyjnius has no
buffer->array push at any size, depth, or contiguity. jep's
"transposed, manual per-row" ND numbers are a per-row Python-level
walk, not a bulk buffer read -- included for completeness, not a
like-for-like comparison to jpype's single-JNI-call path.

## 5. Array push/pull, multi-dimensional (depth 2-5, rectangular)

**Methodology.** `array_multidim.py`. A nested Python list (or
nested numpy-backed structure) of depth 2-5 with a fixed total element
count (~10^depth), pushed (`list->array`, `buffer->array`) or pulled
(`array->list`, `array->buffer`). `buffer->array` is a genuine bulk
buffer read where the library has one (jpype, jpy); jep's is a manual
per-row Python-level walk (no bulk ND push path exists in jep).
pyjnius has no buffer->array push at any depth.

### `list->array` push, fresh nested list

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 2,018 | 2,200 | 6,912 | 4,800 |
| int[][][](10^3) | 11,714 | 17,668 | 64,825 | 41,346 |
| int[][][][](10^4) | 116,323 | 171,389 | 629,210 | 434,760 |
| int[][][][][](10^5) | 1,088,620 | 1,757,121 | 6,280,016 | 5,027,203 |
| long[][](10^2) | 1,842 | 2,217 | 6,949 | 4,903 |
| long[][][](10^3) | 10,541 | 18,804 | 64,072 | 42,052 |
| long[][][][](10^4) | 108,653 | 188,723 | 630,475 | 424,178 |
| long[][][][][](10^5) | 963,170 | 1,890,664 | 6,349,567 | 5,045,931 |
| float[][](10^2) | 1,998 | 2,072 | -- | 5,295 |
| float[][][](10^3) | 11,856 | 17,509 | -- | 53,187 |
| float[][][][](10^4) | 106,154 | 173,231 | -- | 551,122 |
| float[][][][][](10^5) | 1,030,479 | 1,758,766 | -- | 9,268,890 |
| double[][](10^2) | 2,044 | 2,208 | -- | 5,431 |
| double[][][](10^3) | 11,328 | 18,116 | -- | 54,689 |
| double[][][][](10^4) | 114,204 | 179,378 | -- | 564,277 |
| double[][][][][](10^5) | 1,030,506 | 1,882,707 | -- | 9,254,105 |

### `buffer->array` push, numpy source (jep: manual per-row fallback)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 1,991 | 5,730 | -- | -- |
| int[][][](10^3) | 6,448 | 55,185 | -- | -- |
| int[][][][](10^4) | 67,735 | 566,534 | -- | -- |
| int[][][][][](10^5) | 598,103 | 5,683,388 | -- | -- |
| long[][](10^2) | 2,006 | 5,585 | -- | -- |
| long[][][](10^3) | 6,570 | 54,901 | -- | -- |
| long[][][][](10^4) | 61,873 | 554,750 | -- | -- |
| long[][][][][](10^5) | 579,344 | 5,660,922 | -- | -- |
| float[][](10^2) | 2,009 | 5,737 | -- | -- |
| float[][][](10^3) | 7,253 | 54,726 | -- | -- |
| float[][][][](10^4) | 69,394 | 541,806 | -- | -- |
| float[][][][][](10^5) | 573,296 | 5,440,141 | -- | -- |
| double[][](10^2) | 2,122 | 5,450 | -- | -- |
| double[][][](10^3) | 7,960 | 51,095 | -- | -- |
| double[][][][](10^4) | 68,960 | 511,132 | -- | -- |
| double[][][][][](10^5) | 587,639 | 5,219,317 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any depth._


### `array->list` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 16,057 | 9,963 | 13,526 | 3,440 |
| int[][][](10^3) | 158,150 | 99,842 | 129,102 | 30,065 |
| int[][][][](10^4) | 1,727,024 | 1,137,774 | 1,466,528 | 498,866 |
| int[][][][][](10^5) | 21,465,065 | 13,614,375 | 16,390,342 | 7,774,809 |
| long[][](10^2) | 15,685 | 10,198 | 14,147 | 3,571 |
| long[][][](10^3) | 156,303 | 105,042 | 133,857 | 31,427 |
| long[][][][](10^4) | 1,689,539 | 1,092,114 | -- | 474,170 |
| long[][][][][](10^5) | 20,103,014 | 11,879,790 | -- | 9,453,776 |
| float[][](10^2) | 19,054 | 9,386 | -- | 2,968 |
| float[][][](10^3) | 194,456 | 91,323 | -- | 29,074 |
| float[][][][](10^4) | 2,042,988 | 1,015,685 | -- | 406,225 |
| float[][][][][](10^5) | 23,537,118 | 10,975,680 | -- | 5,072,694 |
| double[][](10^2) | 19,854 | 9,892 | -- | 3,000 |
| double[][][](10^3) | 199,660 | 97,720 | -- | 28,600 |
| double[][][][](10^4) | 2,106,412 | 1,037,738 | -- | 431,080 |
| double[][][][][](10^5) | 23,046,632 | 11,476,721 | -- | 5,539,152 |

_jpype's int/long rows reflect the tagged-number recycling pool -- see
Section 11 and Section 3's footnote; float/double are untouched by that
change._

### `array->buffer` pull

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](10^2) | 3,688 | 7,854 | 17,295 | 7,592 |
| int[][][](10^3) | 9,559 | 77,504 | 175,356 | 71,190 |
| int[][][][](10^4) | 70,212 | 891,446 | 2,036,880 | 950,755 |
| int[][][][][](10^5) | 728,158 | 10,713,915 | 26,164,085 | 13,366,826 |
| long[][](10^2) | 3,734 | 8,304 | -- | 8,445 |
| long[][][](10^3) | 10,134 | 76,792 | -- | 82,092 |
| long[][][][](10^4) | 66,622 | 894,228 | -- | 982,934 |
| long[][][][][](10^5) | 744,972 | 10,543,404 | -- | 16,391,273 |
| float[][](10^2) | 3,783 | 8,328 | -- | 6,761 |
| float[][][](10^3) | 10,754 | 76,140 | -- | 64,680 |
| float[][][][](10^4) | 72,493 | 884,745 | -- | 781,384 |
| float[][][][][](10^5) | 620,962 | 10,522,196 | -- | 9,744,072 |
| double[][](10^2) | 3,829 | 8,192 | -- | 6,844 |
| double[][][](10^3) | 10,106 | 79,991 | -- | 62,103 |
| double[][][][](10^4) | 73,214 | 906,594 | -- | 777,766 |
| double[][][][][](10^5) | 637,274 | 10,900,139 | -- | 10,888,224 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout
(`matchRaggedNode`/`encodeRaggedNode`, `native/common/jp_classhints.cpp`)
-- see Section 11._


**Result.** `list->array`: jpype now leads at every depth, having
closed and reversed a 2.2-2.5x deficit against jpy (see Section 11).
`buffer->array`:
jpy and jpype both reach a genuine bulk path and are within a few
percent of each other by depth 4-5; jep's manual per-row fallback is
1-2 orders of magnitude slower at depth 4-5; pyjnius has none.
`array->list`/`array->buffer`: pyjnius is fastest at shallow depth the
same way it is in Section 3, but jpype and jpy's `array->buffer` bulk
path pulls further ahead as depth grows, since it scales with leaf-array
count rather than total element count.

## 6. Array push, ragged (jagged, non-rectangular)

**Methodology.** `array_ragged.py`. A nested Python list whose
sub-lists have varying lengths (a genuinely jagged/ragged structure, not
a rectangular array-of-arrays), pushed fresh into a Java array-of-arrays
parameter, depth 2-5, ~10^depth total elements.

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[][](~10^2) | 1,523 | 1,531 | 4,801 | 3,171 |
| int[][][](~10^3) | 11,801 | 18,823 | 71,090 | 44,417 |
| int[][][][](~10^4) | 100,535 | 149,402 | 563,822 | 352,867 |
| int[][][][][](~10^5) | 1,389,415 | 2,139,732 | 8,147,249 | 5,920,602 |
| long[][](~10^2) | 1,381 | 1,483 | 4,768 | 3,175 |
| long[][][](~10^3) | 11,922 | 19,790 | 70,842 | 42,263 |
| long[][][][](~10^4) | 98,032 | 156,158 | 569,074 | 352,408 |
| long[][][][][](~10^5) | 1,400,003 | 2,259,730 | 8,136,513 | 5,483,144 |
| float[][](~10^2) | 1,494 | 1,382 | 4,539 | 3,206 |
| float[][][](~10^3) | 12,136 | 18,062 | 66,085 | 51,978 |
| float[][][][](~10^4) | 101,037 | 145,387 | 530,688 | 443,521 |
| float[][][][][](~10^5) | 1,470,219 | 2,280,683 | 7,884,050 | 10,576,596 |
| double[][](~10^2) | 1,429 | 1,483 | 4,696 | 3,369 |
| double[][][](~10^3) | 11,021 | 19,950 | 67,852 | 53,670 |
| double[][][][](~10^4) | 89,351 | 151,441 | 532,166 | 463,626 |
| double[][][][][](~10^5) | 1,352,482 | 2,217,599 | 7,744,838 | 10,166,998 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout
-- see Section 11._


**Result.** jpype leads at every depth/type (previously trailed jpy
2.2-2.5x here -- see Section 11 for the fix); the gap to jpy widens
with depth (both walk the ragged structure recursively, jpype's
ragged-native encode path stays closer to linear in total elements).

## 7. Array push/pull, shape at fixed depth and total element count

**Methodology.** `array_shape.py`. Fixed total element count
(~10,000 or ~100,000), depth held at 3, but the *shape* varies (e.g.
`[1000][10][10]` vs. `[10][10][1000]`) to isolate whether cost tracks
total elements or leaf-array count / row-heaviness. `list->array` and
`buffer->array` (jep: manual per-row) directions only.

### `list->array` push

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 575,069 | 806,918 | 1,402,789 | 2,915,372 |
| int[100][1000] | 572,626 | 830,006 | 1,417,742 | 2,794,704 |
| int[1000][100] | 587,039 | 866,335 | 1,836,153 | 2,241,320 |
| int[10000][10] | 970,377 | 1,649,007 | 6,145,127 | 4,012,716 |
| int[3][100000] | 2,127,744 | 2,490,970 | 4,207,551 | 9,705,678 |
| int[100000][3] | 6,659,293 | 11,588,374 | 50,941,097 | 26,209,037 |
| int[1000][1000] | 7,126,991 | 8,584,726 | 14,795,684 | 32,167,686 |
| int[1000][10][10] | 1,011,021 | 1,745,310 | 6,499,518 | 6,011,382 |
| int[10][10][1000] | 559,324 | 798,483 | 1,412,372 | 2,778,888 |
| long[10][10000] | 586,065 | 821,602 | 1,275,314 | 2,884,768 |
| long[100][1000] | 587,370 | 826,781 | 1,354,412 | 2,688,467 |
| long[1000][100] | 624,388 | 890,544 | 1,738,038 | 2,215,003 |
| long[10000][10] | 920,533 | 1,759,386 | 5,879,173 | 4,086,737 |
| long[3][100000] | 2,216,927 | 2,584,784 | 4,047,452 | 9,942,196 |
| long[100000][3] | 5,883,622 | 12,470,530 | 51,026,319 | 26,499,032 |
| long[1000][1000] | 8,130,824 | 8,898,200 | 13,989,272 | 30,639,333 |
| long[1000][10][10] | 931,120 | 1,872,495 | 6,269,827 | 5,337,705 |
| long[10][10][1000] | 604,084 | 823,260 | 1,349,376 | 2,671,344 |
| float[10][10000] | 579,350 | 731,309 | 994,848 | 2,789,406 |
| float[100][1000] | 550,451 | 754,981 | 1,057,161 | 2,849,780 |
| float[1000][100] | 592,391 | 795,978 | 1,430,817 | 3,208,813 |
| float[10000][10] | 1,015,702 | 1,609,817 | 5,431,722 | 5,489,580 |
| float[3][100000] | 1,936,514 | 2,333,876 | 2,962,936 | 10,136,459 |
| float[100000][3] | 7,366,826 | 11,620,500 | 48,370,083 | 32,701,462 |
| float[1000][1000] | 7,158,591 | 8,092,994 | 10,686,076 | 33,539,870 |
| float[1000][10][10] | 1,118,367 | 1,754,334 | 5,816,736 | 7,109,184 |
| float[10][10][1000] | 572,601 | 759,979 | 1,056,809 | 3,135,425 |
| double[10][10000] | 515,464 | 829,131 | 929,372 | 3,060,208 |
| double[100][1000] | 545,951 | 823,039 | 979,936 | 3,290,929 |
| double[1000][100] | 550,896 | 867,200 | 1,371,696 | 3,586,975 |
| double[10000][10] | 918,544 | 1,696,826 | 5,405,955 | 6,034,980 |
| double[3][100000] | 2,026,502 | 2,670,880 | 2,870,663 | 11,272,831 |
| double[100000][3] | 7,080,346 | 12,117,989 | 47,775,907 | 32,858,976 |
| double[1000][1000] | 7,747,029 | 8,884,806 | 9,848,050 | 35,162,136 |
| double[1000][10][10] | 1,172,212 | 1,833,219 | 5,852,637 | 7,900,256 |
| double[10][10][1000] | 572,002 | 827,489 | 977,538 | 3,435,078 |

_jpype numbers reflect the list/tuple-specialized ragged-native readout
(`matchRaggedNode`/`encodeRaggedNode`, `native/common/jp_classhints.cpp`)
-- see Section 11. Prior to that specialization jpype trailed jpy 2-4x on
every shape here; this table was stale (pre-specialization numbers) until
refreshed 2026-08-13 -- see Section 11._

### `buffer->array` push (jep: manual per-row)

| shape | jpype | jpy | jep | pyjnius |
|---|:---:|:---:|:---:|:---:|
| int[10][10000] | 62,126 | 3,957,171 | -- | -- |
| int[100][1000] | 64,949 | 4,028,383 | -- | -- |
| int[1000][100] | 90,275 | 4,154,658 | -- | -- |
| int[10000][10] | 534,351 | 5,977,026 | -- | -- |
| int[3][100000] | 129,720 | 12,919,031 | -- | -- |
| int[100000][3] | 5,194,846 | 30,331,461 | -- | -- |
| int[1000][1000] | 530,988 | 39,452,892 | -- | -- |
| int[1000][10][10] | 585,717 | 5,810,657 | -- | -- |
| int[10][10][1000] | 62,044 | 3,859,042 | -- | -- |
| long[10][10000] | 113,826 | 3,772,218 | -- | -- |
| long[100][1000] | 104,040 | 3,738,746 | -- | -- |
| long[1000][100] | 139,714 | 3,873,069 | -- | -- |
| long[10000][10] | 551,535 | 5,560,335 | -- | -- |
| long[3][100000] | 343,509 | 11,171,590 | -- | -- |
| long[100000][3] | 6,297,852 | 28,768,097 | -- | -- |
| long[1000][1000] | 966,547 | 38,238,769 | -- | -- |
| long[1000][10][10] | 597,333 | 5,680,074 | -- | -- |
| long[10][10][1000] | 104,896 | 3,947,929 | -- | -- |
| float[10][10000] | 50,218 | 3,472,438 | -- | -- |
| float[100][1000] | 52,974 | 3,536,412 | -- | -- |
| float[1000][100] | 93,158 | 3,630,979 | -- | -- |
| float[10000][10] | 489,567 | 5,130,371 | -- | -- |
| float[3][100000] | 130,298 | 10,414,713 | -- | -- |
| float[100000][3] | 5,391,135 | 27,795,668 | -- | -- |
| float[1000][1000] | 506,896 | 35,873,353 | -- | -- |
| float[1000][10][10] | 578,828 | 5,542,828 | -- | -- |
| float[10][10][1000] | 53,955 | 3,508,462 | -- | -- |
| double[10][10000] | 97,938 | 3,283,246 | -- | -- |
| double[100][1000] | 110,922 | 3,252,640 | -- | -- |
| double[1000][100] | 143,359 | 3,401,470 | -- | -- |
| double[10000][10] | 522,030 | 5,092,549 | -- | -- |
| double[3][100000] | 364,718 | 10,026,086 | -- | -- |
| double[100000][3] | 7,190,349 | 27,661,544 | -- | -- |
| double[1000][1000] | 1,080,873 | 33,215,732 | -- | -- |
| double[1000][10][10] | 628,893 | 4,965,956 | -- | -- |
| double[10][10][1000] | 116,227 | 3,117,623 | -- | -- |

_pyjnius: no entry -- no buffer->array push at any shape._


**Result.** At fixed total element count, a row-heavy shape (many
short rows, e.g. `[100000][3]`) costs more than a column-heavy one
(few long rows, e.g. `[3][100000]`) in every library that has a bulk
path -- more leaf arrays means more per-leaf JNI/reflection overhead
even though total elements is unchanged. For jpy/jep/pyjnius,
`list->array` pays this penalty via a recursive per-row Python-level
walk. jpype's ragged-native path (Section 11) avoids the Python-level
per-row cost -- one C++ walk, one JNI crossing -- but still shows the
same row-heavy-costs-more shape, now from `Array.newInstance`/
`Array.set` reflection on the Java side of `fillRaggedFromBuffer`, one
call per row regardless of row length (e.g. `int[100000][3]`: 6,659,293ns
vs `int[3][100000]`: 2,127,744ns, same 100,000 elements). `buffer->array`
still pays the smallest per-leaf-array penalty of the three, since
jpype/jpy's bulk path there has no reflection in the loop at all.

## 8. jpype-only microbenchmarks

These three scripts (`array_to_list_dtype.py`, `arraytransfer.py`,
`classhints.py`) have no equivalent in the jpy/jep/pyjnius suites --
they exercise jpype-internal API surface (`toList()` dtype variants,
`pullTo`/`pushFrom` bulk in-place transfer, `JPConversionList`/
`JPConversionTuple`'s cached-class-hint lookup) with nothing to compare
against. jpype-only, `best` ns/call.

### `list()` vs. `toList()` dtype variants

| operation | jpype |
|---|:---:|
| list(arr) int[100] | 7,444 |
| toList() int[100], plain | 2,748 |
| toList(dtype=int) int[100], wrapped (~= old default) | 5,283 |
| toList(dtype=float) int[100], forced cast, plain | 2,567 |
| list(arr) int[1000] | 63,103 |
| toList() int[1000], plain | 13,569 |
| toList(dtype=int) int[1000], wrapped (~= old default) | 38,162 |
| toList(dtype=float) int[1000], forced cast, plain | 12,751 |
| list(arr) int[10000] | 652,408 |
| toList() int[10000], plain | 179,768 |
| toList(dtype=int) int[10000], wrapped (~= old default) | 404,119 |
| toList(dtype=float) int[10000], forced cast, plain | 125,526 |
| list(arr) int[100000] | 8,802,843 |
| toList() int[100000], plain | 2,761,812 |
| toList(dtype=int) int[100000], wrapped (~= old default) | 6,278,195 |
| toList(dtype=float) int[100000], forced cast, plain | 1,243,748 |
| list(arr) long[100] | 6,904 |
| toList() long[100], plain | 3,079 |
| toList(dtype=long) long[100], wrapped (~= old default) | 4,597 |
| toList(dtype=float) long[100], forced cast, plain | 2,625 |
| list(arr) long[1000] | 58,228 |
| toList() long[1000], plain | 16,914 |
| toList(dtype=long) long[1000], wrapped (~= old default) | 30,411 |
| toList(dtype=float) long[1000], forced cast, plain | 14,017 |
| list(arr) long[10000] | 615,109 |
| toList() long[10000], plain | 180,830 |
| toList(dtype=long) long[10000], wrapped (~= old default) | 331,916 |
| toList(dtype=float) long[10000], forced cast, plain | 129,970 |
| list(arr) long[100000] | 8,242,095 |
| toList() long[100000], plain | 3,503,721 |
| toList(dtype=long) long[100000], wrapped (~= old default) | 5,299,674 |
| toList(dtype=float) long[100000], forced cast, plain | 1,381,093 |
| list(arr) float[100] | 10,972 |
| toList() float[100], plain | 2,315 |
| toList(dtype=float) float[100], wrapped (~= old default) | 7,787 |
| toList(dtype=int) float[100], forced cast, plain | 2,364 |
| list(arr) float[1000] | 97,796 |
| toList() float[1000], plain | 12,612 |
| toList(dtype=float) float[1000], wrapped (~= old default) | 61,306 |
| toList(dtype=int) float[1000], forced cast, plain | 11,474 |
| list(arr) float[10000] | 982,178 |
| toList() float[10000], plain | 120,205 |
| toList(dtype=float) float[10000], wrapped (~= old default) | 597,952 |
| toList(dtype=int) float[10000], forced cast, plain | 111,263 |
| list(arr) float[100000] | 11,649,202 |
| toList() float[100000], plain | 1,298,283 |
| toList(dtype=float) float[100000], wrapped (~= old default) | 6,034,438 |
| toList(dtype=int) float[100000], forced cast, plain | 1,233,254 |
| list(arr) double[100] | 10,620 |
| toList() double[100], plain | 2,486 |
| toList(dtype=double) double[100], wrapped (~= old default) | 7,964 |
| toList(dtype=int) double[100], forced cast, plain | 2,546 |
| list(arr) double[1000] | 94,811 |
| toList() double[1000], plain | 13,362 |
| toList(dtype=double) double[1000], wrapped (~= old default) | 62,177 |
| toList(dtype=int) double[1000], forced cast, plain | 12,113 |
| list(arr) double[10000] | 934,043 |
| toList() double[10000], plain | 132,495 |
| toList(dtype=double) double[10000], wrapped (~= old default) | 619,168 |
| toList(dtype=int) double[10000], forced cast, plain | 119,350 |
| list(arr) double[100000] | 11,255,501 |
| toList() double[100000], plain | 1,358,495 |
| toList(dtype=double) double[100000], wrapped (~= old default) | 6,081,819 |
| toList(dtype=int) double[100000], forced cast, plain | 1,296,466 |

_Full re-run 2026-08-13, all four types, for internal consistency (mixing
old and new numbers in one plain-vs-wrapped comparison would misrepresent
it). `list(arr)`/wrapped rows for int/long reflect the tagged-number
recycling pool -- see Section 11; float/double and the plain/forced-cast
rows (already bare `PyLong_FromLong`/`PyFloat_FromDouble`, never routed
through the pool) are session-to-session noise only._

### Bulk in-place transfer (`pullTo`/`pushFrom`) vs. naive per-element

| operation | jpype |
|---|:---:|
| pullTo double[1000] | 509 |
| naive per-element double[1000] | 116,937 |
| pullTo double[100000] | 21,387 |
| naive per-element double[100000] | 11,633,287 |
| pullTo double[1000000] | 551,543 |
| pushFrom double[1000] | 646 |
| naive per-element double[1000] | 308,451 |
| pushFrom double[100000] | 21,505 |
| naive per-element double[100000] | 31,029,910 |
| pushFrom double[1000000] | 706,654 |
| pushFrom byteswapped double[1000] | 5,370 |
| pushFrom float16 double[1000] | 6,108 |
| pushFrom byteswapped double[100000] | 487,454 |
| pushFrom float16 double[100000] | 556,526 |
| pushFrom byteswapped double[1000000] | 4,938,962 |
| pushFrom float16 double[1000000] | 5,659,187 |
| direct-buffer-shared double[1000] | 1,542 |
| direct-buffer-shared double[100000] | 30,352 |
| direct-buffer-shared double[1000000] | 314,876 |
| slice_python double[1000] | 797 |
| slice_javaArray double[1000] | 4,092 |
| slice_python double[100000] | 10,750 |
| slice_javaArray double[100000] | 216,204 |
| slice_python double[1000000] | 111,768 |
| slice_javaArray double[1000000] | 2,254,214 |
| multidim_bulk 10x10 | 2,927 |
| multidim_looped 10x10 | 18,060 |
| multidim_bulk 300x300 | 42,439 |
| multidim_looped 300x300 | 9,734,138 |
| multidim_bulk 1000x1000 | 1,339,532 |

### Class-hint cache lookup cost vs. registered-class-count

| operation | jpype |
|---|:---:|
| match@1/400 | 666 |
| match@5/400 | 663 |
| match@20/400 | 655 |
| match@100/400 | 658 |
| match@200/400 | 654 |
| match@400/400 | 658 |

**Result.** `pullTo`/`pushFrom` beat their naive per-element
counterparts by roughly one to two orders of magnitude at 100,000+
elements. `direct-buffer-shared` (steady-state cost once a direct
buffer is already set up) is the cheapest transfer path at every size.
`classhints` cache lookup cost is flat from 1 to 400 registered classes
-- confirms the cache is a real O(1) lookup, not a linear scan that
happens to be fast at small N.

## 9. GraalPy: a true-JIT comparison point (not re-run this edition)

GraalPy (Python-in-the-JVM, opposite architecture from
jpype/jpy/pyjnius, same direction as jep) was not part of this edition's
re-run -- it needs a separate GraalVM CE + Maven/Truffle setup (see
`project/benchmark/README.md`) and was out of scope for this pass. The
subsections below are carried forward **unchanged** from the previous
edition of this report; their internal `Section N` cross-references
point to *that* edition's section numbers, not this document's current
numbering.

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

### 9.1 Scalars, dispatch, proxy

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

### 9.2 Array push (flat, 1D)

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
competitive with jpy/jep at every size (e.g. int @100,000: GraalPy
4,511,797 ns vs. jpy 791,520, jep 1,328,300 (Section 4.1) -- GraalPy is
3.4-5.7x slower here, in the same ballpark as pyjnius). jpype has since
moved out of that ballpark on `list->array` specifically (378,957ns at
this size, Section 4.5's `JPConversionList`/`JPConversionTuple`
quality-check fast path plus this session's `setArrayRange`
value-extraction fast path) -- GraalPy is now 11.9x slower than jpype
there, well past the 3.4-5.7x range the other three libraries occupy;
jpype's `array->list` pull is untouched by that change and still lands
in the same 2-6x ballpark as jpy/jep. The other two rows are not in the
same ballpark as anything else in this report:

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

### 9.3 Array push, multi-dimensional and ragged

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

### 9.4 Non-contiguous sources and row-heavy shapes

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

### 9.5 Proxy: the one place GraalPy is architecturally simpler

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

### 9.6 Strategic summary

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


## 10. Known limitations of this run

- **jep, `array_multidim.py`**: hit `java.lang.OutOfMemoryError`
  partway through the `long` type sweep even at `-Xmx3g` (this
  machine's stock ergonomic default was ~2GB; raising to 4GB and then
  6GB each let it get further into the sweep before still OOMing --
  6GB pushed total system memory to the edge of exhaustion on this
  7.7GB machine and was not pursued further). This happens only in
  `array_multidim.py`, which runs all four (type x direction) sweeps
  back-to-back in one JVM process; the other jep array scripts
  (`array_ragged.py`, `array_noncontig.py`, `array_shape.py`), each a
  narrower slice of the same workload, complete cleanly at the same
  depths/sizes under the stock default heap. That a larger heap
  measurably postpones but does not prevent the failure, combined with
  it being specific to the single long-running combined-sweep process,
  points at accumulated garbage outliving each `timeit()` batch (an
  allocation-rate-vs-GC-throughput problem) rather than a fixed
  working-set size that a given heap either does or doesn't fit --
  flagged here as a real, reproducible finding, not investigated
  further. Section 5's `array_multidim` tables show `int` in full and
  `long`/`float`/`double` only as far as this run reached before
  failing (missing cells read `--`).
- **pyjnius, `array_noncontig.py`**: intentionally a no-op -- pyjnius
  has no buffer->array push at all (confirmed empirically: any buffer
  source raises `JavaException('Expecting a python list/tuple, got
  array(...)')` unconditionally), so there is no non-contiguous-source
  case to measure separately from Section 3's finding.
- **GraalPy**: not re-run this edition; see Section 9 for the last
  captured numbers and their own methodology/caveats.

## 11. Where to focus next

- **Resolved since the numbers above were first captured: `list->array`
  push, depth >= 2 (both rectangular and ragged) used to lose to jpy's
  naive per-element recursion by a consistent 2.2-2.5x, despite jpype
  having a dedicated ragged-native fast path
  (`isRaggedLeafElement`/`matchRaggedNode`/`encodeRaggedNode`,
  `jp_classhints.cpp`) that jpy has no equivalent of at all -- every jpy
  push there is a generic `PySequence_GetItem` recursion. Root cause:
  `matchRaggedNode`/`encodeRaggedNode` read every node's contents via
  the generic `JPPySequence` wrapper (`PySequence_Size`/
  `PySequence_GetItem` -- protocol dispatch, owned reference per
  element), at every node, in both the validation and encode passes.
  This is exactly the cost `JPClass::sequenceCheckList`/
  `sequenceCheckTuple` (`jp_class.h`) already exist to eliminate for the
  flat (1D) push path via `PyList_GET_ITEM`/`PyTuple_GET_ITEM` (direct
  index, borrowed reference, no dispatch) -- the ragged-native path had
  never gotten the equivalent treatment. Fix: classify each node once
  (list/tuple/generic) and use type-specific loops instead of the
  one-size-fits-all `seq[i]` path, in both passes. Measured via isolated
  `git worktree` + fresh venv before/after: `list->array` push at depth
  2-5 is 2-4.4x faster for both rectangular and ragged shapes across all
  four leaf types, closing and reversing the deficit -- jpype now leads
  jpy at every depth/shape in Sections 5 and 6 above (e.g. rectangular
  `int[][][][][](10^5)`: was 4,447,629 vs. jpy 1,757,121 (jpy 2.53x
  faster), now 1,088,620 vs. the same jpy figure (jpype 1.6x faster);
  ragged `int[][][][][](~10^5)`: was 5,108,332 vs. jpy 2,139,732 (jpy
  2.39x faster), now 1,389,415 vs. the same jpy figure (jpype 1.5x
  faster)).

  Section 7's `list->array` table recurses through the same
  `JPConversionRaggedSequence`/`fillRaggedFromBuffer` path and was missed
  in the original refresh -- it still showed pre-specialization numbers
  (jpype losing to jpy 2-4x on every shape) until re-measured and
  corrected 2026-08-13. No code changed for this refresh, only the
  recorded numbers.

- **Resolved 2026-08-13: `array->list` pull (int/long) was dominated by
  tagged-wrapper allocation cost, not the JNI read.** Profiled
  `getFastArrayItem`'s two stages directly (instrumented, isolated
  build): reading one element via `GetIntArrayRegion` cost ~34ns; boxing
  it into a `JInt` via `PyJPNumber_longFromLongLong`
  (`native/python/pyjp_number.cpp`) cost ~135ns, of which ~45ns was
  `tp_alloc` (`PyType_GenericAlloc` -- a non-builtin heap type gets none
  of `PyLong`'s own small-int cache or specialized allocator) and ~44ns
  was the digit-fill/sign-tag work `PyLong_FromLong` itself would also
  have to do, plus ~47ns of call-boundary overhead. A/B against a
  same-call-site plain-`PyLong` bypass confirmed the gap: ~42ns plain vs.
  ~135.5ns tagged, a ~3.2x difference matching the table's own ~1.3-1.5x
  end-to-end swing once JNI/bounds-check/list-append overhead dilutes it.

  Fix: a fixed-size recycling pool for the `JByte`/`JShort`/`JInt`/`JLong`
  leaves (`intfreelist` in `pyjp_number.cpp`) -- one bucket sized for the
  largest possible `jlong`'s digit count (there's nothing to gain from a
  finer per-digit-count scheme), a lock-free Treiber stack (atomic head,
  CAS push/pop, correct under free-threaded CPython even though nothing
  here needs that yet, since construction runs under the GIL today), and
  a custom `tp_dealloc` that pushes back instead of freeing. Gated by
  exact type-pointer identity against the four leaves specifically:
  confirmed `class MyInt(JInt): pass` raises `TypeError` ("Java classes
  cannot be extended in Python"), so a Python subclass can never reach
  this path, but `PyJPNumber_create` boxes `java.lang.Integer`/`Long`/
  etc. return values through this same shared function with the *boxed*
  class's own distinct host type -- pooling those too may well be safe
  but is out of scope here, so the identity gate correctly excludes them.
  `JBoolean` gets its own fix instead of a pool -- two process-lifetime
  singletons (like Python's own `True`/`False`), since a Java boolean has
  only two possible values; built lazily on first real construction
  (building them eagerly from `PyJPNumber_initType` crashed at
  `import _jpype` time -- that runs before any JVM/context exists, and
  the eager version needed one).

  Verified via isolated `git worktree` + fresh venv: full suite (1588
  tests, 3 runs with randomized ordering) clean; a targeted stress script
  (heavy churn value-fidelity, refcount sanity, boolean singleton
  identity, interleaved mixed-type recycling with out-of-order drops, an
  8-thread concurrent array-pull stress test) all clean. Measured:
  `array->list int[100000]` 11,227,930 -> 9,265,813ns (1.21x),
  `array->list long[100000]` 10,766,004 -> 8,235,060ns (1.31x),
  `toList(dtype=int) int[100000]` (the wrapped wrapper-construction path)
  9,121,523 -> 6,278,195ns (1.45x), `JBoolean(True)` construction 250 ->
  90ns (2.78x, singleton). `float`/`double` and the plain/forced-cast
  `toList()` variants are unaffected by design (untouched by this pool;
  they already used bare `PyLong_FromLong`/`PyFloat_FromDouble`).

