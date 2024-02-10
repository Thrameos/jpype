#include <jni.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_boxedtype.h"
#include "epypj.h"
#include <vector>

void EJP_rethrow(JNIEnv *env, JPContext *context)
{
	try
	{
		throw;
	} catch (JPypeException& ex)
	{
		ex.toJava(context);
	} catch (...) // GCOVR_EXCL_LINE
	{
		env->functions->ThrowNew(env, context->m_RuntimeException.get(),
				"unknown error occurred");
	}
}

#ifdef __cplusplus
extern "C" {
#endif

void NotImplemented(JPJavaFrame& frame)
{
	frame.ThrowNew(frame.getContext()->m_RuntimeException.get(), "Flag not implemented");
}

#define FLAGS_ACCEPT 1
#define FLAGS_BORROWED 2

JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeNone
(JNIEnv *env, jclass invoker, jlong entry, jint flags)
{
	EJP_TRACE_JAVA_IN("invoke::none");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)();
	func f = (func) entry;
	JPPyObject out = JPPyObject::accept(f());
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeFromInt
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jint i)
{
	EJP_TRACE_JAVA_IN("invoke::fromInt");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(int i);
	func f = (func) entry;
	JPPyObject out = JPPyObject::accept(f(i));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeFromLong
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jlong i)
{
	EJP_TRACE_JAVA_IN("invoke::fromLong");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(jlong i);
	func f = (func) entry;
	JPPyObject out = JPPyObject::accept(f(i));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeFromDouble
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jdouble i)
{
	EJP_TRACE_JAVA_IN("invoke::fromDouble");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(jdouble i);
	func f = (func) entry;
	JPPyObject out = JPPyObject::accept(f(i));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeFromJObject
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject i)
{
	EJP_TRACE_JAVA_IN("invoke::fromJObject");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(jobject i);
	func f = (func) entry;
	JPPyObject out = JPPyObject::accept(f(i));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeUnary
 * Signature: (JLjava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeUnary
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::unary");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(PyObject * obj);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject out = JPPyObject::accept(f(param1.get()));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeAsBoolean
 * Signature: (JLjava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_jpype_python_internal_PyInvoker_invokeAsBoolean
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::asBoolean");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jboolean(*func)(PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	jboolean ret = f(param1.get());
	JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeAsInt
 * Signature: (JLjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeAsInt
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::asInt");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jint(*func)(PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	jint ret = f(param1.get());
	JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeAsLong
 * Signature: (JLjava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_python_internal_PyInvoker_invokeAsLong
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::asLong");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jlong(*func)(PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	printf("Object 0x%p %s\n", param1.get(), Py_TYPE(param1.get())->tp_name);
	jlong ret = f(param1.get());
	JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeAsFloat
 * Signature: (JLjava/lang/Object;)F
 */
JNIEXPORT jfloat JNICALL Java_org_jpype_python_internal_PyInvoker_invokeAsFloat
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::asFloat");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jfloat(*func)(PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	jfloat ret = f(param1.get());
	JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeAsDouble
 * Signature: (JLjava/lang/Object;)D
 */
JNIEXPORT jdouble JNICALL Java_org_jpype_python_internal_PyInvoker_invokeAsDouble
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::asDouble");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jdouble(*func)(PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	jdouble ret = f(param1.get());
	JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeBinary
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeBinary
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2)
{
	EJP_TRACE_JAVA_IN("invoke::binary");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(PyObject*, PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	JPPyObject out = JPPyObject::accept(f(param1.get(), param2.get()));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeBinaryInt
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeBinaryInt
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint jparam2)
{
	EJP_TRACE_JAVA_IN("invoke::binaryInt");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(PyObject*, jint);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject out = JPPyObject::accept(f(param1.get(), jparam2));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeBinaryToInt
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeBinaryToInt
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2)
{
	EJP_TRACE_JAVA_IN("invoke::binaryToInt");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jint(*func)(PyObject*, PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	jint ret = f(param1.get(), param2.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeBinaryToLong
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_python_internal_PyInvoker_invokeBinaryToLong
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2)
{
	EJP_TRACE_JAVA_IN("invoke::binaryToLong");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jint(*func)(PyObject*, PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	jint ret = f(param1.get(), param2.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeTernary
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeTernary
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2, jobject jparam3)
{
	EJP_TRACE_JAVA_IN("invoke::ternary");
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef PyObject * (*func)(PyObject*, PyObject*, PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	JPPyObject param3 = EJP_ToPython(frame, jparam3);
	JPPyObject out = JPPyObject::accept(f(param1.get(), param2.get(), param3.get()));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeDelSlice
 * Signature: (JLjava/lang/Object;II)Ljava/lang/Object;
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeDelSlice
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint param2, jint param3)
{
	EJP_TRACE_JAVA_IN("invoke::delSlice");
	typedef int (*func)(PyObject*, Py_ssize_t, Py_ssize_t);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	jint ret = f(param1.get(), param2, param3);
	if (ret == -1)
		JP_PY_CHECK()
		return ret;
	EJP_TRACE_JAVA_OUT(-1);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeGetSlice
 * Signature: (JLjava/lang/Object;II)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeGetSlice
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint param2, jint param3)
{
	EJP_TRACE_JAVA_IN("invoke::binary");
	typedef PyObject * (*func)(PyObject*, Py_ssize_t, Py_ssize_t);
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject out = JPPyObject::accept(f(param1.get(), param2, param3));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeSetSlice
 * Signature: (JLjava/lang/Object;IILjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeSetSlice
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint i1, jint i2, jobject jparam2)
{
	EJP_TRACE_JAVA_IN("invoke::setSlice");
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	typedef jint(*func)(PyObject*, Py_ssize_t, Py_ssize_t, PyObject*);
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	jint ret = f(param1.get(), i1, i2, param2.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeDelStr
 * Signature: (JLjava/lang/Object;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeDelStr
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jstring jname)
{
	EJP_TRACE_JAVA_IN("invoke::delStr");
	typedef jint(*func)(PyObject*, const char*);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	string name = frame.toStringUTF8(jname);
	jint ret = f(param1.get(), name.c_str());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeDelStr
 * Signature: (JLjava/lang/Object;Ljava/lang/String;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeGetStr
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jstring jname)
{
	EJP_TRACE_JAVA_IN("invoke::getStr");
	typedef PyObject * (*func)(PyObject*, const char*);
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	string name = frame.toStringUTF8(jname);
	JPPyObject out = JPPyObject::accept(f(param1.get(), name.c_str()));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeSetObj
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeSetObj
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2, jobject jparam3)
{
	EJP_TRACE_JAVA_IN("invoke::ternary");
	typedef int (*func)(PyObject*, PyObject*, PyObject*);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	JPPyObject param3 = EJP_ToPython(frame, jparam3);
	jint ret = f(param1.get(), param2.get(), param3.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeSetStr
 * Signature: (JLjava/lang/Object;Ljava/lang/String;Ljava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeSetStr
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1,
		jstring jparam2, jobject jparam3)
{
	EJP_TRACE_JAVA_IN("invoke::ternary");
	typedef int (*func)(PyObject*, const char*, PyObject*);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	string param2 = frame.toStringUTF8(jparam2);
	JPPyObject param3 = EJP_ToPython(frame, jparam3);
	jint ret = f(param1.get(), param2.c_str(), param3.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeSetInt
 * Signature: (JLjava/lang/Object;ILjava/lang/Object;)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeSetInt
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint jparam2, jobject jparam3)
{
	EJP_TRACE_JAVA_IN("invoke::ternary");
	typedef int (*func)(PyObject*, jint, PyObject*);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param3 = EJP_ToPython(frame, jparam3);
	jint ret = f(param1.get(), jparam2, param3.get());
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeSetIntToObj
 * Signature: (JLjava/lang/Object;ILjava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeSetIntToObj
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint jparam2, jobject jparam3)
{
	EJP_TRACE_JAVA_IN("invoke::ternary");
	typedef PyObject * (*func)(PyObject*, jint, PyObject*);
	if ((flags&~(FLAGS_ACCEPT|FLAGS_BORROWED)) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param3 = EJP_ToPython(frame, jparam3);
	JPPyObject out = JPPyObject::accept(f(param1.get(), jparam2, param3.get()));
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeRichCompare
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;I)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeIntOperator1
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jint op)
{
	EJP_TRACE_JAVA_IN("invoke::intoperator1");
	typedef int (*func)(PyObject*, int);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	int ret = f(param1.get(), op);
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeRichCompare
 * Signature: (JLjava/lang/Object;Ljava/lang/Object;I)I
 */
JNIEXPORT jint JNICALL Java_org_jpype_python_internal_PyInvoker_invokeIntOperator2
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobject jparam1, jobject jparam2, jint op)
{
	EJP_TRACE_JAVA_IN("invoke::intoperator2");
	typedef int (*func)(PyObject*, PyObject*, int);
	if (flags != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	JPPyObject param1 = EJP_ToPython(frame, jparam1);
	JPPyObject param2 = EJP_ToPython(frame, jparam2);
	int ret = f(param1.get(), param2.get(), op);
	if (ret == -1)
		JP_PY_CHECK();
	return ret;
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyInvoker
 * Method:    invokeUnary
 * Signature: (J[Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_org_jpype_python_internal_PyInvoker_invokeArray
(JNIEnv *env, jclass invoker, jlong entry, jint flags, jobjectArray jparam1)
{
	EJP_TRACE_JAVA_IN("invoke::unary");
	typedef PyObject * (*func)(jobjectArray a);
	if ((flags&~2) != 0)
	{
		NotImplemented(frame);
		return 0;
	}
	func f = (func) entry;
	PyObject *ret = f(jparam1);
	if (flags&2)
		Py_XINCREF(ret);
	JPPyObject out = JPPyObject::accept(ret);
	return EJP_ToJava(frame, out.get(), flags);
	EJP_TRACE_JAVA_OUT(NULL);
}

#ifdef __cplusplus
}
#endif
