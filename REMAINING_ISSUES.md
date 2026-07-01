# Remaining Test Failures (77 tests)

## Current Status: 153/230 tests passing (66.5%)

Threading and type conversion are working. The remaining failures are in specific Backend method implementations and edge cases.

## Priority 1: "Return value is not compatible" (25 tests)

These tests are calling Backend methods that return Python objects, but the conversion isn't matching the expected Java type.

**Examples**:
- PyDict tests: getOrDefault, pop, popItem, putAndGet, etc.
- PyTuple tests: testOfIterator, testOfVarArgs, testStream
- PyList tests: various operations

**Root Cause**: Backend methods returning Python objects that the probe system isn't correctly converting to the expected Java interface types. This might be:
1. Missing type mappings in concreteDict
2. Methods returning wrong Python types
3. Java expecting more specific interfaces than we're providing

**Investigation needed**:
- Add logging to see what types Backend methods are actually returning
- Check if probe is being called for these return values
- Verify the Java side is asking for the right types

## Priority 2: PyComplex "'float' object is not callable" (12 tests)

**Error**: `python.exceptions.PyTypeError: 'float' object is not callable`

**Tests**: All PyComplexNGTest tests (testAddDouble, testCreateComplexNumber, testConjugate, etc.)

**Backend implementation**:
```python
"newComplex": lambda r,i: complex(r,i),
```

**Root Cause**: Likely a name collision or scoping issue where `complex` is being shadowed by `float` somehow. Need to check:
1. If `complex` is being imported/defined correctly
2. If there's a namespace collision
3. Try using `__builtins__.complex` explicitly

**Fix**: Change to `lambda r,i: __builtins__.complex(r,i)` or similar

## Priority 3: listIterator missing (7 tests)

**Error**: `java.lang.NoSuchMethodError: listIterator`

**Tests**: All PyTupleIteratorNGTest tests

**Root Cause**: PyTuple doesn't implement `listIterator()` method that Java List interface expects.

**Options**:
1. Implement listIterator() in Python PyTuple wrapper
2. Add it to the Java PyTuple interface
3. Make tests not expect listIterator for tuple (since tuples aren't lists in Python)

**Note**: This might be a test issue rather than implementation issue - tuples aren't lists and shouldn't have listIterator

## Priority 4: PyType null pointer (6 tests)

**Error**: `java.lang.NullPointerException: Cannot invoke "python.lang.PyType.getMethod(String)" because "type" is null`

**Tests**: PyTypeNGTest (testGetBase, testGetBases, testGetMethod, testGetName, testIsSubclassOf, testGetSubclasses)

**Root Cause**: Backend is returning null for type-related operations, or type objects aren't being created properly.

**Investigation needed**:
- Check Backend methods for type operations
- See if `_PyType` mapping is correct
- Verify type objects are being created/returned

## Priority 5: Argument mismatch errors (4 tests)

**Examples**:
- `initialize.<locals>._delitem_return() takes 2 positional arguments but 3 were given`
- `initialize.<locals>.<lambda>() takes 0 positional arguments but 1 was given`  
- `decode() argument 'encoding' must be str, not java.lang.String`

**Root Cause**: Backend method signatures don't match what Java is calling

**Fix**: Update lambda signatures in `_PyJPBackendMethods` dict to match Java expectations

## Priority 6: StackOverflow in toArray (2+ tests)

**Tests**: PyDict/PyList toArray methods

**Root Cause**: Recursive conversion causing infinite loop

**Fix**: Need to implement proper toArray that doesn't recurse infinitely

## Priority 7: Miscellaneous (remaining tests)

- PyZip returning wrong format
- PyEnumerate not iterable
- Various expected vs actual value mismatches
- Expected exception type mismatches

## Testing Strategy

1. **Fix PyComplex first** (12 tests) - likely simple fix
2. **Investigate "Return value" errors** - add logging to understand what's happening
3. **Fix argument mismatches** - update Backend method signatures
4. **Address PyType null** - fix type object creation/return
5. **Handle listIterator** - decide if this is a real issue or test assumption problem
6. **Fix StackOverflow** - implement proper toArray
7. **Clean up miscellaneous** - one by one

## Debug Commands

```bash
# Run specific test class
mvn test -Dtest=PyComplexNGTest

# Run specific test method
mvn test -Dtest=PyComplexNGTest#testCreateComplexNumber

# See test output
cat target/surefire-reports/*.dumpstream | grep -A5 -B5 "ERROR"
```

## Next Session Plan

1. Add strategic logging to Backend method calls to see what's being returned
2. Fix PyComplex issue (likely quick win)
3. Systematically work through the categories above
4. Goal: Get to 200+/230 tests passing (87%+)
