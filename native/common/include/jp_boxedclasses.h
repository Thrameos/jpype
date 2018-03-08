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
#ifndef _JPBOXEDCLASS_H_
#define _JPBOXEDCLASS_H_

// Boxed types have special conversion rules so that they can convert
// from python primitives.  This code specializes the class wrappers
// to make that happen.

/**
 * Class to wrap for Boxed types.
 *
 * These are linked to primitives.
 * They specialize the conversion rules to set up our table for conversion.
 */
class JPBoxedClass : public JPObjectClass
{
public:
	JPBoxedClass(jclass c);
	virtual~ JPBoxedClass();

  jvalue convertToJava(PyObject* obj);

private:
  jobject buildObjectWrapper(PyObject* obj);
};

class JPBoxedBooleanClass : public JPBoxedClass
{
public:
	JPBoxedBooleanClass();
	virtual~ JPBoxedBooleanClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedCharacterClass : public JPBoxedClass
{
public:
	JPBoxedCharacterClass();
	virtual~ JPBoxedCharacterClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedByteClass : public JPBoxedClass
{
public:
	JPBoxedByteClass();
	virtual~ JPBoxedByteClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedShortClass : public JPBoxedClass
{
public:
	JPBoxedShortClass();
	virtual~ JPBoxedShortClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedIntegerClass : public JPBoxedClass
{
public:
	JPBoxedIntegerClass();
	virtual~ JPBoxedIntegerClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedLongClass : public JPBoxedClass
{
public:
	JPBoxedLongClass();
	virtual~ JPBoxedLongClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedFloatClass : public JPBoxedClass
{
public:
	JPBoxedFloatClass();
	virtual~ JPBoxedFloatClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

class JPBoxedDoubleClass : public JPBoxedClass
{
public:
	JPBoxedDoubleClass();
	virtual~ JPBoxedDoubleClass();

public:
	virtual EMatchType canConvertToJava(PyObject* obj);
};

#endif // _JPBOXEDCLASS_H_
