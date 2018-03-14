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

JPStringClass::JPStringClass():
	JPObjectClass(JPJni::s_StringClass)
{
}

JPStringClass::~JPStringClass()
{}

PyObject* JPStringClass::asHostObject(jvalue val) 
{
	TRACE_IN("JPStringClass::asHostObject");
	
	if (val.l == NULL)
	{
		return JPPyni::getNone();
	}
	
	jstring v = (jstring)val.l;

	if (JPEnv::getJava()->getConvertStringObjects())
	{
		TRACE1(" Performing conversion");
		string str = JPJni::getStringUTF8(v);
		PyObject* res = JPyString::fromStringUTF8(str);
		return res;
	}
	else
	{
		TRACE1(" Performing wrapping");
		PyObject* res = JPPyni::newStringWrapper(v);
		TRACE1(" Wrapping successfull");
		return res;
	}
	TRACE_OUT;
}

EMatchType JPStringClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);
	JPLocalFrame frame;

	if (obj.isNull() || obj.isNone())
	{
		return _implicit;
	}

	if (obj.isString())
	{
		return _exact;
	}
	
	if (obj.isJavaValue())
	{
		const JPValue& value = obj.asJavaValue();
		if (value.getClass() == this)
		{
			return _exact;
		}
	}
	return _none;
}

jvalue JPStringClass::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	TRACE_IN("JPStringClass::convertToJava");
	jvalue v;
	
	if (obj.isNull() || obj.isNone())
	{
		v.l = NULL;
		return v;
	}
	
	if (obj.isJavaValue())
	{
		const JPValue& value = obj.asJavaValue();
		if (value.getClass() == this)
		{
			v = value.getValue();
			return v;
		}
		else
		{
			JPyErr::setTypeError("Invalid class in conversion");
			JPyErr::raise("convertToJava");
		}
	}

  string str = JPyString(obj).asStringUTF8();
	jstring res = JPJni::newStringUTF8(str);
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

