# Full Python CLI support from `MainInterpreter`

## Status (2026-07-17): DONE

Both halves are implemented and tested against a real reverse-bridge
interpreter (not just compiled):

- `org.jpype.Runner` (`native/jpype_module/src/main/java/org/jpype/Runner.java`)
  implements `runModule`/`runFile`/`runCommand`/`pipInstall` per the
  decisions below. `RunnerNGTest`: 8/8 passing.
- `MainInterpreter.dispatch(String[] args)` implements the `-c`/`-m`/
  bare-file/`-i` argv dispatch (decision #2) on top of `Runner`, called
  from `main()` in place of the old unconditional `interactive()`. Leading
  `-i` flags are collected, the remaining action (if any) is dispatched via
  `Runner`, then `interactive()` runs afterward if `-i` was given; no
  action and no `-i` falls through to `interactive()` directly, matching
  today's original default. `-i` itself isn't covered by an automated
  test - it's an interactive REPL over the process's real stdin, and no
  existing test in this codebase drives `interactive()` at all.
  `MainInterpreterDispatchNGTest`: 6/6 passing.

Full native suite after both changes: 662/662 passing, 0 regressions.

Two real bugs found and fixed along the way, neither specific to the CLI
feature itself:

- `native/common/jp_bridge.cpp`'s `startMain` leaked the GIL on any failure
  path after `Py_InitializeFromConfig` succeeded (both the
  `appendModulePathsToSysPath` failure and any `JPypeException` from
  `launch()`/`_jbridge.initialize()`), hanging any later native call that
  needed the GIL from another thread - in practice, `MainInterpreter.close()`
  during test teardown. Fixed by adding the same explicit
  `PyEval_SaveThread()` the success path already used, on both failure paths.
- `Runner` itself: raw Java `String`s bound into a scratch scope via
  `PyDict.putAny()` come across as boxed Java-object wrappers, not native
  Python `str` (JPype's `convertStrings=false` default) - broke
  `module.startswith(...)` inside `runpy` and `exec(source)`. Fixed by
  routing every string through `PyBuiltIn.str()` before binding.

Follow-on from `plan/LaunchConfig.md` (the launch/config/caching audit).
Goal: let `org.jpype.MainInterpreter` do what the real `python` binary's
command line does — run a module (`-m`), run a script file with
arguments, run an inline command (`-c`), and (as a convenience built on
`-m`) install/run packages like `pip`. Right now none of this exists:
`MainInterpreter.main(String[] args)` always drops into a plain REPL
regardless of `args`.

## Current state (verified against source)

`MainInterpreter.main(String[] args)` (`MainInterpreter.java:244-251`):

```java
public static void main(String[] args)
{
  MainInterpreter interpreter = getInstance();
  interpreter.start(args);
  interpreter.interactive();
}
```

`start(args)` passes `args` down to `NativeLauncherControl.startMain`,
which sets `config.argv`/`config.parse_argv = 1` before `PyConfig_Read`
(`jp_bridge.cpp:379-384`). Because `parse_argv` is set, **CPython's own
argv parser inside `PyConfig_Read` already recognizes `-c`, `-m`,
`-i`, a bare script-file argument, etc.**, and populates
`config.run_command`/`config.run_module`/`config.run_filename`/
`config.inspect` accordingly — this is the exact same parsing the real
`python` binary relies on. But nothing on our side ever reads those
fields back out: `PyConfig_Clear(&config)` runs right after
`Py_InitializeFromConfig` succeeds (`jp_bridge.cpp:402`), discarding
them, and `interactive()` unconditionally calls
`PyRun_InteractiveLoop(stdin, "<stdin>")` (`jp_bridge.cpp:517-531`) no
matter what was in `args`. So today, `java org.jpype.MainInterpreter -m
pip install requests` parses `-m pip install requests` into
`config.run_module` internally, throws that value away, and opens a
REPL instead — a silent no-op on the flags, not an error.

This means the "parse `-c`/`-m`/script args like real Python" half of
the problem is **already solved for free** by `parse_argv = 1` +
`PyConfig_Read`. What's missing is *consuming* the parsed result.

## Why not just call `Py_RunMain()`

CPython's own `python` binary, after `Py_InitializeFromConfig`, calls
`Py_RunMain()` to actually execute the parsed `-c`/`-m`/file/REPL
action. That's the "obviously right" API to reach for — but
`Py_RunMain()` is designed to be the last call before process exit: it
runs the action *and then calls `Py_Finalize()` internally*, returning
an exit code. `MainInterpreter` is fundamentally not that shape — after
running a script or module, the embedding Java application needs the
interpreter to **stay alive** for further `PyCallable`/`PyObject`
Java-side interaction, exactly like it does today after `start()`
returns. `Py_RunMain()` would tear the interpreter down out from under
the caller. So this needs a hand-rolled equivalent of the *dispatch*
part of `Py_RunMain()` (what CPython's internal, non-public
`pymain_run_python()` in `Modules/main.c` does), without the
finalize-and-exit part.

## Design: build it on `Script`/`runpy`, not on `PyConfig` internals

Rather than extracting `config.run_command`/`run_module`/`run_filename`
from the C struct before `PyConfig_Clear()` (fragile — ties us to
reading fields out of a config object mid-lifecycle, and still needs new
native plumbing to hand values back to Java), do the dispatch **entirely
in Python, after the interpreter is already up**, using the same
machinery the real `python -m`/script/`-c` modes are themselves built
on:

- `-c command` → `exec(command, {"__name__": "__main__", ...})` — already
  directly expressible via the existing `Script.exec(String)`
  (`org/jpype/Script.java:71-74`); just need `sys.argv[0]` set to `"-c"`
  and the rest of `args` appended first.
- `-m module [args...]` → `runpy.run_module(module, run_name="__main__",
  alter_sys=True)`. `runpy` already does everything real `python -m`
  does: inserts the right things onto `sys.path`, sets `sys.argv`,
  handles packages-with-`__main__.py`. This is *exactly* how
  `python -m pip install X` already works under the hood — so "pip
  install a package" needs no special-casing, it's just `runModule("pip",
  "install", "X")`.
- `script.py [args...]` → `runpy.run_path(script, run_name="__main__")`,
  or a plain `Script.exec(Files.readString(path))` with `sys.argv` set
  first if line-number/traceback fidelity from `run_path` isn't needed
  (`run_path` is closer to real Python behavior — e.g. it handles
  zipapps/directories with `__main__.py` too — so prefer it).
- `-i` after any of the above → run the action, then fall through to the
  existing `interactive()` (already exactly right, no changes needed
  there).
- Bare invocation, no `-c`/`-m`/file → today's behavior (`interactive()`
  directly) — unchanged, this is the default when nothing else was
  requested.

This avoids fighting `Py_RunMain`/`PyConfig` lifetime issues entirely,
reuses the `Script` class that already exists for exactly this purpose
(`eval`/`exec`/`importModule`), and matches real Python module-execution
semantics via `runpy` instead of hand-rolling `sys.path`/`__main__`
bookkeeping ourselves.

### Proposed Java API (new methods, likely on `MainInterpreter` or a new
small `Interpreter` default-method addition — TBD during implementation,
sketched here for shape only)

```java
// Run `python -m <module> <args...>` equivalent.
public PyObject runModule(String module, String... args);

// Run `python <script> <args...>` equivalent.
public PyObject runFile(Path script, String... args);

// Run `python -c <command> <args...>` equivalent.
public PyObject runCommand(String command, String... args);

// Convenience: python -m pip install <spec> [--upgrade, etc. via args]
public void pipInstall(String... pipArgs);   // runModule("pip", pipArgs) with "install" NOT hardcoded --
                                              // let the caller pass the full pip subcommand (install/uninstall/list/...)
```

`pipInstall` deliberately does **not** hardcode `"install"` — `runModule("pip",
pipArgs)` where the caller passes `"install", "somepkg"` (or
`"uninstall", "somepkg"`, or `"list"`) is more honest about what it's
doing and doesn't need a second method per pip subcommand. Naming/shape
still TBD; revisit once `runModule` exists and pip-install is just its
first real caller.

### `sys.argv` handling

Each of `runModule`/`runFile`/`runCommand` must set `sys.argv` before
invoking (`runpy.run_module`/`run_path` do part of this themselves when
`alter_sys=True`, but `argv[0]` and the rest of the args list still need
to be assembled from the Java `String... args`) and should almost
certainly **restore** the prior `sys.argv` afterward, since
`MainInterpreter` is a long-lived singleton — a Java app that calls
`runModule()` once and then continues doing ordinary `PyObject`/
`PyCallable` work shouldn't be left with a mutated `sys.argv` from a
one-off module run. Real `python -m`/script/`-c` invocations don't need
this because the process exits right after; embedding is different.
This is a real behavioral decision to get right during implementation,
not just a detail — write a test for it (run a module that reads
`sys.argv`, assert it's restored to whatever it was — likely empty or
the original `start()` args — afterward).

### Exceptions

A module/script/command that raises should surface as a normal
`python.exceptions.*` exception on the Java side (same as any other
`Script.exec`/`eval` failure) rather than being swallowed the way a
top-level script's uncaught exception normally just prints a traceback
and exits the process — because, again, the interpreter isn't exiting
here. `SystemExit` specifically (very plausible from something like
`pip`, which calls `sys.exit()` on completion/failure) needs a defined
behavior: probably catch it and translate the exit code into either a
return value or a distinguishable exception, rather than letting it
propagate as some generic Python exception or crash the JVM-embedded
interpreter. Needs a concrete test: `runModule("pip", "install",
"nonexistent-package-xyz")` should raise/return something informative
about the failure, not hang or corrupt interpreter state for subsequent
calls.

## Decisions (locked in 2026-07-17)

1. **New class, not `MainInterpreter` methods.** Mirror `Script`: a small
   class (working name `Runner`) constructed from an `Interpreter`, so it
   works against `SubInterpreter` too and doesn't bloat `MainInterpreter`.
   Houses `runModule`/`runFile`/`runCommand`/`pipInstall`.
2. **Implement `runModule`/`runFile`/`runCommand` first; wire `main()`'s
   auto-dispatch as a separate, later step.** Land and test the
   underlying methods in isolation before touching the CLI entry point
   — `main()`'s dispatch is just thin argv-parsing (`-c`/`-m`/bare-file/
   `-i` detection) around them once they exist.
3. **Catch `SystemExit`, translate to a checked exception carrying the
   exit code.** Don't let it propagate as a generic Python exception,
   don't swallow it. Before implementing, check whether `Script.exec`/
   `eval` already has *some* `SystemExit` behavior today — if so, match
   that existing convention rather than inventing a second one.
4. **Use `runpy.run_path()` for `runFile`, not plain `exec()`.** Matches
   real `python script.py` behavior for free (line numbers,
   `__main__.py`-containing directories, zipapps) — not worth
   maintaining a second, less-faithful code path.
5. **Always restore `sys.argv`** after `runModule`/`runFile`/`runCommand`
   returns, success or exception. `MainInterpreter` is long-lived (unlike
   a real `python` process, which just exits), so leaving `sys.argv`
   mutated after a one-off call is a footgun for any Java code that
   keeps using the interpreter afterward. No real-Python precedent to
   preserve here, since a real CLI process never needs to restore
   anything.

## Relationship to `plan/LaunchConfig.md`'s findings

Independent of this feature — the dead `python.config.prefix`/
`exec_prefix` and duplicate `sys.path`-append bugs found there don't
block this work and don't need to be fixed first. Worth fixing at some
point, but tracked separately.

## Testing (sketch)

- `runModule("this")` (stdlib `this` module — the Zen of Python easter
  egg, has an observable `sys.stdout` side effect) — assert it runs and
  the interpreter is still usable afterward (a subsequent ordinary
  `Script.eval("1+1")` still works).
- `runModule("pip", "install", "<real small package>")` against a throwaway
  venv — assert it actually installs (`import <package>` succeeds after).
- `runFile(path)` against a small script that both prints and defines a
  function, assert output and that `sys.argv` matches what was passed.
- `runCommand("import sys; sys.argv")` — assert `argv[0] == "-c"` per
  real Python semantics.
- `sys.argv` restoration: capture `sys.argv` before and after each of
  the above, assert unchanged.
- `SystemExit` from a `runModule()`/`runCommand()` call — assert it
  surfaces as a specific, documented outcome (exact shape TBD, see open
  question 3) rather than crashing or hanging.
- `-i` combined with a script/module (once `main()` dispatch lands, open
  question 2) — assert the script/module runs, *then* the REPL opens.
