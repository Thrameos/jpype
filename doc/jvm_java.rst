
Controlling the Interpreter (for Java)
==========================================

This is the Java-side counterpart to :doc:`jvm_py`: there is no JVM to
start from this direction (Java is already running), but there is an
embedded CPython interpreter with its own lifecycle, and that lifecycle has
sharper edges than starting/stopping a JVM does. Every claim below is
grounded in ``org.jpype.MainInterpreter`` and ``python.lang.PyTestHarness``,
the shared setup/teardown fixture every ``*NGTest.java`` in this codebase
runs under.

.. contents::
   :local:
   :depth: 1


Starting and stopping
------------------------

``MainInterpreter`` is a singleton (``getInstance()``). ``start(String[])``
boots the embedded CPython interpreter inside the JVM process;
``isStarted()`` reports whether that has already happened. This is the
pattern every test in the codebase uses, via ``PyTestHarness``:

.. code-block:: java

    import org.jpype.MainInterpreter;
    import org.jpype.Script;

    MainInterpreter interpreter = MainInterpreter.getInstance();
    if (!interpreter.isStarted())
        interpreter.start(new String[0]);

    Script context = new Script(interpreter);

    // ... use the interpreter ...

    interpreter.close();

``close()`` is **irrevocable** -- once called, the interpreter is
permanently terminated for the life of the process; there is no restart,
mirroring the JVM-restart limitation described in :doc:`jvm_py`. A typical
long-lived application calls ``start()`` once near the top of ``main()``
and relies on JVM shutdown to tear things down, exactly as
``PyTestHarness``'s ``@AfterSuite`` hook does:

.. code-block:: java

    if (MainInterpreter.getInstance().isStarted())
        MainInterpreter.getInstance().close();


PyBuiltIn and Script
------------------------

``getBuiltIn()`` returns the interpreter's top-level ``PyBuiltIn`` -- the
handle for Python's builtins (``str()``, ``int()``, ``dict()``, ``eval()``,
...). ``org.jpype.Script`` wraps a ``PyBuiltIn`` together with its own
private globals/locals dict, which is usually more convenient than sharing
the interpreter's top-level namespace across unrelated pieces of code (each
``Script`` gets an isolated namespace, the same way a fresh Python module
would):

.. code-block:: java

    import python.lang.PyBuiltIn;

    PyBuiltIn builtin = interpreter.getBuiltIn();
    Script context = new Script(interpreter);   // its own globals/locals


The GIL model, briefly
--------------------------

Every call across the Java/Python boundary -- ``eval()``, ``exec()``, a
``PyCallable`` invocation, even a trivial ``toString()`` on a wrapped
object -- acquires and releases CPython's Global Interpreter Lock around
the call. This makes single-threaded use transparent, but it means **any**
Java thread that touches a ``PyObject`` is a potential GIL contention
point, and a call that never returns control to Java (blocked native code,
an uncaught leak in a custom SPI provider) can starve every other thread
trying to call into Python. :doc:`threading_java` covers this in full,
including the bounded async thread pool behind ``PyCallable.callAsync``. If
you're only calling Python from a single Java thread, this model is
invisible to you; the moment a second thread is involved, read
:doc:`threading_java` before writing that code.


Where to next
---------------

- :doc:`threading_java` -- the GIL-per-call model in full, and what's safe
  to do from a background thread.
- :doc:`quickguide_java` -- a shorter, example-first tour covering the same
  startup/``Script``/``eval`` ground.
