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
#ifndef _JPTYPE_MANAGER_H_
#define _JPTYPE_MANAGER_H_

/**
 * These functions will manage the cache of found type, be it primitive types, class types or the "magic" types.
 */
namespace JPTypeManager
{
  extern JPPrimitiveType *_void;
  extern JPPrimitiveType *_boolean;
  extern JPPrimitiveType *_byte;
  extern JPPrimitiveType *_char;
  extern JPPrimitiveType *_short;
  extern JPPrimitiveType *_long;
  extern JPPrimitiveType *_int;
  extern JPPrimitiveType *_float ;
  extern JPPrimitiveType *_double;

  extern JPClass *_java_lang_Object;
  extern JPClass *_java_lang_Class;
  extern JPClass *_java_lang_String;

  extern JPClass *_java_lang_Boolean;
  extern JPClass *_java_lang_Byte;
  extern JPClass *_java_lang_Char;
  extern JPClass *_java_lang_Short;
  extern JPClass *_java_lang_Integer;
  extern JPClass *_java_lang_Long;
  extern JPClass *_java_lang_Float;
  extern JPClass *_java_lang_Double;

	/**
	 * Initialize the type manager caches
	 */
	void init();
	
	/**
	 * delete allocated typenames, should only be called at program termination
	 */
	void shutdown();


	/**
	 * Get a bridge class by java native name.
	 *
	 * The pointer returned is NOT owned by the caller
	 */
	JPClass* findClassByName(const std::string& cls);

	/**
	 * Get a bridge class from a java native class.
	 *
	 * The pointer returned is NOT owned by the caller
	 *
	 * Will be JType, JClass or JArrayClass based on the type
	 * 
	 * May return NULL if the java class does not exist.
	 * Will return NULL if the jclass is NULL.
	 */
	JPClass* findClass(jclass cls);

	void flushCache();

	int getLoadedClasses();

	/** 
	 * Convert a simple name to a qualified name.
	 *
	 * This produces a something suitable for 
	 * JPJni::findClass() or JPTypeManager::findClassByName()
	 */
	string getQualifiedName(const string& name);
}

#endif // _JPCLASS_H_
