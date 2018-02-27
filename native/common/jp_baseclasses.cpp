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

// Class<java.lang.Object> and Class<java.lang.Class> have special rules

JPObjectBaseClass::JPObjectBaseClass() :
	JPObjectClass(JPJni::s_ObjectClass)
{
}

JPObjectBaseClass::JPObjectBaseClass() :
{
}

EMatchType JPObjectBaseClass::canConvertToJava(HostRef* obj)
{
  EMatchType base=JPObjectClass:canConvertToJava(obj);
	if (base!=_none)
		return base;

	// Implicit rules for java.lang.Object
	JPLocalFrame frame;
	TRACE_IN("JPObjectBaseClass::canConvertToJava");

	// arrays are objects
	if (JPEnv::getHost()->isArray(obj))
	{
		TRACE1("From array");
		return _implicit;
	}

	// Strings are objects too
	if (JPEnv::getHost()->isString(obj))
	{
		TRACE1("From string");
		return _implicit;
	}

	// Class are objects too
	if (JPEnv::getHost()->isClass(obj) || JPEnv::getHost()->isArrayClass(obj))
	{
		TRACE1("implicit array class");
		return _implicit;
	}

	// Let'a allow primitives (int, long, float and boolean) to convert implicitly too ...
	if (JPEnv::getHost()->isInt(obj))
	{
		TRACE1("implicit int");
		return _implicit;
	}

	if (JPEnv::getHost()->isLong(obj))
	{
		TRACE1("implicit long");
		return _implicit;
	}

	if (JPEnv::getHost()->isFloat(obj))
	{
		TRACE1("implicit float");
		return _implicit;
	}

	if (JPEnv::getHost()->isBoolean(obj))
	{
		TRACE1("implicit boolean");
		return _implicit;
	}

	return _none;
	TRACE_OUT;
}

jvalue JPObjectBaseClass::convertToJava(HostRef* obj)
{
	TRACE_IN("JPObjectBaseClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;
	const string& simpleName = m_Name.getSimpleName();

	// assume it is convertible;
	if (JPEnv::getHost()->isNone(obj))
	{
		res.l = NULL;
		return res;
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* ref = JPEnv::getHost()->asObject(obj);
		res.l = frame.keep(ref->getObject());
		return res;
	}

	if (JPEnv::getHost()->isString(obj))
	{
		JPClass* type = JPTypeManager::_java_lang_String;
		res = type->convertToJava(obj);
		res.l = frame.keep(res.l);
		return res;
	}

	if (JPEnv::getHost()->isInt(obj))
	{
		JPClass* t = JPTypeManager::_int;
		res.l = frame.keep(t->convertToJavaObject(obj));
		return res;
	}

	else if (JPEnv::getHost()->isLong(obj))
	{
		JPClass* t = JPTypeManager::_long;
		res.l = frame.keep(t->convertToJavaObject(obj));
		return res;
	}

	else if (JPEnv::getHost()->isFloat(obj))
	{
		JPClass* t = JPTypeManager::_double;
		res.l = frame.keep(t->convertToJavaObject(obj));
		return res;
	}

	else if (JPEnv::getHost()->isBoolean(obj))
	{
		JPClass* t = JPTypeManager::_boolean;
		res.l = frame.keep(t->convertToJavaObject(obj));
		return res;
	}

	else if (JPEnv::getHost()->isArray(obj) && simpleName == "java.lang.Object")
	{
		JPArray* a = JPEnv::getHost()->asArray(obj);
		res = a->getValue();
		res.l = frame.keep(res.l);
		return res;
	}

	else if (JPEnv::getHost()->isClass(obj))
	{
		JPClass* type = JPTypeManager::findClass(JPJni::s_ClassClass);
		res.l = frame.keep(type->convertToJavaObject(obj));
		return res;
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		res = JPEnv::getHost()->getWrapperValue(obj); // FIXME isn't this one global already
		res.l = frame.keep(res.l);
		return res;
	}

	return res;
	TRACE_OUT;
}

//=======================================================

JPClassBaseClass::JPClassBaseClass() :
	JPClassClass(JPJni::s_ClassClass)
{
}

JPClassBaseClass::JPClassBaseClass() :
{
}

EMatchType JPClassBaseClass::canConvertToJava(HostRef* obj)
{
	EMatchType base = JPObjectClass::canConvertToJava(obj);
	if (base!=none)
		return basel

	JPLocalFrame frame;
	if (JPEnv::getHost()->isClass(obj))
	{
		return _exact;
	}
	return _none;
	TRACE_OUT;
}

jvalue JPClassBaseClass::convertToJava(HostRef* obj)
{
	TRACE_IN("JPObjectClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;
	const string& simpleName = m_Name.getSimpleName();

	// assume it is convertible;
	if (JPEnv::getHost()->isNone(obj))
	{
		res.l = NULL;
		return res;
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* ref = JPEnv::getHost()->asObject(obj);
		res.l = frame.keep(ref->getObject());
		return res;
	}

	if (JPEnv::getHost()->isClass(obj))
	{
		JPObjectClass* w = JPEnv::getHost()->asClass(obj);
		jclass lr = w->getNativeClass();
		res.l = lr;
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		res = JPEnv::getHost()->getWrapperValue(obj); // FIXME isn't this one global already
		res.l = frame.keep(res.l);
		return res;
	}

	return res;
	TRACE_OUT;
}

