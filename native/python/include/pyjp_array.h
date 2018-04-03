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
#ifndef _PYJARRAY_H_
#define _PYJARRAY_H_

struct PyJPArray
{
	//AT's comments on porting:
	//  1) Some Unix compilers do not tolerate the semicolumn after PyObject_HEAD	
	//PyObject_HEAD;
	PyObject_HEAD

	// Python-visible methods
	static void        initType(PyObject* module);
	static PyJPArray*  alloc(JPArray* cls);
	static bool        check(PyObject* o);
	static int         __init__(PyJPArray* self, PyObject* args, PyObject* kwargs);
	static void        __dealloc__(PyJPArray* o);
	
	static PyObject* getArrayLength(PyJPArray* self, PyObject* arg);
	static PyObject* getArrayItem(PyJPArray* self, PyObject* arg);
	static PyObject* getArraySlice(PyJPArray* self, PyObject* arg);
	static PyObject* setArraySlice(PyJPArray* self, PyObject* arg);
	static PyObject* setArrayItem(PyJPArray* self, PyObject* arg);
	static PyObject* setArrayValues(PyJPArray* self, PyObject* arg);

	JPArray* m_Object;
};

#endif // _PYJARRAY_H_
