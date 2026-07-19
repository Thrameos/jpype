# DeadCodeAudit: find dead code and poorly-written code in the C++ layer

## Status (2026-07-18): CLOSED. Steps 1-6 done, findings reported, user
approved fixing the 7 surviving findings (2 were corrected out, see
below), fixes applied, full test suite green (1752 passed, 172 skipped, 0
failed via `make -f project/dev.mk test`). `compile_commands.json` and the
raw cppcheck/clang-tidy report files were scratch artifacts, not
committed - regenerate via steps 1-3 below if repeating this later.

### Fixes applied
1. `JPJavaFrame::ExceptionCheck()` - deleted (decl + def), zero callers.
2. `PyJPState_GET` - deleted, dead duplicate of `PyJPModule_getState`.
3. `JPPyString::fromCharUTF16` - deleted (decl + def), zero callers.
4. `dumpPyConfig`/`print_module_path`/`dumpWide`/`dumpWideList` in
   `jp_bridge.cpp` - all deleted (the latter two only existed to support
   `dumpPyConfig`), plus the commented-out `//dumpPyConfig(&config);` call
   site.
5. `JPClass::setField` header declaration (`jp_class.h:193`) - renamed
   params `obj, val` -> `c, obj` to match both the .cpp definition and the
   sibling `setStaticField`.
6. `JPBufferType` ctor (`jp_buffertype.cpp`) - merged the identical
   `java.nio.Buffer`/`java.nio.ByteBuffer` branches into one `||` condition.
7. `JPArrayView` (`jp_array.h`) - added explicit `= delete` copy
   constructor/assignment (not implemented for real, since nothing
   currently copies one - this just turns a latent double-free into a
   compile error if that ever changes).

### Real findings (survived manual cross-repo-grep triage)
1. ~~`JPGarbageCollection::init()`/`shutdown()` dead, delete~~ **CORRECTED
   (user, 2026-07-18): not dead code.** GC hookup is intentionally disabled
   pending reactivation - the FIXME at `jp_context.cpp:648` describes a real
   init-ordering blocker to fix, not abandoned code to remove. Leave as-is;
   if anything, this is a "finish wiring this up" task, not a deletion
   candidate. Don't re-flag this as a delete target in a future pass.
2. `JPJavaFrame::ExceptionCheck()` (`jp_javaframe.cpp:142`) - dead, and the
   line above it is the original author's own `// TODO: why is this never
   used? Should be deleted if obsolete.` (this one has no flag/reactivation
   angle - genuinely a candidate.)
3. `PyJPState_GET` (`pyjp_module.cpp:67`) - dead duplicate of
   `PyJPModule_getState` defined 8 lines below.
4. `JPPyString::fromCharUTF16` (`jp_pythontypes.cpp:188`) - zero callers.
5. `dumpPyConfig`/`print_module_path` (`jp_bridge.cpp`) - dead debug helpers.
6. ~~`JPypeTracer::traceLocks` + `JP_TRACE_LOCKS` dead, delete~~
   **CORRECTED (user, 2026-07-18): not dead code.** Tracing (`JP_TRACING_ENABLE`,
   `ENABLE_TRACING` cmake option) and fault-injection logic (see
   [[jpype_fault_injection_global_by_design]]) are compiler-flag-gated
   debug/instrumentation features, not decayed code - don't recommend
   deleting flag-gated instrumentation just because it has zero call sites
   in a default (flags-off) build/analysis pass. `JP_TRACE_LOCKS` having no
   *invocation* sites (as opposed to the macro's own def being flag-gated)
   may still be worth wiring up or documenting, but is not a delete
   candidate the way #2/#3/#4/#5 are.
7. `JPClass::setField` (`jp_class.cpp:234`) - parameter names in the
   definition don't match the header declaration; confusing, not a bug.
8. `JPBufferType` ctor (`jp_buffertype.cpp:30`) - `java.nio.Buffer` and
   `java.nio.ByteBuffer` branches are identical, mergeable.
9. `JPArrayView` (`jp_array.h:23`) - owns a raw `new`'d buffer with no
   copy ctor/assignment (Rule of Three); latent double-free if ever copied
   by value, not currently triggered anywhere in the codebase.

### Tool noise triaged out (false positives, do not re-flag)
- All `Java_org_jpype_*`/JNI-exported "unused function" hits from cppcheck:
  called from Java via JNI linkage, invisible to a single-TU C++ analyzer.
- `jp_convert.cpp` `toB/toC/.../call2/call4/call8`: used as
  `&Convert<T>::toX` function-pointer template instantiations inside a
  giant switch; cppcheck's template handling misses them.
- `missingReturn` in `pyjp_method.cpp`/`pyjp_module.cpp`/`pyjp_number.cpp`/
  `pyjp_object.cpp`: `JP_PY_CATCH` macro supplies the `return`; cppcheck
  can't see through the macro expansion.
- `nullPointer`/fail-fast hits in `jp_exception.cpp` (`int *i = nullptr; *i
  = 0;`): deliberate crash-on-corrupted-error-state pattern, already
  documented (see `jpype_execcrash_rootcause_fixed` memory).
- `rethrowNoCurrentException` in `jp_tracer.h`/`pyjp_misc.cpp`: bare
  `throw;` idiom used only inside a catch block via macro, already has a
  `// lgtm` suppression annotation from a prior pass.
- `bugprone-unhandled-self-assignment` on `JPPyObject::operator=`: it does
  check `m_PyObject == self.m_PyObject`, just not via `this == &self` -
  correct as written.
- clang-tidy's `misc-unused-parameters` (210 hits) and
  `bugprone-easily-swappable-parameters` (86 hits): expected noise from
  JNI callback signatures and virtual-override boilerplate; not
  individually triaged, low value.

### Step 3 command used
`clang-tidy -p . $(find native/common native/python -name '*.cpp')
--checks='-*,clang-analyzer-deadcode.*,misc-unused-*,bugprone-*,performance-*'`
(dropped `readability-*` from the originally planned checks list - too
noisy relative to signal on a first pass; add back if doing a dedicated
readability pass later.)

### Not done this pass
Step 4 (linker `--gc-sections`/`--print-gc-sections` cross-check) was
skipped - manual whole-repo `grep` on each cppcheck-flagged symbol name
served the same purpose (confirm zero call sites) and was faster than
rebuilding with different link flags. Worth doing for real if a future pass
wants a fully tool-driven signal instead of grep-based confirmation.

## Scope
`native/common/` + `native/python/` (~26.6k lines, the actual JPype native
extension logic). Excludes `native/jni_include` (vendored JNI headers) and
`native/bootstrap` (thin glue) - not worth auditing, mostly third-party or
generated. Also excludes the Java layer (`native/jpype_module/src/main`) -
separate audit if wanted later.

## Method: tool pass first, then manual triage
cppcheck 2.7 and clang-tidy (LLVM 14) are installed. Plan is a mechanical
pass to generate a candidate list, then read every flagged file (not just
the flagged line) to judge real dead code / bad design vs. false positive -
tools are bad at judging intent, and this codebase has known deliberate
oddities that look wrong out of context (e.g. [[jpype_fault_injection_global_by_design]],
the `GCOVR_EXCL_START` fail-fast in `jp_exception.cpp` per
[[jpype_execcrash_rootcause_fixed]]). Don't flag those as bugs again.

## Steps

### 1. Generate `compile_commands.json`
Ninja (the scikit-build-core default generator here) can emit it directly;
no permanent `CMakeLists.txt` change needed, just an extra one-time
config-setting on the same pip editable-install invocation `dev.mk` already
uses:
```
pip install -v --no-build-isolation -e . \
  --config-settings=cmake.define.CMAKE_EXPORT_COMPILE_COMMANDS=ON \
  --config-settings=cmake.define.BUILD_TEST_HARNESS=ON \
  --config-settings=cmake.define.CMAKE_BUILD_TYPE=RelWithDebInfo
```
Result lands at `build/<tag>/compile_commands.json`. Symlink/copy to repo
root (`compile_commands.json`) so clang-tidy's default search finds it.

### 2. cppcheck pass (no compile db needed)
```
cppcheck --enable=unusedFunction,style,performance --inconclusive \
  --suppress=missingIncludeSystem \
  -I native/common/include -I native/python/include \
  native/common native/python 2> cppcheck_report.txt
```
`unusedFunction` only works reliably when cppcheck can see the whole
translation set at once (pass both dirs together, not per-file), so run it
as a single invocation over the two directories, not split up.

### 3. clang-tidy pass (needs compile_commands.json from step 1)
Run with a checks list weighted toward dead-code/quality, not style-only:
```
clang-tidy -p . $(find native/common native/python -name '*.cpp') \
  --checks='-*,clang-analyzer-deadcode.*,misc-unused-*,readability-*,bugprone-*,performance-*' \
  > clang_tidy_report.txt
```
Expect noise from `readability-*` on an older C++ codebase; skim rather
than fix everything it suggests - only pull items that are actual dead
code, unreachable branches, or genuinely confusing constructs.

### 4. Cross-check with linker dead-symbol detection
Rebuild with `-ffunction-sections -fdata-sections` and link with
`-Wl,--gc-sections -Wl,--print-gc-sections` to get a real list of symbols
the linker discards - a second, tool-independent signal for truly unused
functions (catches things cppcheck's single-TU view might miss, e.g. a
function only called from a `.cpp` that never got compiled in due to a
`#if` branch).

### 5. Merge + triage
Combine the three candidate lists (cppcheck unusedFunction, clang-tidy
dead-code/misc-unused, gc-sections dropped symbols) into one list grouped
by file. For each candidate, read the surrounding function/file before
deciding: real dead code (delete), fine-but-ugly (note for a follow-up
cleanup, don't fix inline unless trivial), or false positive tied to a
known deliberate pattern (skip, cite the memory/plan doc that explains it).

### 6. Manual read of hot/core files regardless of tool output
Tools miss "poorly written but technically live" code (needless
complexity, duplicated logic across `pyjp_*.cpp` files, oversized
functions). After the tool triage, do a straight read of the largest/most
central files that haven't already been fully read during triage:
`jp_javaframe.cpp`, `jp_classhints.cpp`, `pyjp_module.cpp`,
`pyjp_class.cpp`, `jp_context.cpp`, `jp_exception.cpp` - these are the
biggest files and the ones most other code depends on, so quality issues
here have the widest blast radius.

### 7. Write findings
One doc (or a `ReportFindings`-style list if this turns into an actual
review pass) listing: file:line, what's wrong, dead vs. quality issue,
recommended action. Do not delete/refactor anything during the audit
itself - this plan is find-only; fixing is a separate follow-on pass the
user explicitly greenlights per file/finding.
