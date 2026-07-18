# Java module coverage cleanup: systematic pass

## Status (2026-07-18): SETUP DONE, PASS NOT STARTED

`jacoco-maven-plugin` (0.8.14, offline-resolvable from `~/.m2` except one
missing transitive jar — `plexus-utils:1.1`, fetched once online, now
cached) wired into `native/jpype_module/pom.xml`: `prepare-agent` execution
plus `report` bound to the `verify` phase. Had to prepend `@{argLine}` to
the existing static surefire `<argLine>` (module-path/add-modules/native-
access flags) so the JaCoCo agent's injected argLine and the hand-written
one both survive — same pattern already used in the sibling `~/javafx2/j2ni`
project's pom.

Verified for real, not just "it compiles": `mvn -o verify
-Dpython.executable=python3.10` — 662/662 tests, 0 failures, 14 skipped,
report generated at `native/jpype_module/target/site/jacoco/index.html`
(`jacoco.xml`/`jacoco.csv` alongside it for scripted analysis). The
`-Dpython.executable=python3.12` run on this checkout currently fails
setup (`Interpreter.getBuiltIn()` returns null, plus a stale `_jpyne.so`
path) — confirmed via `git stash` that this is a **pre-existing
environment issue**, not caused by this change; use python3.10 for this
work until that's separately diagnosed.

Baseline overall: **49% instruction, 40% branch** across the whole module.

## Two coverage sources — this plan only wires/covers one of them

Two independent JaCoCo collection paths exist, instrumenting disjoint code:

1. **`mvn verify` in `native/jpype_module`** (this plan's scope) — the
   TestNG suite, almost entirely **reverse-bridge** (Java calling into
   Python via `python.*`). Newly wired up this session.
2. **`coverage.sh` / `pytest --jacoco`** (`test/jpypetest/conftest.py`) —
   starts a real JVM from Python and attaches the JaCoCo agent with
   `includes=org.jpype.*`, i.e. the **forward-bridge** path (Python calling
   into Java). This is why `org.jpype.javadoc`/`org.jpype.html` (Javadoc-to-
   docstring extraction, `jpype/_jclass.py:169`, exercised by
   `test_javadoc.py`) and `org.jpype.pickle` (`test_pickle.py`) show 0% in
   the `mvn`-only report below despite being real, tested code — **not this
   plan's problem**, don't "fix" them without checking the Python-side
   report first.

Its `includes=org.jpype.*` filter doesn't reach `python.*`, so the two
sources are close to complementary but not proven so class-by-class. A
`jacoco:merge` for one unified report is possible later but not worth
building until the `mvn` report alone leaves unexplained gaps.

## Baseline package table (mvn report, 2026-07-18, python3.10)

| Package | Instr. | Branch | Known Python-side coverage? | Pass status |
|---|---|---|---|---|
| org.jpype.html | 0% | 0% | yes — forward-bridge (`test_javadoc.py`, via `org.jpype.javadoc`) | not started |
| org.jpype.javadoc | 0% | 0% | yes — forward-bridge (`test_javadoc.py`) | not started |
| org.jpype.pickle | 0% | 0% | yes — forward-bridge (`test_pickle.py`) | not started |
| python.exceptions | 27% | n/a | no — reverse-bridge only, all 50 exception classes are this suite's job | not started |
| org.jpype.internal | 41% | 26% | unknown, check before assuming | not started |
| org.jpype.script | 52% | 47% | unknown, check before assuming | not started |
| python.lang | 64% | 49% | no — reverse-bridge only | not started |
| org.jpype | 69% | 47% | partial (BootstrapLoader is a static launcher entry point, may be forward-bridge/native-launch only) | not started |
| python.decimal | 72% | n/a | no | not started |
| org.jpype.manager | 85% | 75% | unknown, check before assuming | not started |
| python.collections | 83% | 100% | no | not started |
| org.jpype.pkg | 85% | 78% | unknown, check before assuming | not started |
| python.pathlib | 81% | n/a | no | not started |
| org.jpype.ref | 88% | 67% | unknown, check before assuming | not started |
| python.io | 87% | 74% | no | not started |
| org.jpype.proxy | 90% | 76% | unknown, check before assuming | not started |
| python.datetime | 93% | 100% | no | not started |

(Instr% column only — regenerate the full table from
`target/site/jacoco/jacoco.csv` at the start of each package's pass since
numbers will move as tests get added.)

## Concrete known low spots inside the worse packages (mvn report only)

Pulled from `jacoco.csv`, classes under 60% instruction coverage — the
starting worklist once a package is picked:

- **org.jpype**: `SubInterpreter` (10%), `SubInterpreterBuilder` (56%),
  `Script` (60%), `BootstrapLoader`/`WrapperService` (0%, likely
  entry-point/SPI-declaration classes — check reachability before assuming
  testable).
- **org.jpype.internal**: `Support` (14%), `DynamicClassLoader` (23%,
  including a 0% anonymous `SimpleFileVisitor`), `Reflector0` (25%),
  `Keywords` (39%), `NativeContext` (56%), `FunctionalAdapters` (0%,
  including a 0% anonymous `Iterator`).
- **org.jpype.script**: `JPypeScriptEngine` (49%), `JPypeScriptEngineFactory`
  (57%).
- **python.exceptions**: **BLOCKED on a real, reproducible native crash —
  do not add tests here yet, see below.** 37 of 49 concrete exception
  classes sit at flat 0% per `jacoco.csv` (12 already covered incidentally:
  `PyArithmeticError`, `PyAttributeError`, `PyBaseException`, `PyException`,
  `PyIndexError`, `PyKeyError`, `PyLookupError`, `PySyntaxError`,
  `PySystemExit`, `PyTypeError`, `PyValueError`, `PyZeroDivisionError`).
  `python.lang.PyExceptionFactory`/`LOOKUP` (checked via grep across
  `native/common`, `native/python`, and all of `src/main/java` — zero
  callers outside its own file and its own test) is **dead code, not the
  real construction path** — don't extend `PyExceptionFactoryNGTest`
  thinking it wires anything up. The real mechanism is
  `jpype/_jbridge.py`'s `_pyexc_convert`/`_jpype._exc` dict (built by
  scanning the real `python.exceptions` package against `builtins.*` at
  startup) called from `JPypeException::convertPythonToJava`
  (`native/common/jp_exception.cpp`).
  **Real bug found while probing this path**: any Python exception raised
  during a reverse-bridge call whose Java-side return type is `void` (e.g.
  `Script.exec(String)`, which maps to `Backend.exec` — a void method)
  crashes the JVM with `SIGSEGV` in
  `JPypeException::toJava(JPJavaFrame&) [clone .cold]`
  (`native/common/jp_exception.cpp`), landing in the deliberate
  fail-fast `catch (JPypeException&)` block (`int *i = nullptr; *i = 0;`)
  that's meant only for "exception handling itself failed" — meaning
  something inside `convertPythonToJava` is itself throwing a second
  `JPypeException` on this path, not just tripping an already-intentional
  safety net. Reproduced deterministically 3/3 times with three different
  triggers (`context.exec("raise AssertionError('test')")`,
  `context.exec("raise ValueError('test')")`,
  `context.exec("int('not a number')")` — i.e. not tied to one exception
  type or to explicit `raise` vs. an organically-raised error).
  `Script.eval(...)` (non-void return) raising is fine — that's the path
  `PyExcNGTest` already exercises successfully for `PyZeroDivisionError`/
  `PyValueError`, which is exactly why this gap wasn't caught earlier: every
  existing exception-path test happens to go through `eval`, none through
  `exec`. A separate, most-likely-unrelated flaky crash was also observed
  once at the same `toJava` frame from `JPypeScriptEngineNGTest
  .testBindingsRoundTrip` (a non-raising `eval` call) but did not reproduce
  on a clean rebuild — not yet understood, noted here so it isn't lost, not
  chased further this session.
  **Until this native bug is root-caused and fixed, don't write more
  `python.exceptions` tests that go through `exec()` on a raising
  statement** — it will crash the whole Maven test JVM, not just fail one
  test. The 12 already-covered classes show `eval()`-based tests are safe;
  a parametrized test over the remaining 37 via `eval()` (e.g. the
  `(_ for _ in ()).throw(X('test'))` idiom, or picking one genuinely
  eval-shaped trigger per exception like `PyExcNGTest` already does for
  `ZeroDivisionError`/`ValueError`) is the fallback if the native fix turns
  out to be a separate undertaking.
- **python.lang**: several 0% classes that look like real, deliberate
  abstractions never exercised standalone — `PyAbstractSet`, `PyContainer`,
  `PyGenerator`, `PyIterable`, `PyMutableSet`, `PySized`, and the dict-view
  family `PyMappingEntrySet(Iterator)`/`PyMappingKeySet`/`PyMappingValues`
  (0%, sizable — 98–124 instructions each). Also `PyCallable.CallBuilder`/
  `CallBuilderEntry` (0%, 119+20 instructions) — a builder-pattern API
  surface with no test touching it at all. `PyMapping` (39%), `PySequence`
  (38%), `PySet` (43%), `PyFrozenSet` (46%), `PyTuple` (56%) are partially
  covered core collection types worth a closer look.

## Classification rules for each gap (apply per class/method, not per package)

When a low/0% spot is found, resolve it into exactly one of:

1. **Reachable, just untested** → add an NGTest case exercising the real
   path (prefer extending an existing NGTest class for the type over a new
   file, unless the class has none yet).
2. **Genuinely unreachable given current call-lifetime invariants** →
   candidate for deletion. Before cutting anything, re-verify the invariant
   by reading real call sites (see [[jpype_feedback_trust_invariants]] —
   this repo has already been burned by both under- and over-trusting
   invariants without checking).
3. **Intentionally low/zero coverage by design, not a gap** → leave alone,
   note why inline (or confirm an existing comment already explains it).
   Known examples from this session/history, don't re-litigate these:
   - `_PyJPModule_fault_code` / fault-injection scaffolding
     ([[jpype_fault_injection_global_by_design]]) — test-only, deliberately
     global, not expected to show normal-path coverage.
   - `_PyJPModule_trace` — documented as intentionally shared/debug-only
     (per `TODO.md`'s MultiPhaseInit C.2 note).
   - The exception-path "grade 10 asbestos" defensive `try/catch` in
     `JPypeException::convertJavaToPython` around the smuggled-exception
     unpack — trigger condition explicitly flagged as near-impossible to
     exercise on purpose (TODO.md, 2026-07-11 edge case entry); a native
     C++ path anyway, not part of this Java-side pass, but the same
     "don't force a test that requires simulating interpreter corruption"
     reasoning likely applies to any Java-side counterpart found here too.
4. **Covered by the *other* coverage source (Python-side forward-bridge
   pytest+jacoco run)**, not actually a gap in the combined picture → note
   it in the table above and move on; don't add a redundant reverse-bridge
   test just to make the `mvn`-only number look better.

## Process

1. Regenerate the `mvn` report (`mvn -o verify -Dpython.executable=python3.10`
   in `native/jpype_module`) at the start of a session before trusting any
   number in this file — it will have moved.
2. Pick a package (order doesn't matter — user confirmed). Work its 0%/low%
   classes one at a time through the classification rules above.
3. For "reachable, just untested": write the test, rerun `mvn verify`,
   confirm the number moved and the full suite is still green (662/662 today
   — update this baseline as tests are added).
4. For "genuinely unreachable": confirm via a real read of call sites (not
   assumption), then delete, in its own small commit separate from any
   test-adding commits, with a short note of what was removed and why.
5. Update this file's table + worklist as packages are closed out, same as
   every other `plan/` doc's status-line convention. Move to `plan/archive/`
   once every package row is resolved (either raised, or explicitly
   justified as intentional/covered-elsewhere).

## Verification bar

Same as every other closed plan/ item in this repo: full suite green
(`mvn -o verify -Dpython.executable=python3.10`, and python3.12 once its
pre-existing environment issue is separately fixed), no new skips beyond
the 14 pre-existing ones, and every classification decision backed by an
actual read of the code/call sites rather than a guess from the package
name alone.
