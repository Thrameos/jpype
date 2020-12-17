#include <jni.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_boxedtype.h"
#include "epypj.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

static void releasePython(void* host)
{
	Py_XDECREF((PyObject*) host);
}

/*
 * Class:     org_jpype_python_internal_PyConstructor
 * Method:    init
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_python_internal_PyConstructor_init
(JNIEnv *, jclass)
{
	return (jlong) &releasePython;
}

/*
 * Class:     org_jpype_python_internal_PyConstructor
 * Method:    incref
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_jpype_python_internal_PyConstructor_incref
(JNIEnv *, jclass, jlong l)
{
	Py_INCREF((PyObject*) l);
}

static jsize toArg(vector<JPPyObject>& args, JPJavaFrame& frame,
		jobjectArray elements, jint begin, jint end)
{
	args.reserve(end - begin);
	for (jsize i = begin; i < end; ++i)
	{
		jobject e = frame.GetObjectArrayElement(elements, i);
		args.push_back(EJP_ToPython(frame, e));
		frame.DeleteLocalRef(e);
	}
	return end - begin;
}

/*
 * Class:     python_lang_PyDict
 * Method:    _ctor
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyDict__1ctor
(JNIEnv *env, jclass, jobject map)
{
	EJP_TRACE_JAVA_IN("dict::new");
	if (map == NULL)
		return (jlong) PyDict_New();
	JPPyObject obj = JPPyObject::call(PyDict_New());
	JPPyObject param1 = EJP_ToPython(frame, map);
	PyDict_Merge(obj.get(), param1.get(), true);
	return (jlong) obj.keep(); // Dangling reference for Java
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PyFloat
 * Method:    _ctor
 * Signature: (D)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyFloat__1ctor
(JNIEnv *env, jclass, jdouble d)
{
	EJP_TRACE_JAVA_IN("float::new");
	return (jlong) PyFloat_FromDouble(d);
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PyList
 * Method:    _ctor
 * Signature: ([Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyList__1ctor
(JNIEnv *env, jclass, jobjectArray elements)
{
	EJP_TRACE_JAVA_IN("list::new");
	
	// If we have no elements return an empty list
	if (elements == NULL)
		return (jlong) PyList_New(0);
	
	// Convert the arguments
	jsize sz = frame.GetArrayLength(elements);
	vector<JPPyObject> args;
	toArg(args, frame, elements, 0, sz);
	
	// Copy to the tuple
	JPPyObject obj = JPPyObject::call(PyList_New(sz));
	for (jsize i = 0; i < sz; ++i)
		PyList_SetItem(obj.get(), i, args[i].keep());
	return (jlong) obj.keep(); // Dangling reference for Java
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PyLong
 * Method:    _ctor
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyLong__1ctor
(JNIEnv *env, jclass, jlong d)
{
	EJP_TRACE_JAVA_IN("float::new");
	return (jlong) PyLong_FromLongLong(d);
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PySet
 * Method:    _ctor
 * Signature: ([Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PySet__1ctor
(JNIEnv *env, jclass, jobjectArray map)
{
	EJP_TRACE_JAVA_IN("set::new");
	if (map == NULL)
	{
		JPPyObject obj = JPPyObject::call(PyTuple_New(0));
		return (jlong) PySet_New(obj.get());
	}
	JPPyObject param1 = EJP_ToPython(frame, map);
	JPPyObject obj = JPPyObject::call(PySet_New(param1.get()));
	return (jlong) obj.keep(); // Dangling reference for Java
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PyString
 * Method:    _ctor
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyString__1ctor
(JNIEnv *env, jclass, jstring str)
{
	EJP_TRACE_JAVA_IN("str::new");
	string param = frame.toStringUTF8(str);
	return (jlong) JPPyString::fromStringUTF8(param).keep();
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     python_lang_PyTuple
 * Method:    _ctor
 * Signature: ([Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyTuple__1ctor
(JNIEnv *env, jclass cls, jobjectArray elements, jint begin, jint end)
{
	EJP_TRACE_JAVA_IN("tuple::new");
	if (elements == NULL)
		return (jlong) PyTuple_New(0);
	
	// Verify the size is correct before proceeding (this could be done on the 
	// Java side)
	jsize sz = frame.GetArrayLength(elements);
	if (end > sz || begin > end)
	{
		frame.ThrowNew(context->m_RuntimeException.get(), "Size out of range");
		return 0;
	}
	
	// Convert the arguments into a tuple
	vector<JPPyObject> args;
	sz = toArg(args, frame, elements, begin, end);
	
	// Create a new tuple
	JPPyObject obj = JPPyObject::call(PyTuple_New(sz));
	for (jsize i = 0; i < sz; ++i)
		PyTuple_SetItem(obj.get(), i, args[i].keep());
	return (jlong) obj.keep(); // Dangling reference for Java
	EJP_TRACE_JAVA_OUT(0);
}

#ifdef __cplusplus
}
#endif