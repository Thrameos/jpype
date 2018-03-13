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

JPObjectBaseClass::~JPObjectBaseClass()
{
}

EMatchType JPObjectBaseClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

  EMatchType base=JPObjectClass::canConvertToJava(obj);
	if (base!=_none)
		return base;

	// Implicit rules for java.lang.Object
	JPLocalFrame frame;
	TRACE_IN("JPObjectBaseClass::canConvertToJava");

	// arrays are objects
	if (obj.isJavaValue())
	{
		TRACE1("From jvalue");
		return _implicit;
	}

	// arrays are objects
	if (obj.isArray())
	{
		TRACE1("From array");
		return _implicit;
	}

	// Strings are objects too
	if (obj.isString())
	{
		TRACE1("From string");
		return _implicit;
	}

	// Class are objects too
	if (obj.isJavaClass() || obj.isArrayClass())
	{
		TRACE1("implicit array class");
		return _implicit;
	}

	// Let'a allow primitives (int, long, float and boolean) to convert implicitly too ...
	if (obj.isInt())
	{
		TRACE1("implicit int");
		return _implicit;
	}

	if (obj.isLong())
	{
		TRACE1("implicit long");
		return _implicit;
	}

	if (obj.isFloat())
	{
		TRACE1("implicit float");
		return _implicit;
	}

	if (obj.isBoolean())
	{
		TRACE1("implicit boolean");
		return _implicit;
	}

	return _none;
	TRACE_OUT;
}

// java.lang.Object can be converted to from all object classes, 
// all primitive types (via boxing), strings, arrays, and python bridge classes
jvalue JPObjectBaseClass::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	TRACE_IN("JPObjectBaseClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;
	res.l = NULL;

	// assume it is convertible;
	if (obj.isNone())
	{
		return res;
	}

	else if (obj.isJavaValue())
	{
		const JPValue& ref = obj.asJavaValue();
		if (ref.getClass()->isObjectType())
			res.l = ref.getObject();
		else
			res.l = ((JPPrimitiveType*)ref.getClass())->convertToJavaObject(pyobj);
	}

	else if (obj.isString())
	{
		res = JPTypeManager::_java_lang_String->convertToJava(obj);
	}

	else if (obj.isInt())
	{
		res.l = JPTypeManager::_int->convertToJavaObject(obj);
	}

	else if (obj.isLong())
	{
		res.l = JPTypeManager::_long->convertToJavaObject(obj);
	}

	else if (obj.isFloat())
	{
		res.l = JPTypeManager::_double->convertToJavaObject(obj);
	}

	else if (obj.isBoolean())
	{
		res.l = JPTypeManager::_boolean->convertToJavaObject(obj);
	}

	else if (obj.isArray())
	{
		JPArray* a = obj.asArray();
		res = a->getValue();
	}

	else if (obj.isJavaClass())
	{
		res.l = JPTypeManager::_java_lang_Class->convertToJavaObject(obj);
	}

	else if (obj.isProxy())
	{
		JPProxy* proxy = obj.asProxy();
		res.l = proxy->getProxy();
	}

	res.l = frame.keep(res.l);
	return res;
	TRACE_OUT;
}

void JPObjectBaseClass::loadSuperClass()
{
	// This does not apply to java.lang.Object
}


//=======================================================

JPClassBaseClass::JPClassBaseClass() :
	JPObjectClass(JPJni::s_ClassClass)
{
}

JPClassBaseClass::~JPClassBaseClass()
{
}

EMatchType JPClassBaseClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	TRACE_IN("JPClassBaseClass::convertToJava");
	EMatchType base = JPObjectClass::canConvertToJava(obj);
	if (base != _none)
		return base;

	JPLocalFrame frame;
	if (obj.isNone())
		return _implicit;

	if (obj.isJavaClass())
		return _exact;

	if (obj.isJavaValue())
	{
		JPClass* cls = obj.asJavaValue().getClass();
		if (cls == JPTypeManager::_java_lang_Class)
			return _exact;
	}
	return _none;
	TRACE_OUT;
}

jvalue JPClassBaseClass::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	TRACE_IN("JPObjectClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;

	// assume it is convertible;
	if (obj.isNone())
	{
		return res;
	}

	else if (obj.isJavaValue())
	{
		const JPValue& ref = obj.asJavaValue();
		res = ref.getValue();
	}

	else if (obj.isJavaClass())
	{
		JPClass* w = obj.asJavaClass();
		jclass lr = w->getNativeClass();
		res.l = lr;
	}

	res.l = frame.keep(res.l);
	return res;
	TRACE_OUT;
}

