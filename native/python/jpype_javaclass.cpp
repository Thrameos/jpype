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

PyObject* JPypeJavaClass::findClass(PyObject* obj, PyObject* args)
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
		JPyArg::parseTuple(args, "s", &cname);
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

PyObject* JPypeJavaClass::findPrimitiveClass(PyObject* obj, PyObject* args)
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
		JPyArg::parseTuple(args, "s", &cname);
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


