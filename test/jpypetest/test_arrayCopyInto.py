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
Test for JArray.copyInto() -- bulk-copy a Java primitive array's elements
into a caller-supplied writable buffer (e.g. a preallocated numpy array).
Ported from the `reverse` branch as part of the phase-3 array transfer
consolidation (plan/ArrayTransferPhase3.md).
"""

import jpype
from jpype import JArray, JInt, JDouble, JString
import common

try:
    import numpy as np
    has_numpy = True
except ImportError:
    has_numpy = False


class ArrayCopyIntoTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        if not has_numpy:
            self.skipTest("NumPy not available")

    def testContiguousFastPath(self):
        values = list(range(100))
        ja = JArray(JInt)(values)
        dest = np.empty(100, dtype=np.int32)
        ja.copyInto(dest)
        np.testing.assert_array_equal(dest, values)

    def testDoubleContiguousFastPath(self):
        values = [i * 1.5 for i in range(50)]
        ja = JArray(JDouble)(values)
        dest = np.empty(50, dtype=np.float64)
        ja.copyInto(dest)
        np.testing.assert_array_equal(dest, values)

    def testDestShapeNeedNotMatch(self):
        # Same total element count, different shape -- copyInto only
        # requires the flat element count and item size to line up.
        ja = JArray(JInt)(list(range(12)))
        dest = np.empty((3, 4), dtype=np.int32)
        ja.copyInto(dest)
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
        ja.copyInto(dest)
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
        sliced.copyInto(dest)
        np.testing.assert_array_equal(dest, values[::2])

    def testSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(5, dtype=np.int32)
        with self.assertRaises(ValueError):
            ja.copyInto(dest)

    def testItemSizeMismatchRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.float64)
        with self.assertRaises(TypeError):
            ja.copyInto(dest)

    def testNonPrimitiveArrayRaises(self):
        ja = JArray(JString)(["a", "b", "c"])
        dest = np.empty(3, dtype=np.int32)
        with self.assertRaises(TypeError):
            ja.copyInto(dest)

    def testEmptyArray(self):
        ja = JArray(JInt)([])
        dest = np.empty(0, dtype=np.int32)
        ja.copyInto(dest)

    def testReadOnlyDestRaises(self):
        ja = JArray(JInt)(list(range(10)))
        dest = np.empty(10, dtype=np.int32)
        dest.flags.writeable = False
        with self.assertRaises((TypeError, ValueError)):
            ja.copyInto(dest)
