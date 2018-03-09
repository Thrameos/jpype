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
#ifndef _PYJARRAYCLASS_H_
#define _PYJARRAYCLASS_H_

struct PyJPArrayClass
{
	//AT's comments on porting:
	//  1) Some Unix compilers do not tolerate the semicolumn after PyObject_HEAD	
	//PyObject_HEAD;
	PyObject_HEAD

	// Module global methods
	static PyObject* findArrayClass(PyObject* obj, PyObject* args);

	// Python-visible methods
	static void        initType(PyObject* module);
	static PyJPArrayClass*  alloc(JPArrayClass* cls);
	static bool        check(PyObject* o);
	static void        __dealloc__(PyObject* o);

	static PyObject* newArray(PyObject* self, PyObject* arg);
	JPArrayClass* m_Class;

	static PyObject* Type;  
};

#endif // _PYJARRAYCLASS_H_
