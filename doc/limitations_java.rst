Known Limitations (Java calling Python)
========================================

This section lists limitations specific to the reverse bridge — Java code
calling into Python objects through ``python.lang``/``python.io``/
``python.collections``/``python.datetime``/``python.decimal``/
``python.pathlib``/``python.exceptions``. Limitations that come from the JVM
or CPython themselves (unsupported platforms, PyPy, restarting or running
multiple JVMs, ``java.library.path``, fault handlers) apply equally in this
direction and are documented once, in :doc:`limitations_py`, rather than
repeated here.


.. _reverse_bridge_limitations_unimplemented_stdlib:

Unimplemented standard library coverage
----------------------------------------

Only a subset of the Python standard library has a typed Java front end
today. Implemented, with a ``WrapperService`` SPI provider:

- ``python.io`` — ``io``/``_io`` (``PyIOBase``/``PyBufferedReader``/
  ``PyTextIOWrapper``/etc.)
- ``python.collections`` — ``collections`` (``PyDeque``/``PyOrderedDict``/
  ``PyDefaultDict``/``PyCounter``/``PyChainMap``)
- ``python.datetime`` — ``datetime`` (``PyDate``/``PyDateTime``/
  ``PyTimeDelta``)
- ``python.decimal`` — ``decimal`` (``PyDecimal``)
- ``python.pathlib`` — ``pathlib`` (``PyPath``)

Anything else — including modules with a scoped-but-not-built plan
(``re.Pattern``/``re.Match``, ``queue.Queue``/``LifoQueue``/
``PriorityQueue``) — crosses into Java as a bare, untyped ``PyObject``.
This still works (every method is reachable through ``PyObject``'s own
attribute/call surface, or via ``$``-mangled direct dispatch on a custom
interface — see :doc:`customizers_java`), it just doesn't get a
purpose-built Java-shaped API the way ``java.util.Map`` gets from
``python.collections``.


.. _reverse_bridge_limitations_no_pickling_numpy:

No reverse-direction serialization or NumPy story
----------------------------------------------------

:doc:`pickling_py` (JPickler, Java objects serialized for Python pickling)
and :doc:`numpy_py` (bulk array transfer) both describe mechanisms for
Python code consuming Java data. Neither has a Java-calling-Python
counterpart today: there is no typed bulk-array or buffer-promotion path
for Java code to pull a NumPy array out of Python efficiently, and no
supported way for Java to participate in pickling a Python object. Treat
data crossing this direction as ``python.io`` streams or individual
``PyObject`` attribute/method calls until one of these is built.


.. _reverse_bridge_limitations_subinterpreters:

Subinterpreters are legacy-style, not fully isolated
--------------------------------------------------------

``org.jpype.Interpreter``/``SubInterpreter`` can start additional Python
subinterpreters alongside the main one (verified with two subinterpreters
plus the main interpreter running concurrently with no cross-talk or
hangs). This is **legacy-style multi-interpreter support**: all
subinterpreters share the same process GIL and the same CPython object
allocator as the main interpreter, because ``_jpype`` is a single-phase-init
extension. It is not full PEP 684 (per-interpreter GIL) isolation — don't
document or rely on it as if separate subinterpreters give independent
concurrency or independent memory ownership.

A related, deliberate guard: a proxy or wrapped object obtained inside one
(sub)interpreter cannot be silently smuggled into another interpreter's
scope. Attempting to do so raises a detection error rather than corrupting
state — this is intentional, not a bug to work around.


.. _reverse_bridge_limitations_async_thread_pool:

Async calls are bounded by a fixed-size thread pool
--------------------------------------------------------

``PyCallable.callAsync``/``callAsyncWithTimeout`` run on a bounded native
thread pool, not one thread per call. Every call into Python from any of
those threads still has to acquire the single process GIL, so the pool
bounds *concurrency*, not *throughput* — see :doc:`threading_java` for the
full GIL model. Don't assume arbitrarily high fan-out from
async calls scales linearly; it's still gated by one interpreter's GIL
underneath.
