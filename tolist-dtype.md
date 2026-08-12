# tolist() Improvements Plan

## Summary

Make `JArray.tolist()` return plain Python `int`/`float` by default instead of tagged objects like `JInt`/`JDouble`, with an optional `dtype` argument to specify wrapper types.

## Key Decisions

1. **Backwards compatibility**: `tolist()` will be changed to return plain Python types by default
2. **dtype argument**: Optional `dtype` parameter accepts `[int, float, JByte, JShort, JInt, JLong, JFloat, JDouble]`
3. **Scope**: Only 1D primitive arrays for now (multi-dim returns nested lists of plain Python types)
4. **Forced cast**: When dtype differs from array type, perform forced cast (like NumPy)

## Current Status

**Status: Implementation complete, tests passing** (16/18 tests pass, 2 tests failing due to dtype=int/float handling)

### Completed Changes

1. **Modified `JPPrimitiveType::getArrayRange()`** - `native/common/jp_primitivetype.cpp`
   - Added dtype parameter to control conversion behavior
   - Fast path for plain Python output when dtype matches or is None
   - Forced cast when dtype differs from array type

2. **Added `convertPrimitiveValue()` helper** - `native/common/jp_primitivetype.cpp`
   - Converts jvalue between primitive types (forced cast)

3. **Modified `JPArray::toList()`** - `native/common/jp_array.cpp`
   - Passes dtype to `getArrayRange()` for primitive arrays
   - Recursively passes dtype to nested `toList()` for multi-dim arrays

4. **Modified Python C Extension** - `native/python/pyjp_array.cpp`
   - Updated `PyJPArray_toList()` to accept optional dtype keyword argument
   - Added `parseDtypeArg()` helper to parse dtype argument
   - Updated method flags from `METH_NOARGS` to `METH_VARARGS | METH_KEYWORDS`

5. **Updated Python Wrapper** - `jpype/_jarray.py`
   - Removed tolist stub to avoid overriding C implementation

### Remaining Issues

**Issue:** `dtype=int` and `dtype=float` not working correctly

When `dtype=int` is specified on a `double[]` array, the test expects:
- `dtype=int` → cast elements to int and return plain Python ints
- `dtype=float` → cast elements to float and return plain Python floats

Currently, `parseDtypeArg(int)` and `parseDtypeArg(float)` return `nullptr`, which means "no casting, return plain Python types". But the tests expect these to trigger a forced cast operation.

The fix needed is to return `nullptr` from `parseDtypeArg` when `int` or `float` is passed, and modify `getArrayRange` to handle the cast-to-plain-Python case specially. When dtype is `float` or `int`, we should:
1. Cast elements to the target type (float or int)
2. Return plain Python types (not wrapped)

The distinction is:
- `dtype=float` → cast to float, return plain Python `float`
- `dtype=JFloat` → cast to float, return `JFloat` wrapper
- `dtype=None` → no cast, return plain Python type based on source

### dtype Argument
Allow specifying a wrapper type as a forced cast operation:

```python
# Default: returns plain Python types
arr = JArray(JInt)([1, 2, 3])
arr.tolist()  # Returns [1, 2, 3] (Python ints)

# dtype=JDouble: forced cast to JDouble
arr.tolist(dtype=JDouble)  # Returns [JDouble(1), JDouble(2), JDouble(3)]

# dtype=JInt: forced cast to JInt (truncates floats)
arr = JArray(JDouble)([1.5, 2.7])
arr.tolist(dtype=JInt)  # Returns [JInt(1), JInt(2)]

# dtype=int/float (Python types, not wrapped)
arr.tolist(dtype=int)  # Returns plain Python ints
arr.tolist(dtype=float)  # Returns plain Python floats
```

### Type Mapping

| dtype argument | Result type |
|---------------|-------------|
| `None` (default) | Plain Python int/float/bool/str |
| `int` | Plain Python int |
| `float` | Plain Python float |
| `JByte` | `JByte` wrapper |
| `JShort` | `JShort` wrapper |
| `JInt` | `JInt` wrapper |
| `JLong` | `JLong` wrapper |
| `JFloat` | `JFloat` wrapper |
| `JDouble` | `JDouble` wrapper |

### Forced Cast Rules

When dtype differs from array type (forced cast, NumPy-style):
- `JByte/JShort/JInt/JLong` → `JFloat/JDouble`: widening, exact
- `JFloat/JDouble` → `JByte/JShort/JInt/JLong`: narrowing, truncates
- `JFloat` → `JDouble`: widening, exact
- `JDouble` → `JFloat`: may lose precision
- `JChar` → numeric: converts char code to int
- numeric → `JChar`: converts int to char (may be lossy)
- `JBoolean` ↔ numeric: not allowed (TypeError)

## Implementation Plan

### 1. Modify C++ `JPPrimitiveType::getArrayRange()`

**File:** `native/common/jp_primitivetype.cpp`

Add dtype parameter to control conversion behavior:

```cpp
// Updated method signature
JPPyObject getArrayRange(JPJavaFrame& frame, jarray a, jsize start, jsize step, jsize len, 
                         JPClass* dtype = nullptr);
```

When `dtype == nullptr` (default - fast path):
- Return plain Python objects directly
- For integers (B,S,I,J): use `PyLong_FromLongLong()`
- For booleans (Z): use `PyBool_FromLong()`
- For chars (C): use `PyUnicode_FromFormat("%c", val)`
- For floats (F,D): use `PyFloat_FromDouble()`

When `dtype != nullptr`:
- If dtype matches component type: use existing `convertToPythonObject()`
- If dtype differs: convert value to target type then box with dtype (forced cast)

### 2. Add Type Promotion/Conversion Helpers

**File:** `native/common/include/jp_primitivetype.h` or `jp_primitive_accessor.h`

```cpp
// Convert a jvalue from one primitive type to another (forced cast)
jvalue convertPrimitiveValue(char srcCode, jvalue srcVal, char dstCode);
```

### 3. Modify `JPArray::toList()`

**File:** `native/common/jp_array.cpp`

```cpp
JPPyObject JPArray::toList(JPClass* dtype = nullptr)
{
    auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
    if (compType != nullptr)
    {
        JPJavaFrame frame = JPJavaFrame::outer();
        return compType->getArrayRange(frame, m_Object.get(), m_Start, m_Step, m_Length, dtype);
    }
    
    // Object[] - recurse into nested Java arrays
    JPPyObject list = JPPyObject::call(PyList_New(m_Length));
    for (jsize i = 0; i < m_Length; ++i)
    {
        JPPyObject item = getItem(i);
        if (item.get() != nullptr && PyObject_IsInstance(item.get(), (PyObject*) PyJPArray_Type))
        {
            // Recursively call toList with same dtype
            item = ((PyJPArray*) item.get())->m_Array->toList(dtype);
        }
        PyList_SET_ITEM(list.get(), i, item.keep());
    }
    return list;
}
```

### 4. Modify Python C Extension

**File:** `native/python/pyjp_array.cpp`

Update `toList` method to accept optional dtype argument:

```cpp
static PyObject *PyJPArray_toList(PyJPArray *self, PyObject *args, PyObject *kwargs)
{
    static const char *kwlist[] = {"dtype", nullptr};
    PyObject *dtype_obj = nullptr;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:tolist", (char**)kwlist, &dtype_obj))
        return nullptr;
    
    JP_PY_TRY("PyJPArray_toList");
    if (self->m_Array == nullptr)
        JP_RAISE(PyExc_ValueError, "Null array");
    
    JPClass* dtype = nullptr;
    if (dtype_obj != nullptr) {
        dtype = parseDtypeArg(dtype_obj);
        if (dtype == nullptr)
            return nullptr;
    }
    
    return self->m_Array->toList(dtype).keep();
    JP_PY_CATCH(nullptr);
}

// Helper to parse dtype argument
// Accepts: int, float, JByte, JShort, JInt, JLong, JFloat, JDouble
static JPClass* parseDtypeArg(PyObject* dtype_obj)
{
    JPJavaFrame frame = JPJavaFrame::outer();
    
    // Handle Python int/float types (requesting plain Python output)
    if (dtype_obj == (PyObject*)&PyLong_Type)
        return nullptr;  // plain int
    if (dtype_obj == (PyObject*)&PyFloat_Type)
        return nullptr;  // plain float
    
    // Check if it's a jpype primitive type (JInt, JDouble, etc.)
    if (PyObject_IsInstance(dtype_obj, (PyObject*) PyJPClass_Type))
    {
        auto* cls = (PyJPClass*) dtype_obj;
        JPClass* jc = cls->m_Class;
        
        // Check if this is a primitive wrapper class
        if (jc->isJavaClass())
        {
            JPClass* compType = jc->getComponentType();
            if (compType && compType->isPrimitive())
            {
                return jc;
            }
        }
        // Also check if it's directly a primitive class
        if (jc->isPrimitive())
        {
            return jc;
        }
    }
    
    JP_RAISE(PyExc_TypeError, "dtype must be int, float, or a jpype primitive type (JByte, JShort, JInt, JLong, JFloat, JDouble)");
    return nullptr;
}
```

Update docstring:

```cpp
static const char *toList_doc =
        "Convert this array into a genuine Python list.\n"
        "\n"
        "For an array of primitives this is a bulk conversion (one JNI\n"
        "critical section for the whole array rather than one JNI call per\n"
        "element via ``list(arr)``). Multi-dimensional arrays produce\n"
        "genuinely nested lists.\n"
        "\n"
        "By default, primitive arrays return plain Python types (int, float,\n"
        "bool, str) for maximum performance. Use the ``dtype`` argument to\n"
        "specify a wrapper type (e.g., JInt, JDouble) to get tagged objects\n"
        "instead. The dtype is applied as a forced cast to each element.\n"
        "\n"
        "Args:\n"
        "    dtype: Optional type for elements. Can be:\n"
        "           - int or float for plain Python types\n"
        "           - JByte, JShort, JInt, JLong, JFloat, JDouble for wrapped types\n"
        "           If not specified, plain Python types are returned.\n"
        "\n"
        "Returns:\n"
        "    A Python list with elements as plain Python types or wrapped types.";
```

Update the method definition:

```cpp
static PyMethodDef arrayMethods[] = {
    // ... other methods
    {"tolist", (PyCFunction) (&PyJPArray_toList), METH_VARARGS | METH_KEYWORDS, (toList_doc)},
    // ...
};
```

### 5. Update Python Wrapper

**File:** `jpype/_jarray.py`

Add type hints and update docstring:

```python
from typing import Optional, Union, List, Type

def tolist(self, dtype: Optional[Union[Type[int], Type[float], 
                                        JByte, JShort, JInt, JLong, JFloat, JDouble]] = None) -> List:
    """Convert this array into a genuine Python list.
    
    For an array of primitives this is a bulk conversion (one JNI
    critical section for the whole array rather than one JNI call per
    element via ``list(arr)``). Multi-dimensional arrays produce
    genuinely nested lists.
    
    By default, primitive arrays return plain Python types (int, float,
    bool, str) for maximum performance. Use the ``dtype`` argument to
    specify a wrapper type (e.g., JInt, JDouble) to get tagged objects
    instead.
    
    Args:
        dtype: Optional type for elements. Can be:
               - int or float for plain Python types
               - JByte, JShort, JInt, JLong, JFloat, JDouble for wrapped types
               If not specified, plain Python types are returned.
    
    Returns:
        A Python list with elements as plain Python types or wrapped types.
    
    Examples:
        >>> arr = JArray(JInt)([1, 2, 3])
        >>> arr.tolist()
        [1, 2, 3]
        >>> arr.tolist(dtype=JDouble)
        [JDouble(1), JDouble(2), JDouble(3)]
        >>> arr.tolist(dtype=int)
        [1, 2, 3]
    """
```

### 6. Update test_arrayToList.py

Add tests for new behavior:

```python
def testDefaultReturnsPlainPythonTypes(self):
    """Test that default tolist() returns plain Python types."""
    # Integer types
    for jtype, pytype in [(JBoolean, bool), (JByte, int), (JShort, int),
                          (JInt, int), (JLong, int)]:
        ja = JArray(jtype)([1, 0, 1])
        out = ja.tolist()
        self.assertIsInstance(out, list)
        self.assertTrue(all(isinstance(x, pytype) for x in out))
    
    # Float types
    for jtype, pytype in [(JFloat, float), (JDouble, float)]:
        ja = JArray(jtype)([1.5, 2.5])
        out = ja.tolist()
        self.assertTrue(all(isinstance(x, pytype) for x in out))

def testDtypeJDouble(self):
    """Test dtype=JDouble returns JDouble wrappers."""
    ja = JArray(JInt)([1, 2, 3])
    out = ja.tolist(dtype=JDouble)
    self.assertTrue(all(isinstance(x, _jpype._JDouble) for x in out))
    self.assertEqual([x for x in out], [1.0, 2.0, 3.0])

def testDtypeJInt(self):
    """Test dtype=JInt returns JInt wrappers."""
    ja = JArray(JDouble)([1.5, 2.7])
    out = ja.tolist(dtype=JInt)
    self.assertTrue(all(isinstance(x, _jpype._JInt) for x in out))
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
```

## Files to Modify

1. `native/common/include/jp_primitivetype.h` - Update `getArrayRange()` signature
2. `native/common/jp_primitivetype.cpp` - Implement fast path and dtype handling
3. `native/common/include/jp_array.h` - Update `toList()` signature
4. `native/common/jp_array.cpp` - Update implementation
5. `native/python/pyjp_array.cpp` - Update Python binding with dtype
6. `jpype/_jarray.py` - Update docstring and type hints
7. `test/jpypetest/test_arrayToList.py` - Add new tests

## Testing

### Unit Tests
1. Test default behavior returns plain Python types (no wrappers)
2. Test `dtype=JDouble` returns JDouble wrappers with correct values
3. Test `dtype=JInt` returns JInt wrappers (truncating floats)
4. Test `dtype=int` returns plain Python ints
5. Test `dtype=float` returns plain Python floats
6. Test cross-type dtype with forced cast (JInt → JDouble, JDouble → JInt)
7. Test with sliced arrays (stepped slices, reversed)
8. Test with multi-dimensional arrays (returns nested plain Python lists)
9. Test with empty arrays

### Benchmark
Add new benchmark file: `project/benchmark/jpype/array_to_list_dtype.py`

Compare:
- `list(arr)` - current iteration (slowest)
- `arr.tolist()` - default, plain Python types (fastest)
- `arr.tolist(dtype=JInt)` - with wrapper (medium)
- `arr.tolist(dtype=int)` - plain Python (fastest)
