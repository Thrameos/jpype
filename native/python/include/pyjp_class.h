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
#ifndef _PYCLASS_H_
#define _PYCLASS_H_

struct PyJPClass
{
	//AT's comments on porting:
	//  1) Some Unix compilers do not tolerate the semicolumn after PyObject_HEAD	
	//PyObject_HEAD;
	PyObject_HEAD

	// Module global methods
	static PyObject* findClass(PyObject* obj, PyObject* args);
	static PyObject* findPrimitiveClass(PyObject* obj, PyObject* args);

	// Internal side utility functions
	static PyJPClass*   alloc(JPClass* cls);
	static bool   check(PyObject* o);
	static void         initType(PyObject* module);
	
	// Python-visible methods
	static PyTypeObject Type;
	static void      __dealloc__(PyJPClass* o);

	static PyObject* getName(PyJPClass* self, PyObject* arg);
	static PyObject* getBaseClass(PyJPClass* self, PyObject* arg);
	static PyObject* getBaseInterfaces(PyJPClass* self, PyObject* arg);
	static PyObject* getClassMethods(PyJPClass* self, PyObject* arg);
	static PyObject* getClassFields(PyJPClass* self, PyObject* arg);
	static PyObject* newClassInstance(PyJPClass* self, PyObject* arg);
	static PyObject* isInterface(PyJPClass* self, PyObject* arg);
	static PyObject* isPrimitive(PyJPClass* self, PyObject* arg);
	static PyObject* isSubclass(PyJPClass* self, PyObject* arg);
	static PyObject* isException(PyJPClass* self, PyObject* arg);
	static PyObject* isArray(PyJPClass* self, PyObject* arg);
	static PyObject* isAbstract(PyJPClass* self, PyObject* arg);

	static PyObject* getConstructors(PyJPClass* self);
	static PyObject* getDeclaredConstructors(PyJPClass* self);
	static PyObject* getDeclaredFields(PyJPClass* self);
	static PyObject* getDeclaredMethods(PyJPClass* self);
	static PyObject* getFields(PyJPClass* self);
	static PyObject* getMethods(PyJPClass* self);
	static PyObject* getModifiers(PyJPClass* self);

	JPClass* m_Class;
};

#endif // _PYCLASS_H_
