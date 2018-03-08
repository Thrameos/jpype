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

// This is the support methods for using a JPArray capsule

PyObject* JPypeJavaArray::findArrayClass(PyObject* obj, PyObject* args)
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

		return JPyCapsule::fromArrayClass(claz);
	}
	PY_STANDARD_CATCH;

	PyErr_Clear();

	Py_RETURN_NONE;
}

PyObject* JPypeJavaArray::getArrayLength(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		PyArg_ParseTuple(arg, "O!", &PyCapsule_Type, &arrayObject);
		JPArray* a = JPyCapsule(arrayObject).asArray();
		return JPyInt::fromLong(a->getLength());
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArrayItem(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx;
		PyArg_ParseTuple(arg, "O!i", &PyCapsule_Type, &arrayObject, &ndx);
		JPArray* a = JPyCapsule(arrayObject).asArray();
		return a->getItem(ndx);
	}
	PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::getArraySlice(PyObject* self, PyObject* arg)
{
	PyObject* arrayObject;
	int lo = -1;
	int hi = -1;
	try
	{

		PyArg_ParseTuple(arg, "O!ii", &PyCapsule_Type, &arrayObject, &lo, &hi);
		JPArray* a = JPyCapsule(arrayObject).asArray();

		int length = a->getLength();
		// stolen from jcc, to get nice slice support
		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		JPClass* component = a->getClass()->getComponentType();
		if (!component->isObjectType())
		{
			// for primitive types, we have fast sequence generation available
			return a->getSequenceFromRange(lo, hi);
		}
		else
		{
			// slow wrapped access for non primitives
			vector<PyObject*> values = a->getRange(lo, hi);
			JPyCleaner cleaner;
			for (unsigned int i = 0; i < values.size(); i++)
			{
				cleaner.add(values[i]);
			}

			JPyList res = JPyList::newList((int)values.size());
			for (unsigned int i = 0; i < values.size(); i++)
			{
				res.setItem(i, values[i]);
			}

			return res;
		}
	} PY_STANDARD_CATCH

	return NULL;
}

PyObject* JPypeJavaArray::setArraySlice(PyObject* self, PyObject* arg)
{
	TRACE_IN("JPypeJavaArray::setArraySlice")
	PyObject* arrayObject;
	int lo = -1;
	int hi = -1;
	PyObject* sequence;
	try {
		PyArg_ParseTuple(arg, "O!iiO", &PyCapsule_Type, &arrayObject, &lo, &hi, &sequence);
		JPArray* a = JPyCapsule(arrayObject).asArray();

		int length = a->getLength();
		if(length == 0)
			Py_RETURN_NONE;

		if (lo < 0) lo = length + lo;
		if (lo < 0) lo = 0;
		else if (lo > length) lo = length;
		if (hi < 0) hi = length + hi;
		if (hi < 0) hi = 0;
		else if (hi > length) hi = length;
		if (lo > hi) lo = hi;

		JPClass* component = a->getClass()->getComponentType();
		if (!component->isObjectType())
		{
			// for primitive types, we have fast setters available
			a->setRangePrimitive(lo, hi, sequence);
		}
		else
		{
			// slow wrapped access for non primitive types
			vector<PyObject*> values;
			values.reserve(hi - lo);
			JPyCleaner cleaner;
			for (Py_ssize_t i = 0; i < hi - lo; i++)
			{
				PyObject* v = JPySequence(sequence).getItem(i);
				values.push_back(v);
				cleaner.add(v);
			}

			a->setRange(lo, hi, values);
		}

		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
	TRACE_OUT
}

PyObject* JPypeJavaArray::setArrayItem(PyObject* self, PyObject* arg)
{
	try {
		PyObject* arrayObject;
		int ndx;
		PyObject* value;
		PyArg_ParseTuple(arg, "O!iO", &PyCapsule_Type, &arrayObject, &ndx, &value);
		JPArray* a = JPyCapsule(arrayObject).asArray();
		a->setItem(ndx, arg);
		Py_RETURN_NONE;
	}
	PY_STANDARD_CATCH

	return NULL;
}


PyObject* JPypeJavaArray::newArray(PyObject* self, PyObject* arg)
{
try	{
		PyObject* arrayObject;
		int sz;
		PyArg_ParseTuple(arg, "O!i", &PyCapsule_Type, &arrayObject, &sz);
		JPArrayClass* a = JPyCapsule(arrayObject).asArrayClass();
		JPArray* v = a->newInstance(sz);
		return JPyCapsule::fromArray(v);
	}
	PY_STANDARD_CATCH

	return NULL;
}
