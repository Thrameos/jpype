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
  {"getJavaClass",   (PyCFunction)&PyJPValue::getJavaClass, METH_NOARGS, ""},
  {"getPythonClass", (PyCFunction)&PyJPValue::getPythonClass, METH_NOARGS, ""},
  {NULL},
};

PyTypeObject PyJPValue::Type = 
{
  PyVarObject_HEAD_INIT(NULL, 0)
  "PyJPValue",                        /* tp_name */
  sizeof(PyJPValue),                  /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)PyJPValue::__dealloc__, /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  (reprfunc)PyJPValue::__str__,       /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  "Java Value",                       /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  classMethods,                       /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  (initproc)PyJPValue::__init__,      /* tp_init */
  0,                                  /* tp_alloc */
  PyJPValue::__new__                  /* tp_new */
};

// Static methods
void PyJPValue::initType(PyObject* module)
{
  PyType_Ready(&PyJPValue::Type);
	Py_INCREF(&PyJPValue::Type);
  PyModule_AddObject(module, "PyJPValue", (PyObject*)&PyJPValue::Type); 
}

bool PyJPValue::check(PyObject* o)
{
  return o->ob_type == &PyJPValue::Type;
}

// These are from the internal methods when we alreayd have the jvalue
PyObject* PyJPValue::alloc(const JPValue& value)
{
  return alloc(value.getClass(), value.getValue());
}

PyObject* PyJPValue::alloc(JPClass* cls, jvalue value)
{
  TRACE_IN("PyJPValue::alloc");
  PyJPValue* res = PyObject_New(PyJPValue, &PyJPValue::Type);
  if (cls->isObjectType())
    value.l = JPEnv::getJava()->NewGlobalRef(value.l);
  res->m_Value = JPValue(cls, value);
	TRACE3("Value", res->m_Value.getClass(), &(res->m_Value.getValue()));
  return (PyObject*)res;
  TRACE_OUT;
}

PyObject* PyJPValue::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPValue* self = (PyJPValue*)type->tp_alloc(type, 0);
	jvalue v;
	self->m_Value = JPValue(NULL, v);
	return (PyObject*)self;
}

// Replacement for convertToJava.
int PyJPValue::__init__(PyJPValue* self, PyObject* args, PyObject* kwargs)
{
  TRACE_IN("PyJPValue::init");
  JPLocalFrame frame;
  try
	{
    JPPyni::assertInitialized();
    PyObject* claz;
    PyObject* value;

    if (!PyArg_ParseTuple(args, "O!O", &PyJPClass::Type, &claz, &value))
		{
			return -1;
		}

    JPClass* type = ((PyJPClass*)claz)->m_Class;
    if (type == NULL)
    {
      return -1;
    }

    jvalue v = type->convertToJava(value);
		self->m_Value = JPValue(type, v);
  }
  PY_STANDARD_CATCH
  return 0;
  TRACE_OUT;
}

PyObject* PyJPValue::__str__(PyJPValue* self)
{
	JPLocalFrame frame;
	stringstream sout;
	sout << "<java value " << self->m_Value.getClass()->getSimpleName() << ">";
	return JPyString::fromString(sout.str());
}

void PyJPValue::__dealloc__(PyJPValue* self)
{
  TRACE_IN("PyJPValue::dealloc");
  JPValue& value = self->m_Value;
	if (value.getClass()!=NULL)
	{
		TRACE3("Value", value.getClass(), &(value.getValue()));
		if (value.getClass()->isObjectType())
		{
			TRACE1("Dereference object");
			JPEnv::getJava()->DeleteGlobalRef(value.getValue().l);
		}
	}
	TRACE2("free", Py_TYPE(self)->tp_free);
	Py_TYPE(self)->tp_free(self);
  TRACE_OUT;
}


/** Get a java object representing the java.lang.Class */
PyObject* PyJPValue::getJavaClass(PyJPValue* self, PyObject* args)
{
  TRACE_IN("PyJPValue::getJavaClass");
  JPLocalFrame frame;
	try
  {
    jvalue v;
    v.l = (jobject)(self->m_Value.getClass()->getNativeClass());
    return JPTypeManager::_java_lang_Class->asHostObject(v);
  }
  PY_STANDARD_CATCH
  return NULL;
  TRACE_OUT;
}

/** Get the python class for this value */
PyObject* PyJPValue::getPythonClass(PyJPValue* self, PyObject* args)
{
  TRACE_IN("PyJPValue::getPythonClass");
  JPLocalFrame frame;
  try
	{
    JPClass* cls = self->m_Value.getClass();
		if (cls->isArray())
		{
			JPArrayClass* acls = (JPArrayClass*)cls;
			return JPPyni::newArrayClass(acls);
		}
		else if (cls->isObjectType())
		{
			JPObjectClass* ocls = (JPObjectClass*)cls;
			return JPPyni::newClass(ocls);
		}
    Py_RETURN_NONE;
  }
  PY_STANDARD_CATCH
  return NULL;
  TRACE_OUT;
}

/*
// =================================================================
// Global functions

PyObject* PyJPValue::convertToJavaValue(PyObject* module, PyObject* args)
{
  TRACE_IN("PyJPValue::convertToJavaValue");
  JPLocalFrame frame;
  try
	{
    JPPyni::assertInitialized();
    PyObject* claz;
    PyObject* value;

    PyArg_ParseTuple(args, "O!O", &PyJPClass::Type, &claz, &value);
    JPClass* type = ((PyJPClass*)claz)->m_Class;
    if (type == NULL)
    {
      Py_RETURN_NONE;
    }

    jvalue v = type->convertToJava(value);
    return alloc(type, v);
  }
  PY_STANDARD_CATCH
  return NULL;
  TRACE_OUT;
}
*/

