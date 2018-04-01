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
// This uses java.lang.ref.PhantomReference and a reference queue to 
// clean up objects.  This is used whenever a Java object is holding 
// a reference to python object that increases the reference count of the 
// python object. 
//
// Currently used for proxy and shared buffers.

namespace { 
	jclass reference_class;
	jclass referenceQueue_class;
	jobject referenceQueue_instance;
	jmethodID referenceQueue_registerRef;
}

namespace JPReference {

JNIEXPORT void JNICALL Java_jpype_ref_JPypeReference_removeHostReference(
	JNIEnv *env, jclass clazz, jlong hostObj)
{
	TRACE_IN("Java_jpype_ref_JPypeReference_removeHostReference");
	JPCallback callback;
	if (hostObj >0)
	{
		PyObject* hostObjRef = (PyObject*)hostObj;
		JPyObject(hostObjRef).decref();
	}
	TRACE_OUT;
}

// Create the hooks for the proxy object
void init()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPReference::init");

	// Load the reference class
	jobject cl = JPJni::getSystemClassLoader();
	reference_class = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReference", cl, 
			JPThunk::jpype_ref_JPypeReference, 
			JPThunk::jpype_ref_JPypeReference_size);
	referenceQueue_class = JPEnv::getJava()->DefineClass("jpype/ref/JPypeReferenceQueue", cl, 
			JPThunk::jpype_ref_JPypeReferenceQueue, 
			JPThunk::jpype_ref_JPypeReferenceQueue_size);


	{ // Install a destructor for the proxy host object with java

		// JVM requires the class to be initialized before RegisterNatives.
		//See: http://bugs.java.com/bugdatabase/view_bug.do?bug_id=6493522
		JPEnv::getJava()->GetMethodID(reference_class, "dispose", "()V");

		// Install the hook for dispose
		JNINativeMethod method2[1];
		method2[0].name = (char*) "removeHostReference";
		method2[0].signature = (char*) "(J)V";
		method2[0].fnPtr = (void*)&Java_jpype_ref_JPypeReference_removeHostReference;
		JPEnv::getJava()->RegisterNatives(reference_class, method2, 1);
	}

	{ // Start a reference queue
		jmethodID ctor = JPEnv::getJava()->GetMethodID(referenceQueue_class, "<init>", "()V");
		jobject obj = JPEnv::getJava()->NewObject(referenceQueue_class, ctor);
		referenceQueue_instance = (jclass)JPEnv::getJava()->NewGlobalRef(obj);
	}

// This had two implementations one launched from a python thread and the other from java.
// It is very unclear as to why more that one path was required.  I have removed the second
// for now as the second was tested much less.

//	if (useJavaThread)
	{
		jmethodID method = JPEnv::getJava()->GetMethodID(referenceQueue_class, "start", "()V");
		JPEnv::getJava()->CallVoidMethod(referenceQueue_instance, method);
	}
//	else
//	{
//		jmethodId method = JPEnv::getJava()->GetMethodID(JPypeReferenceQueueClass, "run", "()V");
//		JPEnv::getJava()->CallVoidMethod(obj, method);
//	}

	// Get the methods we need for later
	referenceQueue_registerRef = JPEnv::getJava()->GetMethodID(referenceQueue_class, "registerRef", "(Ljava/lang/Object;J)V");

	TRACE_OUT;
}

// I honestly have no idea why this would ever be called.  It is pretty fatal as 
// every java object that was connected to a python object is likely to be broken
// resulting in potential segmentation faults. On the other hand if it does not 
// break the links it would leave python objects referenced forever.  Either way
// this method seems like a bad idea.
void stop()
{
	jmethodID method = JPEnv::getJava()->GetMethodID(referenceQueue_class, "stop", "()V");
	JPEnv::getJava()->CallVoidMethod(referenceQueue_instance, method);
}

// This creates a python reference by incrementing the count.
// It will be released when the java object is garbage collected.
void registerRef(jobject jobj, PyObject* pyobj)
{
	JPLocalFrame frame;
	TRACE_IN("registerRef");
	JPyObject(pyobj).incref(); // create a reference which will be held by java

	// Register the reference
	jvalue args[2];
	args[0].l = jobj;
	args[1].j = (jlong)pyobj;
	JPEnv::getJava()->CallVoidMethodA(referenceQueue_instance, referenceQueue_registerRef, args);
	TRACE_OUT;
}

} // namespace
