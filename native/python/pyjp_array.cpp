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
  {"getArrayLength", (PyCFunction)&PyJPArray::getArrayLength, METH_NOARGS, ""},
  {"getArrayItem",   (PyCFunction)&PyJPArray::getArrayItem, METH_VARARGS, ""},
  {"setArrayItem",   (PyCFunction)&PyJPArray::setArrayItem, METH_VARARGS, ""},
  {"getArraySlice",  (PyCFunction)&PyJPArray::getArraySlice, METH_VARARGS, ""},
  {"setArraySlice",  (PyCFunction)&PyJPArray::setArraySlice, METH_VARARGS, ""},

  {NULL},
};

static PyTypeObject arrayClassType = 
{
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  /* tp_name           */ "PyJPArray",
  /* tp_basicsize      */ sizeof(PyJPArray),
  /* tp_itemsize       */ 0,
  /* tp_dealloc        */ (destructor)PyJPArray::__dealloc__,
  /* tp_print          */ 0,
  /* tp_getattr        */ 0,
  /* tp_setattr        */ 0,
  /* tp_compare        */ 0,
  /* tp_repr           */ 0,
  /* tp_as_number      */ 0,
  /* tp_as_sequence    */ 0,
  /* tp_as_mapping     */ 0,
  /* tp_hash           */ 0,
  /* tp_call           */ 0,
  /* tp_str            */ 0,
  /* tp_getattro       */ 0,
  /* tp_setattro       */ 0,
  /* tp_as_buffer      */ 0,
  /* tp_flags          */ Py_TPFLAGS_DEFAULT,
  /* tp_doc            */ "Java Array",
  /* tp_traverse       */ 0,
  /* tp_clear          */ 0,
  /* tp_richcompare    */ 0,
  /* tp_weaklistoffset */ 0,
  /* tp_iter           */ 0,
  /* tp_iternext       */ 0,
  /* tp_methods        */ classMethods,
  /* tp_members        */ 0,
  /* tp_getset         */ 0,
  /* tp_base           */ 0,
  /* tp_dict           */ 0,
  /* tp_descr_get      */ 0,
  /* tp_descr_set      */ 0,
  /* tp_dictoffset     */ 0,
  /* tp_init           */ (initproc)PyJPArray::__init__,
  /* tp_alloc          */ 0,
  /* tp_new            */ PyType_GenericNew
};


// Static methods
void PyJPArray::initType(PyObject* module)
{
  PyType_Ready(&arrayClassType);
	Py_INCREF(&arrayClassType);
  PyModule_AddObject(module, "PyJPArray", (PyObject*)&arrayClassType);
}

PyJPArray* PyJPArray::alloc(JPArray* obj)
{
  PyJPArray* res = PyObject_New(PyJPArray, &arrayClassType);
  res->m_Object = obj;
  return res;
}

int PyJPArray::__init__(PyJPArray* self, PyObject* arg, PyObject* kwargs)
{
  try
  {
		// Parse arguments
    int sz;
		PyJPClass* pycls;
    if (!PyArg_ParseTuple(arg, "O!i", &PyJPClass, &pycls, &sz))
		{
			return -1;
		}

		JPArrayClass* jcls = dynamic_cast<JPArrayClass*>(pycls->m_Class);
		if (jcls == NULL)
		{
			JPyErr::setRuntimeError("invalid __javaclass__");
			return -1;
		}

		// Create a new instance
    self->m_Object = jcls->newInstance(sz);
		return 0;
  }
  PY_STANDARD_CATCH
  return NULL;
}

void PyJPArray::__dealloc__(PyObject* o)
{
  TRACE_IN("PyJPArray::__dealloc__");
  PyJPArray* self = (PyJPArray*)o;
  delete self->m_Object;	
	Py_TYPE(o)->tp_free(o);
  TRACE_OUT;
}

bool PyJPArray::check(PyObject* o)
{
	return PyObject_IsInstance(o, (PyObject*)&arrayClassType);
}

PyObject* PyJPArray::getArrayLength(PyJPArray* self, PyObject* arg)
{
	TRACE_IN("PyJPArray::getArrayLength")
  JPLocalFrame frame;
	try {
		return JPyInt::fromLong(self->m_Object->getLength());
	}
	PY_STANDARD_CATCH
	return NULL;
	TRACE_OUT
}

PyObject* PyJPArray::getArrayItem(PyJPArray* self, PyObject* arg)
{
	TRACE_IN("PyJPArray::getArrayItem")
  JPLocalFrame frame;
	try {
		// Parse arguments
		int ndx;
		if (!PyArg_ParseTuple(arg, "i", &ndx))
		{
			return NULL;
		}

		// Get item
		JPArray* a = self->m_Object;
		int length = a->getLength();
		if (ndx < 0) ndx = length + ndx;
		return a->getItem(ndx);
	}
	PY_STANDARD_CATCH
	return NULL;
	TRACE_OUT
}

PyObject* PyJPArray::setArrayItem(PyJPArray* self, PyObject* arg)
{
	TRACE_IN("PyJPArray::setArrayItem")
  JPLocalFrame frame;
	try {
		// Parse arguments
		int ndx;
		PyObject* value;
		if (!PyArg_ParseTuple(arg, "iO", &ndx, &value))
		{
			return NULL;
		}

		// Set item
		JPArray* a = self->m_Object;
		int length = a->getLength();
		if (ndx < 0) ndx = length + ndx;
		self->m_Object->setItem(ndx, arg);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH
	return NULL;
	TRACE_OUT
}

PyObject* PyJPArray::getArraySlice(PyJPArray* self, PyObject* arg)
{
  JPLocalFrame frame;
	TRACE_IN("PyJPArray::getArraySlice")
	try
	{
    // Parse arguments
		int lo = -1;
		int hi = -1;
		if (!PyArg_ParseTuple(arg, "ii", &lo, &hi))
		{
			return NULL;
		}

		JPArray* a = self->m_Object;
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
	} 
	PY_STANDARD_CATCH
	return NULL;
	TRACE_OUT
}

PyObject* PyJPArray::setArraySlice(PyJPArray* self, PyObject* arg)
{
  JPLocalFrame frame;
	TRACE_IN("PyJPArray::setArraySlice")
	try {
		// Parse arguments
		int lo = -1;
		int hi = -1;
		PyObject* pysequence;
		if (!PyArg_ParseTuple(arg, "iiO", &lo, &hi, &pysequence))
		{
			return NULL;
		}

		// Set slice
		JPArray* a = self->m_Object;
		int length = a->getLength();
		int length2 = JPySequence(pysequence).size();

		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		if (hi-lo != length2)
		{
			// Replicate behavior of np.array under similar situation
			PyErr_Format(PyExc_ValueError, "cannot copy sequence with size %d to array axis with dimension %d", length2, hi-lo);
			return NULL;
		}

		// FIXME handle single values as well as sequences on set
		if (!JPySequence::check(pysequence))
		{
			PyErr_SetString(PyExc_ValueError, "slices must be assigned with a sequencec");
			return NULL;
		}

		JPClass* component = a->getClass()->getComponentType();
		if (!component->isObjectType())
		{
			// for primitive types, we have fast setters available
			a->setRangePrimitive(lo, hi, pysequence);
		}
		else
		{
			JPyCleaner cleaner;
			JPySequence sequence(pysequence);

			// Convert the sequence to a java vector of PyObject*
			vector<PyObject*> values;
			values.reserve(hi - lo);
			for (Py_ssize_t i = 0; i < hi - lo; i++)
			{
				PyObject* v = cleaner.add(sequence.getItem(i));
				values.push_back(v);
			}

			a->setRange(lo, hi, values);
		}

		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH
	return NULL;
	TRACE_OUT
}

