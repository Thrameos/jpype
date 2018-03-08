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

static void deleteJPProxyDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
{
	JPProxy* pv = (JPProxy*)CAPSULE_EXTRACT(data);
	delete pv;
}

PyObject* PyJProxy::createProxy(PyObject*, PyObject* arg)
{
	try {
		JPLocalFrame frame;
		JPyCleaner cleaner;

		PyObject* self;
		PyObject* intf;

		PyArg_ParseTuple(arg, "OO", &self, &intf);

		std::vector<JPObjectClass*> interfaces;
		Py_ssize_t len = JPyObject::length(intf);

		for (Py_ssize_t i = 0; i < len; i++)
		{
			PyObject* subObj = cleaner.add(JPySequence::getItem(intf, i));
			PyObject* claz = JPyObject::getAttrString(subObj, "__javaclass__");
			if ( !PyJClass:check(claz))
			{
				RAISE(JPypeException, "interfaces must be of type _jpype.JavaClass");
			}
			JPObjectClass* c = dynamic_cast<JPObjectClass*>(((PyJPClass*)claz)->m_Class);
			if ( c == NULL)
			{
				RAISE(JPypeException, "interfaces must be object class instances");
			}
			interfaces.push_back(c);
		}
		
		JPProxy* proxy = new JPProxy(self, interfaces);

		PyObject* res = JPyCObject::fromVoidAndDesc(proxy, "jproxy", deleteJPProxyDestructor);
		return res;
	}
	PY_STANDARD_CATCH

	return NULL;
}

