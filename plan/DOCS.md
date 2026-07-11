# DOCS: the reverse-direction manual (Java calling Python)

## TL;DR

`doc/userguide.rst` (6482 lines) documents Python code using Java
(`import java.lang.String`, JPype types, JVM control, proxies for Python
callbacks into Java). There is no equivalent manual for the new direction:
Java code holding and calling methods directly on Python objects via
`python.lang`/`python.exceptions`/(planned) `python.io`. `doc/bridge.md`
is the closest existing thing, and it's an internal design note about type
probing, not user-facing documentation — it predates most of the concrete
classes actually being implemented and is now stale as an API reference
(no `PyString`/`PyDict`/async/etc. content).

This plan is a chapter-by-chapter mirror of `userguide.rst`'s structure,
retargeted at the reverse direction, plus a concrete "how do we know the
docs are true" mechanism: use the `NGTest` suite as ground truth, not
memory or guesswork, since that's the only place the current API surface
is verified to actually work (451/451 passing per
[[jpype_reverse_bridge_testing]]).

## Existing manual structure (source of the mirror)

`doc/userguide.rst` top-level chapters, in order:

1. Introduction
2. JPype Types
3. Controlling the JVM
4. Customization
5. Serialization with JPickler
6. Working with NumPy
7. **Calling Python Code from Java** (existing — but this is proxies:
   Python objects implementing Java interfaces so Java can call back into
   Python-defined callbacks. Different feature from `python.lang`.)
8. AWT/Swing
9. Miscellaneous topics (threading, multiprocessing, GUI shells, javadoc,
   autopep8, garbage collection, diagnostics, known limitations, glossary)

Plus separate documents: `quickguide.rst` (tutorial-style walkthrough),
`api.rst` (API reference index), `install.rst`, `develguide.rst`.

## Mirror mapping

| userguide.rst chapter | Java-calling-Python equivalent | Status |
|---|---|---|
| Introduction | Introduction: "JPype the Java-to-Python bridge" — same pitch, reversed; explain this is the newer, not-yet-shipped surface, and how it relates to the existing Python-to-Java direction (same library, opposite call direction, can be used together) | not started |
| JPype Types | **python.lang Types** — the `PyObject` hierarchy: concrete types (`PyString`, `PyDict`, `PyList`, `PyInt`, `PyFloat`, `PyBytes`, `PySlice`, `PyRange`, `PySet`/`PyFrozenSet`, `PyTuple`, ...) and protocol interfaces (`PyIterable`, `PyCallable`, `PyMapping`, `PySequence`, `PyBuffer`, ...) — mirrors `doc/bridge.md`'s design intent but as verified, shipped API | not started; `bridge.md` is the design precursor, now stale as reference |
| Controlling the JVM | **Controlling the Python interpreter from Java** — starting the embedded interpreter, `PyBuiltIn`/`context`, shutdown, GIL model (this is where [[jpype_gil_reacquire_bug]]'s "every call into Java is a potential GIL release boundary" lesson belongs, translated to user-facing guidance) | not started |
| Customization | **Customization** — how to extend/adapt (this is thin today; revisit once [[jpype_python_io_port_todo]] and SPI (`plan/SPI.md`) land, since SPI is explicitly a customization/extension mechanism) | blocked on SPI landing |
| Serialization with JPickler | Not directly applicable yet — no reverse-direction pickling story exists. Flag as an open question, don't invent content. | open question |
| Working with NumPy | Not directly applicable yet — no reverse-direction numpy story exists (buffer promotion in `plan/IO.md` is adjacent but not this). Flag as open question, don't invent content. | open question |
| Calling Python Code from Java (proxies) | Already exists and already documents the *other* Java→Python mechanism (callbacks). Add a short cross-reference note distinguishing "proxies: Python object playing a Java role" from "python.lang: Java code directly manipulating a live Python object" so readers don't confuse the two. | small addition to existing chapter |
| AWT/Swing | Not applicable | n/a |
| Threading / Synchronization | **Async calls and the thread pool** — `PyCallable.callAsync`/`callAsyncWithTimeout`, the 32-thread pool, GIL-per-call model, what's safe to call from a background thread vs. what isn't | not started; this is the one chapter with the most subtle correctness content (GIL) and should be reviewed carefully against [[jpype_gil_reacquire_bug]] before publishing |
| Garbage collection | **Object lifetime** — how `PyObject` proxies relate to CPython refcounting, whether closing/releasing is needed anywhere (e.g. `PyMemoryView.release()`, streams' `close()` once `plan/IO.md` lands) | not started |
| Known limitations | **Known limitations** — mirror the existing chapter's honesty; enumerate what's NOT implemented yet (`python.io` — see `plan/IO.md`, any protocol gaps) rather than implying full parity with Python. Subinterpreters are now supported (start/register-classes/eval/close, verified with two subinterpreters + main running concurrently with no cross-talk or hangs) but should be documented as *legacy-style* — shared process GIL and object allocator with the main interpreter, not full PEP 684 own-GIL isolation, since `_jpype` is a single-phase-init extension | not started, but cheap and high-value — do this early since the list already exists implicitly across memory files (see [[jpype_subinterpreter_difficulty]], now resolved) |
| Glossary | Extend existing glossary with reverse-direction terms (proxy vs. bridge object, reverse-bridge, `_jbridge.py`, GIL leak terminology) or keep a single shared glossary | not started |

`quickguide.rst`-equivalent: a **reverse quickguide** — "5-minute tour of
calling Python from Java" — is probably higher value early than the full
reference chapters, since it's what a new reader actually wants first. This
should come before the deep chapters, not after.

`api.rst`-equivalent: mostly free — this is Javadoc's job
(`python.lang`/`python.io`/`python.exceptions` package Javadoc, generated
from the doc comments already present on every interface method, e.g.
`PyBytes.decode`'s existing encoding/errors doc block). The Sphinx `api.rst`
mirror should mostly be "link to the generated Javadoc," not hand-written
prose duplicating it.

## Ground truth: source content from tests, not memory

The single biggest risk for this doc effort is writing prose that describes
an API that used to be true, or was never quite true, or drifted during
implementation (this session alone fixed several real bugs — boxed-null
slices, `PyDictItems.add()`, `removePrefix`/`removeSuffix` — that a
doc-writer working from intent rather than the actual test suite would have
gotten wrong). Every code example in the manual should be lifted from or
directly validated against an existing passing `NGTest` file, not
freehand-written:

- `PyStringNGTest.java`, `PyDictItemsNGTest.java`, `PySliceNGTest.java`,
  `PyCallableAsyncNGTest.java`, etc. under
  `native/jpype_module/src/test/java/python/lang/` are the closest thing to
  a spec this project has (451/451 passing, per
  [[jpype_reverse_bridge_testing]]).
- Where `quickguide.rst` already does this for the forward direction, check
  whether `doc/quickguide.py` (a runnable companion script, given the `.py`
  file sitting next to `quickguide.rst`) is executed as part of doc build or
  CI — if so, the reverse quickguide should get the same treatment (a
  runnable Java companion, or at minimum examples copy-pasted verbatim from
  passing test methods) so the manual can't silently drift from working code
  again.

## Open questions

1. Format: match `userguide.rst`'s Sphinx/RST + `.. code-block::` style, or
   is Markdown acceptable (per `doc/bridge.md` precedent)? Given this ships
   as one Sphinx-built manual (`doc/conf.py`, `index.rst` toctree), RST is
   almost certainly required for anything meant to appear in the built
   manual; Markdown is fine for internal design notes like `bridge.md` but
   that's a different audience.
2. Does this get its own top-level chapter in `doc/index.rst`'s toctree, or
   folded into `userguide.rst` as new top-level sections? A new file (e.g.
   `doc/reverseguide.rst`) is probably cleaner given `userguide.rst` is
   already 6482 lines, but the "Calling Python Code from Java" chapter
   already sits inside `userguide.rst`, so there's precedent either way —
   needs a decision, not an assumption.
3. Should the manual document `python.lang` as "shipped/stable" or caveat it
   as pre-release? Per [[jpype_naming_convention]], a naming-check pass is
   still planned before shipping — publishing a manual with method names
   that then get renamed would immediately go stale. Likely answer: wait
   for the naming pass, or explicitly mark the chapter as provisional/
   pre-1.0 API subject to change.
4. `doc/bridge.md` disposition — supersede it with the new "python.lang
   Types" chapter once written (its content is now partially wrong: e.g. it
   proposes probing mechanisms and a class list that doesn't fully match
   what got implemented), or keep it as-is as an internal design record and
   layer the new user-facing chapter on top without touching it. Leaning
   toward: leave `bridge.md` alone as a historical design note (like
   `plan/SPI.md` will become once SPI ships), don't retrofit it into a
   manual chapter.

## Suggested order

1. Cheap, high-value, low-risk-of-going-stale-fast: **Known limitations**
   chapter — mostly transcribing what's already in memory files.
2. **Reverse quickguide** — the 5-minute tour, examples pulled from
   `PyStringNGTest`/`PyDictNGTest`/etc.
3. **python.lang Types** chapter — the meaty reference chapter, supersedes
   `bridge.md`'s user-facing role.
4. **Controlling the Python interpreter from Java** + **Async calls and the
   thread pool** — the two chapters with real correctness content (GIL),
   write these carefully and cross-check against
   [[jpype_gil_reacquire_bug]] line by line.
5. Everything else (Object lifetime / Customization / Glossary) once the
   above exist and once SPI/`python.io` land enough to have real content
   for the customization chapter.
6. Revisit "Serialization" and "NumPy" chapters only if/when a reverse-
   direction story for either actually gets built — don't write placeholder
   chapters for features that don't exist.
