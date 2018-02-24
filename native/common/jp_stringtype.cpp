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

JPStringType::JPStringType():
	JPObjectClass(JPJni::s_StringClass)
{
}

JPStringType::~JPStringType()
{}

HostRef* JPStringType::asHostObject(jvalue val) 
{
	TRACE_IN("JPStringType::asHostObject");
	
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	
	jstring v = (jstring)val.l;

	if (JPEnv::getJava()->getConvertStringObjects())
	{
		TRACE1(" Performing conversion");
		jsize len = JPEnv::getJava()->GetStringLength(v);

		jboolean isCopy;
		const jchar* str = JPEnv::getJava()->GetStringChars(v, &isCopy);

		HostRef* res = JPEnv::getHost()->newStringFromUnicode(str, len);
		
		JPEnv::getJava()->ReleaseStringChars(v, str);

		return res;
	}
	else
	{
		TRACE1(" Performing wrapping");
		HostRef* res = JPEnv::getHost()->newStringWrapper(v);
		TRACE1(" Wrapping successfull");
		return res;
	}
	TRACE_OUT;
}

EMatchType JPStringType::canConvertToJava(HostRef* obj)
{
	JPLocalFrame frame;

	if (obj == NULL || JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isString(obj))
	{
		return _exact;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);

		if (name.getType() == JPTypeName::_string)
		{
			return _exact;
		}
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);
		JPObjectClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			return _exact;
		}
	}
	return _none;
}

jvalue JPStringType::convertToJava(HostRef* obj)
{
	TRACE_IN("JPStringType::convertToJava");
	jvalue v;
	
	if (JPEnv::getHost()->isNone(obj))
	{
		v.l = NULL;
		return v;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPObjectClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			v.l = o->getObject(); 
			return v;
		}
	}

	JCharString wstr = JPEnv::getHost()->stringAsJCharString(obj);

	jchar* jstr = new jchar[wstr.length()+1];
	jstr[wstr.length()] = 0;
	for (size_t i = 0; i < wstr.length(); i++)
	{
		jstr[i] = (jchar)wstr[i];  
	}
	jstring res = JPEnv::getJava()->NewString(jstr, (jint)wstr.length());
	delete[] jstr;
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

