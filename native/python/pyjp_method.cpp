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
#include <jpype_python.h>

static PyMethodDef methodMethods[] = {
  {"getName",        (PyCFunction)&PyJPMethod::getName, METH_VARARGS, ""},
  {"isBeanAccessor", (PyCFunction)&PyJPMethod::isBeanAccessor, METH_VARARGS, ""},
  {"isBeanMutator",  (PyCFunction)&PyJPMethod::isBeanMutator, METH_VARARGS, ""},
  {"matchReport",    (PyCFunction)&PyJPMethod::matchReport, METH_VARARGS, ""},
	{NULL},
};

PyTypeObject PyJPMethod::Type =
{
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "PyJPMethod",
	/* tp_basicsize      */ sizeof(PyJPMethod),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor)PyJPMethod::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ (ternaryfunc)PyJPMethod::__call__,
	/* tp_str            */ (reprfunc)PyJPMethod::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java Method",
	/* tp_traverse       */ 0,
	/* tp_clear          */ 0,
	/* tp_richcompare    */ 0,		
	/* tp_weaklistoffset */ 0,		
	/* tp_iter           */ 0,		
	/* tp_iternext       */ 0,		
	/* tp_methods        */ methodMethods,
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
void PyJPMethod::initType(PyObject* module)
{
	PyType_Ready(&PyJPMethod::Type);
	Py_INCREF(&PyJPMethod::Type);
	PyModule_AddObject(module, "PyJPMethod", (PyObject*)&PyJPMethod::Type);
}

PyJPMethod* PyJPMethod::alloc(JPMethod* m)
{
	PyJPMethod* res = PyObject_New(PyJPMethod, &PyJPMethod::Type);

	res->m_Method = m;
	
	return res;
}

PyObject* PyJPMethod::__call__(PyJPMethod* self, PyObject* pyargs, PyObject* kwargs)
{
	JPLocalFrame frame(32);
	TRACE_IN("PyJPMethod::__call__");
	try {
		TRACE1(self->m_Method->getName());
		JPyCleaner cleaner;
		vector<PyObject*> vargs;
		JPyTuple args(pyargs);
		jlong len = args.size();
		for (jlong i = 0; i < len; i++)
		{
			vargs.push_back(cleaner.add(args.getItem(i))); // return a new ref
		}
		return self->m_Method->invoke(vargs);
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

void PyJPMethod::__dealloc__(PyJPMethod* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* PyJPMethod::__str__(PyJPMethod* self)
{
	JPLocalFrame frame;
	stringstream sout;

	sout << "<method " << self->m_Method->getClassName() << "." << self->m_Method->getName() << ">";

	return JPyString::fromString(sout.str());
}

PyObject* PyJPMethod::isBeanAccessor(PyJPMethod* self, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		bool res = self->m_Method->isBeanAccessor();
		return PyBool_FromLong(res);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::isBeanMutator(PyJPMethod* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		bool res = self->m_Method->isBeanMutator();
		return PyBool_FromLong(res);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::getName(PyJPMethod* self, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		string name = self->m_Method->getName();
		return JPyString::fromString(name);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPMethod::matchReport(PyJPMethod* self, PyObject* pyargs)
{
	JPLocalFrame frame;
	try {
		JPyCleaner cleaner;

		vector<PyObject*> vargs;
		JPySequence args(pyargs);
		jlong len = args.size();
		for (jlong i = 0; i < len; i++)
		{
			vargs.push_back(cleaner.add(args.getItem(i)));
		}

		string report = self->m_Method->matchReport(vargs);

		PyObject* res = JPyString::fromString(report);

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

static PyMethodDef boundMethodMethods[] = {
  {"matchReport", (PyCFunction)&PyJPBoundMethod::matchReport, METH_VARARGS, ""},
	{NULL},
};

PyTypeObject PyJPBoundMethod::Type =
{
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	/* tp_name           */ "PyJPBoundMethod",
	/* tp_basicsize      */ sizeof(PyJPBoundMethod),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor)PyJPBoundMethod::__dealloc__,
	/* tp_print          */ 0,
	/* tp_getattr        */ 0,
	/* tp_setattr        */ 0,
	/* tp_compare        */ 0,
	/* tp_repr           */ 0,
	/* tp_as_number      */ 0,
	/* tp_as_sequence    */ 0,
	/* tp_as_mapping     */ 0,
	/* tp_hash           */ 0,
	/* tp_call           */ (ternaryfunc)PyJPBoundMethod::__call__,
	/* tp_str            */ (reprfunc)PyJPBoundMethod::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java Bound Method",
	/* tp_traverse       */ 0,		
	/* tp_clear          */ 0,		
	/* tp_richcompare    */ 0,		
	/* tp_weaklistoffset */ 0,		
	/* tp_iter           */ 0,		
	/* tp_iternext       */ 0,		
	/* tp_methods        */ boundMethodMethods,
	/* tp_members        */ 0,						
	/* tp_getset         */ 0,
	/* tp_base           */ 0,
	/* tp_dict           */ 0,
	/* tp_descr_get      */ 0,
	/* tp_descr_set      */ 0,
	/* tp_dictoffset     */ 0,
	/* tp_init           */ (initproc)PyJPBoundMethod::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ PyType_GenericNew
};

// Static methods
void PyJPBoundMethod::initType(PyObject* module)
{
	PyType_Ready(&PyJPBoundMethod::Type);
	Py_INCREF(&PyJPBoundMethod::Type);
	PyModule_AddObject(module, "PyJPBoundMethod", (PyObject*)&PyJPBoundMethod::Type);
}

int PyJPBoundMethod::__init__(PyJPBoundMethod* self, PyObject* args, PyObject* kwargs)
{
	try {
		PyObject* javaMethod;
		PyObject* inst;
		if (!PyArg_ParseTuple(args, "O!O", &PyJPMethod::Type, &javaMethod, &inst))
			return -1;

		Py_INCREF(inst);
		Py_INCREF(javaMethod);
		self->m_Instance = inst;
		self->m_Method = (PyJPMethod*)javaMethod;
		return 0;
	}
	PY_STANDARD_CATCH

	return -1;
}

PyObject* PyJPBoundMethod::__call__(PyJPBoundMethod* self, PyObject* pyargs, PyObject* kwargs)
{
	JPLocalFrame frame(32);
	TRACE_IN("PyJPBoundMethod::__call__");
	try {
		PyObject* result=NULL;
		{
			JPyCleaner cleaner;
			TRACE1(self->m_Method->m_Method->getName());

			JPyTuple args(pyargs);	
			vector<PyObject*> vargs;
			jlong len = args.size();
			vargs.push_back(self->m_Instance);
			for (jlong i = 0; i < len; i++)
			{
				vargs.push_back(cleaner.add(args.getItem(i))); // returns a new ref
			}
	
			result = self->m_Method->m_Method->invoke(vargs);
			TRACE1("Call finished");	
		}
		return result;
	}
	PY_STANDARD_CATCH

	return NULL;

	TRACE_OUT;
}

void PyJPBoundMethod::__dealloc__(PyJPBoundMethod* self)
{
	JPLocalFrame frame;
	TRACE_IN("PyJPBoundMethod::__dealloc__");

	Py_XDECREF(self->m_Instance);
	Py_XDECREF(self->m_Method);

	Py_TYPE(self)->tp_free((PyObject*)self);
	TRACE_OUT;
}

PyObject* PyJPBoundMethod::__str__(PyJPBoundMethod* self)
{
	JPLocalFrame frame;
	stringstream sout;
	sout << "<bound method " << self->m_Method->m_Method->getClassName() << "." << self->m_Method->m_Method->getName() << ">";
	return JPyString::fromString(sout.str());
}

PyObject* PyJPBoundMethod::matchReport(PyJPBoundMethod* self, PyObject* args)
{
	JPLocalFrame frame;
	try {
		JPyCleaner cleaner;

		cout << "Match report for " << self->m_Method->m_Method->getName() << endl;

		vector<PyObject*> vargs;
		jlong len = JPySequence(args).size();
		for (jlong i = 0; i < len; i++)
		{
			PyObject* obj = cleaner.add(JPySequence(args).getItem(i));
			vargs.push_back(obj);
		}

		string report = self->m_Method->m_Method->matchReport(vargs);

		PyObject* res = JPyString::fromString(report);

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}
