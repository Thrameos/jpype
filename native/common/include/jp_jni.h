/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _JPJNIUTIL_H_
#define _JPJNIUTIL_H_

namespace JPJni
{
	extern jclass s_ObjectClass;
	extern jclass s_ClassClass;
	extern jclass s_StringClass;
	extern jclass s_NoSuchMethodErrorClass;
	extern jclass s_RuntimeExceptionClass;
	extern jclass s_ProxyClass;
	extern jmethodID s_NewProxyInstanceID;

	extern jlong s_minByte;
	extern jlong s_maxByte;
	extern jlong s_minShort;
	extern jlong s_maxShort;
	extern jlong s_minInt;
	extern jlong s_maxInt;
	extern jfloat s_minFloat;
	extern jfloat s_maxFloat;

	void init();

	void startJPypeReferenceQueue(bool);
	void stopJPypeReferenceQueue();
	void registerRef(jobject refQueue, jobject obj, jlong hostRef);

	string getStringUTF8(jstring str);
	jstring newStringUTF8(const string& str);
//	JCharString unicodeFromJava(jstring str);
//	jstring javaStringFromJCharString(JCharString& str);

	/** 
	 * Get the class for an object.
	 * Returns a local reference to the class.
	 */
	jclass getClass(jobject obj);

	jstring toString(jobject obj);

	/**
	* java.lang.Class.isArray()
	*/
	bool            isArray(jclass);

	/**
	* java.lang.Class.isInterface()
	*/
	bool            isInterface(jclass);

	/**
	* java.lang.reflect.Modifier.isAbstract(java.lang.Class.getModifiers()
	*/
	bool            isAbstract(jclass);

	/**
	* java.lang.reflect.Modifier.isFinal(java.lang.Class.getModifiers()
	*/
	bool            isFinal(jclass);

	/**
	* java.lang.Class.getName()
	*/
  string getSimpleName(jclass);

	/**
	* java.lang.Class.getInterfaces()
	*/
	vector<jclass>  getInterfaces(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getDeclaredFields()
	*/
	vector<jobject> getDeclaredFields(JPLocalFrame& frame, jclass);


	/**
	* java.lang.Class.getConstructors()
	*/
	vector<jobject> getConstructors(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getFields()
	*/
	vector<jobject> getFields(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getDeclaredMethods(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getMethods(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getDeclaredMethods()
	*/
	vector<jobject> getDeclaredConstructors(JPLocalFrame& frame, jclass);

	/**
	* java.lang.Class.getModifiers()
	*/
	long getClassModifiers(jclass);

	jobject getSystemClassLoader();

	/**
	* java.lang.reflect.Member.getName()
	*/
	string getMemberName(jobject);

	/**
	* java.lang.reflect.Modifier.isPublic(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberPublic(jobject);

	/**
	* java.lang.reflect.Modifier.is(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberStatic(jobject);

	/**
	* java.lang.reflect.Modifier.isFinal(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberFinal(jobject);

	/**
	* java.lang.reflect.Modifier.isAbstract(java.lang.reflect.member.getModifiers())
	*/
	bool isMemberAbstract(jobject);

	/**
	* java.lang.reflect.Field.getType
	*/
	jclass getType(jobject fld);

	/**
	* java.lang.reflect.Method.getReturnType
	*/
	jclass getReturnType(jobject);

	/**
	* java.lang.reflect.Method.isSynthetic()
	*/
	bool isMethodSynthetic(jobject);

	/**
	* java.lang.reflect.Method.isSynthetic()
	*/
	bool isVarArgsMethod(jobject);

	/** 
	 * Get the type for elements of an array.  
	 * Only applies to array classes.
	 */
	jclass getComponentType(jclass);

	jint hashCode(jobject);

	/**
	* java.lang.reflect.Method.getParameterTypes
	*/
	vector<jclass> getParameterTypes(jobject, bool);

	bool isConstructor(jobject);
	string getStackTrace(jthrowable th);
	string getMessage(jthrowable th);
	bool isThrowable(jclass c);

	jint intValue(jobject);
	jlong longValue(jobject);
	jdouble doubleValue(jobject);
	jboolean booleanValue(jobject);
	jchar charValue(jobject);

	/** Get the class for using a java native name.
	 *  Use the specification name for the Boxed type as the argument.
	 *  Returns a local reference.
	 */
	jclass findClass(const std::string& name);

	/** Get the class for a primitive type.  
	 *  Use the specification name for the java native Boxed type as the argument.
	 *
	 *  Class names are java native.
	 *  Returns a local reference.
	 */
	jclass findPrimitiveClass(const std::string& name);
};

#endif // _JPJNIUTIL_H_
