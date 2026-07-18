# Systematic proofreading pass over doc/*.rst

## Status (2026-07-17): in progress — 27 of 36 files verified (Groups 1-3
fully done), plan below covers the rest (Group 4)

## Where this fits

Follow-on to `plan/archive/DOCS.md` (which split the old `userguide.rst`
monolith into the current paired `doc/*_py.rst`/`doc/*_java.rst` files).
The split itself was structurally verified at the time, but the *content*
inherited from `userguide.rst` was never audited for accuracy — it
predates this repo's "verify against real source" discipline. Auditing
five files this pass turned up real bugs (syntax-broken example code,
markdown headers that RST silently swallows, undefined variables in
examples, a duplicated/malformed code block, a factual count error), not
just wordiness — so the remaining ~28 files should be assumed guilty until
checked, not skimmed for tone alone.

## What "verified" means for this plan

Not a wordsmithing pass. Per file:

1. Read the whole file.
2. Cut the AI-slop structural pattern if present: repeated
   Overview/How-It-Works/Best-Practices/Summary/Conclusion sections that
   restate the same point 3-4 times. Keep one clean pass through each
   real point instead.
3. Check every code sample actually parses / would run — this is where
   the real bugs were hiding (`proxies_py.rst` had two syntax-broken
   imports and a fake `###` markdown header that Sphinx rendered as
   literal text without ever warning). Reading a code-block and nodding
   along is not enough; mentally (or actually) execute it.
4. Spot-check factual claims against real source rather than trusting the
   prose — grep the actual method/class/behavior being described. Full
   agent-assisted source verification (like the `spi.rst` rewrite got) is
   warranted for chapters describing nontrivial mechanism, not for a
   simple example page.
5. Grammar/typo pass (the `types_py.rst` commit is the bar: compile
   type -> compile time, an augment -> an argument, etc. — small but real
   errors that a spell-checker wouldn't catch because they're valid
   words, just the wrong ones).
6. Sphinx build (`-W`, warnings-as-errors) after every file, same command
   used throughout this effort:
   ```
   python3.12 -m sphinx -q -T -b html -d <scratch>/doctrees -D language=en -W doc <scratch>/htmlout
   ```
7. Commit per logical group (not one file per commit, not all-at-once) —
   that's the pattern already established: one commit for the `spi.rst`
   rewrite, one for the five filler files, one for the `types_py.rst`
   proofread.

## Already verified (do not redo unless something changes upstream)

| File | What was done |
|---|---|
| `spi.rst` | Full rewrite: deduplicated, added annotations/loading/backend/bypass coverage, fixed dot-mangled `METHODS` key bug |
| `threading_py.rst` | Added/verified full `toPython()` catalog including `java.lang.reflect.Method` (was undocumented) |
| `collections_py.rst` | Trimmed comparison/best-practices/conclusion filler |
| `customizers_py.rst` | Trimmed JPype Beans + name-conflict sections' 7-subsection padding down to one pass each |
| `numpy_py.rst` | Fixed dangling-fragment opening paragraph, trimmed filler |
| `pickling_py.rst` | Trimmed Best-Practices/Use-Cases/Conclusion filler |
| `proxies_py.rst` | Fixed 2 syntax-broken example imports, fake `###` headers, mismatched indentation, duplicated/malformed code block |
| `types_py.rst` | Proofreading pass: count error, ~15 grammar/typo fixes, footnote renumbering, 2 code-example bugs |
| `glossary.rst` | Checked, verified accurate against source (exception-aliasing table cross-checked against `_jcustomizer.py`); trivia essay is hand-authored, left alone |
| `CHANGELOG.rst` | Kept current as a side effect of other work; not independently audited for the older historical entries |
| `install.rst` | Fixed stale Python/Java version claims (3.5->3.8, 1.8-13->11+), malformed hyperlink, several typos |
| `intro.rst` | Fixed broken Java import, bracket mismatch in example, 2 malformed hyperlinks, jar-name typo; trimmed Why-Use-JPype/Next-Steps/Summary filler |
| `quickguide_py.rst` | Fixed Jint->JInt typos, Field.get()/Method.invoke() reflection-table bugs, array off-by-one, mismatched footnote, several typos |
| `jvm_py.rst` | Major: fabricated jvmOptions/attachThread/disableGC/stackTrace/initializers startJVM() kwargs that don't exist -- corrected to real signature; trimmed 2 filler Summary sections |
| `tooling_py.rst` | Fixed 2 malformed `code-block: java` directives (single colon, silently not rendered as code + wrong language); typos |
| `dbapi2_py.rst` | Already clean -- a short redirect page pointing at `dbapi2.rst`, no changes needed |
| `debugging_py.rst` | Major: build instructions used stale setuptools flags (`setup.py develop --enable-tracing`) -- repo moved to scikit-build-core/CMake, corrected to real `-C cmake.args="-D...=ON"` syntax (also fixed same staleness back in install.rst); grammar fixes |
| `caller_sensitive.rst` (orphan page, checked as a side effect) | Fixed comma-for-dot typo in a package path, removed 3 duplicate list entries; `:orphan:` marker confirmed present |
| `awt_py.rst` | Checked, verified accurate and non-slop; no changes needed |
| `gui_py.rst` | Verified setupGuiEnvironment/shutdownGuiEnvironment against jpype/_gui.py (accurate); trimmed Description/Parameters/Behavior/Why-Use-This/Example repetition and a restating Summary section |
| `limitations_py.rst` | Fixed self-contradiction (claimed Python 3.5 min, then said 3.8+ required -- same staleness as install.rst), SEGSEGV/SEGBUS->SIGSEGV/SIGBUS, other typos |
| `quickguide_java.rst` | Verified every API call (containsSubstring, putAny, CallBuilder, $int, PyDeque/PyStringIO/Pathlib.using) against real source -- accurate, no changes |
| `types_java.rst` | Verified protocol-interface hierarchy and concrete-type table against real python.lang sources -- accurate, no changes |
| `collections_java.rst` | Verified PyCollections factory signatures against source -- accurate, no changes |
| `datetime_java.rst` | Verified DateTime factory methods, naive/aware semantics, overload-hazard explanation against source -- accurate, no changes |
| `jvm_java.rst` | Verified MainInterpreter API (getInstance/start/isStarted/close/getBuiltIn) against source -- accurate, no changes |
| `threading_java.rst` | Verified GIL-per-call model, callAsync/callAsyncWithTimeout, 32-thread daemon pool against source -- accurate, no changes |
| `customizers_java.rst` | Verified `$`-mangle function verbatim against ProxyType.java, plus PyKwArgs/error-message claims against a real test -- accurate, no changes |
| `tooling_java.rst` | Verified PyMemoryView.release()/NativeReferenceQueue/close()/closed() against source -- accurate, no changes |
| `limitations_java.rst` | Verified python.re/python.queue are still unimplemented (plan/Re.md, plan/Queue.md still "not started") -- accurate, no changes |

## Systematic visit order for the rest

Grouped to keep review context loaded efficiently — visiting paired
`_py.rst`/`_java.rst` chapters back-to-back matters because the two
should describe the *same* underlying mechanism from opposite directions,
so inconsistencies between them are a specific thing to check for that a
solo read of either file won't surface.

### Group 1 — Getting started (read first; first thing a new user sees)

- [x] `install.rst` — done, see above
- [x] `intro.rst` — done, see above

### Group 2 — Python-calling-Java chapters not yet covered

In `index.rst` toctree order:

- [x] `quickguide_py.rst` — done, see above
- [x] `jvm_py.rst` — done, see above
- [x] `tooling_py.rst` — done, see above
- [x] `dbapi2_py.rst` — done, see above (already clean)
- [x] `debugging_py.rst` — done, see above
- [x] `awt_py.rst` — done, see above
- [x] `gui_py.rst` — done, see above
- [x] `limitations_py.rst` — done, see above (the python.re/python.queue
      caveat didn't apply -- this file doesn't mention them; check that
      note against `limitations_java.rst` in Group 3 instead)

### Group 3 — Java-calling-Python (reverse bridge) chapters

In `index.rst` toctree order, checking each against its already-verified
or freshly-verified `_py.rst` sibling for consistency:

- [x] `quickguide_java.rst` — done, see above
- [x] `types_java.rst` — done, see above
- [x] `collections_java.rst` — done, see above
- [x] `datetime_java.rst` — done, see above
- [x] `jvm_java.rst` — done, see above
- [x] `threading_java.rst` — done, see above
- [x] `customizers_java.rst` — done, see above
- [x] `tooling_java.rst` — done, see above
- [x] `limitations_java.rst` — done, see above

### Group 4 — Reference chapters

- [ ] `api.rst`
- [ ] `dbapi2.rst` (note: distinct from `dbapi2_py.rst` above — check
      what the split between these two actually is; if unclear, that's
      itself a finding)
- [ ] `imports.rst`
- [x] `spi.rst` — done, see above
- [ ] `android.rst`
- [ ] `develguide.rst`
- [x] `caller_sensitive.rst` — done as a side effect of `debugging_py.rst`,
      see above

## Verification

- Sphinx build (`-W`) stays clean after every file/group, per the command
  above.
- Every code sample in a touched file either runs as shown or is clearly
  marked as illustrative/partial (e.g. `# ...`) rather than presented as
  complete runnable code that silently doesn't work.
- No new cross-reference (`:doc:`/`:ref:`) breakage — the existing
  anchor-collision discipline from `plan/archive/DOCS.md` still applies.
- Commit per logical group, matching the pattern already set by the three
  commits this pass produced.
