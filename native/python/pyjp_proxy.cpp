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
  {NULL},
};

PyTypeObject PyJPProxy::Type =
{
  PyVarObject_HEAD_INIT(NULL, 0)
  /* tp_name           */ "PyJPProxy",
  /* tp_basicsize      */ sizeof(PyJPProxy),
  /* tp_itemsize       */ 0,
  /* tp_dealloc        */ (destructor)PyJPProxy::__dealloc__,
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
  /* tp_str            */ (reprfunc)PyJPProxy::__str__,
  /* tp_getattro       */ 0,
  /* tp_setattro       */ 0,
  /* tp_as_buffer      */ 0,
  /* tp_flags          */ Py_TPFLAGS_DEFAULT,
  /* tp_doc            */ "Java Proxy",
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
  /* tp_init           */ (initproc)PyJPProxy::__init__,
  /* tp_alloc          */ 0,
  /* tp_new            */ PyType_GenericNew
};

// Static methods
void PyJPProxy::initType(PyObject* module)
{
  PyType_Ready(&PyJPProxy::Type);
	Py_INCREF(&PyJPProxy::Type);
  PyModule_AddObject(module, "PyJPProxy", (PyObject*)&PyJPProxy::Type);
}

int PyJPProxy::__init__(PyJPProxy* self, PyObject* args, PyObject* kwargs)
{
  TRACE_IN("PyJPProxy::init");
	JPLocalFrame frame;
	try {
		JPPyni::assertInitialized();
		JPyCleaner cleaner;

		// Parse arguments
		PyObject* pyintf;
		if (!PyArg_ParseTuple(args, "O", &pyintf))
		{
			return -1;
		}

		// Pack interfaces
		std::vector<JPObjectClass*> interfaces;
		JPySequence intf(pyintf);  // FIXME check that this is a sequence
		jlong len = intf.size();
		for (jlong i = 0; i < len; i++)
		{
			PyObject* subObj = cleaner.add(intf.getItem(i));
      JPClass* cls = JPyObject(subObj).asJavaClass();
			JPObjectClass* c = dynamic_cast<JPObjectClass*>(cls);
			if ( c == NULL)
			{
				RAISE(JPypeException, "interfaces must be object class instances");
			}
			interfaces.push_back(c);
		}
		self->m_Proxy = new JPProxy((PyObject*)self, interfaces);
		return 0;
	}
	PY_STANDARD_CATCH
  return -1;
  TRACE_OUT;
}

PyObject* PyJPProxy::__str__(PyJPProxy* self)
{
	JPLocalFrame frame;
	stringstream sout;
	sout << "<java proxy>";
	return JPyString::fromString(sout.str());
}

void PyJPProxy::__dealloc__(PyJPProxy* self)
{
  TRACE_IN("PyJPProxy::dealloc");
  delete self->m_Proxy;
	Py_TYPE(self)->tp_free(self);
  TRACE_OUT;
}

bool PyJPProxy::check(PyObject* o)
{
  return o->ob_type == &PyJPProxy::Type;
}

