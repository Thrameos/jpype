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
#ifdef HAVE_NUMPY
//	#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
	#define PY_ARRAY_UNIQUE_SYMBOL jpype_ARRAY_API
	#include <numpy/arrayobject.h>
#endif

static PyMethodDef jpype_methods[] = 
{  
  {"isStarted", (PyCFunction)&PyJPModule::isStarted, METH_NOARGS, ""},
  {"startup", &PyJPModule::startup, METH_VARARGS, ""},
  {"attach", &PyJPModule::attach, METH_VARARGS, ""},
  {"shutdown", (PyCFunction)&PyJPModule::shutdown, METH_NOARGS, ""},
  {"setResource", &PyJPModule::setResource, METH_VARARGS, ""},

  {"synchronized", &PyJPMonitor::synchronized, METH_VARARGS, ""},
  {"isThreadAttachedToJVM", (PyCFunction)&PyJPModule::isThreadAttached, METH_NOARGS, ""}, 
  {"attachThreadToJVM", (PyCFunction)&PyJPModule::attachThread, METH_NOARGS, ""},
  {"detachThreadFromJVM", (PyCFunction)&PyJPModule::detachThread, METH_NOARGS, ""},
  {"dumpJVMStats", (PyCFunction)&PyJPModule::dumpJVMStats, METH_NOARGS, ""},
  {"attachThreadAsDaemon", (PyCFunction)&PyJPModule::attachThreadAsDaemon, METH_NOARGS, ""},
//  {"startReferenceQueue", &PyJPModule::startReferenceQueue, METH_VARARGS, ""},
//  {"stopReferenceQueue", (PyCFunction)&PyJPModule::stopReferenceQueue, METH_NOARGS, ""},

  {"findClass", &PyJPClass::findClass, METH_VARARGS, ""},
  {"findArrayClass", &PyJPArrayClass::findArrayClass, METH_VARARGS, ""},
  {"findPrimitiveClass", &PyJPClass::findPrimitiveClass, METH_VARARGS, ""},
  {"createProxy", &PyJPProxy::createProxy, METH_VARARGS, ""},
  {"convertToJValue", &PyJPValue::convertToJavaValue, METH_VARARGS, ""},

  {"convertToDirectBuffer", &PyJPModule::convertToDirectBuffer, METH_VARARGS, ""},

  {"setConvertStringObjects", &PyJPModule::setConvertStringObjects, METH_VARARGS, ""},

  // sentinel
  {NULL}
};
#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_jpype",
    "jpype module",
    -1,
    jpype_methods,
};
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit__jpype()
#else
PyMODINIT_FUNC init_jpype()
#endif
{
	Py_Initialize();
	PyEval_InitThreads();
	  
#if PY_MAJOR_VERSION >= 3
    PyObject* module = PyModule_Create(&moduledef);
#else
	PyObject* module = Py_InitModule("_jpype", jpype_methods);
#endif
	Py_INCREF(module);

	PyJPMonitor::initType(module);	
	PyJPMethod::initType(module);	
	PyJPBoundMethod::initType(module);	
	PyJPClass::initType(module);	
	PyJPArray::initType(module);	
	PyJPArrayClass::initType(module);	
	PyJPField::initType(module);	

#if (PY_VERSION_HEX < 0x02070000)
	jpype_memoryview_init(module);
#endif

#ifdef HAVE_NUMPY
	import_array();
#endif
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

// =========================================================================
// Module Methods

PyObject* PyJPModule::startup(PyObject* obj, PyObject* args)  
{  
	TRACE_IN("PyJPModule::startup");

	// Make sure we are not starting twice
	if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
  }

	try {

		// Parse args
		PyObject* vmOpt;
		PyObject* vmPath;
		char ignoreUnrecognized = true;
		PyArg_ParseTuple(args, "OO!b|", &vmPath, &PyTuple_Type, &vmOpt, &ignoreUnrecognized);

		// Verify options
		if (! (JPyString::check(vmPath)))
		{
			RAISE(JPypeException, "First paramter must be a string or unicode");
		}
		string cVmPath = JPyString(vmPath).asString();

		// Collect arguments for the jvm
		StringVector args;
		JPySequence seq(vmOpt);
		jlong len = seq.size();
		for (jlong i = 0; i < len; i++)
		{
			PyObject* obj = seq.getItem(i);

			if (JPyString::check(obj))
			{
				// TODO support unicode
				string v = JPyString(obj).asString();	
				args.push_back(v);
			}
			else if (JPySequence::check(obj))
			{
				//String name = arg[0];
				//Callable value = arg[1];

				// TODO complete this for the hooks ....
			}
			else {
				RAISE(JPypeException, "VM Arguments must be string or tuple");
			}
		}

		// Load the jvm
		JPEnv::loadJVM(cVmPath, ignoreUnrecognized, args);
		Py_RETURN_NONE;
	}
	//  FIXME if we have a java error and the system is not completely initialized 
	//  then we may segfault here.
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::attach(PyObject* obj, PyObject* args)  
{  
	TRACE_IN("PyJPModule::attach");
	// Make sure we are not initializing twice
  if (JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_OSError, "JVM is already started");
		return NULL;
  }

	try {
		PyObject* vmPath;

		PyArg_ParseTuple(args, "O", &vmPath);

		if (! (JPyString::check(vmPath)))
		{
			RAISE(JPypeException, "First paramter must be a string or unicode");
		}

		string cVmPath = JPyString(vmPath).asString();
		JPEnv::attachJVM(cVmPath);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::dumpJVMStats(PyObject* obj)   
{
 	cerr << "JVM activity report     :" << endl;
	//cerr << "\tmethod calls         : " << methodCalls << endl;
	//cerr << "\tstatic method calls  : " << staticMethodCalls << endl;
	//cerr << "\tconstructor calls    : " << constructorCalls << endl;
	//cerr << "\tproxy callbacks      : " << JProxy::getCallbackCount() << endl;
	//cerr << "\tfield gets           : " << fieldGets << endl;
	//cerr << "\tfield sets           : " << fieldSets << endl;
	cerr << "\tclasses loaded       : " << JPTypeManager::getLoadedClasses() << endl;
	Py_RETURN_NONE;
}

PyObject* PyJPModule::shutdown(PyObject* obj)
{
	TRACE_IN("PyJPModule::shutdown");
	try {
		JPPyni::assertInitialized();
		//dumpJVMStats(obj);

		JPEnv::getJava()->checkInitialized();

		JPTypeManager::shutdown();

		if (JPEnv::getJava()->DestroyJavaVM() )
		{
			RAISE(JPypeException, "Unable to destroy JVM");
		}

		JPEnv::getJava()->shutdown();
		cerr << "JVM has been shutdown" << endl;
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::isStarted(PyObject* obj)
{
	return JPyBool::fromLong(JPEnv::isInitialized());
}

PyObject* PyJPModule::attachThread(PyObject* obj)
{
	TRACE_IN("PyJPModule::attachThread");
	try {
		JPPyni::assertInitialized();
		JPEnv::attachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::detachThread(PyObject* obj)
{
	TRACE_IN("PyJPModule::detachThread");
	try {
		JPPyni::assertInitialized();
		JPEnv::getJava()->DetachCurrentThread();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::isThreadAttached(PyObject* obj)
{	
	try {
		JPPyni::assertInitialized();
		return JPyBool::fromLong(JPEnv::isThreadAttached());
	}
	PY_STANDARD_CATCH;

	return NULL;

}

PyObject* PyJPModule::raiseJava(PyObject* , PyObject* args)
{
	TRACE_IN("PyJPModule::raiseJava");
	try 
	{
		//PyObject* arg;
		//PyArg_ParseTuple(args, "O", &arg);
		//JP Object* obj;
		//JPCleaner cleaner;
		//
		//if (JPyCObject::check(arg) && string((char*)JPyCObject::getDesc(arg)) == "JP Object")
		//{
		//	obj = (JP Object*)JPyCObject::asVoidPtr(arg);
		//}
		//else
		//{
		//	JPyErr::setString(PyExc_TypeError, "You can only throw a subclass of java.lang.Throwable");
		//	return NULL;
		//}
		//
		//// check if claz is an instance of Throwable
		//JPObjectClass* claz = obj->getClass();
		//jclass jc = claz->getClass();
		//cleaner.add(jc);
		//if (! JPJni::isThrowable(jc))
		//{
		//	JPyErr::setString(PyExc_TypeError, "You can only throw a subclass of java.lang.Throwable");
		//	return NULL;
		//}
		//
		//jobject jobj = obj->getObject();
		//cleaner.add(jobj);
		//JPEnv::getJava()->Throw((jthrowable)jobj);

		//PyJavaException::errorOccurred();
	}
	PY_STANDARD_CATCH;
	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::attachThreadAsDaemon(PyObject* obj)
{
	try {
		JPPyni::assertInitialized();
		JPEnv::attachCurrentThreadAsDaemon();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
}


/*
PyObject* PyJPModule::startReferenceQueue(PyObject* obj, PyObject* args)
{
	try {
		JPPyni::assertInitialized();
		int i;
		PyArg_ParseTuple(args, "i", &i);

		JPJni::startJPypeReferenceQueue(i == 1);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;

	return NULL;
}

PyObject* PyJPModule::stopReferenceQueue(PyObject* obj)
{
	try {
		JPPyni::assertInitialized();
		JPJni::stopJPypeReferenceQueue();
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;
	return NULL;
}
*/

PyObject* PyJPModule::setConvertStringObjects(PyObject* obj, PyObject* args)
{
	try {
		JPPyni::assertInitialized();

		// Parse args
		PyObject* flag;
		PyArg_ParseTuple(args, "O", &flag);

		// Set flag
		JPEnv::getJava()->setConvertStringObjects(JPyBool(flag).isTrue());
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH;
	return NULL;
}

/** Set a Jpype Resource.
 *
 * JPype needs to know about a number of python objects to function
 * properly. These resources are set in the initialization methods
 * as those resources are created in python. 
 *
 * Does not require the system to be initialized.
 */
PyObject* PyJPModule::setResource(PyObject* self, PyObject* arg)
{	
	TRACE_IN("PyJPModule::setResource");
	try {
		char* tname;
		PyObject* value;
		PyArg_ParseTuple(arg, "sO", &tname, &value);
		string name = tname;

		if (name == "WrapperClass")
			JPPyni::m_WrapperClass = value;
		else if (name == "StringWrapperClass")
			JPPyni::m_StringWrapperClass = value;
		else if (name == "ProxyClass")
			JPPyni::m_ProxyClass = value;
		else if (name == "JavaExceptionClass")
			JPPyni::m_JavaExceptionClass = value;

		// Base class for JavaClass, used to check isInstance()
		else if (name == "JavaClass")
			JPPyni::m_PythonJavaClass = value;
    // Base class for JavaObject wrappers, used to check isInstance()
		else if (name == "JavaObject")
			JPPyni::m_PythonJavaObject = value;

		else if (name == "GetClassMethod")
			JPPyni::m_GetClassMethod = value;
		else if (name == "SpecialConstructorKey")
			JPPyni::m_SpecialConstructorKey = value;
		else if (name == "JavaArrayClass")
			JPPyni::m_JavaArrayClass = value;
		else if (name == "GetJavaArrayClassMethod")
			JPPyni::m_GetJavaArrayClassMethod = value;
		else
		{
			PyErr_SetString(PyExc_AttributeError, "Unknown jpype resource");
			return NULL;
		}

		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

PyObject* PyJPModule::convertToDirectBuffer(PyObject* self, PyObject* args)
{  
	TRACE_IN("PyJPModule::convertStringToBuffer"); 
	try
	{
		JPPyni::assertInitialized();

		// Use special method defined on the TypeConverter interface ...
		PyObject* src;
		PyArg_ParseTuple(args, "O", &src);

		PyObject* res = NULL;
		if (JPyMemoryView::check(src))
		{
			// converts to byte buffer ...
			JPClass* type = JPTypeManager::_byte;

			TRACE1("Converting");
			res = type->convertToDirectBuffer(src);
		}

		if (res != NULL)
		{
			return res;
		}

		RAISE(JPypeException, "Do not know how to convert to direct byte buffer, only memory view supported");
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT;
}

