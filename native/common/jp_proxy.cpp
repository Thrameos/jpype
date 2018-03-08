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

// Exception safe wrapper
class JPCallbackState
{
	public:
		JPCallbackState()
		{
			state = JPPyni::prepareCallbackBegin();
		}
		~JPCallbackState()
		{
			JPPyni::prepareCallbackFinish(state);
		}
		void* state;
};

JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
	JNIEnv *env, jclass clazz, jstring name, 
	jlong hostObj, jobjectArray args, 
	jobjectArray types, jclass nativeReturnType)
{
	TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	JPCallbackState callbackState;
	JPyCleaner cleaner;

	try {
		string cname = JPJni::asciiFromJava(name);

		PyObject* hostObjRef = (PyObject*)hostObj;

		JPyAdaptor callable = JPyAdaptor(cleaner.add(JPPyni::getCallableFrom(hostObjRef, cname)));

		// If method can't be called, throw an exception
		if (callable.isNull() || callable.isNone())
		{
			JPEnv::getJava()->ThrowNew(JPJni::s_NoSuchMethodErrorClass, cname.c_str());
			return NULL;
		}
					
		jsize argLen = JPEnv::getJava()->GetArrayLength(types);
		JPyTuple pyargs = cleaner.add(JPyTuple::newTuple(argLen));
		for (int i = 0; i < argLen; i++)
		{
			jclass c = (jclass)JPEnv::getJava()->GetObjectArrayElement(types, i);
			jobject obj = JPEnv::getJava()->GetObjectArrayElement(args, i);
			PyObject* o = cleaner.add(JPTypeManager::findClass(c)->asHostObjectFromObject(obj));
			pyargs.setItem(i, o);
		}

		// Call the method in python
		JPyAdaptor returnValue = JPyAdaptor(cleaner.add(callable.call(pyargs, NULL)));

		// Convet the return back to java
		JPClass* returnType = JPTypeManager::findClass(nativeReturnType);
		if (returnValue.isNull() || returnValue.isNone())
		{
			// None is acceptable for void or Objects
			if (returnType!=JPTypeManager::_void && !returnType->isObjectType())
			{
				JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is None when it cannot be");
				return NULL;
			}
		}

		if (returnType == JPTypeManager::_void)
		{
			return NULL;
		}

		if (returnType->canConvertToJava(returnValue) == _none)
		{
			JPEnv::getJava()->ThrowNew(JPJni::s_RuntimeExceptionClass, "Return value is not compatible with required type.");
			return NULL;
		}

	  // Otherwise is an object	
		jobject returnObj = returnType->convertToJavaObject(returnValue);
		returnObj = JPEnv::getJava()->NewLocalRef(returnObj); // Add an extra local reference so returnObj survives cleaner
		return returnObj;
	}
	catch(HostException& ex)
	{ 
		JPyErr::clearError();
		if (JPPyni::isJavaException(&ex))
		{
			JPyCleaner cleaner;
			PyObject* javaExcRef = cleaner.add(JPPyni::getJavaException(&ex));
			JPObject* javaExc = JPyAdaptor(javaExcRef).asJavaObject();
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

	return NULL;
	TRACE_OUT;
}

JNIEXPORT void JNICALL Java_jpype_ref_JPypeReferenceQueue_removeHost(
	JNIEnv *env, jclass clazz, jlong hostObj)
{
	TRACE_IN("Java_jpype_ref_JPypeReferenceQueue_removeHost");

	JPCallbackState callbackState;
	if (hostObj >0)
	{
		PyObject* hostObjRef = (PyObject*)hostObj;
		//JPPyni::printReferenceInfo(hostObjRef);
		JPyObject(hostObjRef).decref();
	}
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

// Create the hooks for the proxy object
void JPProxy::init()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPProxy::init");

	// build the proxy class ...
	jobject cl = JPJni::getSystemClassLoader();

	jclass handler = JPEnv::getJava()->DefineClass("jpype/JPypeInvocationHandler", cl, JPypeInvocationHandler, getJPypeInvocationHandlerLength());
	handlerClass = (jclass)JPEnv::getJava()->NewGlobalRef(handler);

	// Create a field in the class.
	hostObjectID = JPEnv::getJava()->GetFieldID(handler, "hostObject", "J");
	invocationHandlerConstructorID = JPEnv::getJava()->GetMethodID(handler, "<init>", "()V");

  // Install the invoke method	
	JNINativeMethod method[1];
	method[0].name = (char*) "hostInvoke";
	method[0].signature =(char*) "(Ljava/lang/String;J[Ljava/lang/Object;[Ljava/lang/Class;Ljava/lang/Class;)Ljava/lang/Object;";
	method[0].fnPtr = (void*) &Java_jpype_JPypeInvocationHandler_hostInvoke;
	JPEnv::getJava()->RegisterNatives(handlerClass, method, 1);

	// Not quite the right area ... but I'm doing similar here already so let's register the other classes too
	jclass reference = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReference", cl, JPypeReference, getJPypeReferenceLength());
	jclass referenceQueue = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReferenceQueue", cl, JPypeReferenceQueue, getJPypeReferenceQueueLength());
	referenceClass = (jclass)JPEnv::getJava()->NewGlobalRef(reference);
	referenceQueueClass = (jclass)JPEnv::getJava()->NewGlobalRef(referenceQueue);

	//Required due to bug in jvm
	//See: http://bugs.sun.com/bugdatabase/view_bug.do?bug_id=6493522
	JPEnv::getJava()->GetMethodID(referenceQueue, "<init>", "()V");

	// Install a destructor for the proxy host object with java
	JNINativeMethod method2[1];
	method2[0].name = (char*) "removeHost";
	method2[0].signature = (char*) "(J)V";
	method2[0].fnPtr = (void*)&Java_jpype_ref_JPypeReferenceQueue_removeHost;
	JPEnv::getJava()->RegisterNatives(referenceQueueClass, method2, 1);

	TRACE_OUT;

}

JPProxy::JPProxy(PyObject* inst, vector<JPObjectClass*>& intf)
	: m_Instance(inst), m_Interfaces(intf)
{
	m_Instance = inst;  // Do not reference this, this object holds us so we have the same scope.

	// This can't hold any objects with reference counters or we will have a circular reference problem.
	// The python holds the c object, java object must hold a reference to the python object
	// If the c object holds a reference to the java object then we are circular.
	// Thus the proxy must create a fresh java object each time it is used.
	// We could form a weak reference, but that needs special handling to recreate everytime it breaks.
}

JPProxy::~JPProxy()
{
}


jobject JPProxy::getProxy()
{
	JPLocalFrame frame;
	jobject cl = JPJni::getSystemClassLoader();

	// Allocate a handler
	jobject handler = JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->NewObject(handlerClass, invocationHandlerConstructorID));

  // Convert the vector of interfaces to java Class[]	
	jobjectArray ar = JPEnv::getJava()->NewObjectArray((int)m_Interfaces.size(), JPJni::s_ClassClass, NULL);
	for (unsigned int i = 0; i < m_Interfaces.size(); i++)
	{
		JPEnv::getJava()->SetObjectArrayElement(ar, i, (jobject)(m_Interfaces[i]->getNativeClass()));
	}
	JPyObject(m_Instance).incref(); // create a reference which will be held by java
	JPEnv::getJava()->SetLongField(handler, hostObjectID, (jlong)m_Instance);

	jvalue v[3];
	v[0].l = cl;
	v[1].l = ar;
	v[2].l = handler;

	return frame.keep(JPEnv::getJava()->CallStaticObjectMethodA(JPJni::s_ProxyClass, JPJni::s_NewProxyInstanceID, v));
}

bool JPProxy::implements(JPObjectClass* cls)
{
	// check if any of the interfaces matches ...
	for (unsigned int i = 0; i < m_Interfaces.size(); i++)
	{
		if (cls->isAssignableTo(m_Interfaces[i]))
		{
			return true;
		}
	}
	return false;
}
	
