"""Ported subset of test/jpypetest, run on-device via plain unittest (see
testapp/main.py). Not discovered by filename pattern: p4a bundles the app
as compiled .pyc only (no .py sources on-device), and unittest.discover()
scans the filesystem for literal `test_*.py` filenames, which never match
a .pyc-only bundle - it silently finds nothing rather than erroring, which
is what happened the first time this was tried (logged as
"PORTED SUITE: ran=0"). Importing modules by name and loading tests from
the module object works regardless, since that goes through Python's
normal import machinery instead of a directory listing.

Add each newly-ported test/jpypetest/test_*.py file's module name here.

Deliberately NOT ported (see doc/android.rst's "Removed JPype Services"
and "Unsupported Java libraries" for why):

- test_thread.py: uses jpype.attachThreadToJVM()/detachThreadFromJVM(),
  removed on Android - Android's JVM is always already attached.
- test_coverage.py: calls jpype.getDefaultJVMPath()/jpype.startJVM(),
  meaningless on Android's single already-running JVM.
- test_sql_h2.py, test_sql_hsqldb.py, test_sql_sqlite.py: need real JDBC
  driver jars this harness doesn't bundle.
- Anything using subrun (test/jpypetest/subrun.py): spawns a fresh
  subprocess with its own startJVM() call per test - Android has neither
  subprocesses-with-their-own-JVM nor startJVM().
- test_annotation.py, test_reflect.py: their harness fixtures define a
  custom @Retention(RUNTIME) annotation type with a String value() method;
  merely having such a class in the dex (regardless of whether Python
  code ever touches it) crashes the app at startup with an ART/CheckJNI
  abort - "JNI DETECTED ERROR IN APPLICATION: the return type of
  CallObjectMethodA does not match java.lang.String <type>.value()".
  Confirmed with two independent annotation types sharing only that
  shape, so this looks like a genuine ART limitation in this build
  environment, not a bug in either harness file. See the exclusion
  comment in project/android/recipes/jpype1/__init__.py's
  postbuild_arch for the full writeup - root cause not yet identified.
"""
TEST_MODULES = [
    # test_annotation.py deliberately NOT ported: its harness fixture
    # (test/harness/jpype/annotation/TestAnnotation.java) crashes the app
    # at startup on Android - see the exclusion comment in
    # project/android/recipes/jpype1/__init__.py's postbuild_arch for
    # details. Root cause not yet identified.
    'test_array',
    'test_arrayFromBuffer',
    'test_attr',
    'test_boxed',
    'test_boxing',
    'test_boxing_comprehensive',
    'test_boxing_edge_cases',
    'test_buffer',
    'test_bytebuffer',
    'test_caller_sensitive',
    'test_charSequence',
    'test_classhints',
    'test_classloader',
    'test_closeable',
    'test_closed',
    'test_collection',
    'test_comparable',
    'test_conversion',
    'test_conversionInt',
    'test_conversionLong',
    'test_conversionShort',
    'test_core',
    'test_customizer',
    'test_directbuffer',
    'test_docstring',
    'test_exc',
    'test_fault',
    'test_fields',
    'test_forname',
    'test_functional',
    'test_gcfree_leaves',
    'test_generic',
    'test_hash',
    'test_hints',
    'test_inherit',
    'test_javacoverage',
    'test_javadoc',
    # test_keywords.py deliberately NOT ported: uses
    # @pytest.mark.parametrize, a real pytest feature (not just an unused
    # import like test_array.py's was) - would need rewriting as a loop
    # over explicit unittest test methods, not just a straight copy.
    'test_jboolean',
    'test_jbyte',
    'test_jchar',
    'test_jclass',
    'test_jdouble',
    'test_jedi',
    'test_jfloat',
    'test_jint',
    'test_jlong',
    'test_jmethod',
    'test_jobject',
    'test_jpackage',
    'test_jshort',
    'test_jstring',
    'test_jvmfinder',
    'test_lambdas',
    'test_leak2',
    'test_list',
    'test_map',
    'test_module2',
    'test_mro',
    'test_number',
    'test_numeric',
    'test_objectwrapper',
    'test_opts',
    'test_overloads',
    'test_pickle',
    'test_proxy_multithreaded',
    'test_ref',
    # test_reflect.py deliberately NOT ported: same annotation-related
    # startup crash as test_annotation.py above (jpype.reflect.Annotation)
    # - see the exclusion comment in project/android/recipes/jpype1/
    # __init__.py's postbuild_arch for details.
    'test_repr',
    'test_serial',
    'test_sql_generic',
    'test_synchronized',
    'test_utf8',
    'test_varargs',
    'test_virtual',
]
