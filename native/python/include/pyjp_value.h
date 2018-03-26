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
#ifndef _PYJP_VALUE_H_
#define _PYJP_VALUE_H_

struct PyJPValue
{
	PyObject_HEAD

	static PyObject* alloc(const JPValue& value);
	static PyObject* alloc(JPClass* cls, jvalue value);
	static const JPValue&  getValue(PyObject* self);

	// Object A
	static PyTypeObject Type;
	static void        initType(PyObject* module);
	static bool        check(PyObject* o);
	static void        __dealloc__(PyObject* self);

	// Python-visible methods
	static PyObject*   getJavaClass(PyObject* self, PyObject* args);
	static PyObject*   getPythonClass(PyObject* self, PyObject* args);

	// Module global methods
	static PyObject*   convertToJavaValue(PyObject* moduel, PyObject* args);

	JPValue m_Value;
};

#endif // _PYJP_VALUE_H_2
