/*****************************************************************************
   Copyright 2004 Steve Ménard

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

PyObject* JPVoidType::getStaticValue(JPClass* c, jfieldID fid) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

PyObject* JPVoidType::getInstanceValue(jobject c, jfieldID fid) 
{
	RAISE(JPypeException, "void cannot be the type of a field.");
}

PyObject* JPVoidType::asHostObject(jvalue val) 
{
	return JPPyni::getNone();
}
	
PyObject* JPVoidType::asHostObjectFromObject(jobject val) 
{
	return JPPyni::getNone();
}

EMatchType JPVoidType::canConvertToJava(PyObject* pyobj)
{
	return _none;
}

jvalue JPVoidType::convertToJava(PyObject* pyobj)
{
	jvalue res;
	res.l = NULL;
	return res;
}

PyObject* JPVoidType::invokeStatic(JPClass* clazz, jmethodID mth, jvalue* val)
{
	JPEnv::getJava()->CallStaticVoidMethodA(clazz->getNativeClass(), mth, val);
	return JPPyni::getNone();
}

PyObject* JPVoidType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
	JPEnv::getJava()->CallVoidMethodA(obj, mth, val);
	return JPPyni::getNone();
}

void JPVoidType::setStaticValue(JPClass* c, jfieldID fid, PyObject*) 
{
	RAISE(JPypeException, "void cannot be the type of a static field.");
}

void JPVoidType::setInstanceValue(jobject c, jfieldID fid, PyObject*) 
{
	RAISE(JPypeException, "void cannot be the type of a field.");
}

vector<PyObject*> JPVoidType::getArrayRange(jarray, int, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayRange(jarray, int, int, vector<PyObject*>&)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

PyObject* JPVoidType::getArrayItem(jarray, int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

void JPVoidType::setArrayItem(jarray, int, PyObject*)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

jarray JPVoidType::newArrayInstance(int)
{
	RAISE(JPypeException, "void cannot be the type of an array.");
}

