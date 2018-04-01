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
	m_Name(JPJni::getSimpleName(c))
{}

JPClass::~JPClass()
{
	JPEnv::getJava()->DeleteGlobalRef(m_Class);
}


bool JPClass::isInterface() const
{
	return false;
}

bool JPClass::isArray() const
{
	return JPJni::isArray(m_Class);
}

bool JPClass::isAbstract() const
{
	return JPJni::isAbstract(m_Class);
}

bool JPClass::isFinal() const
{
	return JPJni::isFinal(m_Class);
}

long JPClass::getClassModifiers()
{
	return JPJni::getClassModifiers(m_Class);
}

PyObject* convertObjectToHost(jobject obj)
{
	if (obj == NULL)
		return JPPyni::getNone();

	jvalue v;
	v.l = obj;
	
	jclass cls = JPJni::getClass(v.l);
	JPClass* type = JPTypeManager::findClass(cls);
	return type->asHostObject(v);
}

PyObject* JPClass::getStaticValue(JPClass* clazz, jfieldID fid) 
{
	TRACE_IN("JPClass::getStaticValue");
	JPLocalFrame frame;

	jobject r = JPEnv::getJava()->GetStaticObjectField(clazz->getNativeClass(), fid);
	return convertObjectToHost(r);

	TRACE_OUT;
}

PyObject* JPClass::getInstanceValue(jobject c, jfieldID fid) 
{
	TRACE_IN("JPClass::getInstanceValue");
	JPLocalFrame frame;
	jobject r = JPEnv::getJava()->GetObjectField(c, fid);
	return convertObjectToHost(r);

	TRACE_OUT;
}

PyObject* JPClass::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invokeStatic");
	JPLocalFrame frame;
	
	jobject res = JPEnv::getJava()->CallStaticObjectMethodA(claz->getNativeClass(), mth, val);
	return convertObjectToHost(res);

	TRACE_OUT;
}

PyObject* JPClass::invoke(jobject claz, JPClass* clazz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invoke");
	JPLocalFrame frame;

	// Call method
	jobject res = JPEnv::getJava()->CallNonvirtualObjectMethodA(claz, clazz->getNativeClass(), mth, val);
	return convertObjectToHost(res);
	
	TRACE_OUT;
}

void JPClass::setStaticValue(JPClass* c, jfieldID fid, PyObject* obj) 
{
	TRACE_IN("JPClass::setStaticValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetStaticObjectField(c->getNativeClass(), fid, val);
	TRACE_OUT;
}

void JPClass::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
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

vector<PyObject*> JPClass::getArrayRange(jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	JPLocalFrame frame;
	
	vector<PyObject*> res;
	
	for (int i = 0; i < length; i++)
	{
		res.push_back(convertObjectToHost( JPEnv::getJava()->GetObjectArrayElement(array, i+start) ));
	}
	
	return res;  
}


void JPClass::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
	JPLocalFrame frame(8+length);
	jobjectArray array = (jobjectArray)a;	
	for (int i = 0; i < length; i++)
	{
		JPEnv::getJava()->SetObjectArrayElement(array, i+start, convertToJava(vals[i]).l);
	}
}

void JPClass::setArrayItem(jarray a, int ndx, PyObject* val)
{
	JPLocalFrame frame;
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v = convertToJava(val);
	
	JPEnv::getJava()->SetObjectArrayElement(array, ndx, v.l);		
}

PyObject* JPClass::getArrayItem(jarray a, int ndx)
{
	JPLocalFrame frame;
	TRACE_IN("JPClass::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	
	jobject obj = JPEnv::getJava()->GetObjectArrayElement(array, ndx);
	return convertObjectToHost(obj);
	TRACE_OUT;
}

jobject JPClass::convertToJavaObject(PyObject* obj)
{
	return convertToJava(obj).l;
}

PyObject* JPClass::asHostObjectFromObject(jobject obj)
{
	jvalue val;
	val.l = obj;
	return asHostObject(val);
}

bool JPClass::isAssignableTo(const JPClass* o) const
{
	if (o == NULL)
		return false;
	JPLocalFrame frame;
	jclass jo = o->getNativeClass();
	return JPEnv::getJava()->IsAssignableFrom(m_Class, jo);
}

