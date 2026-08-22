"""Android port of test/jpypetest/common.py.

Not a copy-by-reference of the desktop file: that one imports pytest and
uses `@pytest.mark.usefixtures("jvm_session")` to start a shared JVM once
per pytest session (see test/jpypetest/conftest.py). Neither applies on
Android - there is no pytest test runner here (see testapp/main.py, which
uses plain unittest instead, avoiding the open question of whether pytest
itself builds/runs on Android at all), and there is nothing for a
jvm_session-equivalent fixture to do: the JVM is already running and
already attached by the time `import jpype` returns (see
jpype/__init__.py's `_jpype.bootstrap()` call), matching doc/android.rst's
description of Android's execution model.

Keeps the same public API test/jpypetest/*.py test files already rely on
(`JPypeTestCase`, `requireInstrumentation`, `requireNumpy`, `requireAscii`,
`version`) so ported test files need minimal changes - ideally just their
`import common` resolving to this file instead of the desktop one.
"""
import unittest  # Extensively used as common.unittest.

CLASSPATH = None
fast = False


def version(v):
    return tuple([int(i) for i in v.split('.')])


def requireInstrumentation(func):
    def f(self):
        import _jpype
        if not hasattr(_jpype, "fault"):
            raise unittest.SkipTest("instrumentation required")
        rc = func(self)
        _jpype.fault(None)
        return rc
    return f


def requireNumpy(func):
    def f(self):
        try:
            import numpy
            return func(self)
        except ImportError:
            pass
        raise unittest.SkipTest("numpy required")
    return f


def requireAscii(func):
    # The desktop version checks the test suite's own on-disk path; on
    # Android there is no equivalent "source root" to check against, and
    # the app's own data directory is always ASCII in practice, so this is
    # simply a no-op pass-through rather than a real check.
    return func


class UseFunc(object):
    def __init__(self, obj, func, attr):
        self.obj = obj
        self.func = func
        self.attr = attr
        self.orig = getattr(self.obj, self.attr)

    def __enter__(self):
        setattr(self.obj, self.attr, self.func)

    def __exit__(self, exception_type, exception_value, traceback):
        setattr(self.obj, self.attr, self.orig)


class JPypeTestCase(unittest.TestCase):
    def setUp(self):
        import jpype
        self.jpype = jpype.JPackage('jpype')

    def assertElementsEqual(self, a, b):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertEqual(a[i], b[i])

    def assertElementsAlmostEqual(self, a, b, places=None, msg=None,
                                   delta=None):
        self.assertEqual(len(a), len(b))
        for i in range(len(a)):
            self.assertAlmostEqual(a[i], b[i], places, msg, delta)

    def useEqualityFunc(self, func):
        return UseFunc(self, func, 'assertEqual')
