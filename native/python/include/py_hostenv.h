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
#ifndef _PYHOSTENV_H_
#define _PYHOSTENV_H_


class PythonHostEnvironment : public HostEnvironment
{
public :
	PythonHostEnvironment()
	{
	}
	
	virtual ~PythonHostEnvironment()
	{
	}
	
public :
	void setPythonJavaObject(PyObject* obj)
	{
		m_PythonJavaObject = obj;
	}

	void setPythonJavaClass(PyObject* obj)
	{
		m_PythonJavaClass = obj;
	}

	void setJavaArrayClass(PyObject* obj)
	{
		m_JavaArrayClass = obj;
	}

	void setWrapperClass(PyObject* obj)
	{
		m_WrapperClass = obj;
	}

	void setStringWrapperClass(PyObject* obj)
	{
		m_StringWrapperClass = obj;
	}

	void setProxyClass(PyObject* obj)
	{
		m_ProxyClass = obj;
	}

	void setGetJavaClassMethod(PyObject* obj)
	{
		m_GetClassMethod = obj;
		Py_INCREF(obj);
	}


	void setGetJavaArrayClassMethod(PyObject* obj)
	{
		m_GetArrayClassMethod = obj;
		Py_INCREF(obj);
	}

	void setSpecialConstructorKey(PyObject* obj)
	{
		m_SpecialConstructorKey = obj;
		Py_INCREF(obj);
	}

	PyObject* getSpecialConstructorKey()
	{
		return m_SpecialConstructorKey;
	}

	void setJavaExceptionClass(PyObject* obj)
	{
		m_JavaExceptionClass = obj;
	}  

	static void deleteJPObjectDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
    {
        delete (JPObject*)CAPSULE_EXTRACT(data);
    }

	static void deleteJPArrayDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
	{
		delete (JPArray*)CAPSULE_EXTRACT(data);
	}

	static void deleteObjectJValueDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
	{
		jvalue* pv = (jvalue*)CAPSULE_EXTRACT(data);
		JPEnv::getJava()->DeleteGlobalRef(pv->l);
		delete pv;
	}

	static void deleteJValueDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
	{
		jvalue* pv = (jvalue*)CAPSULE_EXTRACT(data);
		delete pv;
	}

	static void deleteJPProxyDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
	{
		JPProxy* pv = (JPProxy*)CAPSULE_EXTRACT(data);
		delete pv;
	}

	

	PyObject* getJavaShadowClass(JPObjectClass* jc);

private :
	PyObject* m_PythonJavaObject;
	PyObject* m_PythonJavaClass;
	PyObject* m_JavaArrayClass;
	PyObject* m_WrapperClass;
	PyObject* m_StringWrapperClass;
	PyObject* m_ProxyClass;
	map<string, PyObject*> m_ClassMap;
	PyObject* m_GetClassMethod;
	PyObject* m_GetArrayClassMethod;


public :
	PyObject* m_SpecialConstructorKey;
	PyObject* m_JavaExceptionClass;

public :
	virtual void* acquireRef(void*);
	virtual void releaseRef(void*);
	virtual bool isRefNull(void*);
	virtual string describeRef(HostRef*);

	virtual void* gotoExternal();
	virtual void returnExternal(void* state);

	virtual void setRuntimeException(const char* msg);
	virtual void setAttributeError(const char* msg);
	virtual void setTypeError(const char* msg);
	virtual void raise(const char* msg);
	
	virtual HostRef* getNone();
	virtual bool isNone(HostRef*);

	virtual bool     isBoolean(HostRef*);
	virtual jboolean booleanAsBoolean(HostRef*);
	virtual HostRef* getTrue();
	virtual HostRef* getFalse();

	virtual bool isSequence(HostRef*);
	virtual HostRef* newMutableSequence(jsize);
	virtual HostRef* newImmutableSequence(jsize);
	virtual jsize getSequenceLength(HostRef*);
	virtual HostRef* getSequenceItem(HostRef*, jsize);
	virtual void setSequenceItem(HostRef*, jsize, HostRef*);
	
	virtual bool isInt(HostRef*);
	virtual HostRef* newInt(jint);
	virtual jint intAsInt(HostRef*);

	virtual bool isLong(HostRef*);
	virtual HostRef* newLong(jlong);
	virtual jlong longAsLong(HostRef*);

	virtual bool isFloat(HostRef*);
	virtual HostRef* newFloat(jdouble);
	virtual jdouble floatAsDouble(HostRef*);

	virtual bool isMethod(HostRef*);
	virtual HostRef* newMethod(JPMethod*);
	virtual JPMethod* asMethod(HostRef*);

	virtual bool isObject(HostRef*);
	virtual HostRef* newObject(JPObject*);
	virtual JPObject* asObject(HostRef*);

	virtual bool isClass(HostRef*);
	virtual HostRef* newClass(JPObjectClass*);
	virtual JPObjectClass* asClass(HostRef*);

	virtual bool isArrayClass(HostRef*);
	virtual HostRef* newArrayClass(JPArrayClass*);
	virtual JPArrayClass* asArrayClass(HostRef*);

	virtual bool isArray(HostRef*);
	virtual HostRef* newArray(JPArray*);
	virtual JPArray* asArray(HostRef*);

	virtual bool                   isProxy(HostRef*);
	virtual JPProxy* asProxy(HostRef*);
	virtual HostRef* getCallableFrom(HostRef*, string&);

	virtual bool isWrapper(PyObject*) ;
	virtual JPClass* getWrapperClass(PyObject*);
	virtual jvalue getWrapperValue(PyObject*);

	virtual bool isWrapper(HostRef*);
	virtual JPClass* getWrapperClass(HostRef*);
	virtual jvalue getWrapperValue(HostRef*);
	virtual HostRef* newStringWrapper(jstring);

	virtual bool isString(HostRef*);
	virtual jsize getStringLength(HostRef*);
	virtual string   stringAsString(HostRef*);
	virtual JCharString stringAsJCharString(HostRef*);
	virtual HostRef* newStringFromUnicode(const jchar*, unsigned int);
	virtual HostRef* newStringFromASCII(const char*, unsigned int);
	virtual bool     isByteString(HostRef*);
	virtual bool     isUnicodeString(HostRef* ref);
	virtual void     getRawByteString(HostRef*, char**, long&);
	virtual void     getRawUnicodeString(HostRef*, jchar**, long&);
	virtual size_t   getUnicodeSize();

	virtual void* prepareCallbackBegin();
	virtual void  prepareCallbackFinish(void* state);

	virtual HostRef* callObject(HostRef* callable, vector<HostRef*>& args);

	virtual void printError();

	virtual bool mapContains(HostRef* map, HostRef* key);
	virtual HostRef* getMapItem(HostRef* map, HostRef* key);	

	virtual bool objectHasAttribute(HostRef* obj, HostRef* key);
	virtual HostRef* getObjectAttribute(HostRef* obj, HostRef* key);

	virtual bool isJavaException(HostException*);
	virtual HostRef* getJavaException(HostException*);
	virtual void clearError();
	virtual void printReferenceInfo(HostRef* obj);
	virtual bool isByteBuffer(HostRef*);
	virtual void getByteBufferPtr(HostRef*, char**, long&);
};

#endif // _PYHOSTENV_H_
