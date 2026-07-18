"""Shared Python-side fixture for the jpype-vs-JEP array-transfer bake-off.

Triggered from Java on both sides (jpype's reverse bridge, JEP's
SharedInterpreter) - the actual array work always happens here, in Python,
so which side calls in doesn't change what's being measured.
"""
import numpy as np

try:
    import jpype
    from jpype import JArray, JDouble
    _HAVE_JPYPE_FORWARD = True
except Exception:
    _HAVE_JPYPE_FORWARD = False


# ---- Model 1: copyInto (bulk-copy, jpype-only fast path) ----

def make_java_double_array(n):
    """Returns a jpype.JArray(JDouble) of n random values - the 'Java has
    a real double[]' starting point for the copyInto benchmark."""
    values = np.random.random(n)
    return JArray(JDouble)(values.tolist())


_COPY_INTO_STATE = {}


def setup_copy_into(n):
    """Sets up the array + dest pair once, outside the timed loop - mirrors
    make_java_double_array/make_dest but keeps both ends of the jpype.JArray
    <-> numpy relationship inside this interpreter, since a jpype.JArray
    handed back out to Java as generic PyObject and passed back in loses
    its concrete type (arrives as a plain PyJavaObject, no .copyInto)."""
    values = np.random.random(n)
    _COPY_INTO_STATE['ja'] = JArray(JDouble)(values.tolist())
    _COPY_INTO_STATE['dest'] = np.empty(n, dtype=np.float64)


def copy_into():
    """ja.copyInto(dest) is jpype's bulk-copy fast path
    (native/python/pyjp_array.cpp)."""
    _COPY_INTO_STATE['ja'].copyInto(_COPY_INTO_STATE['dest'])
    return float(_COPY_INTO_STATE['dest'][0])


def naive_list_sum(lst):
    """The only route available to a bridge with no bulk array API at all
    (JEP's comparator here) - element-by-element marshaling."""
    total = 0.0
    for v in lst:
        total += v
    return total


# ---- Model 2: direct-buffer-shared (both sides have *some* answer here) ----

def sum_direct_buffer(buf):
    """buf crosses as a real python buffer-protocol object (jpype: a
    java.nio.DoubleBuffer wrapped by JPBufferType; JEP: a DirectNDArray).
    np.asarray(buf) must be zero-copy for this to be a fair comparison."""
    arr = np.asarray(buf)
    return float(arr.sum())


_DIRECT_BUFFER_STATE = {}


def setup_direct_buffer(buf):
    """Wraps buf as a numpy view exactly once - the fair comparator to
    JEP's DirectNDArray, which is likewise constructed once outside the
    timed loop and reused every call. Wrapping buf fresh on every call
    (the old sum_direct_buffer(buf) benchmark) measures jpype's per-call
    PyJPBuffer wrapper construction, not the cost of sharing the memory."""
    _DIRECT_BUFFER_STATE['arr'] = np.asarray(buf)


def sum_direct_buffer_shared():
    return float(_DIRECT_BUFFER_STATE['arr'].sum())


# ---- Model 3: slicing (zero-copy view, not a copy) ----

_SLICE_SOURCE = {}


def make_slice_source(n):
    _SLICE_SOURCE['arr'] = np.random.random(n)
    return n


def sum_slice(step):
    arr = _SLICE_SOURCE['arr']
    return float(arr[::step].sum())


_JAVA_ARRAY_SLICE_STATE = {}


def setup_java_array_slice(n):
    """Same round-trip-avoidance as setup_copy_into: build the JArray once
    inside this interpreter rather than passing a jpype-originated object
    back out to Java and back in."""
    values = np.random.random(n)
    _JAVA_ARRAY_SLICE_STATE['ja'] = JArray(JDouble)(values.tolist())


def sum_java_array_slice(step):
    """ja[::step] is a real Python slice object over the wrapped Java
    array; copyInto on the slice is still jpype's bulk-copy fast path."""
    ja = _JAVA_ARRAY_SLICE_STATE['ja']
    sliced = ja[::step]
    dest = np.empty(len(sliced), dtype=np.float64)
    sliced.copyInto(dest)
    return float(dest.sum())


# ---- Model 4: multidimensional bulk transfer ----

def sum_2d_bulk(ja2d):
    """ja2d is a jpype JArray(JDouble, 2) crossed from a real Java
    double[][] - np.asarray() takes jpype's single bulk buffer-protocol
    call for the whole matrix (Support.collectRectangular)."""
    arr = np.asarray(ja2d)
    return float(arr.sum())


def sum_2d_looped(rows):
    """The only route for a bridge with no multidim accelerator: a
    Python-level loop over per-row transfers (mirrors jpy's approach in
    the original SESSION_REPORT, and JEP's only option too)."""
    total = 0.0
    for row in rows:
        total += sum(row)
    return total
