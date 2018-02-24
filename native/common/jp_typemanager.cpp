/*****************************************************************************
   Copyright 2004 Steve Menard

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
#include <jpype.h>

namespace {
//AT's on porting:
// 1) TODO: test on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
	typedef map<JPTypeName::ETypes, JPType*> TypeMap;
	typedef map<int, JPType* > JavaClassMap;

	TypeMap typeMap;
	JavaClassMap javaClassMap;

}

namespace JPTypeManager {

void registerPrimitiveClass(JPPrimitiveType* type)
{
	TRACE_IN("JPTypeManager::registerPrimitiveClass");
	// Add to the etype to type table
	JPTypeName::ETypes etype = type->getEType();
	JPTypeName name = type->getName();
	jclass cls = type->getNativeClass();
	typeMap[etype] = type;

	int hash = JPJni::hashCode(cls);
	javaClassMap[hash] = type;
	TRACE1(JPJni::getName(cls).getSimpleName());
	TRACE1(hash);
	TRACE1(type->getNativeClass());

	TRACE_OUT;
}

void init()
{
	// Preload the primitive types
	registerPrimitiveClass(new JPVoidType());
	registerPrimitiveClass(new JPBooleanType());
	registerPrimitiveClass(new JPByteType());
	registerPrimitiveClass(new JPShortType());
	registerPrimitiveClass(new JPIntType());
	registerPrimitiveClass(new JPLongType());
	registerPrimitiveClass(new JPFloatType());
	registerPrimitiveClass(new JPDoubleType());

	// Preload the string type
	JPStringType* strType= new JPStringType();
	strType->postLoad();
	typeMap[JPTypeName::_string] = strType;
	javaClassMap[JPJni::hashCode(strType->getNativeClass())]=strType;
}

JPType* getPrimitiveType(JPTypeName::ETypes etype)
{
	return typeMap[etype];
}

JPType* findClass(jclass cls)
{
	if (JPEnv::getJava()==0)
		return 0;

	// If  the class lookup failed then we will receive NULL
	if (cls==NULL)
		return NULL;

	int hash = JPJni::hashCode(cls);

	// First check in the map ...
	JavaClassMap::iterator cur = javaClassMap.find(hash);

	if (cur != javaClassMap.end())
	{
		return cur->second;
	}

	TRACE_IN("JPTypeManager::findClass");
	TRACE1(JPJni::getName(cls).getSimpleName());
	TRACE1(hash);

	// No we havent got it .. lets load it!!!
	JPLocalFrame frame;

	JPClass* res;
	if (JPJni::isArray(cls))
	{
		res = new JPArrayClass(cls);
	}
	else
	{
		res = new JPClass(cls);
	}

	// Finish loading it
	res->postLoad();
	// Register it here before we do anything else
	javaClassMap[hash] = res;

	return res;
	TRACE_OUT;
}

void shutdown()
{
	flushCache();

	// delete primitive types
	for(TypeMap::iterator i = typeMap.begin(); i != typeMap.end(); ++i)
	{
		delete i->second;
	}
}

void flushCache()
{
	for(JavaClassMap::iterator i = javaClassMap.begin(); i != javaClassMap.end(); ++i)
	{
		delete i->second;
	}

	javaClassMap.clear();
}

int getLoadedClasses()
{
	// dignostic tools ... unlikely to load more classes than int can hold ...
	return (int)(javaClassMap.size());
}

} // end of namespace JPTypeManager
