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

/** This contains the specializations needed for converting primitive types to and from python types  
 * when passing arguments and returns.
 */

JPPrimitiveType::JPPrimitiveType(const string& boxedName) :
    JPClass(JPJni::findPrimitiveClass(boxedName))
{
	// Get the boxed java class
	m_BoxedClass=(JPObjectClass*)JPTypeManager::findClass(JPJni::findClass(boxedName));
}

JPPrimitiveType::~JPPrimitiveType()
{
}
	

// These are singletons created by the type manager.
JPVoidType::JPVoidType() : JPPrimitiveType("java/lang/Void") {}
JPBooleanType::JPBooleanType() : JPPrimitiveType("java/lang/Boolean") {}
JPByteType::JPByteType() : JPPrimitiveType("java/lang/Byte") {}
JPCharType::JPCharType() : JPPrimitiveType("java/lang/Character") {}
JPShortType::JPShortType() : JPPrimitiveType("java/lang/Short") {}
JPIntType::JPIntType() : JPPrimitiveType("java/lang/Integer") {}
JPLongType::JPLongType() : JPPrimitiveType("java/lang/Long") {}
JPFloatType::JPFloatType() : JPPrimitiveType("java/lang/Float") {}
JPDoubleType::JPDoubleType() : JPPrimitiveType("java/lang/Double") {}

JPObjectClass* JPPrimitiveType::getBoxedClass()
{
	return m_BoxedClass;
}

bool checkWrapper(PyObject* pyobj, JPClass* cls)
{
	JPyAdaptor obj(pyobj);
	if (obj.isWrapper())
	{
		JPClass* name = obj.getWrapperClass();
		return (name == cls);
	}
	return false;
}

jobject JPPrimitiveType::convertToJavaObject(PyObject* pyobj)
{
	JPLocalFrame frame;
	JPObjectClass* c = getBoxedClass();

	// Create a vector with one element needed for newInstance
	vector<PyObject*> args(1);
	args[0] = pyobj;

	// Call the new instance
	JPObject* o = c->newInstance(args);
	jobject res = o->getObject(); 

	// Keep only the jobject
	delete o;
	return frame.keep(res);
}

PyObject* JPByteType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.b);
}

PyObject* JPByteType::asHostObjectFromObject(jobject val)
{
	jint v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPByteType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_byte))
	{
		return _exact;
	}

	return _none;
}

jvalue JPByteType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPPyni::setTypeError("Cannot convert value to Java byte");
			JPPyni::raise("JPByteType::convertToJava");
		}
		res.b = (jbyte)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPPyni::setTypeError("Cannot convert value to Java byte");
			JPPyni::raise("JPByteType::convertToJava");
		}
		res.b = (jbyte)l;
	}
	else if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	return res;
}

PyObject* JPByteType::convertToDirectBuffer(PyObject* pysrc)
{
	JPyAdaptor src(pysrc);
	JPLocalFrame frame;
	TRACE_IN("JPByteType::convertToDirectBuffer");
	if (src.isByteBuffer())
	{

		char* rawData;
		long size;
		JPyString(pysrc).getByteBufferPtr(&rawData, size);

		jobject obj = JPEnv::getJava()->NewDirectByteBuffer(rawData, size);

		jvalue v;
		v.l = obj;
		jclass cls = JPJni::getClass(obj);
		JPClass* type = JPTypeManager::findClass(cls);
		return type->asHostObject(v);
	}

	RAISE(JPypeException, "Unable to convert to Direct Buffer");
	TRACE_OUT;
}

//----------------------------------------------------------------------------

PyObject* JPShortType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.s);
}

PyObject* JPShortType::asHostObjectFromObject(jobject val)
{
	jint v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPShortType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_short))
	{
		return _exact;
	}

	return _none;
}

jvalue JPShortType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPPyni::setTypeError("Cannot convert value to Java short");
			JPPyni::raise("JPShortType::convertToJava");
		}

		res.s = (jshort)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPPyni::setTypeError("Cannot convert value to Java short");
			JPPyni::raise("JPShortType::convertToJava");
		}
		res.s = (jshort)l;
	}
	else if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	return res;
}

PyObject* JPShortType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}


//-------------------------------------------------------------------------------


PyObject* JPIntType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.i);
}

PyObject* JPIntType::asHostObjectFromObject(jobject val)
{
	long v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPIntType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isInt())
	{
		if (obj.isJavaObject())
		{
			return _implicit;
		}
		return _exact;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_int))
	{
		return _exact;
	}

	return _none;
}

jvalue JPIntType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPPyni::setTypeError("Cannot convert value to Java int");
			JPPyni::raise("JPIntType::convertToJava");
		}

		res.i = (jint)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPPyni::setTypeError("Cannot convert value to Java int");
			JPPyni::raise("JPIntType::convertToJava");
		}
		res.i = (jint)l;
	}
	else if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}

	return res;
}

PyObject* JPIntType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//-------------------------------------------------------------------------------

PyObject* JPLongType::asHostObject(jvalue val) 
{
	TRACE_IN("JPLongType::asHostObject");
	return JPyLong::fromLong(val.j);
	TRACE_OUT;
}

PyObject* JPLongType::asHostObjectFromObject(jobject val)
{
	jlong v = JPJni::longValue(val);
	return JPyLong::fromLong(v);
} 

EMatchType JPLongType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		if (obj.isJavaObject())
		{
			return _implicit;
		}
		return _exact;
	}

	if (checkWrapper(obj, JPTypeManager::_long))
	{
		return _exact;
	}

	return _none;
}

jvalue JPLongType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isInt())
	{
		res.j = (jlong)JPyInt(obj).asInt();
	}
	else if (obj.isLong())
	{
		res.j = (jlong)JPyLong(obj).asLong();
	}
	else if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	else
	{
		JPPyni::setTypeError("Cannot convert value to Java long");
		JPPyni::raise("JPLongType::convertToJava");
		res.j = 0; // never reached
	}
	return res;
}

PyObject* JPLongType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
	
}

//-------------------------------------------------------------------------------
PyObject* JPFloatType::asHostObject(jvalue val) 
{
	return JPyFloat::fromFloat(val.f);
}

PyObject* JPFloatType::asHostObjectFromObject(jobject val)
{
	// FIXME this is odd
	jdouble v = JPJni::doubleValue(val);
	return JPyFloat::fromDouble(v);
} 

EMatchType JPFloatType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isFloat())
	{
		if (obj.isJavaObject())
		{
			return _implicit;
		}
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_float))
	{
		return _exact;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPFloatType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	else if (obj.isInt())
	{
		res.d = JPyInt(obj).asInt();
	}
	else if (obj.isLong())
	{
		res.d = JPyLong(obj).asLong();
	}
	else
	{
		jdouble l = JPyFloat(obj).asDouble();
		if (l > 0 && (l < JPJni::s_minFloat || l > JPJni::s_maxFloat))
		{
			JPPyni::setTypeError("Cannot convert value to Java float");
			JPPyni::raise("JPFloatType::convertToJava");
		}
		else if (l < 0 && (l > -JPJni::s_minFloat || l < -JPJni::s_maxFloat))
		{
			JPPyni::setTypeError("Cannot convert value to Java float");
			JPPyni::raise("JPFloatType::convertToJava");
		}
		res.f = (jfloat)l;
	}
	return res;
}

PyObject* JPFloatType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

//---------------------------------------------------------------------------

PyObject* JPDoubleType::asHostObject(jvalue val) 
{
	return JPyFloat::fromFloat(val.d);
}

PyObject* JPDoubleType::asHostObjectFromObject(jobject val)
{
	jdouble v = JPJni::doubleValue(val);
	return JPyFloat::fromDouble(v);
} 

EMatchType JPDoubleType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isFloat())
	{
	  if (obj.isJavaObject())
		{
			return _implicit;
		}
		return _exact;
	}

	if (checkWrapper(obj, JPTypeManager::_double))
	{
		return _exact;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPDoubleType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;
	if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	else if (obj.isInt())
	{
		res.d = JPyInt(obj).asInt();
	}
	else if (obj.isLong())
	{
		res.d = JPyLong(obj).asLong();
	}
	else
	{
		res.d = JPyFloat(obj).asDouble();
	}
	return res;
}

PyObject* JPDoubleType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

//----------------------------------------------------------------

PyObject* JPCharType::asHostObject(jvalue val)   
{
	jchar str[2];
	str[0] = val.c;
	str[1] = 0;
	
	return JPyString::fromUnicode(str, 1);
}

PyObject* JPCharType::asHostObjectFromObject(jobject val)
{
	jchar str[2];
	str[0] = JPJni::charValue(val);
	str[1] = 0;
	
	return JPyString::fromUnicode(str, 1);
} 

EMatchType JPCharType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isString() && JPyString(obj).size() == 1)
	{
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_char))
	{
		return _exact;
	}

	return _none;
}

jvalue JPCharType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	jvalue res;

	if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	else
	{
		JCharString str = JPyString(obj).asJCharString();
		res.c = str[0];
	}
	return res;
}

PyObject* JPCharType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

//----------------------------------------------------------------------------------------

PyObject* JPBooleanType::asHostObject(jvalue val) 
{
	if (val.z)
	{
		return JPPyni::getTrue();
	}
	return JPPyni::getFalse();
}

PyObject* JPBooleanType::asHostObjectFromObject(jobject val)
{
	if (JPJni::booleanValue(val))
	{
		return JPPyni::getTrue();
	}
	return JPPyni::getFalse();
} 

EMatchType JPBooleanType::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	if (checkWrapper(obj, JPTypeManager::_boolean))
	{
		return _exact;
	}

	// FIXME what about isTrue and isFalse? Those should be exact

	return _none;
}

jvalue JPBooleanType::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);
	jvalue res;
	if (obj.isWrapper())
	{
		return obj.getWrapperValue();
	}
	else if (obj.isLong())
	{
		res.z = (jboolean)JPyLong(obj).asLong();
	}
	else
	{
		res.z = (jboolean)JPyInt(obj).asInt();
	}
	return res;
}

PyObject* JPBooleanType::convertToDirectBuffer(PyObject* src)
{
		RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

// --------------------------------------
// Type conversion tables

// FIXME I am almost sure these are not used as it appears to be wrong in several locations.
// Remove this when we are sure.

bool JPVoidType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_void;
}

bool JPByteType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_byte
			|| other == JPTypeManager::_short
			|| other == JPTypeManager::_int
			|| other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

bool JPShortType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_short
			|| other == JPTypeManager::_int
			|| other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}


bool JPIntType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_int
			|| other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

bool JPLongType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_long
			|| other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

bool JPFloatType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_float
			|| other == JPTypeManager::_double;
}

bool JPDoubleType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_double;
}

bool JPCharType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_char;
}

bool JPBooleanType::isAssignableTo(const JPClass* other) const
{
	return other == JPTypeManager::_boolean;
}

