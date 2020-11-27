#include <Python.h>
#include <jni.h>
#include <dlfcn.h>
#include "jpype.h"
#include "pyjp.h"
#include "epypj.h"

PyThreadState *s_ThreadState;
PyThreadState *m_State1;
#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT void JNICALL Java_python_lang_PyEngine_start_1
(JNIEnv *env, jobject engine)
{
	try
	{
		// Get Python started
		jclass exc = env->FindClass("java/lang/RuntimeException");
		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		Py_InitializeEx(0);
		PyEval_InitThreads();
		s_ThreadState = PyThreadState_Get();

		// Import the Python side to create the hooks
		PyObject *jpype = PyImport_ImportModule("jpype");
		Py_DECREF(jpype);
		if (jpype == NULL)
		{

			env->ThrowNew(exc, "jpype module not found");
			return;
		}

		// Next install the hooks into the private side.
		PyObject *jpypep = PyImport_ImportModule("_jpype");
		PyJPModule_loadResources(jpypep);
		Py_DECREF(jpypep);
		if (jpypep == NULL)
		{
			env->ThrowNew(exc, "_jpype module not found");
			return;
		}

		// Then attach the private module to the JVM
		JPContext* context = JPContext_global;
		context->attachJVM(env);

		m_State1 = PyEval_SaveThread();
	}	catch (JPypeException& ex)
	{
		// No guarantees that the exception will make it back so print it first
		printf("Exception caught %s\n", ex.getMessage().c_str());

		// Attempt to rethrow it back to Java.
		ex.toJava(JPContext_global);
	}
}

#ifdef __cplusplus
}
#endif