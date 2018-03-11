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
		jsize len = JPEnv::getJava()->GetStringLength(v);

		jboolean isCopy;
		const jchar* str = JPEnv::getJava()->GetStringChars(v, &isCopy);
		// FIXME
		PyObject* res = JPyString::fromUnicode(str, len);
		JPEnv::getJava()->ReleaseStringChars(v, str);

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
	JPyAdaptor obj(pyobj);
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
	JPyAdaptor obj(pyobj);

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

	JCharString wstr = JPyString(obj).asJCharString();

	// FIXME java uses a different encoding than standard for 4 byte objects.  
	// Convert through JChar may lead to errors.
	jchar* jstr = new jchar[wstr.length()+1];
	jstr[wstr.length()] = 0;
	for (size_t i = 0; i < wstr.length(); i++)
	{
		jstr[i] = (jchar)wstr[i];  
	}
	jstring res = JPEnv::getJava()->NewString(jstr, (jint)wstr.length());
	delete[] jstr; // FIXME not exception safe
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

