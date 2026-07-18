# Java module coverage cleanup: systematic pass

## Status (2026-07-18): SETUP DONE. Closed: python.exceptions (see [[plan/ExecCrashDebug.md]]), org.jpype.internal, org.jpype.script, python.lang (PyMapping views + real bug fixed; protocol interfaces incl. PyGenerator crash root-caused and fixed, see [[plan/GeneratorCastCrash.md]]; PyCallable.CallBuilder + kwargs-string bug fixed; PyMapping/PySequence/PySet/PyFrozenSet/PyTuple all raised to ~100%; PyMutableSet confirmed NOT deletable, see finding in this doc), org.jpype (61%→66%; SubInterpreter/most of SubInterpreterBuilder confirmed environment-gated — needs a real Python 3.12 native rebuild, out of scope here — everything else fixable under 3.10 closed out: Script/Interpreter/SpiResource now 100%), python.decimal (72%→100%; tiny package, only `PyDecimalWrapperService`'s getters were uncovered), python.collections (83%→95%; `PyOrderedDict.moveToEnd(key)`'s one-arg default confirmed unreachable through the real bridge — name-only dispatch always resolves to Python's own `move_to_end` first, see finding below). Remaining packages NOT STARTED: org.jpype.manager, org.jpype.pkg, python.pathlib, org.jpype.ref, python.io, org.jpype.proxy, python.datetime.

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
| python.exceptions | 100% classes reached | n/a | no — reverse-bridge only, all 50 exception classes are this suite's job | **DONE** |
| org.jpype.internal | 74% | 60% | unknown, check before assuming | **DONE** (3 classes left alone, rule 3) |
| org.jpype.script | ~90% | ~82% | unknown, check before assuming | **DONE** |
| python.lang | 77% | 61% | no — reverse-bridge only | in progress, protocol interfaces + PyGenerator crash fixed (see [[plan/GeneratorCastCrash.md]]) |
| org.jpype | 69% | 47% | partial (BootstrapLoader is a static launcher entry point, may be forward-bridge/native-launch only) | not started |
| python.decimal | 72% | n/a | no | **DONE** |
| org.jpype.manager | 85% | 75% | unknown, check before assuming | not started |
| python.collections | 83% | 100% | no | **DONE** |
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

- **org.jpype**: **DONE** (package total 61% → 66% instruction, 22 new tests
  across 3 new files + 2 extended existing files, 845/845 full suite).
  `SubInterpreter` (9%) and most of `SubInterpreterBuilder`'s `start()` path
  stay low — **environment-gated, not a missing-test gap**: this repo's
  native library is built against Python 3.10, and PEP 684 subinterpreters
  require 3.12+, so every `startOrSkip()`-guarded test in
  `SubInterpreterNGTest`/`SubInterpreterBuilderNGTest` self-skips (confirmed
  live: a python3.12 interpreter exists on this machine but the native
  `.so`/jar are built for 3.10, and switching would require a real
  `project/dev.mk` rebuild with `PYTHON=python3.12`, out of scope for a
  coverage-only pass). `BootstrapLoader` (native `loadLibrary` stub) is
  likewise left at 0% — nothing to unit-test in a JNI declaration.
  `WrapperService` is an SPI interface whose default methods are exercised
  by `python.io`'s real provider elsewhere.
  Concretely fixable under 3.10 and closed out this pass:
  - `SubInterpreterBuilder`: added direct tests for the five setter
    overloads (`setOutput(Writer)`, `setError(OutputStream/Writer)`,
    `setInput(InputStream/Reader)`) — plain field assignments, don't need a
    real subinterpreter launch to cover, only needed a test that *calls*
    them. 56% → 69%.
  - `Script` (`org/jpype/ScriptNGTest.java`, new): the two-arg constructor
    (explicit globals/locals) and `importModule(module, as)` were
    previously only reachable through subinterpreter tests (env-gated, see
    above) — turns out neither needs a subinterpreter at all, the main
    interpreter exercises the same code paths. 60% → **100%**.
  - `Interpreter` (`InterpreterStdioNGTest`, extended): added the
    `Writer`/`Reader` overloads of `setOutput`/`setError`/`setInput`
    (already had `OutputStream`/`InputStream` coverage). → **100%**.
  - `Launcher` (`org/jpype/LauncherNGTest.java`, new): only `parseVersion`
    (incl. the swallowed-`NumberFormatException` fallback branch) and a
    fresh-instance getter/`isPrepared()` check are pure/side-effect-free;
    the rest (detective-probe subprocess spawn, pip self-healing, on-disk
    property-file cache, native `System.load`) is bootstrap code that
    drives real subprocesses/network/filesystem and is already exercised
    once, implicitly, by every other test class's one-time interpreter
    startup — not a good target for isolated unit tests without dedicated
    subprocess-mocking infrastructure. 41% → 43%.
  - `SpiLoader` (`org/jpype/SpiLoaderNGTest.java`, new): reflection-invoked
    the private `readResource`'s not-found branch directly (only reachable
    otherwise through a real `ServiceLoader`-discovered provider), plus
    `listPyspiResources`'s empty-directory and file-protocol-sorted paths.
    The jar-protocol branch stays uncovered — only triggers when scanning
    resources actually packaged inside a jar on the classpath, which
    `mvn test`'s exploded-directory classpath never produces.
  - `SpiResource` (`org/jpype/SpiResourceNGTest.java`, new): pure
    `.pyspi`-header string parsing, no I/O — every
    `IllegalArgumentException` branch (missing separator, malformed
    header line, missing `interface:`, lazy-backend rejection, missing
    `module:`/`class:` for kind=class), comment/blank-line skipping, and
    the `kind` default. → **100%**.
- **python.decimal**: **DONE** (package total 72% → 100% instruction,
  848/848 full suite). Tiny package — only `PyDecimalWrapperService`'s
  three trivial getters (`getModuleNames`, `getVersion`, `getResources`)
  were uncovered; `Decimal`/`PyDecimal` were already at 100% and the
  real registration path is already exercised end-to-end by
  `PyDecimalNGTest`. New `PyDecimalWrapperServiceNGTest` calls the
  getters directly, no `PyTestHarness`/JVM bridge needed.
- **python.collections**: **DONE** (package total 83% → 95% instruction,
  852/852 full suite).
  - `PyDeque` (78% → 100%): `rotate()`'s default (`{ rotate(1); }`) was
    already exercised for the two-arg form but the zero-arg default was
    dead from the real bridge's perspective — see the `PyOrderedDict`
    finding below, same root cause. Added `RecordingDeque`, a plain
    (non-bridge) stub `implements PyDeque` — cheap here because `PyDeque`
    only extends `PyObject`+`Iterable`, so the stub is ~20 one-line
    `throw new UnsupportedOperationException()` overrides plus a real
    `rotate(int)` that records its argument — calling `.rotate()` on it
    directly executes the actual default bytecode and asserts it
    delegates to `rotate(1)`.
  - `PyCollectionsWrapperService` (43% → 100%): same trivial-getters
    pattern as `python.decimal`'s `PyDecimalWrapperServiceNGTest`.
  - `PyOrderedDict.moveToEnd(PyObject)` (the one-arg default) stays at
    0%/uncovered — **confirmed unreachable through any real-bridge
    test, not a missing-test gap.** Read `ProxyInstance.invoke`
    (`org/jpype/proxy/ProxyInstance.java`): every proxied call tries
    `hostInvoke` first, dispatching by *mangled method name only* (not
    full signature) — so a 1-arg `moveToEnd(key)` call reaches Python's
    real `OrderedDict.move_to_end(key)`, which has its own `last=True`
    default parameter and succeeds immediately. The `defaultHandler`
    fallback (which would run the Java interface's own `{ moveToEnd(key,
    true); }` bytecode) only fires when `hostInvoke` finds no matching
    Python attribute — which never happens here. This is the same
    name-only-dispatch hazard noted in
    [[jpype_datetime_spi_status]] (there it was a live bug in
    `PyDeque.rotate()`; here it's simply permanently-dead Java code, not
    a bug). Covering it would require a plain stub `implements
    PyOrderedDict`, but that interface pulls in the full
    `PyDict`/`PyMapping`/`java.util.Map` surface (10+ abstract methods
    beyond what `PyDeque` needed) — disproportionate boilerplate for 2
    lines / <2% of the package, so left undone (same
    cost/benefit call as `Launcher`'s bootstrap methods in `org.jpype`).
- **org.jpype.internal**: **DONE** (41%/26% baseline → 74%/60%
  instruction/branch). `Keywords` and `FunctionalAdapters` — plain,
  native-free static utility classes, got ordinary non-bridge NGTest
  classes (`KeywordsNGTest`, `FunctionalAdaptersNGTest`, no
  `PyTestHarness`/JVM bridge needed): `Keywords` 71/74 instructions,
  `FunctionalAdapters` fully covered including its anonymous `Iterator`
  and `MapEntryWithSet`. `Support` — package-private "used exclusively
  through JNI" per its own Javadoc, but every method is plain
  deterministic Java, so `SupportNGTest` calls them directly: array-
  reshaping helpers (`collectRectangular`/`unpack`/`assemble`, including a
  real collect-then-reassemble round trip through a 3D array), `order`
  (endianness per NIO buffer type), `getStackTrace`/enclosing-frame
  truncation, the `Runtime`/`MemoryMXBean` accessors, `getJarPath`. 14% to
  ~80% (369/462 instructions). `DynamicClassLoader` — new
  `DynamicClassLoaderNGTest` builds fresh, isolated instances (never
  touches the static `install()` singleton the real bootstrap/other tests
  depend on) and exercises `addPath`/`addPaths` (including the anonymous
  `SimpleFileVisitor`, now 30/32), `findClass`'s two branches (fallback
  via `Class.forName` when no resource is found vs. reading bytes and
  `defineClass`-ing when one is), `findResource`/`findResources`/
  `addResource`, and `scanJar`'s directory-entry synthesis against a
  hand-built jar missing them. 23% to ~75% (347/465 instructions).
  `Reflector0` (was 25%, now 8/12 = 67% — special-loaded via
  `META-INF/versions/0` but its one real method, `callMethod`, is only
  reached through `JPMethod::invokeCallerSensitive`
  (`native/common/jp_method.cpp`), taken exclusively for Java methods
  annotated `@jdk.internal.reflect.CallerSensitive` like `Class.forName` —
  added `CallerSensitiveNGTest` calling
  `JClass('java.lang.Class').forName(...)` from Python to exercise it;
  remaining gap is just the `InvocationTargetException` unwrap branch,
  not worth forcing).
  **Left alone, classification rule 3 (intentional/impractical, not a real
  gap)**: `NativeContext` (56%) — its own class Javadoc says outright "As
  the JPypeContext can't be tested directly from Java code, it will need
  to be kept light"; it has a private constructor reachable only from
  native code via the package-private `@Exported create()` factory, no
  public accessor exists anywhere to reach the live instance from a test
  even though one exists during every bridge test run, and its remaining
  gaps are boot/shutdown-lifecycle code (`shutdown()`'s JVM-exit sequence,
  `scanExistingJars`) not exercisable mid-suite without spinning up
  separate JVMs. `Signal` (process-global SIGINT/SIGTERM handler install,
  already gets incidental coverage from interpreter startup — installing
  real signal handlers from a dedicated test would affect the whole test
  JVM's signal disposition, too risky for the coverage gained).
  `NativeLauncherControl` (all `native` method declarations; JaCoCo can't
  instrument a native method body at all, so its near-0% is just the
  implicit default constructor, not a real gap — nothing to write a test
  for).
- **org.jpype.script**: **DONE.** `JPypeScriptEngineFactory` — every
  getter is plain deterministic Java with no interpreter dependency, so
  new `JPypeScriptEngineFactoryNGTest` (no bridge) covers all of them
  directly; now fully covered (134/134). `JPypeScriptEngine` — extended
  the existing bridge-based `JPypeScriptEngineNGTest` with
  `getFactory`/`createBindings`/`eval(Reader,...)`, a real runtime error
  (not just a syntax-error fallback) getting wrapped into
  `ScriptException`, `GLOBAL_SCOPE` vs. `ENGINE_SCOPE` precedence, the
  `PyJavaObject` unboxing round trip in `toNative`, both `invokeMethod`
  success/failure paths, and both `getInterface` overloads (including
  their `IllegalArgumentException` branches). One real test-writing gotcha
  hit along the way: a multi-line `class Foo:\n ...\nFoo()` string isn't
  expression-shaped, so `eval()` takes the statement-fallback path and
  returns `null`, not the instance — the class def and the instantiation
  need to be two separate `eval()` calls. 49% to ~89%
  (319/360 instructions).
- **python.exceptions**: **DONE.** Crash unblocked (see
  [[plan/ExecCrashDebug.md]] for the full root-cause writeup: bug 1
  `org.jpype.pkg.PackageManager` couldn't enumerate an application
  module's own package contents under `--module-path` launch, which left
  `_jpype._exc` empty; bug 2 `JPypeException::convertPythonToJava` used
  `JPPyObject::claim()` instead of `JPPyObject::accept()`, turning that
  into an unrecoverable native fail-fast instead of the graceful fallback
  already coded next to it). All 38 classes that were still at flat 0% (12
  others already covered incidentally: `PyArithmeticError`,
  `PyAttributeError`, `PyBaseException`, `PyException`, `PyIndexError`,
  `PyKeyError`, `PyLookupError`, `PySyntaxError`, `PySystemExit`,
  `PyTypeError`, `PyValueError`, `PyZeroDivisionError`) now have a test in
  new `PyExceptionCoverageNGTest` (`src/test/java/python/exceptions/`),
  one method per class, each raising the matching Python builtin via
  `context.exec("raise X(...)")` (or a natural trigger — `eval()` for
  `NameError`/`StopIteration`/the two `Unicode*Error` cases needing real
  codec failures — where that was just as simple) and catching the
  generated Java type. All 50 classes in the package now show nonzero
  instruction coverage; full suite 700/700 (662 + 38 new), 0 failures, 14
  skipped (same pre-existing skips), confirmed via `mvn -o verify
  -Dpython.executable=python3.10`.
  `python.lang.PyExceptionFactory`/`LOOKUP` (checked via grep across
  `native/common`, `native/python`, and all of `src/main/java` — zero
  callers outside its own file and its own test) is **dead code, not the
  real construction path** — don't extend `PyExceptionFactoryNGTest`
  thinking it wires anything up. The real mechanism is
  `jpype/_jbridge.py`'s `_pyexc_convert`/`_jpype._exc` dict (built by
  scanning the real `python.exceptions` package against `builtins.*` at
  startup) called from `JPypeException::convertPythonToJava`
  (`native/common/jp_exception.cpp`).
- **python.lang**: IN PROGRESS.
  - **The `PyMapping*` view family — DONE, and a real bug found.**
    `PyChainMap` (`python.collections`) is the one concrete type in the
    codebase that *doesn't* shadow `PyMapping`'s own `keySet()`/
    `values()`/`entrySet()` defaults the way `PyDict` does (`PyDict`
    builds its own `PyDictKeySet`/`PyDictValues`/`PyDictItems` instead) -
    so it's the real path to `PyMappingKeySet`/`PyMappingValues`/
    `PyMappingEntrySet(Iterator)`. Extended `PyChainMapNGTest` to exercise
    all three views (iteration, contains/containsAll, add/remove/clear,
    `entry.setValue()`, `toArray()`) and found a genuine production bug:
    all three views' `toArray()` used `new ArrayList<>(this).toArray()`,
    but `ArrayList`'s constructor itself calls `c.toArray()` -
    `StackOverflowError` on every call. `PySet.toArray()` already carries
    a comment flagging this exact trap; these three didn't. Fixed the
    same way `PySet` does (manual iteration into a sized array), see
    commit `48370d31`. All four classes now ~80-90% (was 0%).
  - **The bare protocol interfaces — `PyAbstractSet`, `PyContainer`,
    `PyIterable`, `PyGenerator`, `PyCollection`, `PySized`, `PyMutableSet`
    — turned out NOT to be simply "never exercised standalone" as first
    assumed.** Every JPype-owned concrete type (`PySet`, `PyList`, ...)
    shadows these protocol defaults with its own redeclaration, so
    `context.set(...)`/etc. never reaches them - that's the 0% baseline.
    But the *real*, intended path is JPype's structural duck-typing probe
    (`native/python/pyjp_probe.cpp`, exposed as
    `_jpype.pyobject(target_type, python_object)`): a genuinely custom
    Python class/object (not one of JPype's own concrete types) gets
    matched against `collections.abc` protocols and produces a Java proxy
    implementing the matching protocol interface(s) directly - confirmed
    empirically, moved real coverage on `PyAbstractSet`/`PyCollection`
    (which drags `PySized` along for free, since `PyCollection` doesn't
    override `size()`/`isEmpty()`). **`PyMutableSet` is NOT actually
    deletable, correcting an earlier finding in this doc.** It has zero
    implementors and no `protocol_pipeline` entry, so the structural probe
    can never select it (confirmed, see [[plan/GeneratorCastCrash.md]] for
    the full pipeline-name list) - but `jpype/_jbridge.py` still loads it
    eagerly at bootstrap (`JClass("python.lang.PyMutableSet")`) and
    registers it in `_jpype._protocol["mutable_set"]` and
    `_jpype._methods`. Verified by actually deleting the file: every
    `PyTestHarness`-based test failed at `setUpClass` with
    `PyBuiltIn.getContext()` NPE (bootstrap itself broke), not a
    class-loading error scoped to `PyMutableSet` alone. Reverted
    immediately (`git checkout`), suite back to 775/775. So it's dead
    *as reachable API surface* but not dead *as a referenced class* -
    deleting it would require also removing its `_jbridge.py` bootstrap
    registration, which is out of scope for a coverage pass and not worth
    doing for an interface with an empty method table and no callers.
    Leaving it in place, unlisted as a deletion candidate. **`PyGenerator`
    found a real
    crash** doing this - casting a genuine Python generator object to
    `PyGenerator` and calling `.iterator()` on the result segfaulted the
    JVM deterministically. **DONE - root-caused and fixed**: two
    independent bugs (an unrelated-interface method-name collision
    between `PyIterable` and `PyIter`/`PyGenerator` in
    `pyjp_probe.cpp`'s `interrogate()`, plus a real infinite-recursion
    logic bug in `PyGenerator.iterator()`'s own default). Full
    root-cause writeup: [[plan/GeneratorCastCrash.md]]. Real committed
    tests for all five live interfaces (`PyAbstractSet`/`PyContainer`/
    `PyIterable`/`PyCollection`/`PyGenerator`) now live in
    `python.lang.ProtocolInterfaceCoverageNGTest`; 775/775 full suite.
  - **`PyCallable.CallBuilder`/`CallBuilderEntry` — DONE.** Added
    `python.lang.PyCallBuilderNGTest` (10 tests: `arg`/`args`/`kwarg`/
    `kwargs`/`clear`/chaining, `execute`/`executeAsync`/
    `executeAsync(timeout)`, `CallBuilderEntry`'s immutability). Found a
    real production bug in the process: `CallBuilder.kwarg(CharSequence,
    Object)` stored the raw Java `CharSequence` as the dict key, so any
    call using `.kwarg(...)`/`.kwargs(...)` (with real keyword arguments,
    not just positional) raised `PyTypeError: keywords must be strings`
    once `execute()` tried to use the resulting dict as `**kwargs` -
    verified via a debug test that a Java `String` crossing the bridge
    unconverted stays a distinct Java-wrapped object (matched by
    `__eq__`/`__hash__`, but not `isinstance(x, str)`), which CPython's
    keyword-argument machinery rejects outright. Fixed by converting the
    key through `PyBuiltIn.str(...)` before storing it (confirmed via the
    same debug test that this really does yield a genuine Python `str`).
    Both classes now 100% coverage (was 0%). 785/785 full suite (up from
    775).
  - **Partially-covered core types — DONE.** `PyMapping` and `PySequence`
    were both stuck low for the same reason as the earlier protocol
    interfaces: `PyDict`/`PyList`/`PyTuple` shadow all their interesting
    defaults, so the structural probe (`_jpype.pyobject`, both `"mapping"`
    and `"sequence"` are live `protocol_pipeline` names) is again the real
    path in - `python.lang.PyMappingNGTest` (14 tests, custom class
    actually subclassing `collections.abc.Mapping` for the `keys()`/
    `values()`/`items()`/`get()` mixins, not just `.register()`-ed, plus
    hand-written `__setitem__`/`__delitem__`/`clear` so `put`/`putAny`/
    `remove`/`clear`/`putAll` are reachable too) and
    `python.lang.PySequenceNGTest` (8 tests, custom `collections.abc.Sequence`
    class) now cover them: `PyMapping` 39% -> 100%, `PySequence` 38% ->
    100%. `PySet`/`PyFrozenSet`/`PyTuple` are concrete wrapper types (not
    probe-reachable - not in the 15-name pipeline), so these just needed
    direct tests for the specific untested defaults: `PySetNGTest` gained
    `containsAll`/`addAll`/`toArray`/`toArray(T[])`/`stream`/
    `parallelStream` (43% -> 98%, remaining gap is defensive code jacoco
    can't fully branch-cover); `PyFrozenSetNGTest` gained
    `toArray(T[])` (46% -> 100%); `PyTupleNGTest` gained the immutability
    throws for `addAll`/`addAll(int,...)`/`clear`, plus `containsAll`/
    `listIterator()`/`listIterator(int)` (valid and out-of-bounds)/
    `parallelStream` (56% -> 100%). 822/822 full suite (up from 785).

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
