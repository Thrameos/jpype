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
	} catch (JPypeException& ex)
	{
		// No guarantees that the exception will make it back so print it first
		printf("Exception caught %s\n", ex.getMessage().c_str());

		// Attempt to rethrow it back to Java.
		ex.toJava(JPContext_global);
	}
}

// FIXME global
static list<void*> libraries;

/*
 * Class:     python_lang_PyEngine
 * Method:    getSymbol
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_python_lang_PyEngine_getSymbol
(JNIEnv *env, jobject, jstring str)
{
	try
	{
		jboolean copy;
		const char* name = env->GetStringUTFChars(str, &copy);
		printf("Find symbol %s\n", name);
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
		return NULL;
	} catch (JPypeException& ex)
	{
		// No guarantees that the exception will make it back so print it first
		printf("Exception caught %s\n", ex.getMessage().c_str());

		// Attempt to rethrow it back to Java.
		ex.toJava(JPContext_global);
	}
}

/*
 * Class:     python_lang_PyEngine
 * Method:    addLibrary
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_python_lang_PyEngine_addLibrary
(JNIEnv *env, jobject, jstring str)
{
	printf("Load library\n");
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
		printf("failed to load library");
	else
		libraries.push_back(library);
	return;
}

#ifdef __cplusplus
}
#endif