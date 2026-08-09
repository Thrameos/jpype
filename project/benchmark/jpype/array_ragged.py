"""Ragged (jagged) nested-list push, JPype side, 2D through 5D --
plan/ArrayTransferPhase3.md phase 3.9's primary target, and specifically
the case array_multidim.py's `nested_list()` never exercises (every
sibling length there is fixed at 10, so before phase 3.9 there was no
recorded baseline anywhere for genuinely irregular input). Companion to
array_multidim.py; same DeepBench.sum{2,3,4,5}DIntArray push entry point,
but built from a tree whose branching factor varies at every level (fixed
seed, so "before" and "after" runs against the same commit produce
identical trees and therefore a fair per-call comparison) instead of a
uniform 10-wide rectangular one.

Before phase 3.9, JPConversionSequence handled ragged and rectangular
input identically (no shape inspection at all, just per-element
recursion) -- so the pre-3.9 numbers for a ragged tree of comparable
total size are expected to land close to array_multidim.py's own
pre-3.9 rectangular baseline, not a separate code path. Run this script
before and after phase 3.9 (`git checkout <commit>`, rebuild, per
CLAUDE.md) to confirm that directly rather than assume it.
"""
import sys
import os
import random

sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))
from _common import timeit, format_row

import jpype

jpype.startJVM(classpath=['test/classes', 'test/harness'])

DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

DIMS = [2, 3, 4, 5]
SUM_BY_DIMS = {
    2: DeepBench.sum2DIntArray,
    3: DeepBench.sum3DIntArray,
    4: DeepBench.sum4DIntArray,
    5: DeepBench.sum5DIntArray,
}


def nested_list_ragged(dims, avg_n, seed=0):
    """Sibling lengths vary uniformly in [avg_n-4, avg_n+4] at every
    level (including the leaf level) -- genuinely ragged at every depth,
    not just the outermost. Fixed seed so repeated runs (e.g. before vs.
    after a code change) build the exact same tree."""
    rng = random.Random(seed)

    def build(d):
        n = rng.randint(max(1, avg_n - 4), avg_n + 4)
        if d == 1:
            return list(range(n))
        return [build(d - 1) for _ in range(n)]
    return build(dims)


def count_elements(node, dims):
    if dims == 1:
        return len(node)
    return sum(count_elements(child, dims - 1) for child in node)


def calls_for(total_elements):
    n = max(20, 5_000_000 // total_elements)
    warmup = max(5, n // 10)
    return n, warmup


def run(name, fn, total_elements):
    n, warmup = calls_for(total_elements)
    best, median = timeit(fn, n=n, warmup=warmup)
    print(format_row(name, best, median))


print("=== JPype: ragged list->array, multi-dimensional, push (Python -> Java) ===")
for dims in DIMS:
    lst = nested_list_ragged(dims, 10, seed=dims)
    size = count_elements(lst, dims)
    sumfn = SUM_BY_DIMS[dims]
    print(f"  (dims={dims}, actual element count={size})")
    run(f"ragged list->array int{'[]' * dims}(~10^{dims}), fresh",
        lambda lst=lst, sumfn=sumfn: sumfn(lst), size)

jpype.shutdownJVM()
