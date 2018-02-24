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
	typedef map<int, JPClass* > JavaClassMap;
	JavaClassMap javaClassMap;
}

namespace JPTypeManager {
JPClass *_void  = 0;
JPClass *_boolean  = 0;
JPClass *_byte  = 0;
JPClass *_short  = 0;
JPClass *_long  = 0;
JPClass *_int  = 0;
JPClass *_float  = 0;
JPClass *_double  = 0;
JPClass *_java_lang_String  = 0;

void registerPrimitiveClass(JPClass* type)
{
	jclass cls = type->getNativeClass();
	int hash = JPJni::hashCode(cls);
	javaClassMap[hash] = type;
}

void init()
{
	// Preload the primitive types
	registerPrimitiveClass(_void=new JPVoidType());
	registerPrimitiveClass(_boolean=new JPBooleanType());
	registerPrimitiveClass(_byte=new JPByteType());
	registerPrimitiveClass(_short=new JPShortType());
	registerPrimitiveClass(_int=new JPIntType());
	registerPrimitiveClass(_long=new JPLongType());
	registerPrimitiveClass(_float=new JPFloatType());
	registerPrimitiveClass(_double=new JPDoubleType());

	// Preload the string type
	JPStringType* strType= new JPStringType();
	strType->postLoad();
	javaClassMap[JPJni::hashCode(strType->getNativeClass())]=strType;
	_java_lang_String = strType;
}

JPClass* findClass(jclass cls)
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
		res = new JPObjectClass(cls);
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
