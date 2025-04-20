/*
This is a fake module which is installed with the _jpype module to hold the prelaunch hooks.
*/
#ifdef WIN32
#include <Windows.h>
#else
#if defined(_HPUX) && !defined(_IA64)
#include <dl.h>
#else
#include <dlfcn.h>
#endif // HPUX
#endif
#include <jni.h>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

/* Stock System.loadLibrary() does not work for us because they load the
   shared library with local flags.  We need a load which supports shared
   used with all Python modules.
*/
JNIEXPORT jlong JNICALL Java_org_jpype_bridge_BootstrapLoader_loadLibrary
(JNIEnv *env, jclass clazz, jstring lib)
{
    const char *path = env->GetStringUTFChars(lib, nullptr);
	void *handle;
#ifdef WIN32
	wchar_t *wpath = Py_DecodeLocale(path, NULL);
	handle = LoadLibraryW(wpath);
	PyMem_RawFree(wpath);
#else
#if defined(_HPUX) && !defined(_IA64)
	handle = shl_load(path, BIND_DEFERRED | BIND_VERBOSE, 0L);
#else
    handle = dlopen(path, RTLD_GLOBAL | RTLD_LAZY); 
#endif
#endif
	return (jlong) handle;
}

#ifdef __cplusplus
}
#endif
