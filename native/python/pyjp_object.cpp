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

static void deleteJPObjectDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
{
  delete (JPObject*)CAPSULE_EXTRACT(data);
}

PyObject* PyJPObject::alloc(JPObject* v)
{
  return JPyCapsule::fromVoidAndDesc((void*)v, "JPObject", &deleteJPObjectDestructor);
}

bool PyJPObject::check(PyObject* obj)
{
	return JPyCapsule::check(obj) && JPyCapsule(obj).getName() == string("JPObject");
}

