# Split MainInterpreter into MainInterpreter + Launcher

## Status (2026-07-17): DONE

Implemented exactly as designed below. New `org.jpype.Launcher`
(package-private class, ~400 lines) now owns environment discovery;
`MainInterpreter` (down from 964 to ~490 lines) delegates to it via a
`private final Launcher launcher = new Launcher()` field and
`launcher.prepare()`/`launcher.installNatives()`/the four getters. One
correction made mid-implementation to the design below: `Launcher.prepare()`
does **not** call `installNatives()` (unlike the first draft of this plan
said) -- `installNatives()` stays a separate `public` method, called by
`MainInterpreter.start()` after its audit checks, preserving the exact
order the original code had (a standalone `prepare()` call, e.g. to
inspect `python.config.*` properties before deciding whether to `start()`,
must not have a side effect of loading native libraries before the caller
has decided to proceed).

Also removed `MainInterpreter.objs(Object...)` -- it was a varargs helper
used only by the log statements in the methods that moved to `Launcher`;
dead in `MainInterpreter` after the split.

Verified: `mvn -q -o compile` clean; full suite 662/662 (same as before
the split, confirming no behavior change); additionally exercised the
probe/cache-miss path specifically (deleted `~/.jpype/jpype.properties`,
reran `MainInterpreterDispatchNGTest` -- 6/6 passed, log showed the real
probe subprocess running, `_jpype`/`libpython` loading, and the interpreter
launching end to end through the new `Launcher` class) before restoring
the cache file and re-confirming 662/662 on the full suite.

## Current state

`org.jpype.MainInterpreter` (964 lines) mixes three unrelated concerns in
one class:

1. **Interpreter lifecycle** -- the `Interpreter` interface
   (`setBackend`/`getBuiltIn`/`close`/`isJava`/`isStarted`), the singleton,
   the SPI installer hookup, `start()`/`prepare()` as public API.
2. **Environment discovery** -- everything under the `//<editor-fold
   desc="internal">` region except `installNatives`'s log lines: locating a
   `python` executable (`getExecutable`/`checkPath`), running the detective
   probe (`executeProbe`/`loadProbeResource`), the cache subsystem
   (`makeHash`/`getAppPath`/`saveCache`/`loadFromCache`), pip self-heal
   (`runPipInstall`/`findLocalWheel`), applying probed defaults as system
   properties (`applyDefaults`), and loading the native libraries
   (`installNatives`). None of this references `PyBuiltIn`, `NativeContext`,
   or anything else interpreter-lifecycle-shaped -- it is pure "find and
   load a Python environment" logic.
3. **CLI entry point** -- `main`/`dispatch`, added in `plan/archive/PythonCLI.md`.

Verified via grep: none of the environment-discovery private methods
(`resolveLibraries`, `installNatives`, `executeProbe`, `loadFromCache`,
`saveCache`, `runPipInstall`, `findLocalWheel`, `getExecutable`,
`checkPath`, `makeHash`, `loadProbeResource`, `checkWindows`,
`parseVersion`) are referenced anywhere outside `MainInterpreter.java` --
the split is self-contained, no other file needs to change.

## Goal

Extract concern (2) into a new `org.jpype.Launcher` class. `MainInterpreter`
keeps (1) and (3), and delegates to a `Launcher` instance for environment
resolution instead of owning that logic itself.

## Design

New file `native/jpype_module/src/main/java/org/jpype/Launcher.java`:

- Constants that belong to environment discovery, moved as-is:
  `PROBE`, `PYTHON_EXEC`, `PYTHON_LIB`, `JPYPE_LIB`, `JPYPE_ARCH`,
  `JPYPE_NOCACHE`, `JPYPE_INSTALL`, `JPYPE_VER`, `PROPERTIES`.
  (`CONF_*` and `MOD_PATH` stay on `MainInterpreter` -- they configure the
  interpreter launch itself, not environment discovery.)
- Fields: `isWindows`, `pythonExecutable`, `pythonLibrary`, `jpypeLibrary`,
  `jpypeVersion`.
- Methods, moved as-is (same bodies, `private` stays `private`):
  `checkWindows` (static), `loadProbeResource`, `checkPath`,
  `getExecutable`, `makeHash`, `executeProbe`, `getAppPath`, `saveCache`,
  `loadFromCache`, `runPipInstall`, `findLocalWheel`, `applyDefaults`,
  `installNatives`.
- `resolveLibraries()` becomes the public `prepare()` entry point (mirrors
  today's `MainInterpreter.prepare()` guard-and-call shape): no-ops if
  already resolved (`pythonLibrary != null`), otherwise runs the probe/cache
  flow. Deliberately does **not** call `installNatives()` -- today,
  `MainInterpreter.prepare()` only resolves libraries; `installNatives()`
  runs later inside `start()`, after the audit checks that can throw
  (missing libraries, version mismatch). Folding native loading into
  `Launcher.prepare()` would load natives before those checks run,
  changing behavior for a caller who calls `prepare()` standalone to
  inspect properties without starting. `installNatives()` stays a
  separate `public` method on `Launcher`, called by `MainInterpreter.start()`
  after its audit checks, same position as today.
- `parseVersion(String)` becomes `public static` on `Launcher` (pure
  utility, no interpreter-lifecycle dependency) -- `MainInterpreter.start()`
  calls `Launcher.parseVersion(...)` for its compatibility check.
- Getters for the four resolved fields: `getPythonExecutable()`,
  `getPythonLibrary()`, `getJpypeLibrary()`, `getJpypeVersion()`.
- `isPrepared()` (`pythonLibrary != null`) so `MainInterpreter.prepare()`
  can keep its own no-op guard without reaching into `Launcher` internals.

`MainInterpreter` changes:

- New field: `private final Launcher launcher = new Launcher();`
- `prepare()`: delegates to `launcher.prepare()` (rename internal call;
  public signature/behavior unchanged).
- `start()`: replaces direct field reads (`this.pythonLibrary`,
  `this.jpypeLibrary`, `this.jpypeVersion`, `this.pythonExecutable`) with
  `launcher.getPythonLibrary()` etc.; replaces `installNatives()` call and
  `parseVersion(...)` call with `launcher.installNatives()` /
  `Launcher.parseVersion(...)`. `System.getProperty(CONF_EXECUTABLE,
  this.pythonExecutable)` becomes `System.getProperty(CONF_EXECUTABLE,
  launcher.getPythonExecutable())`.
- Removes the now-dead: `PROBE`, `PYTHON_EXEC`, `PYTHON_LIB`, `JPYPE_LIB`,
  `JPYPE_ARCH`, `JPYPE_NOCACHE`, `JPYPE_INSTALL`, `JPYPE_VER`, `PROPERTIES`
  constants; `isWindows`, `pythonExecutable`, `pythonLibrary`,
  `jpypeLibrary`, `jpypeVersion` fields; and all the methods listed above
  under "Methods, moved as-is".
- Everything else (`modulePaths`, `MOD_PATH`, `CONF_*`, singleton,
  `setBackend`/`getBuiltIn`/`close`/`isJava`/`isStarted`, `main`/`dispatch`,
  `interactive`) is untouched.

No public API changes -- `MainInterpreter.prepare()`/`start()` keep their
exact signatures and behavior. `Launcher` is an internal implementation
detail (package-private class would work too, but `public` matches the
rest of `org.jpype`'s style where implementation classes like `Runner`,
`SpiLoader`, `BootstrapLoader` are public without being part of the
documented API surface).

## Testing

No new tests needed -- this is a pure refactor with no behavior change.
Verification is the existing suite staying green:

```
cd native/jpype_module && mvn -q -o test -Dtest=TestSuite -Dpython.executable=python3.10
```

Expect `Tests run: 662, Failures: 0, Errors: 0, Skipped: 14` (same as
before the split). Also re-run the probe/cache path specifically by
deleting `~/.jpype/jpype.properties` once and confirming a fresh `start()`
still resolves and caches correctly (exercises `Launcher`'s cache-miss
path end to end, not just the cache-hit path the test suite normally hits).
