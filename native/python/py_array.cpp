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

#include <jpype_python.h>  

static PyMethodDef classMethods[] = {
  {"getArrayLength", &PyJArray::getArrayLength, METH_NOARGS, ""},
  {"getArrayItem", &PyJArray::getArrayItem, METH_VARARGS, ""},
  {"setArrayItem", &PyJArray::setArrayItem, METH_VARARGS, ""},
  {"getArraySlice", &PyJArray::getArraySlice, METH_VARARGS, ""},
  {"setArraySlice", &PyJArray::setArraySlice, METH_VARARGS, ""},

  {NULL},
};

static PyTypeObject arrayClassType = 
{
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "JavaArray",               /*tp_name*/
  sizeof(PyJPArray),         /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  PyJPArray::__dealloc__,    /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Java Array",              /*tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  classMethods,              /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  0,                         /* tp_init */
  0,                         /* tp_alloc */
  PyType_GenericNew          /* tp_new */
};


// Static methods
void PyJPArrayClass::initType(PyObject* module)
{
  PyType_Ready(&arrayclassClassType);
  PyModule_AddObject(module, "_JavaArray", (PyObject*)&arrayClassType); 
}

PyJPArray* PyJPClass::alloc(JPArray* obj)
{
  PyJPClass* res = PyObject_New(PyJPClass, &arrayClassType);
  res->m_Object = obj;
  return res;
}

void PyJPArrayClass::__dealloc__(PyObject* o)
{
  TRACE_IN("PyJPArray::__dealloc__");
  PyJPArrayClass* self = (PyJPArrayClass*)o;
  Py_TYPE(self)->tp_free(o);
  delete self->m_Object;	
  TRACE_OUT;
}

bool PyJPArray::check(PyObject* o)
{
	return PyObject_IsInstance(pyobj, arrayClassType);
}


PyObject* JPypeJavaArray::getArrayLength(PyObject* o, PyObject* arg)
{
	try {
		PyJArray* self = (PyJArray*)o;
		return JPyInt::fromLong(self->m_Object->getLength());
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* JPypeJavaArray::getArrayItem(PyObject* o, PyObject* arg)
{
	try {
		PyJArray* self = (PyJArray*)o;
		JPArray* a = self->m_Object;
		int ndx;
		PyArg_ParseTuple(arg, "i", &ndx);
		return a->getItem(ndx);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArraySlice(PyObject* self, PyObject* arg)
{
	try
	{
		PyJArray* self = (PyJArray*)o;
		JPArray* a = self->m_Object;

		int lo = -1;
		int hi = -1;
		PyArg_ParseTuple(arg, "ii", &lo, &hi);

		int length = a->getLength();
		// stolen from jcc, to get nice slice support
		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		JPClass* component = a->getClass()->getComponentType();
		if (!component->isObjectType())
		{
			// for primitive types, we have fast sequence generation available
			return a->getSequenceFromRange(lo, hi);
		}
		else
		{
			// slow wrapped access for non primitives
			vector<PyObject*> values = a->getRange(lo, hi);
			JPyCleaner cleaner;
			for (unsigned int i = 0; i < values.size(); i++)
			{
				cleaner.add(values[i]);
			}

			JPyList res = JPyList::newList((int)values.size());
			for (unsigned int i = 0; i < values.size(); i++)
			{
				res.setItem(i, values[i]);
			}

			return res;
		}
	} PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArraySlice(PyObject* o, PyObject* arg)
{
	TRACE_IN("PyJArray::setArraySlice")
	try {
		PyJArray* self = (PyJArray*)o;
		JPArray* a = self->m_Object;

		int lo = -1;
		int hi = -1;
		PyObject* sequence;
		PyArg_ParseTuple(arg, "iiO", &lo, &hi, &sequence);

		int length = a->getLength();
		if(length == 0)
			Py_RETURN_NONE;

		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		JPClass* component = a->getClass()->getComponentType();
		if (!component->isObjectType())
		{
			// for primitive types, we have fast setters available
			a->setRangePrimitive(lo, hi, sequence);
		}
		else
		{
			// slow wrapped access for non primitive types
			vector<PyObject*> values;
			values.reserve(hi - lo);
			JPyCleaner cleaner;
			for (Py_ssize_t i = 0; i < hi - lo; i++)
			{
				PyObject* v = JPySequence(sequence).getItem(i);
				values.push_back(v);
				cleaner.add(v);
			}

			a->setRange(lo, hi, values);
		}

		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT
}

PyObject* JPypeJavaArray::setArrayItem(PyObject* o, PyObject* arg)
{
	try {
		PyJArray* self = (PyJArray*)o;
		int ndx;
		PyObject* value;
		PyArg_ParseTuple(arg, "iO", &ndx, &value);
		self->m_Object->setItem(ndx, arg);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
}
