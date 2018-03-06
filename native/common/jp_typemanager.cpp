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

#include <map>
#include <list>
namespace {
//AT's on porting:
// 1) TODO: test on HP-UX platform. Cause: it is suspected to be an undefined order of initialization of static objects
//
//  2) TODO: in any case, use of static objects may impose problems in multi-threaded environment.
	typedef std::list<JPClass*> JavaClassList;
	typedef std::map<int, JavaClassList> JavaClassMap;
	JavaClassMap javaClassMap;
}

namespace JPTypeManager {

int loadedClasses = 0;
JPPrimitiveType *_void  = 0;
JPPrimitiveType *_boolean  = 0;
JPPrimitiveType *_byte  = 0;
JPPrimitiveType *_char  = 0;
JPPrimitiveType *_short  = 0;
JPPrimitiveType *_long  = 0;
JPPrimitiveType *_int  = 0;
JPPrimitiveType *_float  = 0;
JPPrimitiveType *_double  = 0;
JPClass *_java_lang_Object  = 0;
JPClass *_java_lang_Class  = 0;
JPClass *_java_lang_String  = 0;
JPClass *_java_lang_Boolean = 0;
JPClass *_java_lang_Byte = 0;
JPClass *_java_lang_Character = 0;
JPClass *_java_lang_Short = 0;
JPClass *_java_lang_Integer = 0;
JPClass *_java_lang_Long = 0;
JPClass *_java_lang_Float = 0;
JPClass *_java_lang_Double = 0;

void registerClass(JPClass* type)
{
	TRACE_IN("JPTypeManager::registerClass");
	loadedClasses++;
	int hash = JPJni::hashCode(type->getNativeClass());
	TRACE2(hash, type->getName().getSimpleName());
	type->postLoad();
	javaClassMap[hash].push_back(type);
	TRACE_OUT;
}

void init()
{
	TRACE_IN("JPTypeManager::init");
	// This will load all of the specializations that we need to operate
	// All other classes can be loaded later as JPObjectClass or JPArrayClass.
	
	// Preload basic types (specializations)
	registerClass(_java_lang_Object = new JPObjectBaseClass());
	registerClass(_java_lang_Class = new JPClassBaseClass());

	// Preload the boxed types 
	registerClass(_java_lang_Boolean = new JPBoxedBooleanClass());
	registerClass(_java_lang_Byte = new JPBoxedByteClass());
	registerClass(_java_lang_Character = new JPBoxedCharacterClass());
	registerClass(_java_lang_Short = new JPBoxedShortClass());
	registerClass(_java_lang_Integer = new JPBoxedIntegerClass());
	registerClass(_java_lang_Long = new JPBoxedLongClass());
	registerClass(_java_lang_Float = new JPBoxedFloatClass());
	registerClass(_java_lang_Double = new JPBoxedDoubleClass());

	// Preload the primitive types
	registerClass(_void=new JPVoidType());
	registerClass(_boolean=new JPBooleanType());
	registerClass(_byte=new JPByteType());
	registerClass(_char=new JPCharType());
	registerClass(_short=new JPShortType());
	registerClass(_int=new JPIntType());
	registerClass(_long=new JPLongType());
	registerClass(_float=new JPFloatType());
	registerClass(_double=new JPDoubleType());

	// Preload the string type
	registerClass(_java_lang_String = new JPStringClass());
	TRACE_OUT;
}

JPClass* findClass(jclass cls)
{
	if (JPEnv::getJava() == 0)
		return NULL;

	// If  the class lookup failed then we will receive NULL
	if (cls == NULL)
		return NULL;

	int hash = JPJni::hashCode(cls);

	// First check in the map ...
	JavaClassMap::iterator cur = javaClassMap.find(hash);
	if (cur != javaClassMap.end())
	{
		for (JavaClassList::iterator item=cur->second.begin(); item!=cur->second.end(); ++item)
		{
			if (JPEnv::getJava()->IsSameObject((jobject)((*item)->getNativeClass()),(jobject)cls))
				return *item;
		}
	}

	TRACE_IN("JPTypeManager::findClass");
	TRACE1(JPJni::getName(cls).getSimpleName());

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

	registerClass(res);

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
		for (JavaClassList::iterator j = i->second.begin(); j!= i->second.end(); ++j)
			delete *j;
	}

	javaClassMap.clear();
	loadedClasses = 0;
}

int getLoadedClasses()
{
	// dignostic tools ... unlikely to load more classes than int can hold ...
	return loadedClasses;
}

} // end of namespace JPTypeManager
