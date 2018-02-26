/*****************************************************************************
   Copyright 2004 Steve M�nard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#include <jpype.h>

JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
	JNIEnv *env, jclass clazz, jstring name, 
	jlong hostObj, jobjectArray args, 
	jobjectArray types, jclass nativeReturnType)
{
	TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	void* callbackState = JPEnv::getHost()->prepareCallbackBegin();

	JPCleaner cleaner;

	try {
		string cname = JPJni::asciiFromJava(name);

		HostRef* hostObjRef = (HostRef*)hostObj;

		HostRef* callable = JPEnv::getHost()->getCallableFrom(hostObjRef, cname);
		cleaner.add(callable);

		// If method can't be called, throw an exception
		if (callable == NULL || callable->isNull() || JPEnv::getHost()->isNone(callable))
		{
			JPEnv::getJava()->ThrowNew(JPJni::s_NoSuchMethodErrorClass, cname.c_str());
			JPEnv::getHost()->prepareCallbackFinish(callbackState);
			return NULL;
		}
					
		// convert the arguments into a python list
		jsize argLen = JPEnv::getJava()->GetArrayLength(types);
		vector<HostRef*> hostArgs;

		for (int i = 0; i < argLen; i++)
		{
			jclass c = (jclass)JPEnv::getJava()->GetObjectArrayElement(types, i);
			jobject obj = JPEnv::getJava()->GetObjectArrayElement(args, i);

			jvalue v;
			v.l = obj;

			HostRef* o = JPTypeManager::findClass(c)->asHostObjectFromObject(v);
			cleaner.add(o);
			hostArgs.push_back(o);
		}

		HostRef* returnValue = JPEnv::getHost()->callObject(callable, hostArgs);
		cleaner.add(returnValue);

		JPClass* returnType = JPTypeManager::findClass(nativeReturnType);
		if (returnValue == NULL || returnValue->isNull() || JPEnv::getHost()->isNone(returnValue))
		{
			// None is acceptable for void or Objects
			if (returnType!=JPTypeManager::_void && !returnType->isObjectType())
			{
				JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is None when it cannot be");
				JPEnv::getHost()->prepareCallbackFinish(callbackState);
				return NULL;
			}
		}

		if (returnType==JPTypeManager::_void)
		{
			JPEnv::getHost()->prepareCallbackFinish(callbackState);
			return NULL;
		}

		if (returnType->canConvertToJava(returnValue) == _none)
		{
			JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is not compatible with required type.");
			JPEnv::getHost()->prepareCallbackFinish(callbackState);
			return NULL;
		}
	
		jobject returnObj = returnType->convertToJavaObject(returnValue);
		returnObj = JPEnv::getJava()->NewLocalRef(returnObj); // Add an extra local reference so returnObj survives cleaner
    
		JPEnv::getHost()->prepareCallbackFinish(callbackState);

		return returnObj;

	}
	catch(HostException& ex)
	{ 
		JPEnv::getHost()->clearError();
		if (JPEnv::getHost()->isJavaException(&ex))
		{
			JPCleaner cleaner;
			HostRef* javaExcRef = JPEnv::getHost()->getJavaException(&ex);
			JPObject* javaExc = JPEnv::getHost()->asObject(javaExcRef);
			cleaner.add(javaExcRef);
			jobject obj = javaExc->getObject();
			JPEnv::getJava()->Throw((jthrowable)obj);
		}
		else
		{
            // Prepare a message
            string message = "Python exception thrown: ";
            message += ex.getMessage();
            JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, message.c_str());
		}
	} 
	catch(JavaException&)
	{ 
		cerr << "Java exception at " << __FILE__ << ":" << __LINE__ << endl; 
	}
	catch(JPypeException& ex)
	{
		JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, ex.getMsg());
	}

	JPEnv::getHost()->prepareCallbackFinish(callbackState);

	return NULL;

	TRACE_OUT;
}

JNIEXPORT void JNICALL Java_jpype_ref_JPypeReferenceQueue_removeHostReference(
	JNIEnv *env, jclass clazz, jlong hostObj)
{
	TRACE_IN("Java_jpype_ref_JPypeReferenceQueue_removeHostReference");

	void* callbackState = JPEnv::getHost()->prepareCallbackBegin();

	if (hostObj >0)
	{
		HostRef* hostObjRef = (HostRef*)hostObj;
		//JPEnv::getHost()->printReferenceInfo(hostObjRef);
		delete hostObjRef;
	}

	JPEnv::getHost()->prepareCallbackFinish(callbackState);

	//return NULL;
	TRACE_OUT;
}

namespace { // impl detail, gets initialized by JPProxy::init()
	jclass handlerClass;
	jclass referenceClass;
	jclass referenceQueueClass;
	jmethodID invocationHandlerConstructorID;
	jfieldID hostObjectID;
}

void JPProxy::init()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPProxy::init");

	// build the proxy class ...
	jobject cl = JPJni::getSystemClassLoader();

	jclass handler = JPEnv::getJava()->DefineClass("jpype/JPypeInvocationHandler", cl, JPypeInvocationHandler, getJPypeInvocationHandlerLength());
	handlerClass = (jclass)JPEnv::getJava()->NewGlobalRef(handler);
	
	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature =(char*) "(Ljava/lang/String;J[Ljava/lang/Object;[Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &Java_jpype_JPypeInvocationHandler_hostInvoke;


	hostObjectID = JPEnv::getJava()->GetFieldID(handler, "hostObject", "J");
	invocationHandlerConstructorID = JPEnv::getJava()->GetMethodID(handler, "<init>", "()V");

	JPEnv::getJava()->RegisterNatives(handlerClass, method, 1);

	// Not quite the right area ... but I;m doing similar here already so let's register the other classes too
	jclass reference = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReference", cl, JPypeReference, getJPypeReferenceLength());
	jclass referenceQueue = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReferenceQueue", cl, JPypeReferenceQueue, getJPypeReferenceQueueLength());
	referenceClass = (jclass)JPEnv::getJava()->NewGlobalRef(reference);
	referenceQueueClass = (jclass)JPEnv::getJava()->NewGlobalRef(referenceQueue);

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	JPEnv::getJava()->GetMethodID(referenceQueue, "<init>", "()V");

	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHostReference";
	method2[0].signature = (char*) "(J)V";
	method2[0].fnPtr = (void*)&Java_jpype_ref_JPypeReferenceQueue_removeHostReference;

	JPEnv::getJava()->RegisterNatives(referenceQueueClass, method2, 1);

	TRACE_OUT;

}

JPProxy::JPProxy(HostRef* inst, vector<jclass>& intf)
{
	JPLocalFrame frame;
	m_Instance = inst->copy();
	
	jobjectArray ar = JPEnv::getJava()->NewObjectArray((int)intf.size(), JPJni::s_ClassClass, NULL);
	m_Interfaces = (jobjectArray)JPEnv::getJava()->NewGlobalRef(ar);

	for (unsigned int i = 0; i < intf.size(); i++)
	{
		m_InterfaceClasses.push_back((jclass)JPEnv::getJava()->NewGlobalRef(intf[i]));
		JPEnv::getJava()->SetObjectArrayElement(m_Interfaces, i, m_InterfaceClasses[i]);
	}
	
	m_Handler = JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->NewObject(handlerClass, invocationHandlerConstructorID));

	JPEnv::getJava()->SetLongField(m_Handler, hostObjectID, (jlong)inst->copy());
}

JPProxy::~JPProxy()
{
	if (m_Instance != NULL)
	{
		m_Instance->release();
	}
	JPEnv::getJava()->DeleteGlobalRef(m_Handler);
	JPEnv::getJava()->DeleteGlobalRef(m_Interfaces);

	for (unsigned int i = 0; i < m_InterfaceClasses.size(); i++)
	{
		JPEnv::getJava()->DeleteGlobalRef(m_InterfaceClasses[i]);
	}

}


jobject JPProxy::getProxy()
{
	JPLocalFrame frame;
	jobject cl = JPJni::getSystemClassLoader();

	jvalue v[3];
	v[0].l = cl;
	v[1].l = m_Interfaces;
	v[2].l = m_Handler;

	return frame.keep(JPEnv::getJava()->CallStaticObjectMethodA(JPJni::s_ProxyClass, JPJni::s_NewProxyInstanceID, v));
}

