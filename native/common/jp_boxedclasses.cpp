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

JPBoxedClass::JPBoxedClass(jclass c) :
	JPObjectClass(c)
{
}

JPBoxedClass::~JPBoxedClass()
{
}

jobject JPBoxedClass::buildObjectWrapper(PyObject* obj)
{
	JPLocalFrame frame;

	vector<PyObject*> args(1);
	args.push_back(obj);

	JPValue val = newInstance(args);
	jobject out = val.getObject();

	return frame.keep(out);
}


// Specializations for each of the boxed types.  
// This sets up the table of conversions that we allow

//============================================================


JPBoxedBooleanClass::JPBoxedBooleanClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Boolean"))
{
}

JPBoxedBooleanClass::~JPBoxedBooleanClass()
{
}

EMatchType JPBoxedBooleanClass::canConvertToJava(PyObject* pyobj)
{
	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	return base;
}

//============================================================


JPBoxedCharacterClass::JPBoxedCharacterClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Character"))
{
}

JPBoxedCharacterClass::~JPBoxedCharacterClass()
{
}

EMatchType JPBoxedCharacterClass::canConvertToJava(PyObject* pyobj)
{
	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	return base;
}

//============================================================

JPBoxedByteClass::JPBoxedByteClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Byte"))
{
}

JPBoxedByteClass::~JPBoxedByteClass()
{
}

EMatchType JPBoxedByteClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isInt())
		return _explicit;
	return base;
}

//============================================================

JPBoxedShortClass::JPBoxedShortClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Short"))
{
}

JPBoxedShortClass::~JPBoxedShortClass()
{
}

EMatchType JPBoxedShortClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isInt())
		return _explicit;
	return base;
}

//============================================================


JPBoxedIntegerClass::JPBoxedIntegerClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Integer"))
{
}

JPBoxedIntegerClass::~JPBoxedIntegerClass()
{
}

EMatchType JPBoxedIntegerClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isInt())
		return _explicit;
	return base;
}

//============================================================

JPBoxedLongClass::JPBoxedLongClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Long"))
{
}

JPBoxedLongClass::~JPBoxedLongClass()
{
}

EMatchType JPBoxedLongClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isLong())
		return _explicit;
	return base;
}

//============================================================

JPBoxedFloatClass::JPBoxedFloatClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Float"))
{
}

JPBoxedFloatClass::~JPBoxedFloatClass()
{
}

EMatchType JPBoxedFloatClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isFloat())
		return _explicit;
	return base;
}

//============================================================

JPBoxedDoubleClass::JPBoxedDoubleClass() :
	JPBoxedClass(JPJni::findClass("java/lang/Double"))
{
}

JPBoxedDoubleClass::~JPBoxedDoubleClass()
{
}

EMatchType JPBoxedDoubleClass::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	EMatchType base = JPObjectClass::canConvertToJava(pyobj);
	if (base == _none && obj.isFloat())
		return _explicit;
	return base;
}

