/*****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
 **************************************************************************** */
#include <jni.h>
#include "jpype.h"
#include "pyjp.h"
#include "jp_boxedtype.h"
#include "epypj.h"
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_jpype_python_internal_PyModuleDef
 * Method:    _getModuleDef
 * Signature: (Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_org_jpype_python_internal_PyModuleDef__1find
  (JNIEnv *env, jclass cls, jobject pymodule)
{
	EJP_TRACE_JAVA_IN("moduleDef::find");
	JPPyObject param1 = EJP_ToPython(frame, pymodule);
	struct PyModuleDef* def = PyModule_GetDef(param1.get()); 
	return (jlong) def;  // This is a reference, so no need to worry
	EJP_TRACE_JAVA_OUT(0);
}

/*
 * Class:     org_jpype_python_internal_PyModuleDef
 * Method:    _getName
 * Signature: (J)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_jpype_python_internal_PyModuleDef__1getName
  (JNIEnv *env, jclass cls, jlong jdef)
{
	EJP_TRACE_JAVA_IN("moduleDef::getName");
	struct PyModuleDef* def = (struct PyModuleDef*) jdef;
	if (def->m_name == NULL)
		return NULL;
	return frame.NewStringUTF(def->m_name);
	EJP_TRACE_JAVA_OUT(NULL);	
}

/*
 * Class:     org_jpype_python_internal_PyModuleDef
 * Method:    _getMethods
 * Signature: (J)[[Ljava/lang/Object;
 */
JNIEXPORT jobjectArray JNICALL Java_org_jpype_python_internal_PyModuleDef__1getMethods
  (JNIEnv *env, jclass cls, jlong jdef)
{
	EJP_TRACE_JAVA_IN("moduleDef::getName");
	struct PyModuleDef* def = (struct PyModuleDef*) jdef;
	
	// Count the number of methods
	int methodCount = 0;
	PyMethodDef *methodDef = def->m_methods;
	while( methodDef->ml_name!=NULL)
	{
		methodCount++;
		methodDef++;
	}
	
	jobjectArray out = frame.NewObjectArray(3, 
			context->_java_lang_Object->getJavaClass(), NULL);
	jobjectArray names = frame.NewObjectArray(methodCount, 
			context->_java_lang_Object->getJavaClass(), NULL);
	jlongArray ptr = frame.NewLongArray(methodCount);
	jlongArray flags = frame.NewLongArray(methodCount);
	frame.SetObjectArrayElement(out, 0, names);
	frame.SetObjectArrayElement(out, 1, ptr);
	frame.SetObjectArrayElement(out, 2, flags);
	jboolean copy;
	jlong* p1 = frame.GetLongArrayElements(ptr, &copy);
	jlong* p2 = frame.GetLongArrayElements(flags, &copy);
	
	methodCount = 0;
	methodDef = def->m_methods;
	while( methodDef->ml_name!=NULL)
	{
		jstring name = frame.fromStringUTF8(methodDef->ml_name);
		frame.SetObjectArrayElement(names, methodCount, name);
		p1[methodCount] = (jlong) (methodDef->ml_meth);
		p2[2] = methodDef->ml_flags;
		methodCount++;
		methodDef++;
		frame.DeleteLocalRef(name);
	}
	frame.ReleaseLongArrayElements(ptr, p1, JNI_COMMIT);
	frame.ReleaseLongArrayElements(flags, p2, JNI_COMMIT);
	
	return out;
	EJP_TRACE_JAVA_OUT(NULL);	
}

#ifdef __cplusplus
}
#endif
