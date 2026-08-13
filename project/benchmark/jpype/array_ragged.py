"""Ragged (jagged) nested-list push, JPype side, 2D through 5D --
specifically the case array_multidim.py's `nested_list()` never
exercises (every sibling length there is fixed at 10, so a genuinely
irregular tree has no baseline there). Companion to array_multidim.py;
same DeepBench.void{2,3,4,5}D{Type}Array push entry point, but built from a
tree whose branching factor varies at every level (fixed seed, so
"before" and "after" runs against the same commit produce identical
trees and therefore a fair per-call comparison) instead of a uniform
10-wide rectangular one. Swept across the same four primitive element
types (int32, int64, float32, float64) as array_flat.py/array_multidim.py.

Writes project/benchmark/jpype/array_ragged_results.csv alongside the
printed output.
"""
import sys
import os
import random

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row, CsvLog

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

DIMS = [2, 3, 4, 5]

TYPES = [
    ('int', {
        2: DeepBench.void2DIntArray, 3: DeepBench.void3DIntArray,
        4: DeepBench.void4DIntArray, 5: DeepBench.void5DIntArray,
    }),
    ('long', {
        2: DeepBench.void2DLongArray, 3: DeepBench.void3DLongArray,
        4: DeepBench.void4DLongArray, 5: DeepBench.void5DLongArray,
    }),
    ('float', {
        2: DeepBench.void2DFloatArray, 3: DeepBench.void3DFloatArray,
        4: DeepBench.void4DFloatArray, 5: DeepBench.void5DFloatArray,
    }),
    ('double', {
        2: DeepBench.void2DDoubleArray, 3: DeepBench.void3DDoubleArray,
        4: DeepBench.void4DDoubleArray, 5: DeepBench.void5DDoubleArray,
    }),
]

csv_log = CsvLog(
    os.path.join(os.path.dirname(__file__), 'array_ragged_results.csv'),
    ['category', 'direction', 'source', 'dtype', 'dims', 'size', 'n', 'best_ns', 'median_ns'])


def nested_list_ragged(dims, avg_n, seed=0, leaf=int):
    """Sibling lengths vary uniformly in [avg_n-4, avg_n+4] at every
    level (including the leaf level) -- genuinely ragged at every depth,
    not just the outermost. Fixed seed so repeated runs (e.g. before vs.
    after a code change) build the exact same tree.

    leaf must produce a genuine (exact-type) Python float for a
    float[]/double[] target -- the ragged-native list-push fast path's
    leaf check (isRaggedLeafElement, jp_classhints.cpp) requires
    PyFloat_CheckExact for F/D leaves, same as PyLong_CheckExact for I/J;
    a plain Python int is valid (Java widens it) but misses this fast
    path and silently falls back to the general per-element path."""
    rng = random.Random(seed)

    def build(d):
        n = rng.randint(max(1, avg_n - 4), avg_n + 4)
        if d == 1:
            return [leaf(i) for i in range(n)]
        return [build(d - 1) for _ in range(n)]
    return build(dims)


def count_elements(node, dims):
    if dims == 1:
        return len(node)
    return sum(count_elements(child, dims - 1) for child in node)


def calls_for(total_elements):
    n = max(30, 6_000_000 // total_elements)
    warmup = max(6, n // 8)
    return n, warmup


def run(name, fn, total_elements, dtype, dims):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))
    csv_log.write(category='array_ragged', direction='push', source='list',
                   dtype=dtype, dims=dims, size=total_elements, n=n,
                   best_ns=best, median_ns=median)


for label, SUM_BY_DIMS in TYPES:
    leaf = float if label in ('float', 'double') else int
    print(f"=== JPype: ragged list->array, multi-dimensional, push (Python -> Java), {label} ===")
    for dims in DIMS:
        lst = nested_list_ragged(dims, 10, seed=dims, leaf=leaf)
        size = count_elements(lst, dims)
        sumfn = SUM_BY_DIMS[dims]
        print(f"  (dims={dims}, actual element count={size})")
        run(f"ragged list->array {label}{'[]' * dims}(~10^{dims}), fresh",
            lambda lst=lst, sumfn=sumfn: sumfn(lst), size, label, dims)

csv_log.close()
jpype.shutdownJVM()
