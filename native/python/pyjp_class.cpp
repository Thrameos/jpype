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

// This section represents a bit of a chicken and egg problem.
// We have the ability to access the java.lang.Class instance which contains
// all of the methods that we would need to access these fields as methods.
// But we need to get access to these fields for java.lang.Object python
// wrapper which exists before the java.lang.Class can be set up.
//
// Thus we have to replicate a whole bunch of methods from java.lang.Class for
// something which should just be a jclass instance.
//
// Perhaps at some point we can find a why to bootstrap our way through 
// the need for a __javaclass__ object, but for now this is the easiest solution.

#include <jpype_python.h>  

static PyMethodDef classMethods[] = {
  {"getName",                 (PyCFunction)&PyJPClass::getName, METH_NOARGS, ""},
  {"getBaseClass",            (PyCFunction)&PyJPClass::getBaseClass, METH_NOARGS, ""},
  {"getClassFields",          (PyCFunction)&PyJPClass::getClassFields, METH_NOARGS, ""},
  {"getClassMethods",         (PyCFunction)&PyJPClass::getClassMethods, METH_NOARGS, ""},
  {"newClassInstance",        (PyCFunction)&PyJPClass::newClassInstance, METH_VARARGS, ""},
  {"isInterface",             (PyCFunction)&PyJPClass::isInterface, METH_NOARGS, ""},
  {"getBaseInterfaces",       (PyCFunction)&PyJPClass::getBaseInterfaces, METH_NOARGS, ""},
  {"isSubclass",              (PyCFunction)&PyJPClass::isSubclass, METH_VARARGS, ""},
  {"isPrimitive",             (PyCFunction)&PyJPClass::isPrimitive, METH_NOARGS, ""},
  {"isThrowable",             (PyCFunction)&PyJPClass::isThrowable, METH_NOARGS, ""},
  {"isArray",                 (PyCFunction)&PyJPClass::isArray, METH_NOARGS, ""},
  {"isAbstract",              (PyCFunction)&PyJPClass::isAbstract, METH_NOARGS, ""},
  {"getSuperclass",           (PyCFunction)&PyJPClass::getBaseClass, METH_NOARGS, ""},
  {"getConstructors",         (PyCFunction)&PyJPClass::getConstructors, METH_NOARGS, ""},
  {"getDeclaredConstructors", (PyCFunction)&PyJPClass::getDeclaredConstructors, METH_NOARGS, ""},
  {"getDeclaredFields",       (PyCFunction)&PyJPClass::getDeclaredFields, METH_NOARGS, ""},
  {"getDeclaredMethods",      (PyCFunction)&PyJPClass::getDeclaredMethods, METH_NOARGS, ""},
  {"getFields",               (PyCFunction)&PyJPClass::getFields, METH_NOARGS, ""},
  {"getMethods",              (PyCFunction)&PyJPClass::getMethods, METH_NOARGS, ""},
  {"getModifiers",            (PyCFunction)&PyJPClass::getModifiers, METH_NOARGS, ""},
  {NULL},
};

PyTypeObject PyJPClass::Type =
{
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name           */ "PyJPClass",
	/* tp_basicsize      */ sizeof(PyJPClass),
	/* tp_itemsize       */ 0,
	/* tp_dealloc        */ (destructor)PyJPClass::__dealloc__,
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
	/* tp_str            */ (reprfunc)PyJPClass::__str__,
	/* tp_getattro       */ 0,
	/* tp_setattro       */ 0,
	/* tp_as_buffer      */ 0,
	/* tp_flags          */ Py_TPFLAGS_DEFAULT,
	/* tp_doc            */ "Java Class",
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
	/* tp_init           */ (initproc)PyJPClass::__init__,
	/* tp_alloc          */ 0,
	/* tp_new            */ (newfunc)PyJPClass::__new__
};

// Static methods
void PyJPClass::initType(PyObject* module)
{
	PyType_Ready(&PyJPClass::Type);
	Py_INCREF(&PyJPClass::Type);
	PyModule_AddObject(module, "PyJPClass", (PyObject*)&PyJPClass::Type);
}

bool PyJPClass::check(PyObject* o)
{
	return o->ob_type == &PyJPClass::Type;
}

PyJPClass* PyJPClass::alloc(JPClass* cls)
{
	PyJPClass* res = PyObject_New(PyJPClass, &PyJPClass::Type);
	res->m_Class = cls;
	return res;
}

PyObject* PyJPClass::__new__(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	PyJPClass* self = (PyJPClass*)type->tp_alloc(type, 0);
	self->m_Class = NULL;
	return (PyObject*)self;
}


// Replacement for convertToJava.
int PyJPClass::__init__(PyJPClass* self, PyObject* args, PyObject* kwargs)
{
  TRACE_IN("PyJPClass::init");
  JPLocalFrame frame;
  try
	{
    JPPyni::assertInitialized();

		char* cname;
		int primitive = 0;
    if (!PyArg_ParseTuple(args, "s|i", &cname, &primitive))
		{
			JPyErr::setRuntimeError("Unable to get string and integer");
			return -1;
		}

		if (primitive)
		{
			// Special handling for primitives, needed for JWrapper
			TRACE2("primitive class",cname);
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
			self->m_Class = claz;

		}
		else
		{
			TRACE2("object class", cname);
			string name = JPTypeManager::getQualifiedName(cname);
			self->m_Class = JPTypeManager::findClassByName(name);
			// This may throw java.lang.NoClassDefFoundError
		}

		// Fail if we don't get a class
		if (self->m_Class == NULL)
		{
			printf("Failed\n");
			stringstream sout;
			sout << "unable to find java class named "<< cname;
			JPyErr::setRuntimeError(sout.str().c_str());
			return -1;
		}

		return 0;
  }
  PY_STANDARD_CATCH
  return -1;
  TRACE_OUT;
}

void PyJPClass::__dealloc__(PyJPClass* self)
{
	TRACE_IN("PyJPClass::__dealloc__");
	if (self->m_Class != NULL)
	{
		TRACE1(self->m_Class->getSimpleName());
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
	TRACE_OUT;
}

PyObject* PyJPClass::__str__(PyJPClass* self)
{
	JPLocalFrame frame;
	stringstream sout;
	sout << "<java class " << self->m_Class->getSimpleName() << ">";
	return JPyString::fromString(sout.str());
}

PyObject* PyJPClass::getName(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		string name = self->m_Class->getSimpleName();
		return JPyString::fromString(name);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseClass(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		// Must be an object class type to have a base class.
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

		return (PyObject*)PyJPClass::alloc(base);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getBaseInterfaces(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	JPyCleaner cleaner;
	try {
		// Must be an object class type to have a interfaces.
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		const vector<JPObjectClass*>& baseItf = cls->getInterfaces();
		JPyTuple result = JPyTuple::newTuple((int)baseItf.size());
		for (unsigned int i = 0; i < baseItf.size(); i++)
		{
			JPObjectClass* base = baseItf[i];
			result.setItem(i, cleaner.add((PyObject*)PyJPClass::alloc(base)));
		}
		return result;
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassFields(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	JPyCleaner cleaner;
	try {
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		// FIXME wouldn't it be better to give static and instance fields on a separate list
		map<string, JPField*> staticFields = cls->getStaticFields();
		map<string, JPField*> instFields = cls->getInstanceFields();

		JPyTuple res = JPyTuple::newTuple((int)(staticFields.size()+instFields.size()));

		int i = 0;
		for (map<string, JPField*>::iterator curStatic = staticFields.begin(); curStatic != staticFields.end(); curStatic ++)
		{
			res.setItem(i, cleaner.add((PyObject*)PyJPField::alloc(curStatic->second)));
			i++;
		}

		for (map<string, JPField*>::iterator curInst = instFields.begin(); curInst != instFields.end(); curInst ++)
		{
			res.setItem(i, cleaner.add( (PyObject*)PyJPField::alloc(curInst->second)));
			i++;
		}

		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::getClassMethods(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	JPyCleaner cleaner;
	try {
		// Class methods only applies to object types
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		vector<JPMethod*> methods = cls->getMethods();
		JPyTuple res = JPyTuple::newTuple((int)methods.size());
		int i = 0;
		for (vector<JPMethod*>::iterator curMethod = methods.begin(); curMethod != methods.end(); curMethod ++)
		{
			PyJPMethod* methObj = PyJPMethod::alloc(*curMethod);
			res.setItem(i++, cleaner.add((PyObject*)methObj));
		}
		return res;

	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::newClassInstance(PyJPClass* o, PyObject* arg)
{
	JPLocalFrame frame;
	try {
		// FIXME why not make this apply to array objects
		
		// Constructors only apply to object classes
		PyJPClass* self = (PyJPClass*)o;
		JPObjectClass* cls = dynamic_cast<JPObjectClass*>(self->m_Class);
		if ( cls == NULL)
		{
			Py_RETURN_NONE;
		}

		// Call the constructor
		JPyCleaner cleaner;
		vector<PyObject*> args;
		jlong len = JPySequence(arg).size();
		for (jlong i = 0; i < len; i++)
		{
			args.push_back(cleaner.add(JPySequence(arg).getItem(i)));
		}

		const JPValue& val = cls->newInstance(args);
	  return (PyObject*)PyJPValue::alloc(val);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isInterface(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	try
	{
		return PyBool_FromLong(self->m_Class->isInterface());
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isSubclass(PyJPClass* self, PyObject* arg)
{
	JPLocalFrame frame;
	try
	{
		char* other;

		// This is another chicken and egg problem.  We need to check
		// sub class from within the customizer hooks and those
		// are during object construction.  Thus we will check by
		// string rather than directly using the class.
		//
		PyArg_ParseTuple(arg, "s", &other);
		string name = JPTypeManager::getQualifiedName(other);
		JPClass* otherClass = JPTypeManager::findClassByName(name);
		return JPyBool::fromLong(self->m_Class->isAssignableTo(otherClass));
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* PyJPClass::isThrowable(PyJPClass* self, PyObject* args)
{
	JPLocalFrame frame;
	try
	{
		bool res = JPJni::isThrowable(self->m_Class->getNativeClass());
		return PyBool_FromLong(res);
	}
	PY_STANDARD_CATCH;
	return NULL;
}


// internal function used to support the next few methods
PyObject* convert(vector<jobject> objs, JPClass* classType)
{
	if (classType == NULL)
	{
		RAISE(JPypeException, "conversion object class not found.");
	}
	JPyCleaner cleaner;
	JPyTuple res = JPyTuple::newTuple((int)objs.size());
	for (size_t i = 0; i < objs.size(); i++)
	{
		jvalue v;
		v.l = objs[i];
		PyObject* ref = cleaner.add(classType->asHostObject(v));
		res.setItem(i, ref);
	}
	return res;
}

PyObject* PyJPClass::getDeclaredMethods(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getDeclaredMethods(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getConstructors(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getConstructors(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}
PyObject* PyJPClass::getDeclaredConstructors(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getDeclaredConstructors(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getDeclaredFields(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getDeclaredFields(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Field"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getFields(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getFields(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Field"));
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getModifiers(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		jlong mod = self->m_Class->getClassModifiers();
		PyObject* res = JPyLong::fromLong(mod);
		return res;
	}
	PY_STANDARD_CATCH;
	return NULL;	
}

PyObject* PyJPClass::getMethods(PyJPClass* self)
{
	JPLocalFrame frame;
	try {
		vector<jobject> mth = JPJni::getMethods(frame, self->m_Class->getNativeClass());
		return convert(mth, JPTypeManager::findClassByName("java/lang/reflect/Method"));
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isPrimitive(PyJPClass* self, PyObject* args)
{
	JPLocalFrame frame;
	try {
		return JPyBool::fromLong(self->m_Class->isObjectType()==0);
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isArray(PyJPClass* self, PyObject* args)
{
	JPLocalFrame frame;
	try {
		return JPyBool::fromLong(self->m_Class->isArray());
	}
	PY_STANDARD_CATCH;
	return NULL;
}

PyObject* PyJPClass::isAbstract(PyJPClass* self, PyObject* args)
{
	JPLocalFrame frame;
	try {
		return JPyBool::fromLong(self->m_Class->isAbstract());
	}
	PY_STANDARD_CATCH;
	return NULL;
}


// =================================================================
// Global functions

// FIXME both of these methods could be in the __init__ method for PyJPClass
// eliminating unneeds global entry points.
/*
PyObject* PyJPClass::findClass(PyObject* obj, PyObject* args)
{
	TRACE_IN("JPypeModule::findClass");
	JPLocalFrame frame;
	try {
		JPPyni::assertInitialized();

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
	TRACE_IN("PyJPClass::findPrimitiveClass");
	JPLocalFrame frame;
	try {
		JPPyni::assertInitialized();
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

*/
