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
#include <jpype_python.h>

static PyMethodDef fieldMethods[] = {
  {"getName",              (PyCFunction)&PyJPField::getName, METH_VARARGS, ""},
  {"isFinal",              (PyCFunction)&PyJPField::isFinal, METH_VARARGS, ""},
  {"isStatic",             (PyCFunction)&PyJPField::isStatic, METH_VARARGS, ""},
  {"getStaticAttribute",   (PyCFunction)&PyJPField::getStaticAttribute, METH_VARARGS, ""},
  {"setStaticAttribute",   (PyCFunction)&PyJPField::setStaticAttribute, METH_VARARGS, ""},
  {"getInstanceAttribute", (PyCFunction)&PyJPField::getInstanceAttribute, METH_VARARGS, ""},
  {"setInstanceAttribute", (PyCFunction)&PyJPField::setInstanceAttribute, METH_VARARGS, ""},
	{NULL},
};

static PyTypeObject fieldClassType =
{
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "PyJPField",
	/* tp_basicsize      */ sizeof(PyJPField),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor) PyJPField::__dealloc__,
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
	/* tp_doc            */ "Java Field",
	/* tp_traverse       */ 0,		
	/* tp_clear          */ 0,		
	/* tp_richcompare    */ 0,		
	/* tp_weaklistoffset */ 0,		
	/* tp_iter           */ 0,		
	/* tp_iternext       */ 0,		
	/* tp_methods        */ fieldMethods,
	/* tp_members        */ 0,					
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ 0,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyType_GenericNew
};

// Static methods
void PyJPField::initType(PyObject* module)
{
	PyType_Ready(&fieldClassType);
	Py_INCREF(&fieldClassType);
	PyModule_AddObject(module, "PyJPField", (PyObject*)&fieldClassType);
}

PyJPField* PyJPField::alloc(JPField* m)
{
	PyJPField* res = PyObject_New(PyJPField, &fieldClassType);
	res->m_Field = m;
	return res;
}

void PyJPField::__dealloc__(PyJPField* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* PyJPField::getName(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		string name = self->m_Field->getName();
		return JPyString::fromString(name);
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* PyJPField::getStaticAttribute(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		return self->m_Field->getStaticAttribute();
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* PyJPField::setStaticAttribute(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		PyObject* value;
		PyArg_ParseTuple(arg, "O", &value);
		self->m_Field->setStaticAttribute(value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::setInstanceAttribute(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		PyObject* jo;
		PyObject* value;
		PyArg_ParseTuple(arg, "O!O", &PyJPValue::Type, &jo, &value);
		const JPValue& val = PyJPValue::getValue(value);
		self->m_Field->setAttribute(val.getObject(), value);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPField::getInstanceAttribute(PyJPField* self, PyObject* arg)
{
	TRACE_IN("getInstanceAttribute");
  JPLocalFrame frame(8);
	try {
		PyObject* jo;
		if (!PyArg_ParseTuple(arg, "O!", &PyJPValue::Type, &jo))
		{
			return NULL;
		}
		const JPValue& val = PyJPValue::getValue(jo);
		return self->m_Field->getAttribute(val.getObject());
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

PyObject* PyJPField::isStatic(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		return PyBool_FromLong(self->m_Field->isStatic());
	}
	PY_STANDARD_CATCH
	return NULL;
}

PyObject* PyJPField::isFinal(PyJPField* self, PyObject* arg)
{
  JPLocalFrame frame;
	try {
		return PyBool_FromLong(self->m_Field->isFinal());
	}
	PY_STANDARD_CATCH
	return NULL;
}
