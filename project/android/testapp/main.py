"""Minimal headless verification app for the project/android/ build harness.

Everything is written to stdout, which p4a's webview bootstrap forwards to
logcat, so `buildozer android debug deploy run logcat` is the whole
interaction loop - see project/android/README.md.

Two checks:

1. Golden path: `import jpype` (which triggers `_jpype.bootstrap()`
   automatically, see jpype/__init__.py) followed by a trivial JClass call.
   Confirms the whole chain - NDK cross-compile, Android_JNI_GetEnv() glue,
   org.jpype Java classes reaching the APK's dex, PyJPModule_bootstrap() -
   actually works end to end.

2. Regression check for #1257: PyJPModule_bootstrap() previously let a C++
   exception escape uncaught across the C-linkage boundary into CPython,
   aborting the whole process (SIGABRT) instead of raising a catchable
   Python exception. Reproducing the exact resource-loading failure from
   the original report isn't practical from here (it depended on the
   reporter's own modified bindings), but calling `_jpype.bootstrap()` a
   second time exercises the same JP_PY_TRY/JP_PY_CATCH-wrapped function
   again. What matters is only whether the process survives to print a
   result at all - whether the second call raises or succeeds cleanly are
   both fine outcomes now that bootstrap genuinely works; only a SIGABRT /
   "terminating due to uncaught exception" in logcat (this script never
   reaching its next print) would indicate the original #1257 crash
   signature came back.
"""
print("=== jpype android testapp starting ===")

try:
    import jpype
    cls = jpype.JClass('java.lang.String')
    print("GOLDEN PATH: PASS (%s)" % cls)
except Exception as ex:
    print("GOLDEN PATH: FAIL: %r" % (ex,))

# Run regardless of the golden path result above - this is the actual
# #1257 regression check and shouldn't be skipped just because some other,
# unrelated part of the golden path failed.
try:
    import _jpype
    _jpype.bootstrap()
    print("REGRESSION CHECK #1257: PASS - second bootstrap() completed, no crash")
except Exception as ex:
    print("REGRESSION CHECK #1257: PASS - caught %r instead of crashing" % (ex,))

print("=== jpype android testapp done ===")
