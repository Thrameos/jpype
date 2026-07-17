# DOCS: split the monolith into paired Python/Java manuals

## Status (2026-07-17): DONE — the split is complete, `userguide.rst` deleted

All originally-planned chapters, all discovered-but-unplanned chapters, and
the narrative front matter are now extracted (see "Unplanned activities"
below for the discovered chapters and the correctness-gap fixes).
`userguide.rst` no longer exists — its content lives entirely in the files
under `doc/`, and `index.rst`'s toctree lists them directly.

`limitations_py.rst` extracted from `userguide.rst` (old "JPype Known
limitations" section, lines 6215-6518) and `limitations_java.rst` written
from scratch, covering real, verified state: unimplemented stdlib coverage
(`python.re`/`python.queue` scoped-not-built vs. `python.io`/
`python.collections`/`python.datetime`/`python.decimal`/`python.pathlib`
all landed — note `python.decimal`/`python.pathlib` exist in the tree
despite `plan/archive/Decimal.md` still reading "not started", that plan
doc is stale and should be corrected separately), no reverse pickling/
NumPy story, subinterpreters' legacy-style (shared-GIL) isolation model,
and the async thread-pool's GIL-bound concurrency ceiling. `userguide.rst`'s
old section replaced with a short pointer (not yet deleted outright — full
file deletion waits until every section is extracted per the file map
below). `index.rst` toctree updated. No Sphinx available in this
environment to do a real build check; verified manually instead (no new
`.. _label:` collisions introduced, single pre-existing unrelated
`JClass` duplicate label left untouched).

Step 2: `quickguide.rst` -> `quickguide_py.rst` (git mv, content unchanged
apart from the intro paragraph now cross-referencing `quickguide_java`).
Discovered along the way: `doc/quickguide.py` is a standalone generator
script that printed `quickguide.rst`'s RST table structure (not wired into
any build step — no Makefile/CI reference found) — renamed alongside it to
`quickguide_py.py` to keep the pairing, left otherwise untouched since it
isn't invoked anywhere. New `quickguide_java.rst` written as a "5-minute
tour" grounded in real `NGTest` sources (`PyBuiltInNGTest`, `PyStringNGTest`,
`PyDictNGTest`, `PyDequeNGTest`, `PyStringIONGTest`, `PyPathNGTest`,
`PyExcNGTest`): starting/closing `MainInterpreter`, `Script` as a scoped
`PyBuiltIn`, `eval`/`exec`, basic `python.lang` types, calling
`PyCallable`s via `call(Object...)`, the `<Module>.using(context)` stdlib
front ends (`python.collections`/`python.io`/`python.pathlib`), and
`python.exceptions` catching. Every code snippet's method signatures were
checked against the real interfaces (`PyDeque.append(PyObject)` needed
`context.$int(1)`, not a bare `int`, for example). `index.rst` toctree
updated (`quickguide` -> `quickguide_py`, `quickguide_java` added). No
other `.rst` files referenced the bare `quickguide` name, so no further
cross-reference fixes were needed. Anchor-collision check re-run, still
clean.

Step 3: `types_py.rst` extracted from `userguide.rst`. Correction to the
original file map: the chapter actually starts at line 1227 (the
`.. _jpype_types:` anchor + `JPype Types` `*`-level title), not line 1399
as this plan originally said — 1399 was only where the "Primitive Types"
`=`-level subsection begins, missing the chapter's own intro and the
"Stay strong in a weak language"/"Java conversions"/"Java casting" content
(anchors `cast`, `JObject`, `null`) that precedes it. Extracted the full
range 1227-2129. Kept the page title at `*`-level (matching the source,
since this extraction is a whole former `*`-chapter, unlike
`limitations_py.rst` which was only an `=`-subsection) rather than
forcing every heading down a level to fit an `=`-level title — docutils
assigns heading levels by first-occurrence order per document, not by a
fixed character-to-level table, so `*` as a page's own title character is
valid and required no changes to the ~15 `=`/`-`/`~` headings already
inside the extracted body. `types_java.rst` written as a `python.lang`
reference: the `PyObject` root (interface, not concrete class — instances
only ever come from `PyBuiltIn` factories, `eval()`/`exec()`, or a
`PyCallable`'s return value), the protocol interfaces
(`PyIterable`/`PySequence`/`PyMapping`/`PyCallable`/`PyBuffer`/`PyNumber`/
`PyIndex`/`PySized`/`PyCombinable`), and the concrete-ish per-type
sections (`PyString`, `PyInt`/`PyFloat`/`PyComplex`, `PyDict`, `PyList`/
`PyTuple`, `PySet`/`PyFrozenSet`, `PyBytes`/`PyByteArray`, `PySlice`/
`PyRange`, `PyMemoryView`), each grounded in the real interface
declarations under `native/jpype_module/src/main/java/python/lang/` and
snippets checked against `PyStringNGTest`/`PyBuiltInNGTest`/`PyDictNGTest`/
`PyBytesNGTest`. Found and fixed a real anchor collision this step
introduced: the userguide.rst pointer stub re-declared `.. _jpype_types:`,
which now also lives in `types_py.rst` — removed the anchor from the stub
(kept the stub's heading text only). `index.rst` toctree updated
(`types_py`, `types_java` added, placed right after `userguide`). The
`quickguide_py.rst` `` :doc:`userguide` `` reference was deliberately left
pointing at `userguide` rather than retargeted to `types_py` — the
sentence it's in points at "additional details on the use of the JPype
module" broadly, not specifically the Types chapter, and most of that
broader content still lives in `userguide.rst` at this point in the
split. Anchor-collision check re-run after the fix, clean again (only the
pre-existing unrelated `JClass` duplicate remains). Named implicit
hyperlink targets referenced from the extracted text (`` `Method
Resolution`_ ``, `` `Type Matching`_ ``, `` `String Conversions`_ ``,
`` `Exception Handling`_ ``, `` :ref:`optimize_data_transfers` ``) all
still resolve — their section titles/labels still live in `userguide.rst`,
and Sphinx resolves named targets across the whole doctree, not just
within one file, so no action was needed there; they'll need re-checking
once those chapters are themselves extracted.

This plan absorbs and supersedes the original 2026-07-11 version of this
file (reverse-manual content mapping only). Two things changed the scope:

1. `doc/spi.rst` shipped (as part of the `e5a93a46` reverse-bridge merge) as
   a standalone, Java-facing, RST customization doc — proof the "new file
   per topic" approach works and reads fine under Sphinx/RTD.
2. User direction (2026-07-17): don't just add a new reverse-direction
   manual alongside the existing monolith — **split `doc/userguide.rst`
   itself** (6729 lines, all Python-view) into per-topic files, and write
   the Java-view ("Java calls Python") equivalent for each topic as a
   paired file, so the two audiences get symmetric, cross-linked coverage
   instead of one 6729-line file plus one bolted-on reverse chapter.

## Naming convention (decided)

Flat layout, no subdirectories — `doc/` stays a single directory, same as
today. Reasons (discussed and confirmed with user): Sphinx `:doc:`/`:ref:`
targets stay short and unambiguous (`` :doc:`types_java` `` vs. a
directory-relative path that depends on which file is doing the
including), and the `index.rst` toctree stays one flat list — no per-
subdir `index.rst` glue for RTD to build correctly.

Paired topic files: **`<topic>_py.rst`** (Python calling Java — the
existing content, extracted from `userguide.rst`) and **`<topic>_java.rst`**
(Java calling Python — new content, per the mapping table below). Suffix
always present on both sides, even though `_py.rst` is "the original
direction" — consistency beats brevity here, and an unsuffixed bare
`types.rst` would look like it's the canonical/default one.

Files that aren't paired by direction (`install.rst`, `api.rst`,
`dbapi2.rst`, `spi.rst`, `android.rst`, `develguide.rst`, `CHANGELOG.rst`)
keep their current bare names — the `_py`/`_java` suffix only applies to
content that has (or will have) a direction-mirrored counterpart.
`doc/spi.rst` itself predates this convention and is Java-only content
with no Python-view counterpart to pair against (SPI is a Java-side
extension mechanism); leave its name as-is, don't rename for
consistency's sake alone.

`quickguide.rst` becomes `quickguide_py.rst`, paired with a new
`quickguide_java.rst` (the "reverse quickguide" from the original plan) —
it's exactly the paired-topic shape even though it currently lives outside
`userguide.rst`.

## Splitting userguide.rst — proposed file map

Derived from the file's actual top-level (`====`) headers (verified by
reading the file, not guessed). Each row is a `_py.rst` extraction
target; sub-headers (`----`, `~~~~`) nest inside their parent file as-is,
they are not further split at this pass.

| New `_py.rst` file | Extracted from (current line range) | Covers |
|---|---|---|
| `intro_py.rst` | 1-982 | Why JPype, prerequisites, install pointer, first program, use cases, philosophy, other JVM languages, alternatives, about this guide |
| `concepts_py.rst` | 986-1398 | Core concepts, JVM startup best practices, weak-typing conversions/casting |
| `types_py.rst` | 1399-2132 | Primitive types, Objects & Classes (arrays, buffers, boxed, Number, Object, String, Exception, anonymous/lambda/inner classes) |
| `importing_py.rst` | 2133-2385 | Importing Java classes, name mangling, method resolution, type matching |
| `exceptions_py.rst` | 2386-2537 | Exception handling |
| `jvm_py.rst` | 1631-2235 | Starting and shutting down the JVM |
| `customizers_py.rst` | 1640-2053 | Class customizers, type conversion customizers, JPype Beans, name-conflict resolution |
| `collections_py.rst` | 3562-3970 | Specialized collection wrappers, Pythonic-construct integration |
| `pickling_py.rst` | 1657-1871 (not 3971-4191 as originally guessed) | JPickler |
| `numpy_py.rst` | 1873-2216 (not 4192-4554 as originally guessed) | Array transfer, buffer-backed arrays, NumPy primitives |
| `proxies_py.rst` | 2221-2681 | Passing Python callables to Java functional interfaces, `Implements`, `JProxy`, reference loops |
| `threading_py.rst` | 3727-4247 | Threading, `java.lang.Thread` customization, synchronization, multiprocessing (GUI shells turned out to be their own separate chapter, `managing_crossplatform_gui_environments`, not part of this one -- left unsplit, not in original scope) |
| `dbapi2_py.rst` | 5724-5812 | (mostly redundant with existing `dbapi2.rst` — likely becomes a short pointer/summary, not a full extraction; confirm during execution) |
| `tooling_py.rst` | 2685-2723 (GC sub-topic only, as scoped; Javadoc/autopep8/performance/code completion at their own stale-guessed range remain unsplit) | Javadoc, autopep8, performance, code completion, garbage collection |
| `debugging_py.rst` | 5996-6216 | Debugging Java code from Python, diagnostics, caller-sensitive methods |
| `limitations_py.rst` | 6217-6520 | Known limitations |
| `glossary.rst` | 6521-6729 | Freezing, trivia, glossary — direction-neutral, no `_java` counterpart planned; bare name |

`userguide.rst` itself is deleted once all sections are extracted and
`index.rst`'s toctree is updated to list the new files directly (no
umbrella file needed — `index.rst` already serves as the top-level
landing page).

Mechanical notes for whoever does the extraction:
- 267 `.. _label:` anchors already exist in `userguide.rst` (`doc/
  add_labels.py` is the tool that generated them). Check each extracted
  file keeps its own anchors intact and that none collide across the new
  files (unlikely — they're header-text-derived and headers don't repeat
  across chapters, but verify, don't assume).
- Only one existing cross-file reference existed: `quickguide_py.rst`'s
  `` :doc:`userguide` `` (still `userguide`, not yet retargeted to
  `types_py` — left alone since `userguide.rst` still holds that content
  and `types_py.rst` doesn't exist yet; retarget when step 3 splits it
  out).
- No other `.rst` file links into `userguide.rst` by path or `:ref:`
  (checked: only the toctree entry in `index.rst` and that one
  `quickguide_py.rst` reference).

## Mirror mapping: `_py.rst` topic -> `_java.rst` content

Carried over from the original plan, retargeted to the new filenames.
"Status" reflects real landed code as of 2026-07-17, not the stale
2026-07-11 assessment.

| `_py.rst` file | `_java.rst` equivalent content | Status |
|---|---|---|
| `intro_py.rst` | `intro_java.rst` — same pitch, reversed direction; lead with the differentiator (Python objects satisfying arbitrary hand-written Java interfaces via `$`-mangled dispatch, not a fixed pre-generated binding — this is the framing the user endorsed 2026-07-11, keep it) | not started |
| `types_py.rst` | `types_java.rst` — the `python.lang` `PyObject` hierarchy: concrete types (`PyString`, `PyDict`, `PyList`, `PyInt`, `PyFloat`, `PyBytes`, `PySlice`, `PyRange`, `PySet`/`PyFrozenSet`, `PyTuple`, ...) and protocol interfaces (`PyIterable`, `PyCallable`, `PyMapping`, `PySequence`, `PyBuffer`, ...); supersedes `doc/bridge.md`'s design-note role as the user-facing reference. Ground truth: `native/jpype_module/src/test/java/python/lang/*NGTest.java`. | **DONE** |
| `collections_py.rst` | `collections_java.rst` — `python.collections` (`PyDeque`/`PyOrderedDict`/`PyDefaultDict`/`PyCounter`/`PyChainMap`); `PyCollections.using(context)` entry point; the `PyDict` vs `PyMapping` type-shape callout (dict subclasses vs. `ChainMap`'s weaker `MutableMapping` shape). Ground truth: `python/collections/*NGTest.java` (43 tests). Cross-reference `plan/SPI_tutorial.md` for the underlying SPI mechanism (usage here, authoring there — keep that split). | **DONE** |
| *(new pairing, no `_py` precedent)* | `datetime_java.rst` — `python.datetime` (`PyDate`/`PyDateTime`/`PyTimeDelta`); `DateTime.using(context)`; naive-vs-aware callout; `$`-name-collision hazard for the `java.time`-accepting factory convenience methods. Ground truth: `python/datetime/*NGTest.java` (25 tests). No natural `_py` counterpart topic in `userguide.rst` — file it as direction-only, cross-linked from `types_java.rst` instead of paired. | **DONE** |
| `jvm_py.rst` | `jvm_java.rst` — starting the embedded interpreter, `PyBuiltIn`/`context`, shutdown, GIL model (translate [[jpype_gil_reacquire_bug]]'s "every call into Java is a potential GIL release boundary" into user-facing guidance) | **DONE** |
| `customizers_py.rst` | `customizers_java.rst` — SPI usage (already has a home in `doc/spi.rst`; this file should mostly cross-link there, not duplicate) plus the still-undocumented **`$`-mangled direct dispatch** mechanism (`plan/archive/NameMangling.md`, `plan/archive/DispatchFallback.md`) — when to use `$foo` vs. a mapped method, the collision-avoidance rationale, worked example from `DispatchFallbackNGTest` including at least one adversarial hazard case (`NoSuchMethodError` vs `PyTypeError` failure modes) | **DONE** |
| `pickling_py.rst` | none planned | no reverse-direction pickling story exists — flagged as an open note in the file itself | **DONE** |
| `numpy_py.rst` | none planned | no reverse-direction numpy story exists — same treatment as pickling | **DONE** |
| `proxies_py.rst` | *(cross-reference only)* — `proxies_py.rst` already documents Java calling back into Python via `JProxy`/`Implements` (Python object playing a Java role). Add a short "don't confuse this with `types_java.rst`" note in both directions distinguishing that from `python.lang` (Java code directly manipulating a live Python object) — small addition, not a new chapter | **DONE** |
| `threading_py.rst` | `threading_java.rst` — `PyCallable.callAsync`/`callAsyncWithTimeout`, the thread pool, GIL-per-call model, what's safe from a background thread. Highest-correctness-risk chapter (GIL) — review line-by-line against [[jpype_gil_reacquire_bug]] before publishing, don't just transcribe from memory. | **DONE** |
| `tooling_py.rst` | *(garbage collection sub-topic only)* — `PyObject` proxy lifetime vs. CPython refcounting, `PyMemoryView.release()`, stream `close()` (`python.io`, landed). Rest of `tooling_py.rst` (Javadoc/autopep8/performance/code completion) has no reverse-direction equivalent yet. | **DONE (GC sub-topic only, as scoped)** |
| `limitations_py.rst` | `limitations_java.rst` — enumerate what's not implemented in the reverse direction. `python.io`/`python.collections`/`python.datetime` are all landed, drop them from any gap list; pull remaining gaps from `plan/Pathlib.md`/`plan/Decimal.md`/`plan/Re.md`/`plan/Queue.md` status. Subinterpreters: supported but legacy-style (shared process GIL/allocator, not full PEP 684 isolation) — document as such, not as full isolation. | **cheap, do first** — content mostly already exists across memory/plan files, low risk of drifting stale before publish |

`api.rst` equivalent: no new file needed. `python.lang`/`python.io`/
`python.exceptions`/etc. package Javadoc already carries the real
per-method documentation (doc comments exist on every interface method,
e.g. `PyBytes.decode`'s encoding/errors block) — `api.rst` should grow a
short "reverse-bridge API reference" pointer section linking to the
generated Javadoc, not hand-written prose duplicating it.

## Ground truth: source content from tests, not memory

Unchanged from the original plan — still the single biggest risk. Every
Java-side code example must be lifted from or directly checked against a
passing `NGTest` file, not freehand-written from what the API is
remembered to look like:

- `native/jpype_module/src/test/java/python/lang/*NGTest.java`,
  `python/collections/*NGTest.java`, `python/datetime/*NGTest.java`,
  `python/io/*NGTest.java` are the closest thing this project has to a
  spec.
- `doc/quickguide.py` (now `quickguide_py.py`) is **not** wired into any
  build or CI step — confirmed 2026-07-17, no Makefile/CI reference found.
  It's a standalone generator script that once printed the RST table
  structure now living in `quickguide_py.rst`; the two have since diverged
  (the `.rst` is hand-maintained). Treat it as a historical artifact, not
  live tooling. `quickguide_java.rst` was instead written with every
  snippet checked directly against real `NGTest` method signatures (no
  generator).

## Open questions

1. Should `_py.rst` extraction and `_java.rst` authoring happen chapter-
   by-chapter as one unit (split `types_py.rst` out, immediately write
   `types_java.rst` next to it), or as two passes (finish all extraction
   first, then write all Java content)? Leaning one-unit-per-chapter —
   keeps the cross-references honest while both halves are fresh, and
   gives incremental shippable value instead of a big-bang rewrite sitting
   unreviewed.
2. Should `python.lang` be documented as shipped/stable or caveated as
   pre-release? Per [[jpype_naming_convention]] the naming pass already
   landed (444587f4), so this is less of a blocker than the 2026-07-11
   version of this plan assumed — lean toward documenting it as real,
   current API, no longer provisional-pending-rename.
3. `doc/bridge.md` disposition — once `types_java.rst` exists, leave
   `bridge.md` alone as a historical internal design note (same treatment
   as any closed-out `plan/archive/*.md`), don't retrofit it into a manual
   chapter or delete it.
4. `dbapi2_py.rst`: confirm during execution whether `userguide.rst`'s
   "Database Access with jpype.dbapi2" section (5724-5812) is genuinely
   redundant with the standalone `dbapi2.rst` before deciding whether it's
   a full extraction or a two-line pointer.

## Suggested execution order

1. **DONE** — `limitations_py.rst` split + `limitations_java.rst`.
2. **DONE** — `quickguide_py.rst` rename + `quickguide_java.rst`
   ("5-minute tour of calling Python from Java"); examples pulled from
   `PyBuiltInNGTest`/`PyStringNGTest`/`PyDictNGTest`/`PyDequeNGTest`/
   `PyStringIONGTest`/`PyPathNGTest`/`PyExcNGTest`. Note: `quickguide_java.rst`
   ended up in its own prose-plus-code-block style rather than
   `quickguide_py.rst`'s Java/Python dual-column table — the reverse
   direction doesn't have a natural "same operation, two languages" pairing
   the way forward-bridge snippets do (there's no Python-side code to show
   side by side with the Java driver code), so a table would have had an
   empty or redundant second column throughout.
3. **DONE** — `types_py.rst` split + `types_java.rst`, the meaty reference
   chapter, supersedes `bridge.md`'s user-facing role. `collections_py.rst`
   split + `collections_java.rst` (source chapter was at `userguide.rst`
   lines 2653-3059, not the range originally guessed) plus `datetime_java.rst`
   (cross-linked from `types_java.rst`, no `_py` pair, per plan) are also
   done: `PyCollections.using(context)`/`DateTime.using(context)` entry
   points, the `PyDict` vs `PyMapping` type-shape callout, and the
   `java.time`-accepting-factory naming-collision hazard, all grounded in
   `python/collections/*NGTest.java` (43 tests) and
   `python/datetime/*NGTest.java` (25 tests). Same duplicate-anchor pattern
   as step 3's types half recurred (`.. _collections:` left in both the new
   file and the `userguide.rst` stub) and was caught the same way (anchor-
   collision grep) and fixed the same way (drop the anchor from the stub,
   keep the heading text).
4. **DONE** — `jvm_py.rst` split (`userguide.rst` lines 1631-2235,
   "Controlling the JVM") + `jvm_java.rst` (`MainInterpreter` singleton
   start/`isStarted()`/irrevocable `close()`, `PyBuiltIn`/`Script`, a
   "GIL model, briefly" section previewing step 4's second half). Then
   `threading_py.rst` split (`userguide.rst` lines 3727-4247, "Concurrent
   Processing" — the plan's original 5034-5723 guess was stale; the real
   chapter covers threading/`java.lang.Thread` customization/multiprocessing
   but turned out *not* to include the GUI-shell content the plan expected,
   that's the separate, still-unsplit `managing_crossplatform_gui_environments`
   chapter) + `threading_java.rst`, written carefully and cross-checked
   line-by-line against [[jpype_gil_reacquire_bug]] and
   [[jpype_gil_leak_diagnostic_method]]: the GIL-per-call model (every
   Java->Python call acquires/releases automatically, no explicit API),
   subinterpreters' shared-GIL legacy-style isolation, `PyCallable.callAsync`/
   `callAsyncWithTimeout` on the bounded 32-thread daemon pool (concurrency
   bound, not throughput — grounded in `PyCallable.java`'s own doc comments
   and `PyCallableAsyncNGTest`'s 50-concurrent-call test), and an explicit
   "what's safe from a background thread" section. Same duplicate-anchor
   bug pattern recurred a third time (`.. _controlling_the_jvm:` and
   `.. _concurrent_processing:` both left in their `userguide.rst` stubs)
   and was caught/fixed the same way. `limitations_java.rst`'s existing
   `` :doc:`threading_java` (once written) `` forward-reference updated now
   that the file exists. `index.rst` toctree updated.
5. **DONE** — `customizers_py.rst` split (`userguide.rst` lines 1640-2053,
   "Customization") + `customizers_java.rst`: summarizes and cross-links
   `doc/spi.rst` for the SPI-provider mechanism (deliberately not
   duplicated), and covers the `` ``$``-mangled direct-dispatch ``
   mechanism in full — the root-based trigger (`PyObject` anywhere in an
   interface's hierarchy), the `mangle()` transform (`$foo` -> real
   attribute `foo`, anything else -> map-only `.foo`), and a worked example
   grounded in `DispatchFallbackNGTest` (`PyAliceBobCharlieDerik` fixture)
   including all four adversarial failure modes it tests (clean
   `NoSuchMethodError` for a missing attribute, `PyTypeError` for a
   non-callable one, real exception passthrough, `PyTypeError` for a
   return-type mismatch). `proxies_py.rst` split (`userguide.rst` lines
   2221-2681, "Calling Python Code from Java" — the `.. _Proxies:` chapter)
   with the planned cross-reference note added in both directions
   (`proxies_py.rst` <-> `types_java.rst`), distinguishing a Python object
   playing a Java role (`@JImplements`/`JProxy`) from Java code directly
   holding a live Python object (`python.lang`) — no `_java.rst` counterpart
   for this one, matching the plan's cross-reference-only scope.
   `tooling_py.rst` split, scoped narrowly to just the Garbage collection
   sub-topic (`userguide.rst` lines 2685-2723, the `=`-level subsection
   inside "Miscellaneous topics") per the plan's explicit "(garbage
   collection sub-topic only)" scoping — the rest of that source chapter
   (dbapi2, Javadoc, autopep8, performance, code completion, debugging,
   glossary) is intentionally left unsplit, out of scope for this pass.
   `tooling_java.rst` written to match: `PyObject` proxy lifetime tied to
   Java GC via `org.jpype.ref.NativeReferenceQueue` (a phantom-reference
   cleaner — the reverse-direction analog of the linked-GC mechanism
   `tooling_py.rst` describes from the Python side), plus explicit-release
   patterns for scarce resources that shouldn't wait on GC timing
   (`PyMemoryView.release()`, `python.io`'s `close()`/`closed()`), grounded
   in `PyMemoryViewNGTest`/`PyStringIONGTest`. Same duplicate-anchor bug
   pattern recurred again (`.. _customization:` left in its stub) and was
   caught/fixed the same way; the `proxies_py.rst` and `tooling_py.rst`
   stubs were written anchor-free from the start this time, avoiding the
   bug rather than fixing it after the fact. `index.rst` toctree updated.
6. **DONE** — `pickling_py.rst` split (`userguide.rst` lines 1657-1871,
   "Serialization with JPickler" — not the originally-guessed 3971-4191
   range) and `numpy_py.rst` split (lines 1873-2216, "Working with NumPy" —
   not the originally-guessed 4192-4554 range; both chapters turned out to
   sit right after "Customization"/"Collections", not down near the
   threading/dbapi2 area the stale guesses implied). No `_java.rst`
   counterpart for either — `python.io`/`python.collections`/
   `python.datetime` don't expose a JPickler- or NumPy-array-shaped front
   end, so each file opens with an explicit "no reverse-direction story
   exists" note rather than inventing placeholder content. Both stubs in
   `userguide.rst` written anchor-free from the start (the recurring
   duplicate-anchor bug from steps 3-5 avoided proactively throughout this
   step). `index.rst` toctree updated. Verification: anchor-collision grep
   shows only the pre-existing unrelated `.. _JClass:` duplicate; all
   `:doc:` cross-references across `doc/*.rst` resolve to an existing file.

Per user direction (2026-07-17): **start execution with step 1**
(`limitations_py.rst` + `limitations_java.rst`) in the next session.

All six steps of the suggested execution order are now DONE.

## Unplanned activities (2026-07-17, same session as step 6)

Per user direction ("continue with the plan (and unplanned activities)"),
finished off every remaining reference-style chapter that was either
explicitly planned but not yet done, or discovered along the way and not
in this plan's original file map at all:

- **`dbapi2_py.rst`** (planned, `userguide.rst` lines 1519-1607,
  "Database Access with jpype.dbapi2") — confirmed redundant with the
  existing full reference `doc/dbapi2.rst` per the plan's own suspicion;
  became a short pointer rather than a full extraction. No `_java`
  counterpart (no reverse-direction `jpype.dbapi2` story).
- **`debugging_py.rst`** (planned, lines 1536-1752, "Using JPype for
  debugging Java code") — full extraction including all ten subsections
  (Attaching a Debugger through Caller-Sensitive Methods). No `_java`
  counterpart.
- **`glossary.rst`** (planned, bare name, lines 1551-1759, "Freezing" +
  "Useless Trivia" + "Glossary") — combined into one direction-neutral
  file per the plan's own description, no `_py`/`_java` pairing.
- **`awt_py.rst`** (unplanned, discovered — "AWT/Swing", lines 1293-1331)
  — not in the original file map at all. Extracted with the same
  "no reverse-direction story" open-note treatment as pickling/numpy.
- **`gui_py.rst`** (unplanned, discovered — "Managing Cross-Platform GUI
  Environments", lines 1309-1471 pre-extraction numbering) — also not in
  the original file map; same treatment.
- **`tooling_py.rst` extended** (unplanned) — the plan explicitly scoped
  `tooling_py.rst` to garbage collection only and left Javadoc/autopep8/
  performance/code completion unsplit "pending its own extraction pass."
  That pass happened now: those four sub-topics were appended to
  `tooling_py.rst` (retitled "Miscellaneous Tooling Topics") ahead of the
  Garbage collection section, fully retiring the "Miscellaneous topics"
  chapter from `userguide.rst`.
- **Correctness gap found and fixed**: `types_py.rst` (from step 3)
  contained internal cross-references (`` `Method Resolution`_ ``,
  `` `Exception Handling`_ ``) to sections that didn't exist in the file —
  the original step-3 extraction stopped short of the actual chapter
  boundary. `userguide.rst` lines 1237-1629 ("Importing Java classes",
  "Name mangling", "Method Resolution", "Type Matching", "Exception
  Handling", "Raising Exceptions from Python to Java", "Exception
  Aliasing") were still sitting unextracted directly under the "JPype
  Types" stub. Appended to `types_py.rst`, resolving the dangling refs.

All of the above followed the same anchor-free-stub discipline established
in steps 4-5; verification (anchor-collision grep, `:doc:` target
resolution, toctree file-existence check) passed clean after each file,
with only the pre-existing unrelated `.. _JClass:` duplicate remaining.
`index.rst` toctree updated throughout.

## Front-matter extraction + userguide.rst deletion (2026-07-17, per user direction)

The remaining `userguide.rst` content (lines 1-1225: "Introduction", "Why
Use JPype?", "Prerequisites", "Installation", "Your First JPype Program",
"JPype Use Cases" (three narrative case studies), "The JPype Philosophy",
"Languages Other Than Java", "Alternatives", "About this Guide", "Getting
JPype started", and "JPype Concepts" including "Best Practices on JVM
Startup") was extracted verbatim into a new bare-named `intro.rst`,
direction-neutral (no `_java` counterpart — it's JPype's own narrative
entry point, cross-linked to `quickguide_java` for the reverse direction's
equivalent). With that gone, everything left in `userguide.rst` was just
"Moved to ..." stubs pointing at already-extracted files, all already
present in `index.rst`'s toctree — so per the plan's own stated end-state,
`userguide.rst` was deleted and `index.rst`'s toctree entry swapped from
`userguide` to `intro`. The one real incoming reference,
`quickguide_py.rst`'s `` :doc:`userguide` ``, was repointed to `` :doc:`intro` ``.

## Correctness-gap sweep (2026-07-17, same session)

Beyond the anchor-collision/`:doc:`-target checks already run after every
extraction, a broader sweep looked for **cross-document implicit phrase
references** (`` `Some Text`_ `` without an explicit `:ref:` role) — these
only resolve within the same source document in RST, so any one that used
to work when all this content lived together in one `userguide.rst` breaks
silently once the target section moves to a different file. Found and
fixed:

- **Duplicate `.. _JClass:` anchor** (`api.rst` vs `types_py.rst`, present
  since before this session's work but never fixed) — renamed the
  `types_py.rst` one to `.. _jpype_types_jclass_factory:` (unreferenced by
  name elsewhere, safe rename).
- **`` `caller sensitive`_ ``** in `types_py.rst` -> target anchor now
  lives in `debugging_py.rst`. Converted to
  `` :ref:`caller-sensitive <caller sensitive>` ``.
- **`` `String Conversions`_ ``** in `types_py.rst` -> target lives in
  `jvm_py.rst`. Converted to `:ref:` with an explicit `:doc:` pointer.
- **`Proxies_`, `` `Code completion`_ ``** in `intro.rst` -> targets live
  in `proxies_py.rst`/`tooling_py.rst`. Converted to `:ref:`.
- **`` `Starting the JVM`_ ``, `` `Name Mangling`_ ``** in `intro.rst` ->
  targets live in `jvm_py.rst`/`types_py.rst`. Converted to `:ref:` with
  `:doc:` pointers.

Found but left alone (pre-existing, predates the `userguide.rst` split,
out of scope for this pass): `doc/dbapi2.rst` has a handful of same-file
phrase refs (`` `Connection Objects`_ ``, `` `Cursor Objects`_ ``,
`` `JDBC Types`_ ``) that don't resolve to any title or anchor in that
file — a pre-existing bug in a file this plan never touches.

Final verification (anchor-collision grep, `:doc:`-target resolution,
`:ref:`-target resolution, toctree file-existence check) passed clean
across all of `doc/*.rst`.

## Toctree reading-order pass (2026-07-17, same session)

The flat `index.rst` toctree alternated `foo_py`/`foo_java` pairs
(`types_py, types_java, collections_py, collections_java, ...`), which
reads fine as a reference index but is jarring for anyone reading the
docs front-to-back for one direction only -- half the chapters they hit
are about the direction they don't care about yet.

Restructured into four grouped `.. toctree::` blocks (each with a
`:caption:`), so each direction reads as one coherent track:

1. **Getting started** -- `install`, `intro` (shared, direction-neutral).
2. **Python calling Java** -- `quickguide_py`, `types_py`, `collections_py`,
   `jvm_py`, `customizers_py`, `proxies_py`, `threading_py`, `tooling_py`,
   `pickling_py`, `numpy_py`, `dbapi2_py`, `debugging_py`, `awt_py`,
   `gui_py`, `limitations_py`.
3. **Java calling Python (reverse bridge)** -- `quickguide_java`,
   `types_java`, `collections_java`, `datetime_java`, `jvm_java`,
   `threading_java`, `customizers_java`, `tooling_java`,
   `limitations_java`.
4. **Reference** -- `glossary`, `api`, `dbapi2`, `imports`, `spi`,
   `android`, `CHANGELOG`, `develguide`.

Before reordering, swept every `_py.rst`/`_java.rst`/`intro.rst` for
prose that assumes a specific read order ("prior chapter", "read X
first", etc. -- checked via grep, not just `:doc:`/`:ref:` link
resolution, since those work regardless of position). Found two real
same-track ordering constraints and satisfied both:

- `threading_py.rst` opens "Much of this material depends on the use of
  Proxies_ covered in the prior chapter" -- in the original flat
  toctree `proxies_py` came *after* `threading_py`/`threading_java`
  (dangling claim, pre-existing since the split, not previously
  caught). Reordered the Python track so `proxies_py` precedes
  `threading_py`.
- `threading_java.rst` opens "Read `jvm_java`'s 'GIL model, briefly'
  section first" -- already satisfied (`jvm_java` already precedes
  `threading_java`), no change needed.

All other cross-references between chapters are `:doc:`/`:ref:` pointers
to the *other* direction's counterpart chapter (e.g. `types_py.rst` ->
`:doc:`types_java``), which are cross-track signposts, not same-track
reading prerequisites, so they don't constrain ordering.

## develguide.rst rework (2026-07-17, same session)

`develguide.rst` predated the reverse bridge entirely (no mention of
`native/jpype_module`, `python.lang`, `MainInterpreter`, SPI, etc.) and
carried a lot of first-person historical narrative and opinion from the
original author (Thrameos) mixed in with genuine architecture. Per user
direction: pull out anything a general user needs for crash troubleshooting
into the user guide, trim what's easily discoverable via reading the code
(agent tools make that cheap now), keep the "why" design reasoning that a
full comprehensive read would be needed to reconstruct, and add the missing
embedded-Python architecture. 1150 -> 993 lines despite a net new section.

Moved to `debugging_py.rst`'s existing "Using a Debugger" section (`` .. _using-debugger: ``):
the Open-JDK 8 `gdb.error: No type named nmethod` issue + Open-JDK 9 fix,
the `PYTHONMALLOC` memory-pool-recycling debugging tip, and a plain-language
explanation of JPype's deliberate-crash mechanism (segfault-on-purpose so
`gdb` gets a clean backtrace, when it's expected, and to file a GitHub issue
if seen) -- all real content a general user hitting a native crash needs,
previously only in the developer guide. `develguide.rst`'s "Deliberate
Crash for Debugging" section was cut down to the one developer-specific
fact worth keeping (the code lives in `jp_context.cpp`, triggered on
unrecoverable startup/resource failures) with a pointer to `debugging_py.rst`
for the walkthrough.

Trimmed: the "History" section's first-person narrative (10+ paragraphs ->
folded into 2 sentences of Overview), the "Style" section's opining, and
"On CPython extensions"'s multi-paragraph rant -- kept the factual claims
each was wrapped around (naming convention rule; always use the vtable
macros, don't call fields/methods directly). Rewrote the stale "Future
directions" section (still described an 0.7/0.8/0.9 roadmap where 0.8's
"pickle support" item is now shipped, and never mentioned the reverse
bridge shipping at all) to state what's actually done and point at the
still-real forward-looking item (moving overload resolution into the Java
thunk; a from-scratch Panama/FFM-based JNI replacement being prototyped
separately, see [[jpype_j2ni_panama_future]] in memory -- not otherwise
referenced from this repo).

Added: new "Embedded Python (reverse bridge)" top-level section (between
"Java native code" and "Tracing and crash diagnosis") covering
`org.jpype.MainInterpreter`'s singleton-not-factory design (CPython can't
currently restart/run twice in a process) and `SubInterpreter`/
`SubInterpreterBuilder` as the escape hatch, the `python.*` front-end
packages as ordinary `WrapperService` SPI providers (cross-referencing
`spi.rst` rather than duplicating it), dispatch (`$`-mangled fallback,
mirroring the forward bridge's own name-mangling) and lifetime (native
reference queue vs. `JPReferenceQueue` symmetry), and the GIL-per-call
model in one paragraph (full detail already in `threading_java.rst`). This
is architecture-level "why", not a duplicate of the `_java.rst` user
chapters or `spi.rst` -- cross-references those rather than restating them.

Verification: `:doc:` target resolution (all 8 distinct targets referenced
from `develguide.rst` exist), anchor-collision check, phrase-ref check --
all clean.
