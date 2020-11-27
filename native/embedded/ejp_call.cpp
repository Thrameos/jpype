#include <Python.h>
#include <jni.h>
#include "jpype.h"
#include "epypj.h"
#include "pyjp.h"
#include "jp_classloader.h"
#include "jp_primitive_accessor.h"

// Wrapper for methods that have different conventions.
// Required for methods which:
//  - Return a borrowed reference.
//  - Steal a reference.
//  - Fail to return a required argument for Java class conventions.
//  - Have overloads the require passing null
//  - Implemented as macros.

static PyObject *PyBytes_FromStringAndSizeE(jobject buffer) // PyInvocation.Unary
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

static PyObject *PyDict_GetItemB(PyObject *dict, PyObject *key)
{
	PyObject *o = PyDict_GetItem(dict, key);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyDict_GetItemStringB(PyObject *dict, const char *key)
{
	PyObject *o = PyDict_GetItemString(dict, key);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyDict_GetItemWithErrorB(PyObject *dict, PyObject *key)
{
	PyObject *o = PyDict_GetItemWithError(dict, key);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyEval_GetBuiltinsB()
{
	PyObject *obj = PyEval_GetBuiltins();
	Py_XINCREF(obj);
	return obj;
}

static PyObject *PyEval_GetGlobalsB()
{
	PyObject *obj = PyEval_GetGlobals();
	Py_XINCREF(obj);
	return obj;
}

static PyObject *PyEval_GetLocalsB()
{
	PyObject *obj = PyEval_GetLocals();
	Py_XINCREF(obj);
	return obj;
}

static PyObject *PyFrame_Interactive(PyObject *globals, PyObject *locals)
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

static PyObject *PyFrame_RunString(PyObject *obj, PyObject *globals, PyObject *locals)
{
	JPContext *context = PyJPModule_getContext();
	JPJavaFrame frame = JPJavaFrame::outer(context);
	JPValue *value = PyJPValue_getJavaSlot(obj);
	return PyRun_String(frame.toStringUTF8((jstring) (value->getJavaObject())).c_str(), Py_file_input, globals, locals);
}

static PyObject *PyFunction_GetAnnotationsB(PyObject *func)
{
	PyObject *o = PyFunction_GetAnnotations(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyFunction_GetClosureB(PyObject *func)
{
	PyObject *o = PyFunction_GetClosure(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyFunction_GetCodeB(PyObject *func)
{
	PyObject *o = PyFunction_GetCode(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyFunction_GetDefaultsB(PyObject *func)
{
	PyObject *o = PyFunction_GetDefaults(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyFunction_GetGlobalsB(PyObject *func)
{
	PyObject *o = PyFunction_GetGlobals(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyFunction_GetModuleB(PyObject *func)
{
	PyObject *o = PyFunction_GetModule(func);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyList_GetItemB(PyObject *func, Py_ssize_t i)
{
	PyObject *o = PyList_GetItem(func, i);
	Py_XINCREF(o);
	return o;
}

static int PyList_SetItemS(PyObject *func, Py_ssize_t i, PyObject *value)
{
	// FIXME what should happen if it fails to steal.
	Py_XINCREF(value);
	int ret = PyList_SetItem(func, i, value);
	if (ret == -1)
	{
		// If we fail to steal the reference then restore the previous count
		Py_XDECREF(value);
	}
	return ret;
}

static int PyMapping_DelItemE(PyObject *self, PyObject *key)
{
	return PyMapping_DelItem(self, key);
}

static int PyMapping_DelItemStringE(PyObject *self, const char *key)
{
	return PyMapping_DelItemString(self, key);
}

static PyObject *PyMethod_FunctionB(PyObject *self)
{
	PyObject *o = PyMethod_Function(self);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyMethod_SelfB(PyObject *self)
{
	PyObject *o = PyMethod_Self(self);
	Py_XINCREF(o);
	return o;
}

static int PyNumber_IntValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLong(obj);
	if (PyFloat_Check(obj))
		return (jint) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

static jlong PyNumber_LongValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jlong) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

static jfloat PyNumber_FloatValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jfloat) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

static jdouble PyNumber_DoubleValue(PyObject* obj)
{
	if (PyLong_Check(obj))
		return PyLong_AsLongLong(obj);
	if (PyFloat_Check(obj))
		return (jdouble) PyFloat_AsDouble(obj);
	PyErr_SetString(PyExc_TypeError, "Bad Object");
	return -1;
}

static PyObject *PyNumber_Power2(PyObject *self, PyObject *a)
{
	return PyNumber_Power(self, a, NULL);
}

static PyObject *PyNumber_InPlacePower2(PyObject *self, PyObject *a)
{
	return PyNumber_InPlacePower(self, a, NULL);
}

static int PyObject_DelAttrE(PyObject *self, PyObject *key)
{
	return PyObject_DelAttr(self, key);
}

static int PyObject_DelAttrStringE(PyObject *self, const char *key)
{
	return PyObject_DelAttrString(self, key);
}

static PyObject *PyTuple_GetItemB(PyObject *self, Py_ssize_t i)
{
	PyObject *o = PyTuple_GetItem(self, i);
	Py_XINCREF(o);
	return o;
}

static PyObject *PyType_Name(PyObject *self)
{
	return PyUnicode_FromString(Py_TYPE(self)->tp_name);
}


#define REGISTER_CALL(X,Y) \
	v[0].j = (jlong) Y; \
	v[1].l = frame.NewObjectA(Long, newLongId, v);\
	v[0].l = frame.NewStringUTF(X); \
	ret = frame.CallObjectMethodA(map, putId, v); \
	 frame.DeleteLocalRef(ret); \
	frame.DeleteLocalRef(v[0].l); \
	frame.DeleteLocalRef(v[1].l)

/**
 * Register C callback hooks for Java.
 *
 * This list must match those in PyMethodInfo.
 *
 * @param frame
 */
void EJP_RegisterCalls(JPJavaFrame &frame)
{
	jclass Long = frame.FindClass("java/lang/Long");
	jmethodID newLongId = frame.GetMethodID(Long, "<init>", "(J)V");
	jclass HashMap = frame.FindClass("java/util/HashMap");
	jmethodID newHashMapId = frame.GetMethodID(HashMap, "<init>", "()V");
	jobject map = frame.NewObjectA(HashMap, newHashMapId, NULL);
	jmethodID putId = frame.GetMethodID(HashMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
	jobject ret;
	jvalue v[2];

	REGISTER_CALL("PyByteArray_FromObject", PyByteArray_FromObject); // PyInvocation.Unary
	REGISTER_CALL("PyBytes_FromStringAndSizeE", PyBytes_FromStringAndSizeE); // PyInvocation.FromJObject
	REGISTER_CALL("PyCallable_Check", PyCallable_Check); // PyInvocation.AsInt
	REGISTER_CALL("PyDictProxy_New", PyDictProxy_New); // PyInvocation.Unary
	REGISTER_CALL("PyDict_Clear", PyDict_Clear); // PyInvocation.Unary
	REGISTER_CALL("PyDict_Contains", PyDict_Contains); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyDict_Copy", PyDict_Copy); // PyInvocation.Unary
	REGISTER_CALL("PyDict_DelItem", PyDict_DelItem); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyDict_DelItemString", PyDict_DelItemString); // PyInvocation.DelStr
	REGISTER_CALL("PyDict_GetItemB", PyDict_GetItemB); // PyInvocation.Binary
	REGISTER_CALL("PyDict_GetItemStringB", PyDict_GetItemStringB); // PyInvocation.GetStr
	REGISTER_CALL("PyDict_GetItemWithErrorB", PyDict_GetItemWithErrorB); // PyInvocation.Binary
	REGISTER_CALL("PyDict_Items", PyDict_Items); // PyInvocation.Unary
	REGISTER_CALL("PyDict_Keys", PyDict_Keys); // PyInvocation.Unary
	REGISTER_CALL("PyDict_Merge", PyDict_Merge); // PyInvocation.IntOperator2
	REGISTER_CALL("PyDict_MergeFromSeq2", PyDict_MergeFromSeq2); // PyInvocation.IntOperator2
	REGISTER_CALL("PyDict_New", PyDict_New); // PyInvocation.NoArgs
	REGISTER_CALL("PyDict_SetDefault", PyDict_SetDefault); // PyInvocation.Ternary
	REGISTER_CALL("PyDict_SetItem", PyDict_SetItem); // PyInvocation.SetObj
	REGISTER_CALL("PyDict_SetItemString", PyDict_SetItemString); // PyInvocation.SetStr
	REGISTER_CALL("PyDict_Size", PyDict_Size); // PyInvocation.AsInt
	REGISTER_CALL("PyDict_Update", PyDict_Update); // PyInvocation.Binary
	REGISTER_CALL("PyDict_Values", PyDict_Values); // PyInvocation.Unary
	REGISTER_CALL("PyEval_GetBuiltinsB", PyEval_GetBuiltinsB); // PyInvocation.NoArgs
	REGISTER_CALL("PyEval_GetGlobalsB", PyEval_GetGlobalsB); // PyInvocation.NoArgs
	REGISTER_CALL("PyEval_GetLocalsB", PyEval_GetLocalsB); // PyInvocation.NoArgs
	REGISTER_CALL("PyFloat_FromDouble", PyFloat_FromDouble); // PyInvocation.FromDouble
	REGISTER_CALL("PyFrame_Interactive", PyFrame_Interactive); // PyInvocation.Binary
	REGISTER_CALL("PyFrame_RunString", PyFrame_RunString); // PyInvocation.Ternary
	REGISTER_CALL("PyFrozenSet_New", PyFrozenSet_New); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetAnnotationsB", PyFunction_GetAnnotationsB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetClosureB", PyFunction_GetClosureB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetCodeB", PyFunction_GetCodeB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetDefaultsB", PyFunction_GetDefaultsB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetGlobalsB", PyFunction_GetGlobalsB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_GetModuleB", PyFunction_GetModuleB); // PyInvocation.Unary
	REGISTER_CALL("PyFunction_SetAnnotations", PyFunction_SetAnnotations); // PyInvocation.Binary
	REGISTER_CALL("PyFunction_SetClosure", PyFunction_SetClosure); // PyInvocation.Binary
	REGISTER_CALL("PyIter_Next", PyIter_Next); // PyInvocation.Unary
	REGISTER_CALL("PyList_Append", PyList_Append); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyList_AsTuple", PyList_AsTuple); // PyInvocation.Unary
	REGISTER_CALL("PyList_GetItemB", PyList_GetItemB); // PyInvocation.BinaryInt
	REGISTER_CALL("PyList_GetSlice", PyList_GetSlice); // PyInvocation.GetSlice
	REGISTER_CALL("PyList_Insert", PyList_Insert); // PyInvocation.SetInt
	REGISTER_CALL("PyList_Reverse", PyList_Reverse); // PyInvocation.AsInt
	REGISTER_CALL("PyList_SetItemS", PyList_SetItemS); // PyInvocation.SetInt
	REGISTER_CALL("PyList_SetSlice", PyList_SetSlice); // PyInvocation.SetSlice
	REGISTER_CALL("PyList_Sort", PyList_Sort); // PyInvocation.AsInt
	REGISTER_CALL("PyLong_FromVoidPtr", PyLong_FromVoidPtr); // PyInvocation.Unary
	REGISTER_CALL("PyMapping_DelItemE", PyMapping_DelItemE); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyMapping_DelItemStringE", PyMapping_DelItemStringE); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyMapping_GetItemString", PyMapping_GetItemString); // PyInvocation.Binary
	REGISTER_CALL("PyMapping_HasKey", PyMapping_HasKey); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyMapping_HasKeyString", PyMapping_HasKeyString); // PyInvocation.DelStr
	REGISTER_CALL("PyMapping_Items", PyMapping_Items); // PyInvocation.Unary
	REGISTER_CALL("PyMapping_Keys", PyMapping_Keys); // PyInvocation.Unary
	REGISTER_CALL("PyMapping_SetItemString", PyMapping_SetItemString); // PyInvocation.SetStr
	REGISTER_CALL("PyMapping_Size", PyMapping_Size); // PyInvocation.AsLong
	REGISTER_CALL("PyMapping_Values", PyMapping_Values); // PyInvocation.Unary
	REGISTER_CALL("PyMemoryView_FromObject", PyMemoryView_FromObject); // PyInvocation.Unary
	REGISTER_CALL("PyMethod_FunctionB", PyMethod_FunctionB); // PyInvocation.Unary
	REGISTER_CALL("PyMethod_New", PyMethod_New); // PyInvocation.Binary
	REGISTER_CALL("PyMethod_SelfB", PyMethod_SelfB); // PyInvocation.Unary
	REGISTER_CALL("PyNumber_Absolute", PyNumber_Absolute); // PyInvocation.Unary
	REGISTER_CALL("PyNumber_Add", PyNumber_Add); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_And", PyNumber_And); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_AsSsize_t", PyNumber_AsSsize_t); // PyInvocation.BinaryToLong
	REGISTER_CALL("PyNumber_Divmod", PyNumber_Divmod); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_DoubleValue", PyNumber_DoubleValue); // PyInvocation.AsDouble
	REGISTER_CALL("PyNumber_FloatValue", PyNumber_FloatValue); // PyInvocation.AsFloat
	REGISTER_CALL("PyNumber_FloorDivide", PyNumber_FloorDivide); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceAdd", PyNumber_InPlaceAdd); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceAnd", PyNumber_InPlaceAnd); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceFloorDivide", PyNumber_InPlaceFloorDivide); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceLshift", PyNumber_InPlaceLshift); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceMatrixMultiply", PyNumber_InPlaceMatrixMultiply); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceMultiply", PyNumber_InPlaceMultiply); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceOr", PyNumber_InPlaceOr); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlacePower", PyNumber_InPlacePower); // PyInvocation.Ternary
	REGISTER_CALL("PyNumber_InPlacePower2", PyNumber_InPlacePower2); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceRemainder", PyNumber_InPlaceRemainder); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceRshift", PyNumber_InPlaceRshift); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceSubtract", PyNumber_InPlaceSubtract); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceTrueDivide", PyNumber_InPlaceTrueDivide); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_InPlaceXor", PyNumber_InPlaceXor); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_IntValue", PyNumber_IntValue); // PyInvocation.AsInt
	REGISTER_CALL("PyNumber_Invert", PyNumber_Invert); // PyInvocation.Unary
	REGISTER_CALL("PyNumber_LongValue", PyNumber_LongValue); // PyInvocation.AsLong
	REGISTER_CALL("PyNumber_Lshift", PyNumber_Lshift); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_MatrixMultiply", PyNumber_MatrixMultiply); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Multiply", PyNumber_Multiply); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Negative", PyNumber_Negative); // PyInvocation.Unary
	REGISTER_CALL("PyNumber_Or", PyNumber_Or); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Positive", PyNumber_Positive); // PyInvocation.Unary
	REGISTER_CALL("PyNumber_Power", PyNumber_Power); // PyInvocation.Ternary
	REGISTER_CALL("PyNumber_Power2", PyNumber_Power2); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Remainder", PyNumber_Remainder); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Rshift", PyNumber_Rshift); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Subtract", PyNumber_Subtract); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_ToBase", PyNumber_ToBase); // PyInvocation.BinaryInt
	REGISTER_CALL("PyNumber_TrueDivide", PyNumber_TrueDivide); // PyInvocation.Binary
	REGISTER_CALL("PyNumber_Xor", PyNumber_Xor); // PyInvocation.Binary
	REGISTER_CALL("PyObject_Call", PyObject_Call); // PyInvocation.Ternary
	REGISTER_CALL("PyObject_DelAttrE", PyObject_DelAttrE); // PyInvocation.Binary
	REGISTER_CALL("PyObject_DelAttrStringE", PyObject_DelAttrStringE); // PyInvocation.GetStr
	REGISTER_CALL("PyObject_DelItem", PyObject_DelItem); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyObject_Dir", PyObject_Dir); // PyInvocation.Unary
	REGISTER_CALL("PyObject_Format", PyObject_Format); // PyInvocation.Binary
	REGISTER_CALL("PyObject_GetAttr", PyObject_GetAttr); // PyInvocation.Binary
	REGISTER_CALL("PyObject_GetAttrString", PyObject_GetAttrString); // PyInvocation.Binary
	REGISTER_CALL("PyObject_GetItem", PyObject_GetItem); // PyInvocation.Binary
	REGISTER_CALL("PyObject_GetIter", PyObject_GetIter); // PyInvocation.Unary
	REGISTER_CALL("PyObject_HasAttr", PyObject_HasAttr); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyObject_HasAttrString", PyObject_HasAttrString); // PyInvocation.DelStr
	REGISTER_CALL("PyObject_Hash", PyObject_Hash); // PyInvocation.AsLong
	REGISTER_CALL("PyObject_IsInstance", PyObject_IsInstance); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyObject_IsSubclass", PyObject_IsSubclass); // PyInvocation.BinaryToInt
	REGISTER_CALL("PyObject_IsTrue", PyObject_IsTrue); // PyInvocation.AsBoolean
	REGISTER_CALL("PyObject_Length", PyObject_Length); // PyInvocation.AsLong
	REGISTER_CALL("PyObject_LengthHint", PyObject_LengthHint); // PyInvocation.IntOperator1
	REGISTER_CALL("PyObject_Not", PyObject_Not); // PyInvocation.AsBoolean
	REGISTER_CALL("PyObject_Repr", PyObject_Repr); // PyInvocation.Unary
	REGISTER_CALL("PyObject_RichCompareBool", PyObject_RichCompareBool); // PyInvocation.IntOperator2
	REGISTER_CALL("PyObject_SetAttr", PyObject_SetAttr); // PyInvocation.Ternary
	REGISTER_CALL("PyObject_SetAttrString", PyObject_SetAttrString); // PyInvocation.SetStr
	REGISTER_CALL("PyObject_SetItem", PyObject_SetItem); // PyInvocation.SetObj
	REGISTER_CALL("PyObject_Size", PyObject_Size); // PyInvocation.AsLong
	REGISTER_CALL("PyObject_Str", PyObject_Str); // PyInvocation.Unary
	REGISTER_CALL("PyObject_Type", PyObject_Type); // PyInvocation.Unary
	REGISTER_CALL("PySequence_Concat", PySequence_Concat); // PyInvocation.Binary
	REGISTER_CALL("PySequence_Contains", PySequence_Contains); // PyInvocation.BinaryToInt
	REGISTER_CALL("PySequence_Count", PySequence_Count); // PyInvocation.BinaryToInt
	REGISTER_CALL("PySequence_DelItem", PySequence_DelItem); // PyInvocation.IntOperator1
	REGISTER_CALL("PySequence_DelSlice", PySequence_DelSlice); // PyInvocation.DelSlice
	REGISTER_CALL("PySequence_GetItem", PySequence_GetItem); // PyInvocation.BinaryInt
	REGISTER_CALL("PySequence_GetSlice", PySequence_GetSlice); // PyInvocation.GetSlice
	REGISTER_CALL("PySequence_InPlaceConcat", PySequence_InPlaceConcat); // PyInvocation.Binary
	REGISTER_CALL("PySequence_InPlaceRepeat", PySequence_InPlaceRepeat); // PyInvocation.BinaryInt
	REGISTER_CALL("PySequence_Index", PySequence_Index); // PyInvocation.BinaryToInt
	REGISTER_CALL("PySequence_List", PySequence_List); // PyInvocation.Unary
	REGISTER_CALL("PySequence_Repeat", PySequence_Repeat); // PyInvocation.BinaryInt
	REGISTER_CALL("PySequence_SetItem", PySequence_SetItem); // PyInvocation.SetInt
	REGISTER_CALL("PySequence_SetSlice", PySequence_SetSlice); // PyInvocation.SetSlice
	REGISTER_CALL("PySequence_Tuple", PySequence_Tuple); // PyInvocation.Unary
	REGISTER_CALL("PySet_Add", PySet_Add); // PyInvocation.Binary
	REGISTER_CALL("PySet_Clear", PySet_Clear); // PyInvocation.Unary
	REGISTER_CALL("PySet_Contains", PySet_Contains); // PyInvocation.AsInt
	REGISTER_CALL("PySet_Discard", PySet_Discard); // PyInvocation.BinaryToInt
	REGISTER_CALL("PySet_New", PySet_New); // PyInvocation.Unary
	REGISTER_CALL("PySet_Pop", PySet_Pop); // PyInvocation.Unary
	REGISTER_CALL("PySlice_New", PySlice_New); // PyInvocation.Ternary
	REGISTER_CALL("PyTuple_GetItemB", PyTuple_GetItemB); // PyInvocation.BinaryInt
	REGISTER_CALL("PyTuple_GetSlice", PyTuple_GetSlice); // PyInvocation.GetSlice
	REGISTER_CALL("PyTuple_Size", PyTuple_Size); // PyInvocation.AsInt
	REGISTER_CALL("PyType_Name", PyType_Name); // PyInvocation.Unary
	REGISTER_CALL("PyUnicode_FromOrdinal", PyUnicode_FromOrdinal); // PyInvocation.FromInt

	// Register with Java
	jclass PyTypeBuilder = frame.getContext()->getClassLoader()->findClass(frame, "org.jpype.python.PyTypeBuilder");
	jmethodID addMethods = frame.GetStaticMethodID(PyTypeBuilder, "addMethods", "(Ljava/util/Map;)V");
	v[0].l = map;
	frame.CallStaticObjectMethodA(PyTypeBuilder, addMethods, v);
}
