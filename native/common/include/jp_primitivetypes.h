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
#ifndef _JPPRIMITIVETYPE_H_
#define _JPPRIMITIVETYPE_H_

class JPPrimitiveType : public JPClass
{
protected :
	JPPrimitiveType(const string& boxedNative);
	
	virtual ~JPPrimitiveType();
	
private :
	JPObjectClass* m_BoxedClass;

public :
	virtual bool       isObjectType() const
	{ 
		return false;
	}

	virtual jobject	   convertToJavaObject(HostRef* obj);

	virtual PyObject* getArrayRangeToSequence(jarray array, int start, int length) = 0;

	/** Special implementation for primitive types.
	 */
	virtual void       setArrayRange(jarray array, int start, int length, PyObject* sequence) = 0;

	/** 
	 * Conversion type to change a primitive to a boxed type.
	 */
	JPObjectClass* getBoxedClass();
};

class JPVoidType : public JPPrimitiveType
{
public :
	JPVoidType();
	
	virtual ~JPVoidType()
	{
	}
	
public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length)
	{
		RAISE(JPypeException, "not impled for void*");
	}
	virtual void      setArrayRange(jarray, int start, int length, PyObject* seq)
	{
		RAISE(JPypeException, "not impled for void*");
	}

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass* other) const;
};

class JPByteType : public JPPrimitiveType
{
public :
	JPByteType();
	
	virtual ~JPByteType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	// this returns tuple instead of list, for performance reasons
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);


	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubclass(const JPClass* other) const;
};

class JPShortType : public JPPrimitiveType
{
public :
	JPShortType();
	
	virtual ~JPShortType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass* other) const;
};

class JPIntType : public JPPrimitiveType
{
public :
	JPIntType();
	
	virtual ~JPIntType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int, int, PyObject*);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubclass(const JPClass* other) const;
};

class JPLongType : public JPPrimitiveType
{
public :
	JPLongType();
	
	virtual ~JPLongType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass* other) const;
};

class JPFloatType : public JPPrimitiveType
{
public :
	JPFloatType();
	
	virtual ~JPFloatType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass& other) const;
};

class JPDoubleType : public JPPrimitiveType
{
public :
	JPDoubleType();
	
	virtual ~JPDoubleType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);

	virtual bool isSubclass(const JPClass* other) const;
};

class JPCharType : public JPPrimitiveType
{
public :
	JPCharType();
	
	virtual ~JPCharType()
	{
	}

public : // JPType implementation
	virtual HostRef*   getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*   getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*   asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*   invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*   invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*>  getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef*  getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass* other) const;
};

class JPBooleanType : public JPPrimitiveType
{
public :
	JPBooleanType();
	
	virtual ~JPBooleanType()
	{
	}

public : // JPType implementation
	virtual HostRef*  getStaticValue(JPClass* c, jfieldID fid);
	virtual void       setStaticValue(JPClass* c, jfieldID fid, HostRef* val);
	virtual HostRef*  getInstanceValue(jobject c, jfieldID fid);
	virtual void       setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*  asHostObject(jvalue val);
	virtual HostRef*   asHostObjectFromObject(jobject val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	virtual HostRef*  invokeStatic(JPClass*, jmethodID, jvalue*);
	virtual HostRef*  invoke(jobject, JPClass*, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual void      setArrayRange(jarray, int start, int length, PyObject* sequence);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	
	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubclass(const JPClass& other) const;
};

#endif // _JPPRIMITIVETYPE_H_

