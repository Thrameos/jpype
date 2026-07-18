# Python 3.12 support, subinterpreter test completion, and test-quality audit

## Status (2026-07-18): Phase 1 DONE, Phase 2 mostly already done, Phase 3 not started

**Phase 1 root cause found and fixed**: the `mvn verify -Dpython.executable=python3.12`
`NullPointerException` was a plain stale-build issue, not a source regression.
`build/cp312-cp312-linux_x86_64/_jpype.so` was compiled before the last
edits to `jp_exception.cpp`, `pyjp_probe.cpp`, and `jp_pythontypes.cpp` (those
three files were newer than the `.so`), and `~/.jpype/jpype.properties` had
cached the stale probe result on top of that. Rebuilt via
`make -f project/dev.mk PYTHON=python3.12 all` (per
[[jpype_feedback_use_devmk_not_adhoc_ninja]], not ad hoc ninja) and deleted
`~/.jpype/jpype.properties`. Both `mvn -o verify -Dpython.executable=python3.12`
and `...=python3.10` now reach `BUILD SUCCESS`, 932/932, 0 failures/errors.
[[jpype_build_env_gotchas]]'s line 28 claim was actually correct in spirit —
just describing a state that had since gone stale from later edits — not a
real regression. Corrected memory accordingly.

**Phase 2 steps 1-3 turn out to already be done**, discovered while verifying
this plan's own "current state" claims rather than trusting them: under a
fresh python3.12 build, `SubInterpreterNGTest` runs 8/8 passing with 0 skips
(not skipping at all), and `SubInterpreterBuilderNGTest` runs 8/8 passing
with 0 skips (an earlier stale surefire `.txt` report, from an 11:56 run
before the rebuild, had shown 6/8 skipped — the fresh 13:36 junitreports XML
supersedes it). `NativeLauncherControl.isGilHeld()`
(`native/common/jp_bridge.cpp:598`) already has the `_PyThreadState_GetCurrent()
!= nullptr` fix in place, landed in commit `e5a93a46` ("Multi-phase init,
python.io completion, cross-interpreter safety, and dispatch mangling") —
well before this plan was scoped. [[jpype_subinterpreter_difficulty]]'s
"isGilHeld() false-positive remains open" note is now stale; only step 4
(coverage gap) remains open, tracked below.

Follows on from [[plan/archive/Coverage.md]] (the reverse-bridge coverage
pass, now fully closed). That pass ran exclusively under
`-Dpython.executable=python3.10` — every python3.12-specific gap it found
(`org.jpype`'s `SubInterpreter`/`SubInterpreterBuilder` coverage,
`isGilHeld()`'s known false positive) was explicitly carved out as
environment-gated rather than fixed. This plan picks those up, plus a
third, independent thread: auditing the test suite itself (including
tests added during the coverage pass) for cases that assert nothing real.

Three largely-independent phases; do them in order, since phase 2 needs a
working python3.12 build (phase 1) to run at all, and phase 3 benefits
from a settled, green baseline on both interpreters before triaging.

## Phase 1: get `-Dpython.executable=python3.12` green again

**Current state, verified fresh this session, not assumed from memory:**
`mvn -o verify -Dpython.executable=python3.12` fails at test setup —
1147 run, 75 failures, all `PyTestHarness.setUpClass` throwing
`NullPointerException: Cannot invoke
"python.lang.PyBuiltIn.getContext()" because the return value of
"org.jpype.Interpreter.getBuiltIn()" is null`. `python3.12 -c "import
_jpype"` works fine standalone (plain Python import, `_jpype.isStarted()`
returns `False` as expected) — the failure is specific to the
Maven-launched embedded-JVM-hosts-Python path, not a missing/broken
`.so`.

This directly contradicts [[jpype_build_env_gotchas]]'s note that "mvn
tests against python3.12 work correctly end-to-end ... confirmed via
SubInterpreterNGTest, 483/483 both python3.10 and python3.12" — that
memory is now 7+ days stale and the regression (or environment drift)
needs to be root-caused fresh, not assumed fixed. `~/.jpype/jpype.properties`
already has a plausible-looking, non-`NOT_FOUND` python3.12 entry, so
this is probably not the stale-cache issue documented in the same memory
— check that first (quick, cheap to rule out) but don't stop there if it
doesn't explain it.

Leads worth checking, in rough cheapest-first order:
- Delete `~/.jpype/jpype.properties` (or `-Djpype.nocache=true`) and
  re-run, to rule out a subtler stale-cache issue than the obvious
  `NOT_FOUND` case the memory describes.
- [[jpype_feedback_use_devmk_not_adhoc_ninja]]: a stale native `.so`
  vs. jar mismatch caused a multi-hour phantom-crash chase before —
  rebuild via `project/dev.mk` or `mvn package`, not ad hoc
  `ninja`/`pip install`, and confirm the jar's build timestamp is newer
  than the `.so`'s.
  `.so` build dir is `build/cp312-cp312-linux_x86_64/` (per
  [[jpype_build_env_gotchas]]'s per-version build-dir fix) — confirm
  it's actually current, not left over from whenever that memory's
  483/483 run happened.
- If both check out, this may be a real regression introduced by any of
  the source changes made since that memory was written (multiple
  sessions' worth: object-model migration, name-mangling, SPI installer,
  datetime/pathlib/decimal wrapper services, the `TypeManager` leak fix
  from the coverage pass, etc.) — `git bisect` against the python3.12
  build specifically may be needed if the cheap checks don't explain it.
- `Interpreter.getBuiltIn()` returning null implies the interpreter
  startup sequence itself didn't run far enough to populate the builtin
  namespace — get a stack trace / add logging at the actual native
  launch point (`Launcher`/`MainInterpreter`, per
  [[jpype_final_mission_status]]'s Launcher/MainInterpreter split) rather
  than only observing the downstream null.

**Decision made 2026-07-18**: user chose to leave the pom.xml default as
python3.12 (no change) — both versions stay supported, python3.10 remains
an explicit `-Dpython.executable=python3.10` override as it has been.

## Phase 2: finish subinterpreter testing — DONE 2026-07-18

Step 4 (the only remaining item, see Status above) closed out:
`SubInterpreterBuilder` went from instr 86%/branch 69% to **100%/100%**
(added `testStartWiresWriterOutputErrorAndReaderInput` and
`testStartWiresOutputStreamErrorAndInputStreamInput` in
`SubInterpreterBuilderNGTest`, covering the Writer/Reader and
OutputStream/InputStream wiring branches of `start()` for `err`/`in` that
only `out` had real coverage for before). `SubInterpreter` went from instr
79%/branch 57% to instr 3-missed/branch 1-missed (added
`testGetBackendAndInstallerReturnRealInstances`,
`testSetBackendAfterStartThrows`, `testSetInstallerAfterStartThrows`,
`testStartAfterCloseThrowsIllegalState`, `testCloseAfterCloseThrowsIllegalState`,
`testStartWhileAlreadyStartedIsIdempotentNoOp` in `SubInterpreterNGTest`).

**One deliberate gap left uncovered**: `SubInterpreter.start()`'s
`isGilHeld()`-true branch (`SubInterpreter.java:168-170`, the
`LOGGER.severe("GIL still held ... leaked GIL acquire")` line) is a
defensive log for a genuine native-level GIL leak. Per
[[jpype_feedback_trust_invariants]], not fabricating a fake leaked-GIL state
just to paint this line green — the call-lifetime invariant (a correctly
functioning `startSubInterpreter` never leaves the GIL held) already rules
it out by construction, and forcing it would mean corrupting real native
state to hit a log line.

940/940 tests pass under python3.12, all existing tests still pass under
python3.10 (new subinterpreter tests self-skip there via `startOrSkip()`'s
existing `SkipException` pattern).

Original phase 2 scoping, largely superseded by the above (kept for
context on how steps 1-3 were verified before being found already-done):

Prerequisite: Phase 1 green, since `SubInterpreterNGTest`/
`SubInterpreterBuilderNGTest` self-skip via `SkipException` on
pre-3.12 builds and are otherwise untested by this repo's own CI-equivalent
(`mvn -o verify -Dpython.executable=python3.10`).

Per [[jpype_subinterpreter_difficulty]] (last real update 2026-07-10):
subinterpreter *mechanics* are confirmed correct — start/register-classes/
eval/close lifecycle, two subinterpreters plus the main interpreter running
concurrently/interleaved, no cross-talk, no deadlock (proven via
`testConcurrentThreadDoesNotBlockAfterSubEval`, not just inferred). The
one known remaining defect is `NativeLauncherControl.isGilHeld()`
reporting a false "still held" after subinterpreter start/eval, because
`PyGILState_Check()`'s TLS bookkeeping is never populated by the
subinterpreter's manual `PyThreadState_Swap` path — a diagnostic bug, not
a functional one, but it fails `SubInterpreterNGTest`'s `@AfterMethod`
assertion every time.

Verify this is all still accurate against current code before acting on
it — re-read `native/common/jp_bridge.cpp`'s `isGilHeld()` and
`native/python/jp_pythontypes.cpp`'s `JPPyCallAcquire` subinterpreter
branch fresh, don't assume the memory's line numbers/behavior still hold.

Steps:
1. Confirm `SubInterpreterNGTest`/`SubInterpreterBuilderNGTest` actually
   run (not skip) once Phase 1 is green, and see current pass/fail state.
2. Fix `isGilHeld()`'s diagnostic false positive — the memory's own
   suggested direction is checking `_PyThreadState_GetCurrent() !=
   nullptr` instead of (or in addition to) `PyGILState_Check()`, but
   re-derive/confirm rather than blindly applying a 8-day-old suggestion.
3. Once the diagnostic is trustworthy, re-run the existing stress tests
   (`testMixAndMatchNoCrossTalkNoHangs`,
   `testConcurrentThreadDoesNotBlockAfterSubEval`) and confirm they now
   pass end-to-end including the `@AfterMethod` check, not just their own
   in-body assertions.
4. Revisit `org.jpype`'s coverage gap from the retired coverage plan —
   `SubInterpreter`/most of `SubInterpreterBuilder` were confirmed
   environment-gated there specifically because this didn't work under
   3.10. With 3.12 green, treat this package's coverage the same way the
   coverage plan treated every other package: jacoco baseline, uncovered-
   line inspection, targeted tests. Run the full jacoco pass under
   python3.12 (not 3.10) for this one package, since real code only
   executes there.
5. Decide whether the `isGilHeld()` false positive also affects anything
   outside `SubInterpreterNGTest` (any other test/assertion that calls it
   as a "did we leak" check) and fix those callers' expectations too if so.

## Phase 3: test-quality audit — remove meaningless tests

**The pattern to find**: a test that implements an interface (often with
every unused method throwing `UnsupportedOperationException`, a pattern
this repo's own tests use legitimately for isolating one default method —
see `PyDequeNGTest.RecordingDeque`, `PyIOBaseNGTest.RecordingIOBase` from
the coverage pass) but whose actual `@Test` body doesn't assert anything
that would fail if the production code were broken — e.g. it calls a
method and only checks "didn't throw," or asserts a value trivially
implied by how the test itself constructed its input, rather than
verifying real behavior of the code under test.

This is explicitly **not** a blanket objection to stub-based tests — the
coverage pass added a dozen files using exactly this shape
(`grep -rl UnsupportedOperationException native/jpype_module/src/test`
currently finds 12), and most of them are legitimate (they isolate a real
default-method body and assert its actual computed output, e.g.
`RecordingDeque.rotate()`'s no-arg call really does delegate to
`rotate(1)`, verified by checking the recorded argument). The audit needs
to distinguish "stub used to isolate one real assertion" from "stub used
to pad a coverage number with no real assertion" — and should
specifically **include the tests this session itself just wrote**, not
exempt them by authorship.

Steps:
1. Enumerate every `@Test` method across `native/jpype_module/src/test`
   (roughly 930 at last count) — this is too large to eyeball by hand, so
   script a first pass: flag methods whose body has no `assert*` call at
   all (trivially meaningless), then separately flag methods whose only
   assertions are on values the test itself hard-coded into its own stub
   (e.g. "stub returns 42, assert result is 42" with no path through real
   production logic) as candidates for closer manual review.
2. For each flagged candidate, read the production code path it's meant
   to exercise and judge: does this assertion fail if that code were
   subtly broken (wrong branch taken, off-by-one, swapped argument
   order)? If not, it's a meaningless test — remove it, or fix the
   assertion to actually pin down real behavior if the code path is worth
   keeping covered at all.
3. Don't touch coverage numbers as the success metric for this phase —
   removing a meaningless test that happened to hit a line is a net
   improvement even if jacoco's percentage drops; that's the whole point.
   If a genuinely important line loses coverage as a side effect, that's
   a signal the line needs a *real* test, not that the meaningless one
   should stay.
4. Cross-reference against [[jpype_feedback_avoid_dict_capsule_caching]]-
   style prior "we built X, benchmarked/tested it, and reverted" findings
   in this repo's plan/archive — some existing tests may be pinning down
   design decisions (e.g. `GlobalPool`'s deliberate no-generation-guard
   behavior) that read as "just calls a method" but actually encode an
   important invariant in a comment; read the comment before deleting
   anything that looks trivial at a glance.

## Verification

- Phase 1: `mvn -o verify -Dpython.executable=python3.12` reaches
  `BUILD SUCCESS` with 0 failures/errors, same test count (modulo
  subinterpreter tests un-skipping) as the python3.10 run.
- Phase 2: `SubInterpreterNGTest`/`SubInterpreterBuilderNGTest` pass
  cleanly under python3.12 including `@AfterMethod`, repeated runs (not
  just once) given this repo's history of intermittent subinterpreter
  races.
- Phase 3: full suite still green after removals; net test count may
  drop — that's expected and fine, call it out explicitly in the summary
  rather than treating a lower test count as a regression.
- All three phases: both python3.10 and python3.12 full-suite runs green
  at the end, matching this repo's established "verify for real, not just
  compiles" bar from the coverage pass.
</content>
