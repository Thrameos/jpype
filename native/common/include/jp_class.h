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

	virtual const JPTypeName& getName() const
	{
		return m_Name;
	}

	virtual jclass getNativeClass() const
	{
		return m_Class;
	}

	virtual bool      isObjectType() const = 0;

	virtual HostRef*   asHostObject(jvalue val) = 0;
	virtual EMatchType  canConvertToJava(HostRef* obj) = 0;
	virtual jvalue      convertToJava(HostRef* obj) = 0;

	virtual HostRef* getStaticValue(jclass c, jfieldID fid);
	virtual void      setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef* getInstanceValue(jobject c, jfieldID fid);
	virtual void      setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);

	virtual jobject convertToJavaObject(HostRef* obj);

	virtual HostRef* invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef* invoke(jobject, jclass clazz, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length) = 0;
	virtual void setArrayRange(jarray, int start, int len, PyObject*) = 0;

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPClass& other) const;

	virtual void postLoad()
	{}

protected :
	jclass m_Class;
	JPTypeName m_Name;
};

#endif // _JPPOBJECTTYPE_H_
