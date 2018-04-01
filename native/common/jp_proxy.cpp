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
#include <jp_callback.h>

JNIEXPORT jobject JNICALL Java_jpype_JPypeInvocationHandler_hostInvoke(
	JNIEnv *env, jclass clazz, jstring name, 
	jlong hostObj, jobjectArray args, 
	jobjectArray types, jclass nativeReturnType)
{
	TRACE_IN("Java_jpype_JPypeInvocationHandler_hostInvoke");

	JPCallback callback;
	JPyCleaner cleaner;

	try {
		string cname = JPJni::getStringUTF8(name);

		PyObject* hostObjRef = (PyObject*)hostObj;

		JPyObject callable = JPyObject(cleaner.add(JPPyni::getCallableFrom(hostObjRef, cname)));

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
		JPyObject returnValue = JPyObject(cleaner.add(callable.call(pyargs, NULL)));

		// Convert the return back to java
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
	catch(PythonException& ex)
	{ 
		JPyErr::clear();
		if (ex.isJavaException())
		{
			JPyCleaner cleaner;
			PyObject* javaExcRef = cleaner.add(ex.getJavaException());
			const JPValue& value = JPyObject(javaExcRef).asJavaValue();
			jobject obj = value.getObject();
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

namespace { // impl detail, gets initialized by JPProxy::init()
	jclass s_ProxyClass;
	jmethodID s_NewProxyInstanceID;
	jclass handlerClass;
	jmethodID invocationHandlerConstructorID;
	jfieldID hostObjectID;
}

// Create the hooks for the proxy object
void JPProxy::init()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPProxy::init");
	
	// We need to have access to java.lang.reflect.Proxy
	s_ProxyClass = (jclass)JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->FindClass("java/lang/reflect/Proxy") );
	s_NewProxyInstanceID = JPEnv::getJava()->GetStaticMethodID(s_ProxyClass, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;");

	// Load the invocation handler
	jobject cl = JPJni::getSystemClassLoader();
	jclass handler = JPEnv::getJava()->DefineClass("jpype/JPypeInvocationHandler", cl, 
			JPThunk::jpype_JPypeInvocationHandler, 
			JPThunk::jpype_JPypeInvocationHandler_size);
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
	// This will create a new proxy that points to the python object.
	// This will hold a python reference until the java item is garbage
	// collected.
	JPLocalFrame frame;
	jobject cl = JPJni::getSystemClassLoader();

	// Allocate a handler
	jobject handler = JPEnv::getJava()->NewGlobalRef(JPEnv::getJava()->NewObject(handlerClass, invocationHandlerConstructorID));
	
	// Link the scopes
	JPReference::registerRef(handler, m_Instance); 

  // Convert the vector of interfaces to java Class[]	
	jobjectArray ar = JPEnv::getJava()->NewObjectArray((int)m_Interfaces.size(), JPJni::s_ClassClass, NULL);
	for (unsigned int i = 0; i < m_Interfaces.size(); i++)
	{
		JPEnv::getJava()->SetObjectArrayElement(ar, i, (jobject)(m_Interfaces[i]->getNativeClass()));
	}
	JPEnv::getJava()->SetLongField(handler, hostObjectID, (jlong)m_Instance);

	jvalue v[3];
	v[0].l = cl;
	v[1].l = ar;
	v[2].l = handler;

	return frame.keep(JPEnv::getJava()->CallStaticObjectMethodA(s_ProxyClass, s_NewProxyInstanceID, v));
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
	
