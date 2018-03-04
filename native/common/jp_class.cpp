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

JPClass::JPClass(jclass c) :
	m_Class((jclass)JPEnv::getJava()->NewGlobalRef(c)),
	m_Name(JPJni::getName(c))
{}

JPClass::~JPClass()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Class);
}

HostRef* convertObjectToHostRef(jobject obj)
{
	if (obj == NULL)
		return JPEnv::getHost()->getNone();

	jvalue v;
	v.l = obj;
	
	jclass cls = JPJni::getClass(v.l);
	JPClass* type = JPTypeManager::findClass(cls);
	return type->asHostObject(v);
}

HostRef* JPClass::getStaticValue(JPClass* clazz, jfieldID fid) 
{
	TRACE_IN("JPClass::getStaticValue");
	JPLocalFrame frame;

	jobject r = JPEnv::getJava()->GetStaticObjectField(clazz->getNativeClass(), fid);
	return convertObjectToHostRef(r);

	TRACE_OUT;
}

HostRef* JPClass::getInstanceValue(jobject c, jfieldID fid) 
{
	TRACE_IN("JPClass::getInstanceValue");
	JPLocalFrame frame;
	jobject r = JPEnv::getJava()->GetObjectField(c, fid);
	return convertObjectToHostRef(r);

	TRACE_OUT;
}

HostRef* JPClass::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invokeStatic");
	JPLocalFrame frame;
	
	jobject res = JPEnv::getJava()->CallStaticObjectMethodA(claz->getNativeClass(), mth, val);
	return convertObjectToHostRef(res);

	TRACE_OUT;
}

HostRef* JPClass::invoke(jobject claz, JPClass* clazz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invoke");
	JPLocalFrame frame;

	// Call method
	jobject res = JPEnv::getJava()->CallNonvirtualObjectMethodA(claz, clazz->getNativeClass(), mth, val);
	return convertObjectToHostRef(res);
	
	TRACE_OUT;
}

void JPClass::setStaticValue(JPClass* c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPClass::setStaticValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetStaticObjectField(c->getNativeClass(), fid, val);
	TRACE_OUT;
}

void JPClass::setInstanceValue(jobject c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPClass::setInstanceValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetObjectField(c, fid, val);
	TRACE_OUT;
}

jarray JPClass::newArrayInstance(int sz)
{
	return JPEnv::getJava()->NewObjectArray(sz, getNativeClass(), NULL);
}

vector<HostRef*> JPClass::getArrayRange(jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	JPLocalFrame frame;
	
	vector<HostRef*> res;
	
	for (int i = 0; i < length; i++)
	{
		res.push_back(convertObjectToHostRef( JPEnv::getJava()->GetObjectArrayElement(array, i+start) ));
	}
	
	return res;  
}


void JPClass::setArrayRange(jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPLocalFrame frame(8+length);
	jobjectArray array = (jobjectArray)a;	
	for (int i = 0; i < length; i++)
	{
		JPEnv::getJava()->SetObjectArrayElement(array, i+start, convertToJava(vals[i]).l);
	}
}

void JPClass::setArrayItem(jarray a, int ndx, HostRef* val)
{
	JPLocalFrame frame;
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v = convertToJava(val);
	
	JPEnv::getJava()->SetObjectArrayElement(array, ndx, v.l);		
}

HostRef* JPClass::getArrayItem(jarray a, int ndx)
{
	JPLocalFrame frame;
	TRACE_IN("JPClass::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	
	jobject obj = JPEnv::getJava()->GetObjectArrayElement(array, ndx);
	return convertObjectToHostRef(obj);
	TRACE_OUT;
}

jobject JPClass::convertToJavaObject(HostRef* obj)
{
	return convertToJava(obj).l;
}

HostRef* JPClass::asHostObjectFromObject(jobject obj)
{
	jvalue val;
	val.l = obj;
	return asHostObject(val);
}

HostRef* JPClass::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

bool JPClass::isSubTypeOf(const JPClass& other) const
{
	JPLocalFrame frame;
	jclass ourClass = getNativeClass();
	jclass otherClass = other.getNativeClass();
	// IsAssignableFrom is a jni method and the order of parameters is counterintuitive
	bool otherIsSuperType = JPEnv::getJava()->IsAssignableFrom(ourClass, otherClass);
	return otherIsSuperType;
}

