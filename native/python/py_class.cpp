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
  {"getName",              &PyJPClass::getName, METH_NOARGS, ""},
  {"getBaseClass",         &PyJPClass::getBaseClass, METH_NOARGS, ""},
  {"getClassFields",       &PyJPClass::getClassFields, METH_NOARGS, ""},
  {"getClassMethods",      &PyJPClass::getClassMethods, METH_NOARGS, ""},
  {"newClassInstance",     &PyJPClass::newClassInstance, METH_VARARGS, ""},

  {"isInterface", &PyJPClass::isInterface, METH_NOARGS, ""},
  {"getBaseInterfaces", &PyJPClass::getBaseInterfaces, METH_NOARGS, ""},
  {"isSubclass", &PyJPClass::isSubclass, METH_VARARGS, ""},
  {"isPrimitive", &PyJPClass::isPrimitive, METH_NOARGS, ""},

  {"isException", &PyJPClass::isException, METH_NOARGS, ""},
  {"isArray", &PyJPClass::isArray, METH_NOARGS, ""},
  {"isAbstract", &PyJPClass::isAbstract, METH_NOARGS, ""},
  {"getSuperclass",&PyJPClass::getBaseClass, METH_NOARGS, ""},

  {"getConstructors", (PyCFunction)&PyJPClass::getConstructors, METH_NOARGS, ""},
  {"getDeclaredConstructors", (PyCFunction)&PyJPClass::getDeclaredConstructors, METH_NOARGS, ""},
  {"getDeclaredFields", (PyCFunction)&PyJPClass::getDeclaredFields, METH_NOARGS, ""},
  {"getDeclaredMethods", (PyCFunction)&PyJPClass::getDeclaredMethods, METH_NOARGS, ""},
  {"getFields", (PyCFunction)&PyJPClass::getFields, METH_NOARGS, ""},
  {"getMethods", (PyCFunction)&PyJPClass::getMethods, METH_NOARGS, ""},
  {"getModifiers", (PyCFunction)&PyJPClass::getModifiers, METH_NOARGS, ""},

  {NULL},
};

static PyTypeObject classClassType = 
{
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"JavaClass",              /*tp_name*/
	sizeof(PyJPClass),      /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	PyJPClass::__dealloc__,                   /*tp_dealloc*/
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
	"Java Class",                  /*tp_doc */
	0,		                   /* tp_traverse */
	0,		                   /* tp_clear */
	0,		                   /* tp_richcompare */
	0,		                   /* tp_weaklistoffset */
	0,		                   /* tp_iter */
	0,		                   /* tp_iternext */
	classMethods,                   /* tp_methods */
	0,						   /* tp_members */
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
void PyJPClass::initType(PyObject* module)
{
	PyType_Ready(&classClassType);
	PyModule_AddObject(module, "_JavaClass", (PyObject*)&classClassType); 
}

PyJPClass* PyJPClass::alloc(JPClass* cls)
{
	PyJPClass* res = PyObject_New(PyJPClass, &classClassType);

	res->m_Class = cls;
	
	return res;
}

void PyJPClass::__dealloc__(PyObject* o)
{
	TRACE_IN("PyJPClass::__dealloc__");

	PyJPClass* self = (PyJPClass*)o;

	Py_TYPE(self)->tp_free(o);

	TRACE_OUT;
}

PyObject* PyJPClass::getName(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;

		string name = self->m_Class->getSimpleName();

		PyObject* res = JPyString::fromString(name.c_str());

		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseClass(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		JPObjectClass* base = cls->getSuperClass();
		if (base == NULL)
		{
			Py_RETURN_NONE;
		}

		PyObject* res  = (PyObject*)PyJPClass::alloc(base);
		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseInterfaces(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		const vector<JPObjectClass*>& baseItf = cls->getInterfaces();

		PyObject* result = JPySequence::newTuple((int)baseItf.size());
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			JPObjectClass* base = baseItf[i];
			PyObject* obj = (PyObject*)PyJPClass::alloc(base);
			JPySequence::setItem(result, i, obj);
		}
		return result;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassFields(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		map<string, JPField*> staticFields = cls->getStaticFields();
		map<string, JPField*> instFields = cls->getInstanceFields();

		PyObject* res = JPySequence::newTuple((int)(staticFields.size()+instFields.size()));

		int i = 0;
		for (map<string, JPField*>::iterator curStatic = staticFields.begin(); curStatic != staticFields.end(); curStatic ++)
		{
			PyObject* f = (PyObject*)PyJPField::alloc(curStatic->second);

			JPySequence::setItem(res, i, f);
			i++;
			Py_DECREF(f);
		}

		for (map<string, JPField*>::iterator curInst = instFields.begin(); curInst != instFields.end(); curInst ++)
		{
			PyObject* f = (PyObject*)PyJPField::alloc(curInst->second);

			JPySequence::setItem(res, i, f);
			i++;
			Py_DECREF(f);
		}


		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassMethods(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		vector<JPMethod*> methods = cls->getMethods();

		PyObject* res = JPySequence::newTuple((int)methods.size());

		int i = 0;
		for (vector<JPMethod*>::iterator curMethod = methods.begin(); curMethod != methods.end(); curMethod ++)
		{

			JPMethod* mth= *curMethod;
			PyJPMethod* methObj = PyJPMethod::alloc(mth);
			
			JPySequence::setItem(res, i, (PyObject*)methObj);
			i++;
			Py_DECREF(methObj);
		}

		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::newClassInstance(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		JPyCleaner cleaner;
		vector<PyObject*> args;
		Py_ssize_t len = JPyObject::length(arg);
		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* obj = cleaner.add(JPySequence::getItem(arg, i));
			args.push_back(ref);
		}

		JPObject* resObject = cls->newInstance(args);
	  return JPyCapsule::fromVoidAndDesc((void*)resObject, "JPObject", &PythonHostEnvironment::deleteJPObjectDestructor);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isInterface(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isSubclass(PyObject* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		char* other;

		PyArg_ParseTuple(arg, "s", &other);
		string name = JPTypeManager::getQualifiedName(other);
		JPClass* otherClass = JPTypeManager::findClassByName(name);
		return PyBool_FromLong(self->m_Class->isSubclass(otherClass));
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isException(PyObject* o, PyObject* args)
{
	JPLocalFrame frame;
	try 
	{
		PyJPClass* self = (PyJPClass*)o;

		bool res = JPJni::isThrowable(self->m_Class->getNativeClass());
		return PyBool_FromLong(res);
	}
	PY_STANDARD_CATCH;
	return NULL;
}

bool PyJPClass::check(PyObject* o)
{
	return o->ob_type == &classClassType;
}

PyObject* convert(vector<jobject> objs, JPClass* classType)
{
	if (classType == NULL)
	{
		RAISE(JPypeException, "conversion object class not found.");
	}
	JPyCleaner cleaner;
	PyObject* res = JPySequence::newTuple((int)objs.size());
	for (size_t i = 0; i < objs.size(); i++)
	{
		jvalue v;
		v.l = objs[i];
		PyObject* ref = cleaner.add(classType->asHostObject(v));
		JPySequence::setItem(res, i, (PyObject*)ref->data());
	}
	return res;
}


PyObject* PyJPClass::getDeclaredMethods(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredMethods(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getConstructors(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getConstructors(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}
PyObject* PyJPClass::getDeclaredConstructors(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredConstructors(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClass("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getDeclaredFields(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getDeclaredFields(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClass("java/lang/reflect/Field"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getFields(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getFields(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClass("java/lang/reflect/Field"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getModifiers(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		long mod = self->m_Class->getClassModifiers();
		PyObject* res = JPyLong::fromLongLong(mod);
		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getMethods(PyObject* o)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		vector<jobject> mth = JPJni::getMethods(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isPrimitive(PyObject* o, PyObject* args)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		return PyBool_FromLong(self->m_Class->isObjectType()==0);
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isArray(PyObject* o, PyObject* args)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		return PyBool_FromLong(self->m_Class->isArray());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isAbstract(PyObject* o, PyObject* args)
{
	JPLocalFrame frame;
	try {
		PyJPClass* self = (PyJPClass*)o;
		return PyBool_FromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH;
	return NULL;
}


// =================================================================
// Global functions
PyObject* PyJPClass::findClass(PyObject* obj, PyObject* args)
{
	TRACE_IN("JPypeModule::findClass");
	JPLocalFrame frame;
	if (! JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_RuntimeError, "Java Subsystem not started");
		return NULL;
	}

	try {
		char* cname;
		PyArg_ParseTuple(args, "s", &cname);
		TRACE1(cname);

		string name = JPTypeManager::getQualifiedName(cname);
		JPObjectClass* claz = dynamic_cast<JPObjectClass*>(JPTypeManager::findClassByName(name));
		if (claz == NULL)
		{
			Py_RETURN_NONE;
		}

		PyObject* res = (PyObject*)PyJPClass::alloc(claz);

		return res;
	}
	PY_STANDARD_CATCH;  

	PyErr_Clear();
	Py_RETURN_NONE;

	TRACE_OUT;
}

PyObject* PyJPClass::findPrimitiveClass(PyObject* obj, PyObject* args)
{
	TRACE_IN("JPypeModule::findClass");
	JPLocalFrame frame;
	if (! JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_RuntimeError, "Java Subsystem not started");
		return NULL;
	}

	try {
		char* cname;
		PyArg_ParseTuple(args, "s", &cname);
		TRACE1(cname);

		if (cname==NULL)
		{
			Py_RETURN_NONE;
		}

		std::string name = cname;
		JPClass* claz = NULL;

		if ( name=="boolean")
			claz = JPTypeManager::_boolean;
		else if ( name=="byte")
			claz = JPTypeManager::_byte;
		else if ( name=="char")
			claz = JPTypeManager::_char;
		else if ( name=="short")
			claz = JPTypeManager::_short;
		else if ( name=="int")
			claz = JPTypeManager::_int;
		else if ( name=="long")
			claz = JPTypeManager::_long;
		else if ( name=="float")
			claz = JPTypeManager::_float;
		else if ( name=="double")
			claz = JPTypeManager::_double;

		if (claz == NULL)
		{
			Py_RETURN_NONE;
		}

		PyObject* res = (PyObject*)PyJPClass::alloc(claz);

		return res;
	}
	PY_STANDARD_CATCH;  

	PyErr_Clear();
	Py_RETURN_NONE;

	TRACE_OUT;
}


