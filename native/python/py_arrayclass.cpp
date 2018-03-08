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
  {"newArray",     &PyJPArrayClass::newArray, METH_VARARGS, ""},
  {NULL},
};

static PyTypeObject arrayclassClassType = 
{
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "JavaArrayClass",          /*tp_name*/
  sizeof(PyJPArrayClass),    /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  PyJPArrayClass::__dealloc__, /*tp_dealloc*/
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
  "Java Array Class",        /*tp_doc */
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

PyObject* PyJPArrayClass::Type = (PyObject*)&arrayclassClassType; 

// Static methods
void PyJPArrayClass::initType(PyObject* module)
{
  PyType_Ready(&arrayclassClassType);
  PyModule_AddObject(module, "_JavaArrayClass", (PyObject*)&arrayclassClassType); 
}

PyJPArrayClass* PyJPClass::alloc(JPArrayClass* cls)
{
  PyJPClass* res = PyObject_New(PyJPClass, &arrayclassClassType);
  res->m_Class = cls;
  return res;
}

void PyJPArrayClass::__dealloc__(PyObject* o)
{
  TRACE_IN("PyJPArrayClass::__dealloc__");
  PyJPArrayClass* self = (PyJPArrayClass*)o;
  Py_TYPE(self)->tp_free(o);
  TRACE_OUT;
}

bool PyJPArrayClass::check(PyObject* o)
{
	return PyObject_IsInstance(pyobj, arrayclassClassType);
}

PyObject* PyJArrayClass::newArray(PyObject* o, PyObject* arg)
{
  try  
  {
    PyJPArrayClass* self = (PyJPClass*)o;
    int sz;
    PyArg_ParseTuple(arg, "i", &sz);
    JPArrayClass* a = self->m_Class;
    JPArray* v = a->newInstance(sz);
    return JPyCapsule::fromArray(v);
  }
  PY_STANDARD_CATCH
  return NULL;
}

// =================================================================
// Global functions

PyObject* PyJArrayClass::findArrayClass(PyObject* obj, PyObject* args)
{
  if (! JPEnv::isInitialized())
  {
    PyErr_SetString(PyExc_RuntimeError, "Java Subsystem not started");
    return NULL;
  }

  try {
    char* cname;
    PyArg_ParseTuple(args, "s", &cname);

    string name = JPTypeManager::getQualifiedName(cname);
    JPArrayClass* claz = dynamic_cast<JPArrayClass*>(JPTypeManager::findClassByName(name));
    if (claz == NULL)
    {
      Py_RETURN_NONE;
    }

    return PyJArrayClass::alloc(claz);
  }
  PY_STANDARD_CATCH;

  PyErr_Clear();

  Py_RETURN_NONE;
}


