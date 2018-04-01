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
	virtual const string& getSimpleName() const
	{
		return m_Name;
	}

	virtual jclass getNativeClass() const
	{
		return m_Class;
	}

// Conversion methods
	virtual PyObject*   asHostObject(jvalue val) = 0;

	/** This is used by Proxy.  The class and the 
	 * object type can differ in the case of primitives.
	 */
	virtual PyObject*   asHostObjectFromObject(jobject val);

	/** Convert a host ref to a java value.
	 * This preserves primitives.
	 */
	virtual jvalue     convertToJava(PyObject* obj) = 0;

	/** Convert a host ref to a java object.
	 * This will box primitives.
	 */
	virtual jobject    convertToJavaObject(PyObject* obj);

	/** Determine if a host ref can be converted to an object of this type.
	 */
	virtual EMatchType canConvertToJava(PyObject* obj) = 0;

// Probe methods
	bool			         isAbstract() const;
	bool	             isArray() const;
	bool			         isFinal() const;
	virtual bool       isInterface() const;
	virtual bool       isObjectType() const = 0;
	long               getClassModifiers();

	/** Returns true if this class can be assigned to a target class.
	 * This will be true if this class implements or extends the target class.
	 * (For some reason Java uses the name isAssignableFrom which confuses the hell
	 * out of everyone.)
	 */
	bool               isAssignableTo(const JPClass* o) const;

// Get/Set a static field
  /** Get a static value.  
	 * This allocates a new host reference. The host reference must
	 * be deleted.
	 */
	virtual PyObject*   getStaticValue(JPClass* c, jfieldID fid);

	/** Set a static field to a value.
	 * This is required because JNI has different methods for each different 
	 * type.
	 */
	virtual void       setStaticValue(JPClass* c, jfieldID fid, PyObject* val);

	// Get/Set a member field
	virtual PyObject*   getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, PyObject* val);

  // Invoke a method that produces this class (static)
	virtual PyObject*   invokeStatic(JPClass*, jmethodID, jvalue*);

  // Invoke a method that produces this class (member)
	virtual PyObject*   invoke(jobject, JPClass* clazz, jmethodID, jvalue*);

	// Array methods
	virtual jarray     newArrayInstance(int size);
	virtual vector<PyObject*> getArrayRange(jarray, int start, int length);
	virtual PyObject*   getArrayItem(jarray, int ndx);
	virtual void       setArrayItem(jarray, int ndx, PyObject* val);
	virtual void       setArrayRange(jarray, int start, int length, vector<PyObject*>& vals);

	// Loading support
	virtual void postLoad()
	{}

protected :
	jclass m_Class;
	string m_Name;
};

#endif // _JPPOBJECTTYPE_H_
