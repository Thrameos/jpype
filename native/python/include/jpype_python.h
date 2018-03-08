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
#ifndef _JPYPE_PYTHON_H_
#define _JPYPE_PYTHON_H_



// This file defines the _jpype module's interface and initializes it
#include <Python.h>
#include <jpype.h>

// TODO figure a better way to do this .. Python dependencies should not be in common code

#include <pyport.h>
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

// =================================================================
#define PY_CHECK(op) op; { \
	PyObject* __ex = PyErr_Occurred(); \
	if (__ex) { 	\
		throw PythonException(); \
	}\
};

#if (    (PY_VERSION_HEX <  0x02070000) \
     || ((PY_VERSION_HEX >= 0x03000000) \
      && (PY_VERSION_HEX <  0x03010000)) )

    #include "capsulethunk.h"
    #define USE_CAPSULE 1
    typedef void* CAPSULE_DESTRUCTOR_ARG_TYPE;
    typedef void (*PyCapsule_Destructor)(void*);
    #define CAPSULE_EXTRACT(obj) (obj)
#else
    typedef PyObject* CAPSULE_DESTRUCTOR_ARG_TYPE;
    #define CAPSULE_EXTRACT(obj) (PyCapsule_GetPointer(obj, PyCapsule_GetName(obj)))
#endif
/**
 * Exception wrapper for python-generated exceptions
 */
class PythonException : public HostException
{
public :
	PythonException();	
	PythonException(const PythonException& ex);

	virtual ~PythonException();

	virtual string getMessage();
	
	bool isJavaException();
	PyObject* getJavaException();
	
public :
	PyObject* m_ExceptionClass;
	PyObject* m_ExceptionValue;
};

#undef PY_CHECK

#define PY_STANDARD_CATCH \
catch(JavaException& ex) \
{ \
	try { \
		JPypeJavaException::errorOccurred(); \
	} \
	catch(...) \
	{ \
		JPPyni::setRuntimeException("An unknown error occured while handling a Java Exception"); \
	}\
}\
catch(JPypeException& ex)\
{\
	try { \
		JPPyni::setRuntimeException(ex.getMsg()); \
	} \
	catch(...) \
	{ \
		JPPyni::setRuntimeException("An unknown error occured while handling a JPype Exception"); \
	}\
}\
catch(PythonException& ex) \
{ \
} \
catch(...) \
{\
	JPPyni::setRuntimeException("Unknown Exception"); \
} \

#define PY_LOGGING_CATCH \
catch(JavaException& ex) \
{ \
	try { \
	cout << "Java error occured : " << ex.message << endl; \
		JPypeJavaException::errorOccurred(); \
	} \
	catch(...) \
	{ \
		JPPyni::setRuntimeException("An unknown error occured while handling a Java Exception"); \
	}\
}\
catch(JPypeException& ex)\
{\
	try { \
		cout << "JPype error occured" << endl; \
		JPPyni::setRuntimeException(ex.getMsg()); \
	} \
	catch(...) \
	{ \
		JPPyni::setRuntimeException("An unknown error occured while handling a JPype Exception"); \
	}\
}\
catch(PythonException& ex) \
{ \
	cout << "Pyhton error occured" << endl; \
} \
catch(...) \
{\
	cout << "Unknown error occured" << endl; \
	JPPyni::setRuntimeException("Unknown Exception"); \
} \

// =================================================================

namespace JPypeModule
{
	PyObject* startup(PyObject* obj, PyObject* args);
	PyObject* attach(PyObject* obj, PyObject* args);
	PyObject* dumpJVMStats(PyObject* obj);
	PyObject* shutdown(PyObject* obj);
	PyObject* synchronized(PyObject* obj, PyObject* args);
	PyObject* isStarted(PyObject* obj);
	PyObject* attachThread(PyObject* obj);
	PyObject* detachThread(PyObject* obj);
	PyObject* isThreadAttached(PyObject* obj);
	PyObject* getJException(PyObject* obj, PyObject* args);
	PyObject* raiseJava(PyObject* obj, PyObject* args);
	PyObject* attachThreadAsDaemon(PyObject* obj);
	PyObject* startReferenceQueue(PyObject* obj, PyObject* args);
	PyObject* stopReferenceQueue(PyObject* obj);

	PyObject* setConvertStringObjects(PyObject* obj, PyObject* args);
	PyObject* setResource(PyObject* obj, PyObject* args);
}

namespace JPypeJavaArray
{
	PyObject* findArrayClass(PyObject* obj, PyObject* args);
	PyObject* getArrayLength(PyObject* self, PyObject* arg);
	PyObject* getArrayItem(PyObject* self, PyObject* arg);
	PyObject* getArraySlice(PyObject* self, PyObject* arg);
	PyObject* setArraySlice(PyObject* self, PyObject* arg);
	PyObject* newArray(PyObject* self, PyObject* arg);
	PyObject* setArrayItem(PyObject* self, PyObject* arg);
	PyObject* setArrayValues(PyObject* self, PyObject* arg);
};

namespace JPypeJavaNio
{
	PyObject* convertToDirectBuffer(PyObject* self, PyObject* arg);
};

namespace JPypeJavaException
{
	void errorOccurred();
};

#include "py_monitor.h"
#include "py_method.h"
#include "py_array.h"
#include "py_arrayclass.h"
#include "py_class.h"
#include "py_field.h"
#include "py_proxy.h"
#include "py_value.h"

#include "jpype_memory_view.h"

// Utility method
//PyObject* detachRef(HostRef* ref);

#endif // _JPYPE_PYTHON_H_
