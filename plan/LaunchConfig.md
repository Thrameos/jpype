# Java→Python launch: parameter resolution and caching

## Status (2026-07-17): DOCUMENTED, not yet acted on

This is a reference/audit document, not a feature plan. It captures how
`org.jpype.MainInterpreter` currently resolves the parameters used to
launch the embedded Python interpreter — properties, environment
variables, the probe/cache subsystem — none of which was previously
written down anywhere in `plan/` or `doc/`. Two real bugs and a handful of
untested/undocumented surfaces turned up while mapping this; they're
recorded here as candidate follow-up work, not fixed yet. See
`plan/PythonCLI.md` for the follow-on feature (full CLI support from
`MainInterpreter`) that motivated this audit.

## Where this fits

This is the reverse-bridge counterpart to `jpype.startJVM()`'s parameter
handling (forward bridge, Python launching a JVM). The two are unrelated
mechanisms that happen to share vocabulary — `jpype.addJVMOption()`/
`getJVMOptions()` (`jpype/_core.py`) configure **JVM** startup; everything
below configures **Python** startup from the Java side. Don't conflate
them when reading or writing docs — see finding 6 below, this has already
caused doc-review confusion once.

## The pipeline

```
MainInterpreter.getInstance().start(args...)
  └─ prepare() → resolveLibraries()        # gather config, hit/populate cache, probe
  └─ installNatives()                       # System.load() the native libs
  └─ NativeLauncherControl.startMain(...)   # JNI call, all resolved config passed down
       └─ jp_bridge.cpp: Java_org_jpype_internal_NativeLauncherControl_startMain
            → build PyConfig, PyConfig_Read, Py_InitializeFromConfig
            → launch(env, interpreter): import jpype/_jpype,
              jpype._core.initializeResources() → _jbridge.py: initialize()
                 → bridge.setBackend(...) / bridge.setInstaller(...)
```

`org.jpype.SubInterpreter`/`SubInterpreterBuilder` are a **separate,
much smaller** config path (see "Subinterpreters" below) — they do not
go through any of the `python.config.*` machinery described here at all.

`_jbridge.py`'s `initialize()` receives **no** direct launch
configuration from the native side — it runs strictly after
`Py_InitializeFromConfig` has already completed and `jpype`/`_jpype` have
already been imported. All interpreter-launch configuration is consumed
entirely on the Java/C++ side before any Python code runs; `_jbridge.py`
only wires up the backend/installer object graph afterward.

## Parameters recognized (`MainInterpreter.java:76-100`)

`python.config.*` system properties, one per `PyConfig` field actually
threaded through `NativeLauncherControl.startMain`:

| Property | `PyConfig` field | Notes |
|---|---|---|
| `python.config.program_name` | `program_name` | defaults to the resolved executable |
| `python.config.home` | `home` | |
| `python.config.path` | *(not a PyConfig field directly)* | extra `sys.path` entries, see below |
| `python.config.prefix` | `prefix` | **dead — see Finding 1** |
| `python.config.exec_prefix` | `exec_prefix` | **dead — see Finding 1** |
| `python.config.executable` | `executable` | |
| `python.config.isolated` | *(selects `PyConfig_InitIsolatedConfig` vs `PyConfig_InitPythonConfig`)* | |
| `python.config.fault_handler` | `faulthandler` | |
| `python.config.quiet` | `quiet` | |
| `python.config.verbose` | `verbose` | |
| `python.config.site_import` | `site_import` | |
| `python.config.user_site_directory` | `user_site_directory` | |
| `python.config.write_bytecode` | `write_bytecode` | |

Plus non-`PyConfig.*`-prefixed properties: `python.module.path` (read
once, in the `MainInterpreter` constructor, `:528`), `python.executable`,
`python.lib`, and the `jpype.*` cache/probe/install properties (`jpype.lib`,
`jpype.arch`, `jpype.nocache`, `jpype.install`, `jpype.version`,
`jpype.properties` — the cache filename).

Environment variables:

- `PATH` — searched by `checkPath()` (`:539`) to find a `python`/
  `python.exe` on the system path.
- `PYTHONHOME` — read by `getExecutable()` (`:568`) to derive
  `<home>/bin/python3` (Linux) or `<home>/python.exe` (Windows), **only
  if the `python.executable` system property is unset**.
- `PYTHONPATH` — not read as config; *mutated* inside `executeProbe()`'s
  `ProcessBuilder` (`:625-631`) when `jpype.path` is set, so the probe
  subprocess can see a dev source tree.

### Executable-resolution precedence (`getExecutable()`, `:568-597`)

1. `System.getProperty("python.executable")`
2. `System.getenv("PYTHONHOME")` + platform suffix
3. First `python`/`python.exe` found on `PATH`
4. Otherwise: `RuntimeException("Unable to locate Python executable")`

This executable is used to run the detective probe. The
`python.config.executable` value actually passed to `PyConfig.executable`
at launch is `System.getProperty(CONF_EXECUTABLE, this.pythonExecutable)`
(`:397`) — i.e. it can differ from the probed executable if a caller sets
`python.config.executable` after `prepare()`/before `start()`. That's
intentional (it's the whole point of `prepare()` existing as a distinct
step — see its Javadoc, `:301-309`), just worth knowing.

### Module search paths

`start()` (`:374-382`) builds the final path list as
`this.modulePaths` (from `python.module.path`, set once at construction)
**plus** `python.config.path` split on `File.pathSeparator` — i.e. the
probe's dumped `sys.path` and any explicit `python.module.path` entries
are combined into one list and passed to the native layer as
`modulePath`.

## The probe/cache subsystem (`resolveLibraries()`, `:754-793`)

This is the highest-complexity, least-documented, and least-tested part
of the launch path — roughly a quarter of `MainInterpreter.java`
(`:599-913`) with no `plan/*.md` coverage before this document and no
`doc/*.rst` coverage at all.

1. **Probe**: `executeProbe()` (`:610-658`) runs
   `<executable> -c <probe.py source>` via `ProcessBuilder`, where
   `probe.py` (`native/jpype_module/src/main/resources/org/jpype/resources/probe.py`)
   emits Java-`Properties`-formatted output: `python.config.home`,
   `python.config.base_home`, `python.config.path` (`sys.path`, joined on
   `os.pathsep`), `python.lib`, `jpype.lib`, `jpype.version`,
   `jpype.arch`.
2. **Cache key**: `makeHash(pythonExecutable)` (`:599-608`) — a
   non-cryptographic hash of the **executable path string only**. No
   mtime, no content hash, no Python version check baked into the key.
3. **Cache storage**: a flat `Properties` file at `~/.jpype/jpype.properties`
   (or `%AppData%/Roaming/JPype` on Windows — `getAppPath()`, `:665-671`),
   keyed `<hash>` (executable path, for reference) and `<hash>-<propname>`
   for every probed property (`saveCache()`, `:680-714`).
4. **Cache load** (`loadFromCache()`, `:716-752`): pulls every
   `<hash>-*` entry back into a `Properties` object. The **only**
   validity check is `Files.exists(python.lib path)` (`:744-745`) —
   `jpype.lib` and `jpype.version` are trusted without re-checking
   existence or correctness.
5. **Defaults application**: `applyDefaults()` (`:861-872`) copies every
   probed (or cache-loaded) property into `System.setProperty()` **only
   if not already set** — explicit system properties set before
   `start()`/`prepare()` always win over probed values. This is the
   mechanism by which `python.config.home`/`python.config.path` etc. get
   populated without the caller ever setting them explicitly.
6. **Invalidation**: `jpype.nocache=true` is the *only* escape hatch
   (checked in `resolveLibraries()`, `:760`). No TTL, no content hash, no
   explicit "clear cache" API.
7. **Self-heal**: if the probe fails and `jpype.install=true`,
   `runPipInstall()` (`:799-841`) looks for a local `.whl` under
   `~/.jpype/dep` first (`findLocalWheel()`, `:846-859`), else falls back
   to `pip install "JPype1>=<ver>" --only-binary :all:`, then re-probes.

### Finding: cache staleness is silent

Because the cache key is only the executable path string, and reload
only re-validates `python.lib`'s existence, a stale cache entry survives
scenarios like:

- `pip install --upgrade JPype1` in the same venv (moves `_jpype`'s `.so`
  to a new hash-suffixed wheel-cache path; `jpype.lib` in the cache now
  points at a deleted/moved file).
- A venv deleted and recreated at the same path with a different Python
  patch version.

The failure doesn't surface until `installNatives()`'s `System.load(jpypeLibrary)`
throws `UnsatisfiedLinkError` (`:896`) — a much less informative failure
than a clean cache-miss/re-probe would have been. Candidate fix: also
validate `jpype.lib` (and maybe `jpype.version` against
`NativeContext.VERSION`) in `loadFromCache()`, not just `python.lib`.

## Native-side `PyConfig` construction (`native/common/jp_bridge.cpp`)

`Java_org_jpype_internal_NativeLauncherControl_startMain` (`:322-444`):

1. `PyConfig_InitIsolatedConfig` vs `PyConfig_InitPythonConfig`, chosen
   by `isolated` (`:343-346`).
2. Booleans copied straight onto `config.*` (`:349-354`).
3. `program_name`/`home`/`executable` assigned via `assignWideString()`
   (`:357-359`).
4. `PyConfig_Read(&config)` (`:364`) computes the default `sys.path`
   etc., **before** Java's module paths are appended.
5. `module_search_paths_set = 1` + `appendStringArray()` appends
   `modulePath` directly into `config.module_search_paths` (`:368-376`).
6. `argv`/`parse_argv` set from the Java `args` array (`:379-384`).
7. `Py_InitializeFromConfig(&config)` (`:388`).
8. `PyConfig_Clear(&config)` (`:402`/`:408`), always.
9. `appendModulePathsToSysPath()` (`:230-261`, called `:414`) appends the
   **same** module paths a second time, directly onto the live `sys.path`
   list object, after init — see Finding 3 below.
10. `launch()` (`:263-315`) imports `jpype`/`_jpype`, calls
    `jpype._core.initializeResources()`.
11. GIL handling: `Py_InitializeFromConfig` leaves the GIL held on the
    calling thread; compensated with an explicit `PyEval_SaveThread()`
    (`:420-432`) rather than `PyGILState_Release`, with a comment
    explaining why the naive approach would leak the GIL forever.

`dumpPyConfig()` (`:198-227`) is a debug helper that dumps the full
`PyConfig` including `prefix`/`exec_prefix`; its only call site is
commented out (`:387`) — worth knowing it exists next time this needs
debugging, since re-enabling it would immediately show Finding 1 below
(empty `prefix`/`exec_prefix`) if anyone went looking.

### Finding 1: `python.config.prefix`/`exec_prefix` are dead

`startMain`'s JNI signature receives `prefix`/`exec_prefix` parameters
(threaded all the way from `MainInterpreter.start()`, `:394`/`:396`), but
`jp_bridge.cpp` never assigns them to `config.prefix`/`config.exec_prefix`
— only `program_name`/`home`/`executable` go through `assignWideString()`.
The comment at `:360-361` ("prefix and exec_prefix are usually calculated
by Python based on home") explains why it might be *safe* to skip, but
the Java-side property still exists, is documented (informally, via the
property name itself) as a config knob, and silently does nothing if
set. Candidate fix: either wire it up (`assignWideString(env, prefix,
config.prefix)` etc.) or remove the property/parameter and say so
explicitly rather than leaving a plausible-looking no-op knob.

### Finding 3: module paths land in `sys.path` twice

Step 5 above appends Java's module paths into `config.module_search_paths`
*before* `Py_InitializeFromConfig`. Step 9 (`appendModulePathsToSysPath`)
appends the *same* array again, directly onto the live `sys.path` list,
*after* init — with no dedup (`PyList_Append` unconditionally,
`jp_bridge.cpp:255`). Every launch that passes module paths therefore
leaves literal duplicate entries in `sys.path`. Not fatal (Python
tolerates duplicate `sys.path` entries), but wasteful and looks like a
leftover from an earlier design that appended post-init only, with the
pre-init `config.module_search_paths` append added later without
removing the old call site. Candidate fix: drop one of the two append
sites — pre-init (`config.module_search_paths`) is the more correct one
since it's visible to `PyConfig_Read`'s own path calculation; the
post-init `appendModulePathsToSysPath` call looks like the one to remove,
but confirm nothing relies on `sys.path` mutation happening strictly
after `Py_InitializeFromConfig` (e.g. import machinery already having
run) before removing it.

## Subinterpreters: a disjoint, smaller config surface

`org.jpype.SubInterpreterBuilder`/`SubInterpreter` (see
`plan/archive/SubInterpreterBuilder.md`) do **not** consult any
`python.config.*` property, `python.module.path`, or `args`/`argv` at
all. Their entire config surface is the seven `PyInterpreterConfig`
booleans (`USE_MAIN_OBMALLOC`, `ALLOW_FORK`, `ALLOW_EXEC`,
`ALLOW_THREADS`, `ALLOW_DAEMON_THREADS`, `CHECK_MULTI_INTERP_EXTENSIONS`,
`OWN_GIL`) plus stdio redirection. `Java_..._startSubInterpreter`
(`jp_bridge.cpp:446-514`) builds a bare `PyInterpreterConfig` struct
literal and calls `Py_NewInterpreterFromConfig` — it never touches
`PyConfig`/`module_search_paths`/`argv`; a subinterpreter always inherits
the process-wide module search path the main interpreter already
established. Easy to wrongly assume a `SubInterpreter` respects
`python.config.home` etc. — it does not, by design (PEP 684 doesn't
expose most `PyConfig` fields per-subinterpreter the way `Py_InitializeFromConfig`
does for the main interpreter).

Also note: `SubInterpreterBuilder`'s own default (`ownGil()`) and the
bare, no-arg `SubInterpreter.start()`'s hardcoded legacy flags are
**different defaults** (own-GIL/own-obmalloc vs. shared-GIL/shared-obmalloc)
— see `plan/archive/SubInterpreterBuilder.md` and
`doc/limitations_java.rst`'s subinterpreter-isolation section (fixed
2026-07-17 to describe this correctly) for the full story.

## Other findings

- **Finding 6 (naming confusion)**: `jpype.addJVMOption()`/`getJVMOptions()`
  (`jpype/_core.py:48,54,74-75,421`) configure the **forward**-bridge JVM
  launch (Python starting a JVM via `startJVM()`) — nothing to do with
  anything in this document. Both mechanisms exist in the same codebase
  under similarly-shaped names (`JVMOption` vs `python.config.*`/
  `SubInterpreterBuilder.Option`); when writing or reviewing docs, always
  say which direction ("forward"/"reverse") is meant.
- **Finding 7 (fragile naming convention)**: `installNatives()`
  (`:874-903`) derives the Linux bootstrap library name via
  `jpypeLibrary.replace("jpype.", "jpyne.")` (`:888`) — a string-substitution
  assumption on the probed `_jpype` module file path with no explanation
  in `plan/` of the `jpype`/`jpyne` naming convention it depends on.
- **Test coverage gap**: `native/jpype_module/pom.xml:69-75` hardcodes
  Surefire `systemPropertyVariables` (`jpype.path=../..`,
  `jpype.nocache=false`, `python.executable=python3.12`,
  `python.config.home=/usr`) and every NGTest suite starts
  `MainInterpreter` with these defaults and nothing else — no test
  exercises `python.config.prefix`, `isolated`, `quiet`, `verbose`,
  `fault_handler`, the `jpype.nocache`/cache-hit path, or the
  `jpype.install` self-heal path at all.
- **Docs gap**: `doc/jvm_java.rst` (the Java-side interpreter-lifecycle
  chapter) covers only `start()`/`isStarted()`/`close()`, `PyBuiltIn`/
  `Script`, and the GIL-per-call model — none of the `python.config.*`
  surface, the probe/cache subsystem, or `prepare()`'s two-step
  configure-then-start pattern (documented only in its own Javadoc,
  `:301-309`) is mentioned there. Worth a follow-up doc pass once the
  bugs above are resolved one way or the other (fixing dead config before
  documenting it is better than documenting a no-op).

## Out of scope for this document

No code changes were made while producing this audit. The two concrete
bugs (dead `prefix`/`exec_prefix`, duplicate `sys.path` entries) and the
cache-staleness gap are recorded as candidate follow-up work, not
scheduled. `plan/PythonCLI.md` is the actual next action item that grew
out of this audit.
