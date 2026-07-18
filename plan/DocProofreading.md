# Systematic proofreading pass over doc/*.rst

## Status (2026-07-17): in progress — 8 of 36 files verified, plan below covers the rest

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

## Systematic visit order for the rest

Grouped to keep review context loaded efficiently — visiting paired
`_py.rst`/`_java.rst` chapters back-to-back matters because the two
should describe the *same* underlying mechanism from opposite directions,
so inconsistencies between them are a specific thing to check for that a
solo read of either file won't surface.

### Group 1 — Getting started (read first; first thing a new user sees)

- [ ] `install.rst`
- [ ] `intro.rst` — was heavily rewritten in an earlier session (480-line
      diff) but not reverified this session; worth a real reread now that
      the rest of the doc set has shifted under it, not just a diff-trust.

### Group 2 — Python-calling-Java chapters not yet covered

In `index.rst` toctree order:

- [ ] `quickguide_py.rst` — likely the single highest-traffic page;
      cross-check its code samples especially hard (this is exactly the
      kind of file `proxies_py.rst` turned out to be).
- [ ] `jvm_py.rst`
- [ ] `tooling_py.rst`
- [ ] `dbapi2_py.rst`
- [ ] `debugging_py.rst`
- [ ] `awt_py.rst`
- [ ] `gui_py.rst`
- [ ] `limitations_py.rst` — per `plan/archive/DOCS.md`, written from
      verified state as of 2026-07-17; re-check it's still accurate
      (e.g. `python.re`/`python.queue` status may have moved — check
      `plan/Re.md`/`plan/Queue.md`) rather than assuming it's stale.

### Group 3 — Java-calling-Python (reverse bridge) chapters

In `index.rst` toctree order, checking each against its already-verified
or freshly-verified `_py.rst` sibling for consistency:

- [ ] `quickguide_java.rst` (pairs with `quickguide_py.rst` above)
- [ ] `types_java.rst` (pairs with verified `types_py.rst` — good chance
      to check the two actually describe symmetric behavior)
- [ ] `collections_java.rst` (pairs with verified `collections_py.rst`)
- [ ] `datetime_java.rst` (no `_py` sibling — cross-check against
      `jpype/protocol.py`'s `Instant`/SQL date-type customizers instead,
      similar to how `threading_py.rst`'s toPython catalog was verified)
- [ ] `jvm_java.rst` (pairs with `jvm_py.rst` above)
- [ ] `threading_java.rst` (pairs with verified `threading_py.rst`)
- [ ] `customizers_java.rst` (pairs with verified `customizers_py.rst` —
      was read once already this session as a cross-check for `spi.rst`
      and looked reasonable, but wasn't proofread line-by-line)
- [ ] `tooling_java.rst` (pairs with `tooling_py.rst` above)
- [ ] `limitations_java.rst` (pairs with `limitations_py.rst` above; per
      `plan/archive/DOCS.md` written from verified state 2026-07-17,
      same re-check caveat as `limitations_py.rst`)

### Group 4 — Reference chapters

- [ ] `api.rst`
- [ ] `dbapi2.rst` (note: distinct from `dbapi2_py.rst` above — check
      what the split between these two actually is; if unclear, that's
      itself a finding)
- [ ] `imports.rst`
- [x] `spi.rst` — done, see above
- [ ] `android.rst`
- [ ] `develguide.rst`
- [ ] `caller_sensitive.rst` — orphan page (not in any toctree), reached
      only via `:doc:` from `CHANGELOG.rst`; confirm it still has an
      `:orphan:` marker and isn't just silently unreachable.

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
