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

static void deleteObjectJValueDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
{
	jvalue* pv = (jvalue*)CAPSULE_EXTRACT(data);
	JPEnv::getJava()->DeleteGlobalRef(pv->l);
	delete pv;
}

static void deleteJValueDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
{
	jvalue* pv = (jvalue*)CAPSULE_EXTRACT(data);
	delete pv;
}

PyObject* PyJPValue::allocPrimitive(jvalue* v)
{
	return JPyCapsule::fromVoidAndDesc((void*)v, "object jvalue", deleteObjectJValueDestructor);
}

PyObject* PyJPValue::allocObject(jvalue* v)
{
  return JPyCapsule::fromVoidAndDesc((void*)v, "jvalue", deleteJValueDestructor);
}

PyObject* PyJPValue::convertToJValue(PyObject* self, PyObject* arg)
{
	if (! JPEnv::isInitialized())
	{
		PyErr_SetString(PyExc_RuntimeError, "Java Subsystem not started");
		return NULL;
	}
	JPLocalFrame frame;
	try {
		PyObject* claz;
		PyObject* value;

		PyArg_ParseTuple(arg, "OO", &claz, &value);
		if ( !PyJPClass::check(claz))
		{
			RAISE(JPypeException, "argument 1 must be a _jpype.JavaClass");
		}

		JPClass* type = ((PyJPClass*)claz)->m_Class;
		if (type==NULL)
		{
			Py_RETURN_NONE;
		}

		jvalue v = type->convertToJava(value);
		jvalue* pv = new jvalue();

		// Transfer ownership to python
		if (type->isObjectType())
		{
			pv->l = JPEnv::getJava()->NewGlobalRef(v.l);
			return PyJPValue::allocObject(pv);
		}
		else
		{
			*pv = v;
			return PyJPValue::allocPrimitive(pv);
		}
	}
	PY_STANDARD_CATCH

	return NULL;
}

