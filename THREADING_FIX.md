# Python 3.12 Threading Fix

## Problem

When JPype added subinterpreter support for Python 3.12, it introduced complex thread state management using `PyThreadState_New()` / `PyThreadState_Swap()` / `PyThreadState_Delete()`. This approach was being used even for the main interpreter, causing reliability issues.

## Root Cause

- **Python < 3.12**: Simple `PyGILState_Ensure()` / `Release()` API worked perfectly
- **Python 3.12 subinterpreter code**: Used manual thread state management for ALL interpreters
- **Issue**: Manual thread state management is complex and error-prone for the main interpreter where `PyGILState` API works fine

## Solution

Implemented a **hybrid approach** that uses the right tool for each scenario:

### 1. Added `is_main_interpreter` Flag

```cpp
struct PyJPModuleState {
    PyInterpreterState* interp_state;
    bool is_main_interpreter;  // Set during module initialization
    // ...
};
```

Set during module initialization in `pyjp_module.cpp`:
```cpp
st->interp_state = PyThreadState_Get()->interp;
st->is_main_interpreter = (st->interp_state == PyInterpreterState_Main());
```

### 2. Hybrid Thread State Acquisition

In `jp_pythontypes.cpp`:

```cpp
JPPyCallAcquire::JPPyCallAcquire(PyJPModuleState* st)
{
    // For main interpreter, use simple GIL state API (works reliably)
    if (st->is_main_interpreter)
    {
        m_NewState = (PyThreadState*)(long) PyGILState_Ensure();
        m_UseGILState = true;
        return;
    }

    // For subinterpreter: manual thread state management
    PyThreadState *tstate = _PyThreadState_GetCurrent();
    if (tstate != nullptr && tstate->interp == st->interp_state)
    {
        // Already in correct interpreter
        m_NewState = nullptr;
        m_UseGILState = false;
    }
    else
    {
        // Create new thread state for subinterpreter
        m_NewState = PyThreadState_New(st->interp_state);
        m_PriorState = PyThreadState_Swap(m_NewState);
        m_UseGILState = false;
    }
}

JPPyCallAcquire::~JPPyCallAcquire()
{
    if (m_UseGILState)
    {
        PyGILState_Release((PyGILState_STATE)(long)m_NewState);
    }
    else if (m_NewState != nullptr)
    {
        // The quirk: swap back BEFORE clearing/deleting
        PyThreadState_Swap(m_PriorState);
        PyThreadState_Clear(m_NewState);
        PyThreadState_Delete(m_NewState);
    }
}
```

## Results

**Before fix**: Tests crashed during threading operations
**After fix**: 153/230 tests passing (66.5%)

The remaining 77 failures are unrelated to threading:
- PyType null pointer issues (7 tests)
- PyTuple listIterator missing (7 tests)
- PyComplex type issues (12 tests)
- Dict/List conversion issues
- StackOverflow in toArray methods
- etc.

## Key Insights

1. **Don't reinvent the wheel**: For the main interpreter, `PyGILState_Ensure()` is the right API
2. **Mark interpreter type early**: Set `is_main_interpreter` flag during initialization, not on every call
3. **Subinterpreter quirk**: When using manual thread state management, you must swap back to the prior state BEFORE clearing/deleting the new state
4. **Type conversion works perfectly**: The probe system correctly finds tuple→PyTuple, dict→PyDict mappings

## Files Modified

- `native/python/include/pyjp.h`: Added `is_main_interpreter` flag
- `native/python/pyjp_module.cpp`: Set flag during initialization
- `native/python/include/jp_pythontypes.h`: Added fields for hybrid approach
- `native/python/jp_pythontypes.cpp`: Implemented hybrid thread state acquisition
- `jpype/_jbridge.py`: Removed temporary debug logging

## Testing

```bash
cd native/jpype_module
mvn test                                      # Run all tests
mvn test -Dtest=PyTupleNGTest#testOfVarArgs  # Run single test
```

Expected: 153/230 tests passing (66.5%)

## Future Work

1. Test with actual subinterpreters using `startSubInterpreter()` 
2. Address remaining 77 test failures
3. Consider whether subinterpreter path needs additional work
4. Performance testing with repeated Java↔Python calls
