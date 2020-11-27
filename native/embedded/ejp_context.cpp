#include <Python.h>
#include <jni.h>
#include "epypj.h"
//
//EJPContext::EJPContext()
//{
//	printf("New context\n");
//	JP_PY_CALL(a1, PyDict_New());
//	JP_PY_CALL(a2, PyDict_New());
//	globals = a1;
//	locals = a2;
//	printf("  globals %p\n", globals.get());
//	printf("  locals %p\n", locals.get());
//	PyObject *builtins = PyEval_GetBuiltins();
//	printf("  builtin %p\n", builtins);
//	PyDict_Update(globals.get(), builtins);
//}
//
//EJPContext::~EJPContext()
//{
//}
//
//#ifdef __cplusplus
//extern "C"
//{
//#endif
//
//JNIEXPORT jobject JNICALL Java_org_jpype_PyContext_interactive_1(
//		JNIEnv *env, jobject obj, jlong jctxt)
//{
//	JP_JAVA_TRY("PyContext::interactive");
//	EJPContext *ctxt = (EJPContext*) jctxt;
//	JPPyThreadLock lock;
//	try
//	{
//		JP_PY_CALL(rc1, PyRun_String("import code as _code",
//				Py_single_input, ctxt->globals.get(), ctxt->locals.get()));
//		JP_PY_CALL(rc2, PyRun_String("_code.interact(local=locals())",
//				Py_single_input, ctxt->globals.get(), ctxt->locals.get()));
//		return convertToJava(env, rc2.keep());
//	}	catch (...)
//	{
//		if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_SystemExit))
//		{
//			PyErr_Clear();
//			return NULL;
//		}
//		throw;
//	}
//	JP_JAVA_CATCH(NULL);
//}
//
//JNIEXPORT jobject JNICALL Java_org_jpype_PyContext_runString_1(
//		JNIEnv *env, jobject cls, jlong jctxt, jstring jcmd)
//{
//	JP_JAVA_TRY("PyContext::runString");
//	EJPContext *ctxt = (EJPContext*) jctxt;
//	JPPyThreadLock lock;
//	JPJavaString cmd(env, jcmd);
//	JP_PY_CALL(rc, PyRun_String(cmd.str(), Py_single_input, ctxt->globals.get(), ctxt->locals.get()));
//	return convertToJava(env, rc.keep());
//	JP_JAVA_CATCH(NULL);
//}
//
//#ifdef __cplusplus
//}
//#endif