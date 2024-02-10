#include <Python.h>
#include <jni.h>
#include <dlfcn.h>
#include "jpype.h"
#include "pyjp.h"
#include "epypj.h"

PyThreadState *s_ThreadState;
PyThreadState *m_State1;
#ifdef __cplusplus
extern "C" {
#endif

static void fail(JNIEnv *env, const char* msg)
{
	// This is a low frequency path so we don't need efficiency.
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");
	env->ThrowNew(runtimeException, msg);
}

static void convertException(JNIEnv *env, JPypeException& ex)
{
	// This is a low frequency path so we don't need efficiency.
	// We can't use ex.toJava() because this is part of initialization.
	jclass runtimeException = env->FindClass("java/lang/RuntimeException");

	// If it is a Java exception, we can simply throw it
	if (ex.getExceptionType() == JPError::_java_error)
	{
		env->Throw(ex.getThrowable());
		return;
	}

	// No guarantees that the exception will make it back so print it first
	PyObject *err = PyErr_Occurred();
	if (err != NULL)
	{
		PyErr_Print();
		env->ThrowNew(runtimeException, "Exception in Python");
	} else
	{
		env->ThrowNew(runtimeException, ex.getMessage().c_str());
	}
}

JNIEXPORT void JNICALL Java_org_jpype_python_internal_Native_start
(JNIEnv *env, jobject engine)
{
	try
	{
		// FIXME for testing we need to add "." to the path.
		Py_SetPath(L".:/usr/lib/python36.zip:/usr/lib/python3.6:/usr/lib/python3.6:/usr/lib/python3.6/lib-dynload");

		// Get Python started
		PyImport_AppendInittab("_jpype", &PyInit__jpype);
		Py_InitializeEx(0);
		PyEval_InitThreads();
		s_ThreadState = PyThreadState_Get();

		// Import the Python side to create the hooks
		PyObject *jpype = PyImport_ImportModule("jpype");
		if (jpype == NULL)
		{
			fail(env, "jpype module not found");
			return;
		}
		Py_DECREF(jpype);

		// Next install the hooks into the private side.
		PyObject *jpypep = PyImport_ImportModule("_jpype");
		if (jpypep == NULL)
		{
			fail(env, "_jpype module not found");
			return;
		}
		PyJPModule_loadResources(jpypep);
		Py_DECREF(jpypep);
		
		// Then attach the private module to the JVM
		JPContext* context = JPContext_global;
		context->attachJVM(env);
		
		// Initialize the resources in the jpype module
		PyObject *obj = PyObject_GetAttrString(jpype, "_core");
		PyObject *obj2 = PyObject_GetAttrString(obj, "initializeResources");
		PyObject *obj3 = PyTuple_New(0);
		PyObject_Call(obj2, obj3, NULL);
		Py_DECREF(obj);
		Py_DECREF(obj2);
		Py_DECREF(obj3);
		
		// Everything is up and ready

		// Next, we need to release the state so we can return to Java.
		m_State1 = PyEval_SaveThread();
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during start");
	}
}


// This section will allow us to look up symbols in shared libraries from within
// Java.  We will then call those symbols using the invoker. 

// FIXME global
static list<void*> libraries;

/*
 * Class:     python_lang_PyEngine
 * Method:    getSymbol
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_python_internal_Native_getSymbol
(JNIEnv *env, jobject, jstring str)
{
	try
	{
		jboolean copy;
		const char* name = env->GetStringUTFChars(str, &copy);
		for (std::list<void*>::iterator iter = libraries.begin();
				iter != libraries.end(); iter++)
		{
			void* res = NULL;
#ifdef WIN32
			res = (void*) GetProcAddress(*iter, name.c_str());
#else
			res = dlsym(*iter, name);
#endif
			if (res != NULL)
				return (jlong) res;
		}
		env->ReleaseStringUTFChars(str, name);
		return 0;
	} catch (JPypeException& ex)
	{
		convertException(env, ex);
	}	catch (...)
	{
		fail(env, "C++ exception during getSymbol");
	}
	return 0;
}

/*
 * Class:     python_lang_PyEngine
 * Method:    addLibrary
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_jpype_python_internal_Native_addLibrary
(JNIEnv *env, jobject, jstring str)
{
	try
	{
		jboolean copy;
		const char* name = env->GetStringUTFChars(str, &copy);
		void *library = NULL;
#ifdef WIN32
		library = LoadLibrary(name);
#else
#if defined(_HPUX) && !defined(_IA64)
		jvmLibrary = shl_load(name, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
		library = dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
#endif // HPUX
#endif
		env->ReleaseStringUTFChars(str, name);

		if (library == NULL)
			fail(env, "Failed to load library");
		else
			libraries.push_back(library);
		return;
	}	catch (...)
	{
		fail(env, "C++ exception in addLibrary");
	}
}

#ifdef __cplusplus
}
#endif