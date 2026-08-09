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
Correctness tests for the phase 3.9 ragged-native list push redesign
(plan/ArrayTransferPhase3.md): JPConversionRaggedSequence's fast path for
a nested Python list of int/long/float/double being pushed into a
multi-dimensional primitive array, at depth >= 2. Covers rectangular and
genuinely ragged (jagged) input at multiple depths/types, the scope
boundary (short/byte/char/boolean must stay on the untouched, existing
path), the mixed-type fallback, and overload disambiguation.
"""

import jpype
from jpype import JArray, JInt, JLong, JFloat, JDouble, JShort, JByte, JChar, JBoolean
import common


def to_nested_list(arr):
    if hasattr(arr, '__len__') and not isinstance(arr, (bytes, str)):
        try:
            return [to_nested_list(x) for x in arr]
        except TypeError:
            return list(arr)
    return arr


class ArrayRaggedPushTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)
        self.DeepBench = jpype.JClass('jpype.benchmark.DeepBench')

    # ---- rectangular round trip, construction path ----

    def testRectangularInt2D(self):
        data = [[1, 2, 3], [4, 5, 6]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularInt3D(self):
        data = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularLong2D(self):
        data = [[1, 2, 3], [2 ** 40, -(2 ** 40), 0]]
        ja = JArray(JLong, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRectangularFloat2D(self):
        data = [[1.5, 2.5], [3.5, 4.5]]
        ja = JArray(JFloat, 2)(data)
        self.assertElementsAlmostEqual(
            [x for row in to_nested_list(ja) for x in row],
            [x for row in data for x in row], places=5)

    def testRectangularDouble3D(self):
        data = [[[1.1, 2.2], [3.3, 4.4]], [[5.5, 6.6], [7.7, 8.8]]]
        ja = JArray(JDouble, 3)(data)
        got = to_nested_list(ja)
        for a, b in zip(
                [x for p in got for r in p for x in r],
                [x for p in data for r in p for x in r]):
            self.assertAlmostEqual(a, b, places=9)

    # ---- rectangular round trip, method-argument-dispatch path (not the
    # JArray(...) constructor -- exercises JPArrayClass::findJavaConversionImpl
    # via a declared array-typed parameter instead) ----

    def testRectangularInt2DAsArgument(self):
        data = [[1, 2], [3, 4]]
        self.assertEqual(self.DeepBench.sum2DIntArray(data), sum(x for row in data for x in row))

    def testRectangularInt3DAsArgument(self):
        data = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]
        expected = sum(x for p in data for r in p for x in r)
        self.assertEqual(self.DeepBench.sum3DIntArray(data), expected)

    # ---- genuinely ragged (jagged) input -- the primary target of this
    # phase, not deferred ----

    def testRaggedInt3DWorkedExample(self):
        # The exact worked example from the phase 3.9 design doc's wire
        # format section.
        data = [[[1, 2], [3]], [[4, 5, 6]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedUnevenLeafLengths(self):
        data = [[1, 2, 3], [4], [5, 6]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedUnevenMidLevelLengths(self):
        data = [[[1, 2], [3, 4], [5, 6]], [[7, 8]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistAtStart(self):
        data = [[], [1, 2], [3]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistInMiddle(self):
        data = [[1, 2], [], [3]]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistAtEnd(self):
        data = [[1, 2], [3], []]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptySublistAtDeeperLevel(self):
        data = [[[1, 2], []], [[3]]]
        ja = JArray(JInt, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedEmptyOuterList(self):
        ja = JArray(JInt, 2)([])
        self.assertEqual(to_nested_list(ja), [])

    def testRaggedSingleElementNextToLarge(self):
        data = [[1], list(range(50))]
        ja = JArray(JInt, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedLong3D(self):
        data = [[[1, 2 ** 40], [3]], [[-(2 ** 40), 5, 6]]]
        ja = JArray(JLong, 3)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testRaggedFloat2D(self):
        data = [[1.5, 2.5, 3.5], [4.5]]
        ja = JArray(JFloat, 2)(data)
        got = to_nested_list(ja)
        for gr, dr in zip(got, data):
            self.assertElementsAlmostEqual(gr, dr, places=5)

    def testRaggedDouble2D(self):
        data = [[1.1], [2.2, 3.3, 4.4]]
        ja = JArray(JDouble, 2)(data)
        got = to_nested_list(ja)
        for gr, dr in zip(got, data):
            self.assertElementsAlmostEqual(gr, dr, places=9)

    def testRaggedAsArgument(self):
        data = [[1, 2], [3]]
        expected = sum(x for row in data for x in row)
        self.assertEqual(self.DeepBench.sum2DIntArray(data), expected)

    # ---- scope boundary: short/byte/char/boolean leaf types must stay on
    # the existing, untouched JPConversionSequence path at every depth --
    # this phase must not touch their behavior at all ----

    def testShortLeafUnaffected(self):
        data = [[1, 2], [3]]
        ja = JArray(JShort, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testByteLeafUnaffected(self):
        data = [[1, 2], [3]]
        ja = JArray(JByte, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testCharLeafUnaffected(self):
        data = [['a', 'b'], ['c']]
        ja = JArray(JChar, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    def testBooleanLeafUnaffected(self):
        data = [[True, False], [True]]
        ja = JArray(JBoolean, 2)(data)
        self.assertEqual(to_nested_list(ja), data)

    # ---- mixed-type fallback: a non-conforming element (bool, or a value
    # of the wrong exact type) partway through disqualifies the whole
    # match for this conversion and falls through to the general path,
    # which must still succeed (or fail) exactly as it does today ----

    def testMixedIntThenBoolFallsBackAndSucceeds(self):
        # bool is a subclass of int but fails PyLong_CheckExact -- must
        # not be silently misencoded as an int; falls back to the general
        # per-element path, which does accept it (bool -> int is a valid
        # implicit conversion there).
        data = [[1, 2], [True, 4]]
        ja = JArray(JInt, 2)(data)
        got = to_nested_list(ja)
        self.assertEqual(got, [[1, 2], [1, 4]])

    def testMixedIntThenFloatRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], [3, 4.5]])

    def testMixedIntThenStringRaises(self):
        with self.assertRaises(TypeError):
            JArray(JInt, 2)([[1, 2], [3, "x"]])

    # ---- overload disambiguation: two candidates differing only in array
    # leaf element type -- confirms matches() correctly qualifies *both*
    # candidates for a ragged plain-int list (no buffer built for either,
    # since that only happens in convert() for the winner). int[][] and
    # long[][] are unrelated Java types (no widening relationship between
    # array types the way there is for the scalar primitives), so two
    # equal-quality (_implicit) matches is genuinely ambiguous -- same
    # behavior JPMethodDispatch::findOverload already gives for any other
    # pair of equally-qualified, unrelated candidates, not something this
    # phase changes. The interesting assertion here is what it is *not*:
    # not a spurious pick, not a crash, not a silently-wrong result.

    def testOverloadResolutionReportsAmbiguity(self):
        data = [[1, 2], [3]]
        with self.assertRaises(TypeError):
            self.DeepBench.overloadArrayType(data)
