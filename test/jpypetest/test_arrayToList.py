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
Test for JArray.tolist() -- bulk-convert a Java array into a genuine
Python list. Closes the `array->list` pull gap: primitive arrays are
read in one JNI critical section instead of one JNI call per element via
list(arr)/_JavaArrayIter.
"""

import jpype
from jpype import (JArray, JBoolean, JByte, JChar, JShort, JInt, JLong,
                    JFloat, JDouble, JString, JObject)
import common


class ArrayToListTestCase(common.JPypeTestCase):
    def setUp(self):
        common.JPypeTestCase.setUp(self)

    def testEachPrimitiveType(self):
        cases = [
            (JBoolean, [True, False, True, True]),
            (JByte, [1, -1, 127, -128, 0]),
            (JShort, [1, -1, 32767, -32768, 0]),
            (JInt, [1, -1, 2**31 - 1, -2**31, 0]),
            (JLong, [1, -1, 2**62, -2**62, 0]),
            (JFloat, [1.5, -2.5, 0.0]),
            (JDouble, [1.5, -2.5, 0.0, 3.14159]),
        ]
        for jtype, values in cases:
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)(values)
                out = ja.tolist()
                self.assertIsInstance(out, list)
                self.assertEqual(out, list(ja))

    def testChar(self):
        ja = JArray(JChar)("hello")
        self.assertEqual(ja.tolist(), list("hello"))

    def testEmptyArray(self):
        self.assertEqual(JArray(JInt)([]).tolist(), [])

    def testSingleElement(self):
        self.assertEqual(JArray(JInt)([42]).tolist(), [42])

    def testMatchesListConstructor(self):
        values = list(range(200))
        ja = JArray(JInt)(values)
        self.assertEqual(ja.tolist(), list(ja))
        self.assertEqual(ja.tolist(), values)

    def testSteppedSlice(self):
        values = list(range(20))
        ja = JArray(JInt)(values)
        self.assertEqual(ja[::2].tolist(), values[::2])
        self.assertEqual(ja[::3].tolist(), values[::3])
        self.assertEqual(ja[::-1].tolist(), values[::-1])
        self.assertEqual(ja[5:15:2].tolist(), values[5:15:2])

    def testMultiDimRectangular(self):
        rows, cols = 4, 5
        mat = JArray(JInt, 2)(rows)
        expected = []
        for r in range(rows):
            row = [r * cols + c for c in range(cols)]
            mat[r] = JArray(JInt)(row)
            expected.append(row)
        self.assertEqual(mat.tolist(), expected)

    def testMultiDimJagged(self):
        jag = JArray(JInt, 2)(3)
        jag[0] = JArray(JInt)([1, 2, 3])
        jag[1] = JArray(JInt)([4])
        jag[2] = JArray(JInt)([])
        self.assertEqual(jag.tolist(), [[1, 2, 3], [4], []])

    def testThreeDim(self):
        arr = JArray(JInt, 3)(2)
        arr[0] = JArray(JInt, 2)([JArray(JInt)([1, 2]), JArray(JInt)([3, 4])])
        arr[1] = JArray(JInt, 2)([JArray(JInt)([5, 6, 7])])
        self.assertEqual(arr.tolist(), [[[1, 2], [3, 4]], [[5, 6, 7]]])

    def testObjectArray(self):
        strs = JArray(JString)(["a", "b", "c"])
        self.assertEqual(strs.tolist(), ["a", "b", "c"])

    def testObjectArrayWithNulls(self):
        strs = JArray(JString)(3)
        strs[0] = "x"
        self.assertEqual(strs.tolist(), ["x", None, None])

    def testDefaultReturnsPlainPythonTypes(self):
        """Test that default tolist() returns plain Python types."""
        # Integer types
        for jtype, pytype in [(JBoolean, bool), (JByte, int), (JShort, int),
                              (JInt, int), (JLong, int)]:
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)([1, 0, 1])
                out = ja.tolist()
                self.assertIsInstance(out, list)
                self.assertTrue(all(isinstance(x, pytype) for x in out))

        # Float types
        for jtype, pytype in [(JFloat, float), (JDouble, float)]:
            with self.subTest(jtype=jtype):
                ja = JArray(jtype)([1.5, 2.5])
                out = ja.tolist()
                self.assertTrue(all(isinstance(x, pytype) for x in out))

    def testDtypeJDouble(self):
        """Test dtype=JDouble returns JDouble wrappers."""
        ja = JArray(JInt)([1, 2, 3])
        out = ja.tolist(dtype=JDouble)
        self.assertTrue(all(isinstance(x, JDouble) for x in out))
        self.assertEqual([x for x in out], [1.0, 2.0, 3.0])

    def testDtypeJInt(self):
        """Test dtype=JInt returns JInt wrappers."""
        ja = JArray(JDouble)([1.5, 2.7])
        out = ja.tolist(dtype=JInt)
        self.assertTrue(all(isinstance(x, JInt) for x in out))
        self.assertEqual([x for x in out], [1, 2])

    def testDtypeInt(self):
        """Test dtype=int returns plain Python ints."""
        ja = JArray(JDouble)([1.5, 2.7])
        out = ja.tolist(dtype=int)
        self.assertTrue(all(isinstance(x, int) for x in out))
        self.assertEqual(out, [1, 2])

    def testDtypeFloat(self):
        """Test dtype=float returns plain Python floats."""
        ja = JArray(JInt)([1, 2, 3])
        out = ja.tolist(dtype=float)
        self.assertTrue(all(isinstance(x, float) for x in out))
        self.assertEqual(out, [1.0, 2.0, 3.0])

    def testDtypeWithSlices(self):
        """Test dtype works with sliced arrays."""
        values = list(range(20))
        ja = JArray(JInt)(values)
        out = ja[::2].tolist(dtype=JDouble)
        self.assertEqual([x for x in out], [float(x) for x in values[::2]])

    def testMultiDimPlain(self):
        """Test multi-dim returns nested lists of plain Python types."""
        rows, cols = 2, 3
        mat = JArray(JInt, 2)(rows)
        for r in range(rows):
            mat[r] = JArray(JInt)([r * cols + c for c in range(cols)])

        out = mat.tolist()
        expected = [[r * cols + c for c in range(cols)] for r in range(rows)]
        self.assertEqual(out, expected)
