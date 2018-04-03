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



jobject JPPrimitiveType::convertToJavaObject(PyObject* pyobj)
{
	JPLocalFrame frame;
	JPObjectClass* c = getBoxedClass();

	// Create a vector with one element needed for newInstance
	vector<PyObject*> args(1);
	args[0] = pyobj;

	// Call the new instance
	JPValue val = c->newInstance(args);
	jobject res = val.getObject();
	return frame.keep(res);
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

