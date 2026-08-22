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
"""
TEST_MODULES = [
    'test_jchar',
    'test_jboolean',
]
