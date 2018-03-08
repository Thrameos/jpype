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
  {"isStarted", (PyCFunction)&JPypeModule::isStarted, METH_NOARGS, ""},
  {"startup", &JPypeModule::startup, METH_VARARGS, ""},
  {"attach", &JPypeModule::attach, METH_VARARGS, ""},
  {"shutdown", (PyCFunction)&JPypeModule::shutdown, METH_NOARGS, ""},
  {"setResource", &JPypeModule::setResource, METH_VARARGS, ""},

  {"synchronized", &JPypeModule::synchronized, METH_VARARGS, ""},
  {"isThreadAttachedToJVM", (PyCFunction)&JPypeModule::isThreadAttached, METH_NOARGS, ""}, 
  {"attachThreadToJVM", (PyCFunction)&JPypeModule::attachThread, METH_NOARGS, ""},
  {"detachThreadFromJVM", (PyCFunction)&JPypeModule::detachThread, METH_NOARGS, ""},
  {"dumpJVMStats", (PyCFunction)&JPypeModule::dumpJVMStats, METH_NOARGS, ""},
  {"attachThreadAsDaemon", (PyCFunction)&JPypeModule::attachThreadAsDaemon, METH_NOARGS, ""},
  {"startReferenceQueue", &JPypeModule::startReferenceQueue, METH_VARARGS, ""},
  {"stopReferenceQueue", (PyCFunction)&JPypeModule::stopReferenceQueue, METH_NOARGS, ""},

  {"findClass", &PyJPClass::findClass, METH_VARARGS, ""},
  {"findArrayClass", &PyJPArrayClass::findArrayClass, METH_VARARGS, ""},
  {"findPrimitiveClass", &PyJPClass::findPrimitiveClass, METH_VARARGS, ""},
  {"createProxy", &PyJProxy::createProxy, METH_VARARGS, ""},
  {"convertToJValue", &PyJValue::convertToJValue, METH_VARARGS, ""},

  {"getArrayLength", &JPypeJavaArray::getArrayLength, METH_VARARGS, ""},
  {"getArrayItem", &JPypeJavaArray::getArrayItem, METH_VARARGS, ""},
  {"setArrayItem", &JPypeJavaArray::setArrayItem, METH_VARARGS, ""},
  {"getArraySlice", &JPypeJavaArray::getArraySlice, METH_VARARGS, ""},
  {"setArraySlice", &JPypeJavaArray::setArraySlice, METH_VARARGS, ""},
  {"newArray", &JPypeJavaArray::newArray, METH_VARARGS, ""},

  {"convertToDirectBuffer", &JPypeJavaNio::convertToDirectBuffer, METH_VARARGS, ""},

  {"setConvertStringObjects", &JPypeModule::setConvertStringObjects, METH_VARARGS, ""},

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

void JPypeJavaException::errorOccurred()
{
	TRACE_IN("PyJavaException::errorOccurred");
	JPLocalFrame frame(8);
	JPyCleaner cleaner;
	jthrowable th = JPEnv::getJava()->ExceptionOccurred();
	JPEnv::getJava()->ExceptionClear();

	jclass ec = JPJni::getClass(th);
	JPObjectClass* jpclass = dynamic_cast<JPObjectClass*>(JPTypeManager::findClass(ec));
	// FIXME nothing checks if the class is valid before using it

	JPyObject jexclass = cleaner.add(JPyEnv::newClass(jpclass));
	PyObject* pyth = cleaner.add(JPyEnv::newObject(new JPObject(jpclass, th)));

	JPyTuple args = cleaner.add(JPyTuple::newTuple(2));
	JPyTuple arg2 = cleaner.add(JPyTuple::newTuple(1));
	arg2.setItem( 0, args);
	args.setItem( 0, hostEnv->m_SpecialConstructorKey);
	args.setItem( 1, (PyObject*)pyth->data());

	PyObject* pyexclass = cleaner.add(jexclass.getAttrString("PYEXC"));
	JPyErr::setObject(pyexclass, arg2);

	TRACE_OUT;
}
