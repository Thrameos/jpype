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
#ifndef _PYFIELD_H_
#define _PYFIELD_H_

struct PyJPField
{
	PyObject_HEAD
	
	static void       initType(PyObject* module);
	static PyJPField* alloc(JPField* mth);

	// Python-visible methods
	static void       __dealloc__(PyJPField* o);
	static PyObject*  getName(PyJPField* self, PyObject* arg);
	static PyObject*  getStaticAttribute(PyJPField* self, PyObject* arg);
	static PyObject*  setStaticAttribute(PyJPField* self, PyObject* arg);
	static PyObject*  setInstanceAttribute(PyJPField* self, PyObject* arg);
	static PyObject*  getInstanceAttribute(PyJPField* self, PyObject* arg);
	static PyObject*  isStatic(PyJPField* self, PyObject* arg);
	static PyObject*  isFinal(PyJPField* self, PyObject* arg);

	JPField* m_Field;
};

#endif // _PYFIELD_H_
