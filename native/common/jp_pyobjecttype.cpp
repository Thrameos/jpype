/*
 * Copyright 2020 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "jpype.h"
#include "pyjp.h"
#include "jp_pyobjecttype.h"

JPPyObjectType::JPPyObjectType(JPJavaFrame& frame, jclass clss,
		const string& name,
		JPClass* super,
		JPClassList& interfaces,
		jint modifiers)
: JPClass(frame, clss, name, super, interfaces, modifiers)
{
	m_SelfID = frame.GetFieldID(clss, "_self", "J");
}

JPPyObjectType::~JPPyObjectType()
{
}

JPPyObject JPPyObjectType::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{

	// This loses type
	if (value.l == NULL)
	{
		return JPPyObject::getNone();
	}

	JPClass *cls = this;
	if (!cast)
	{
		cls = frame.findClassForObject(value.l);
		if (cls != this)
			return cls->convertToPythonObject(frame, value, true);
	}

	// Just unwrap it.
	PyObject* pyobj = (PyObject*) frame.GetLongField(value.l, m_SelfID);
	return JPPyObject::use(pyobj);
}

