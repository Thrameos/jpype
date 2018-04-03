/*****************************************************************************
   Copyright 2004-2008 Steve Menard
   
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#ifndef JP_PRIMITIVE_UTILS_H__
#define JP_PRIMITIVE_UTILS_H__

#include <Python.h>
#include <jpype.h>

typedef unsigned int uint;

#ifdef HAVE_NUMPY
    #define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
    #define NO_IMPORT_ARRAY
    #include <numpy/arrayobject.h>
#else
    #define NPY_BOOL 0
    #define NPY_BYTE 0
    #define NPY_SHORT 0
    #define NPY_INT 0
    #define NPY_INT64 0
    #define NPY_FLOAT32 0
    #define NPY_FLOAT64 0
#endif

#define CONVERSION_ERROR_HANDLE \
PyObject* exe = PyErr_Occurred(); \
if(exe != NULL) \
{\
    stringstream ss;\
    ss <<  "unable to convert element: " << PyUnicode_FromFormat("%R",o) <<\
            " at index: " << i;\
    RAISE(JPypeException, ss.str());\
}

#if (PY_VERSION_HEX >= 0x02070000)
// for python 2.6 we have also memory view available, but it does not contain the needed functions.
#include <jpype_memory_view.h>

template <typename jarraytype, typename jelementtype, typename setFnc>
inline bool
setViaBuffer(jarray array, int start, uint length, PyObject* sequence, setFnc setter) {
    //creates a PyMemoryView from sequence check for typeError,
    // if no underlying py_buff exists.
    if(! PyObject_CheckBuffer(sequence)) {
        return false;
    }

    // ensure memory is contiguous and 'C' ordered, this may involve a copy.
    PyObject* memview = PyMemoryView_GetContiguous(sequence, PyBUF_READ, 'C');
    // this function is defined in jpype_memory_view, but unusable?!
//    PyObject* memview = PyMemoryView_FromObject(sequence);

    // check for TypeError, if no underlying py_buff exists.
    PyObject* err = PyErr_Occurred();
    if (err) {
        PyErr_Clear();
        return false;
    }

    // create a memory view
    Py_buffer* py_buff = PyMemoryView_GET_BUFFER(memview);

    // ensure length of buffer contains enough elements somehow.
    if ((py_buff->len / sizeof(jelementtype)) != length) {
        std::stringstream ss;
        ss << "Underlying buffer does not contain requested number of elements! Has "
           << py_buff->len << ", but " << length <<" are requested. Element size is "
           << sizeof(jelementtype);
        RAISE(JPypeException, ss.str());
    }

    jarraytype a = (jarraytype)array;
    jelementtype* buffer = (jelementtype*) py_buff->buf;
    JPJavaEnv* env = JPEnv::getJava();

    try {
        (env->*setter)(a, start, length, buffer);
    } RETHROW_CATCH( /*cleanup*/ Py_DECREF(py_buff); Py_DECREF(memview); );

    // deallocate py_buff and memview
    Py_DECREF(py_buff);
    Py_DECREF(memview);
    return true;
}
#else
template <typename a, typename b, typename c>
bool setViaBuffer(jarray, int, uint, PyObject*, c) {
    return false;
}
#endif

/**
 * gets either a numpy ndarray or a python list with a copy of the underling java array,
 * containing the range [lo, hi].
 *
 * Parameters:
 * -----------
 * lo = low index
 * hi = high index
 * npy_type = e.g NPY_FLOAT64
 * jtype = eg. jdouble
 * convert = function to convert elements to python types. Eg: PyInt_FromLong
 */
template<typename jtype, typename py_wrapper_func>
inline PyObject* getSlice(jarray array, int lo, int hi, int npy_type,
        py_wrapper_func convert)
{
    jtype* val = NULL;
    jboolean isCopy;
    PyObject* res = NULL;
    uint len = hi - lo;

    try
    {
#ifdef HAVE_NUMPY
        npy_intp dims[] = {len};
        res = PyArray_SimpleNew(1, dims, npy_type);
#else
        res = PyList_New(len);
#endif
        if (len > 0)
        {
            val = (jtype*) JPEnv::getJava()->GetPrimitiveArrayCritical(array, &isCopy);
#ifdef HAVE_NUMPY
            // use typed numpy arrays for results
            memcpy(((PyArrayObject*) res)->data, &val[lo], len * sizeof(jtype));
#else
            // use python lists for results
            for (Py_ssize_t i = lo; i < hi; i++)
                PyList_SET_ITEM(res, i - lo, convert(val[i]));
#endif
            // unpin array
            JPEnv::getJava()->ReleasePrimitiveArrayCritical(array, val, JNI_ABORT);
        }
        return res;
    }
    RETHROW_CATCH(if (val != NULL) { JPEnv::getJava()->ReleasePrimitiveArrayCritical(array, val, JNI_ABORT); });
}

// FIXME this is looking like a jp_pyni function, should be moved there.
inline bool checkValue(PyObject* pyobj, JPClass* cls)
{
	JPyObject obj(pyobj);
	if (obj.isJavaValue())
	{
		const JPValue& value = obj.asJavaValue();
		return (value.getClass() == cls);
	}
	return false;
}

#endif
