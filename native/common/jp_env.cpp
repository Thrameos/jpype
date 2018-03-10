/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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

/*
 * FIXME: use a less coupled way to call PyGILState_Ensure/Release()
 * in JPCleaner::~JPCleaner() if we wan't to target a non Python
 * implementation.
 */
#include <Python.h>

namespace { // impl details
	 JPJavaEnv*       s_Java = NULL;
}

JPJavaEnv* JPEnv::getJava()
{
	return s_Java;
}

bool JPEnv::isInitialized()
{
	return getJava() != NULL ;
}

void JPEnv::loadJVM(const string& vmPath, char ignoreUnrecognized, const StringVector& args)
{
	TRACE_IN("JPEnv::loadJVM");
	
	JavaVMInitArgs jniArgs;
	jniArgs.options = NULL;
	
	JPJavaEnv::load(vmPath);

	// prepare this ...
	jniArgs.version = JNI_VERSION_1_4;
	jniArgs.ignoreUnrecognized = ignoreUnrecognized;
		
	jniArgs.nOptions = (jint)args.size();
	jniArgs.options = (JavaVMOption*)malloc(sizeof(JavaVMOption)*jniArgs.nOptions);
	memset(jniArgs.options, 0, sizeof(JavaVMOption)*jniArgs.nOptions);
	
	for (int i = 0; i < jniArgs.nOptions; i++)
	{
		jniArgs.options[i].optionString = (char*)args[i].c_str();
	}

	s_Java = JPJavaEnv::CreateJavaVM((void*)&jniArgs);
	free(jniArgs.options);
    
	if (s_Java == NULL) {
		RAISE(JPypeException, "Unable to start JVM");
	}

	// First thing we need to get all of our jni methods loaded
	JPJni::init();

	// After we have the jni methods we can preload the type tables
	JPTypeManager::init();

	// Then we can install the proxy code
	JPProxy::init();

	TRACE_OUT;
}

void JPEnv::attachJVM(const string& vmPath)
{
	TRACE_IN("JPEnv::attachJVM");

	JPJavaEnv::load(vmPath);

	s_Java = JPJavaEnv::GetCreatedJavaVM(); 
	
	if (s_Java == NULL) {
		RAISE(JPypeException, "Unable to attach to JVM");
	}

	JPTypeManager::init();
	JPJni::init();
	JPProxy::init();

	TRACE_OUT;
	
}

void JPEnv::attachCurrentThread()
{
	s_Java->AttachCurrentThread();
}

void JPEnv::attachCurrentThreadAsDaemon()  
{
	s_Java->AttachCurrentThreadAsDaemon();
}

bool JPEnv::isThreadAttached()
{
	return s_Java->isThreadAttached();
}

// FIXME.  I dont understand this one.
void JPEnv::registerRef(PyObject* ref, PyObject* targetRef)
{
	TRACE_IN("JPEnv::registerRef");
	JPLocalFrame frame;
	const JPValue& objRef = JPyAdaptor(s_Host).asJavaValue(ref);
	TRACE1("A");
	jobject srcObject = objRef.getObject();
	JPJni::registerRef(s_Java->getReferenceQueue(), srcObject, (jlong)targetRef->copy());
	TRACE_OUT;
	TRACE1("B");
}


static int jpype_traceLevel = 0;

#ifdef JPYPE_TRACING_INTERNAL
	static const bool trace_internal = true;  
#else
	static const bool trace_internal = false;
#endif


void JPypeTracer::traceIn(const char* msg)
{
	if (trace_internal)
	{
		for (int i = 0; i < jpype_traceLevel; i++)
		{
			JPYPE_TRACING_OUTPUT << "  ";
		}
		JPYPE_TRACING_OUTPUT << "<B msg=\"" << msg << "\" >" << endl;
		JPYPE_TRACING_OUTPUT.flush();
		jpype_traceLevel ++;
	}
}

void JPypeTracer::traceOut(const char* msg, bool error)
{
	if (trace_internal)
	{
		jpype_traceLevel --;
		for (int i = 0; i < jpype_traceLevel; i++)
		{
			JPYPE_TRACING_OUTPUT << "  ";
		}
		if (error)
		{
			JPYPE_TRACING_OUTPUT << "</B> <!-- !!!!!!!! EXCEPTION !!!!!! " << msg << " -->" << endl;
		}
		else
		{
			JPYPE_TRACING_OUTPUT << "</B> <!-- " << msg << " -->" << endl;
		}
		JPYPE_TRACING_OUTPUT.flush();
	}
}  

void JPypeTracer::trace1(const char* name, const string& msg)  
{
	if (trace_internal)
	{
		for (int i = 0; i < jpype_traceLevel; i++)
		{
			JPYPE_TRACING_OUTPUT << "  ";
		}
		JPYPE_TRACING_OUTPUT << "<M>" << name << " : " << msg << "</M>" << endl;
		JPYPE_TRACING_OUTPUT.flush();
	}
}

JPLocalFrame::JPLocalFrame(size_t i)
{
	// FIXME check return code
	popped=false;
	JPEnv::getJava()->PushLocalFrame((int)i);
}

jobject JPLocalFrame::keep(jobject obj)
{
	popped=true;
	return JPEnv::getJava()->PopLocalFrame(obj);
}

JPLocalFrame::~JPLocalFrame()
{
	if (!popped)
	{
		JPEnv::getJava()->PopLocalFrame(NULL);
	}
}

JCharString::JCharString(const jchar* c)
{
	m_Length = 0;
	while (c[m_Length] != 0)
	{
		m_Length ++;
	}
	
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c[i];
	}
}

JCharString::JCharString(const JCharString& c)
{
	m_Length = c.m_Length;
	m_Value = new jchar[m_Length+1];
	m_Value[m_Length] = 0;
	for (unsigned int i = 0; i < m_Length; i++)
	{
		m_Value[i] = c.m_Value[i];
	}
	
}

JCharString::JCharString(size_t len)
{
	m_Length = len;
	m_Value = new jchar[len+1];
	for (size_t i = 0; i <= len; i++)
	{
		m_Value[i] = 0;
	}
}

JCharString::~JCharString()
{
	if (m_Value != NULL)
	{
		delete[] m_Value;
	}
}
	
const jchar* JCharString::c_str()
{
	return m_Value;
}
