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
#ifndef _JPPOBJECTTYPE_H_
#define _JPPOBJECTTYPE_H_

// Base type for ArrayClass, PrimitiveType, ObjectClass, and StringClass
class JPClass
{
protected :
	JPClass(jclass c);
	
public :
	virtual ~JPClass();

	// Accessors
	virtual const JPTypeName& getName() const
	{
		return m_Name;
	}

	virtual jclass getNativeClass() const
	{
		return m_Class;
	}

// Conversion methods
	virtual HostRef*   asHostObject(jvalue val) = 0;

	// FIXME This is used only by JPProxy. 
	// it is not obvious way it is required at all.
	virtual HostRef*   asHostObjectFromObject(jobject val);

	/** Convert a host ref to a java value.
	 * This preserves primitives.
	 */
	virtual jvalue     convertToJava(HostRef* obj) = 0;

	/** Convert a host ref to a java object.
	 * This will box primitives.
	 */
	virtual jobject    convertToJavaObject(HostRef* obj);

	/** Determine if a host ref can be converted to an object of this type.
	 */
	virtual EMatchType canConvertToJava(HostRef* obj) = 0;

// Get/Set a static field
  /** Get a static value.  
	 * This allocates a new host reference. The host reference must
	 * be deleted.
	 */
	virtual HostRef*   getStaticValue(JPClass* c, jfieldID fid);

	/** Set a static field to a value.
	 * This is required because JNI has different methods for each different 
	 * type.
	 */
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);

	// Get/Set a member field
	virtual HostRef*   getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);

  // Invoke a method that produces this class (static)
	virtual HostRef*   invokeStatic(JPClass*, jmethodID, jvalue*);

  // Invoke a method that produces this class (member)
	virtual HostRef*   invoke(jobject, JPClass* clazz, jmethodID, jvalue*);

	// Array methods
	virtual jarray     newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual HostRef*   getArrayItem(jarray, int ndx);
	virtual void       setArrayItem(jarray, int ndx, HostRef* val);

	virtual void       setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	// Probe methods
	virtual bool       isObjectType() const = 0;
	virtual bool       isSubTypeOf(const JPClass& other) const;

	// Loading support
	virtual void postLoad()
	{}

protected :
	jclass m_Class;
	JPTypeName m_Name;
};

#endif // _JPPOBJECTTYPE_H_
