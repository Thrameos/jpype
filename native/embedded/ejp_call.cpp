#include <Python.h>
#include <jni.h>
#include "jpype.h"
#include "epypj.h"
#include "pyjp.h"
#include "jp_classloader.h"
#include "jp_primitive_accessor.h"

// This file contains methods that we are adding to the Python API to support
// Java.  The goal here will be to reduce these as much as possible.

// FIXME What should be the naming convention for these functions?

// Wrapper for methods that have different conventions.
// Required for methods which:
//  - Steal a reference.
//  - Fail to return a required argument for Java class conventions.
//  - Have overloads the require passing null
//  - Implemented as macros.

extern "C" PyObject *PyBytes_FromStringAndSizeE(jobject buffer) // PyInvocation.Unary
{
	try
	{
		JPContext* context = PyJPModule_getContext();
		JPJavaFrame frame = JPJavaFrame::outer(context);
		if (frame.IsInstanceOf(buffer, context->_java_nio_ByteBuffer->getJavaClass()))
		{
			void *v = frame.GetDirectBufferAddress(buffer);
			Py_ssize_t sz = (Py_ssize_t) frame.GetDirectBufferCapacity(buffer);
			return PyBytes_FromStringAndSize((char*) v, sz);
		}
		JPPrimitiveArrayAccessor<jbyteArray, jbyte*> accessor(frame, (jarray) buffer,
				&JPJavaFrame::GetByteArrayElements, &JPJavaFrame::ReleaseByteArrayElements);
		return PyBytes_FromStringAndSize((char*) accessor.get(), accessor.size());
	}	catch (JPypeException& ex)
	{
		ex.toPython();
	}
	return NULL;
}

extern "C" PyObject *PyFrame_Interactive(PyObject *globals, PyObject *locals)
{
	JP_PY_TRY("PyJPModule_convertToDirectByteBuffer");
	try
	{
		JPPyObject u1 = JPPyObject::call(PyRun_String("import code as _code",
				Py_single_input, globals, locals));
		JPPyObject u2 = JPPyObject::call(PyRun_String("_code.interact(local=locals())",
				Py_single_input, globals, locals));
		return u2.keep();
	}	catch (...)
	{
		if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_SystemExit))
		{
			PyErr_Clear();
			return NULL;
		}
		throw;
	}
	JP_PY_CATCH(NULL);
}

extern "C" PyObject *PyFrame_RunString(PyObject *obj, PyObject *globals, PyObject *locals)
{
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *value = PyJPValue_getJavaSlot(obj);
	string cmd = frame.toStringUTF8((jstring) (value->getJavaObject()));
	return PyRun_String(cmd.c_str(), Py_file_input, globals, locals);
}

extern "C" int PyList_SetItemS(PyObject *func, Py_ssize_t i, PyObject *value)
{
	Py_XINCREF(value);
	return PyList_SetItem(func, i, value);
}

extern "C" int PyNumber_IntValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLong(obj);
	if (PyFloat_Check(obj))
		return (jint) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

extern "C" jlong PyNumber_LongValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jlong) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

extern "C" jfloat PyNumber_FloatValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jfloat) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

extern "C" jdouble PyNumber_DoubleValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jdouble) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

extern "C" PyObject *PyNumber_Power2(PyObject *self, PyObject *a)
{
	return PyNumber_Power(self, a, NULL);
}

extern "C" PyObject *PyNumber_InPlacePower2(PyObject *self, PyObject *a)
{
	return PyNumber_InPlacePower(self, a, NULL);
}

extern "C" int PyObject_DelAttrE(PyObject *self, PyObject *key)
{
	return PyObject_DelAttr(self, key);
}

extern "C" int PyObject_DelAttrStringE(PyObject *self, const char *key)
{
	return PyObject_DelAttrString(self, key);
}

extern "C" PyObject *PyType_Name(PyObject *self)
{
	return PyUnicode_FromString(Py_TYPE(self)->tp_name);
}
