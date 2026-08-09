# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************

"""
Tests for JArray.pullTo()/JArray.pushFrom() -- bulk-copy a Java primitive
array's elements out to a caller-supplied writable buffer, and the mirror
operation, bulk-copying a caller-supplied readable buffer's elements into
an existing Java primitive array in place. pullTo was ported from the
`reverse` branch as JArray.copyInto; pushFrom and the naming
(pullTo/pushFrom, matching the J2NI View.pull/push precedent) were added
alongside the byte-order/float16 bulk fast path.
"""

import jpype
from jpype import JArray, JInt, JDouble, JString
import common

try:
    import numpy as np
    has_numpy = True
except ImportError:
    has_numpy = False


class ArrayPullToTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testContiguousFastPath(self):
        values = list(range(100))
        ja = JArray(JInt)(values)
        dest = np.empty(100, dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)

    def testDoubleContiguousFastPath(self):
        values = [i * 1.5 for i in range(50)]
        ja = JArray(JDouble)(values)
        dest = np.empty(50, dtype=np.float64)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)

    def testDestShapeNeedNotMatch(self):
        # Same total element count, different shape -- pullTo only
        # requires the flat element count and item size to line up.
        ja = JArray(JInt)(list(range(12)))
        dest = np.empty((3, 4), dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest.flatten(), list(range(12)))

    def testNonContiguousDest(self):
        # A strided (non-contiguous) destination forces the general
        # (GetPrimitiveArrayCritical + stride-walk) path instead of the
        # single-Get<Type>ArrayRegion fast path.
        values = list(range(20))
        ja = JArray(JInt)(values)
        backing = np.zeros(40, dtype=np.int32)
        dest = backing[::2]
        self.assertFalse(dest.flags['C_CONTIGUOUS'])
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, values)
        # Untouched interleaved elements stay zero.
        np.testing.assert_array_equal(backing[1::2], np.zeros(20))

    def testSteppedSource(self):
        # A sliced (stepped) Java array as the source also forces the
        # general path (m_Step != 1).
        values = list(range(20))
        ja = JArray(JInt)(values)
        sliced = ja[::2]
        dest = np.empty(10, dtype=np.int32)
        sliced.pullTo(dest)
        np.testing.assert_array_equal(dest, values[::2])

    def testSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(5, dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pullTo(dest)

    def testItemSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.float64)
        with self.assertRaises(TypeError):
            ja.pullTo(dest)

    def testNonPrimitiveArrayRaises(self):
        ja = JArray(JString)(["a", "b", "c"])
        dest = np.empty(3, dtype=np.int32)
        with self.assertRaises(TypeError):
            ja.pullTo(dest)

    def testEmptyArray(self):
        ja = JArray(JInt)([])
        dest = np.empty(0, dtype=np.int32)
        ja.pullTo(dest)

    def testReadOnlyDestRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.int32)
        dest.flags.writeable = False
        with self.assertRaises((TypeError, ValueError)):
            ja.pullTo(dest)


class ArrayPushFromTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testContiguousFastPath(self):
        # Matching dtype, native byte order -- RAW_NATIVE, single
        # SetIntArrayRegion call.
        ja = JArray(JInt)(10)
        src = np.arange(10, dtype=np.int32)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testDoubleContiguousFastPath(self):
        ja = JArray(JDouble)(50)
        src = np.arange(50, dtype=np.float64) * 1.5
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testSrcShapeNeedNotMatch(self):
        ja = JArray(JInt)(12)
        src = np.arange(12, dtype=np.int32).reshape(3, 4)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.flatten())

    def testDtypeMismatchFallsBackAndConverts(self):
        # float64 -> int (truncating) is genuine dtype coercion: RAW_NONE,
        # must go through the general per-element converter path.
        ja = JArray(JInt)(8)
        src = np.arange(8, dtype=np.float64) + 0.9
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.astype(np.int32))

    def testByteSwappedMatchingDtype(self):
        # Same numeric kind/width as the target, but non-native byte
        # order -- RAW_SWAPPED.
        native = np.arange(16, dtype=np.int32) - 5
        swapped = native.astype(native.dtype.newbyteorder())
        self.assertNotEqual(swapped.dtype.byteorder, '=')
        ja = JArray(JInt)(16)
        ja.pushFrom(swapped)
        np.testing.assert_array_equal(np.asarray(ja), native)

    def testFloat16ToFloat(self):
        ja = JArray(jpype.JFloat)(20)
        src = (np.arange(20, dtype=np.float32) - 10).astype(np.float16)
        ja.pushFrom(src)
        np.testing.assert_allclose(np.asarray(ja), src.astype(np.float32))

    def testFloat16ToDouble(self):
        ja = JArray(JDouble)(20)
        src = (np.arange(20, dtype=np.float32) - 10).astype(np.float16)
        ja.pushFrom(src)
        np.testing.assert_allclose(np.asarray(ja), src.astype(np.float64))

    def testFloat16ToInt(self):
        ja = JArray(JInt)(10)
        src = np.arange(10, dtype=np.float16) * 3
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src.astype(np.int32))

    def testNonContiguousSrc(self):
        # A strided (non-contiguous) source forces the general
        # (GetPrimitiveArrayCritical + stride-walk) path.
        backing = np.arange(40, dtype=np.int32)
        src = backing[::2]
        self.assertFalse(src.flags['C_CONTIGUOUS'])
        ja = JArray(JInt)(20)
        ja.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja), src)

    def testSteppedDest(self):
        # A sliced (stepped) Java array as the destination forces the
        # general path (m_Step != 1).
        ja = JArray(JInt)(list(range(20)))
        sliced = ja[::2]
        src = np.arange(10, dtype=np.int32) * 100
        sliced.pushFrom(src)
        np.testing.assert_array_equal(np.asarray(ja)[::2], src)
        np.testing.assert_array_equal(np.asarray(ja)[1::2], np.arange(1, 20, 2))

    def testSizeMismatchRaises(self):
        ja = JArray(JInt)(10)
        src = np.arange(5, dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.pushFrom(src)

    def testNonPrimitiveArrayRaises(self):
        ja = JArray(JString)(3)
        src = np.arange(3, dtype=np.int32)
        with self.assertRaises(TypeError):
            ja.pushFrom(src)

    def testEmptyArray(self):
        ja = JArray(JInt)([])
        src = np.empty(0, dtype=np.int32)
        ja.pushFrom(src)

    def testLargeParallelPath(self):
        # Crosses Support.PARALLEL_THRESHOLD_ELEMENTS were it to apply --
        # it doesn't for the flat push/pull path (no per-row concept), but
        # this exercises the fast contiguous path at scale regardless.
        n = 2_000_000
        ja = JArray(JInt)(n)
        src = np.arange(n, dtype=np.int32)
        ja.pushFrom(src)
        dest = np.empty(n, dtype=np.int32)
        ja.pullTo(dest)
        np.testing.assert_array_equal(dest, src)
