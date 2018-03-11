/*****************************************************************************
   Copyright 2004-2008 Steve Menard

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
#ifndef _JP_PYNI_H__
#define _JP_PYNI_H__

// predeclaration of PyObject
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
typedef void (*PyCapsule_Destructor)(PyObject *);
#endif

//  Note: Python uses a sized size type.  Thus we will map it to jlong.
//  Note: for conversions we will use jint and jlong types so that we map directly to java.

/**
 * Exception wrapper for python-generated exceptions
 * Produced by JPyErr::raise()
 */
class PythonException 
{
public :
	PythonException();	
	PythonException(const PythonException& ex);

	virtual ~PythonException();

	virtual string getMessage();
	
	bool isJavaException();

//	bool isJavaException(HostException* ex);
//	PyObject* getJavaException(HostException* ex);

	/** Gets the python object for an exception */
	PyObject* getJavaException();
	
public :
	PyObject* m_ExceptionClass;
	PyObject* m_ExceptionValue;
};


// This is just sugar to make sure that we are able to handle changes in 
// Python gracefully.  Adaptors do not handle memory management.  
// The Cleaner is used to make memory is handled properly.

class JPyObject 
{
	public:
		JPyObject(PyObject* obj) : pyobj(obj) {}
		JPyObject(const JPyObject &self) : pyobj(self.pyobj) {}

		void incref();
		void decref();

		jlong length();

		bool hasAttr(PyObject* k);
		PyObject* getAttr(PyObject* k);
		PyObject* getAttrString(const char* k);
		void setAttrString(const char* k, PyObject *v);
		PyObject* call(PyObject* a, PyObject* w);

		bool isNull() const 
		{
			return pyobj == NULL;
		}

		bool isNone() const;
		bool isInstance(PyObject* t) const;
		bool isSubclass(PyObject* t) const;

		operator PyObject*() const
		{
			return pyobj;
		}

	protected:
		PyObject* pyobj;
};

namespace JPPyni
{
	/**
	 * Must be inside of try catch block.
	 * throws PythonException on fail.
	 */
	void assertInitialized();

	/** Handle for exceptions.
	 * This will rethrow the exception and pass the exception to python.
	 */
	void handleException();

	JPyObject newMethod(JPMethod* m);

	JPyObject newClass(JPObjectClass* m);

	/** Create a new python object of type JavaArrayCladd
	 */
	JPyObject newArrayClass(JPArrayClass* m);

	/** Create a new python array of type JavaArray
	 */
	JPyObject newArray(JPArray* m);

	/** Find a callable method of a function 
	 * Used by the proxy.
	 */
	JPyObject getCallableFrom(PyObject* ref, string& name);

	void printError();
	PyObject* newStringWrapper(jstring jstr);

	/** Create a python JObject */
  PyObject* newObject(const JPValue& value);

	void* prepareCallbackBegin();
	void prepareCallbackFinish(void* state);

	/** Returns a new reference to None */
	PyObject* getNone();

	/** Returns a new reference to True */
	PyObject* getTrue();

	/** Returns a new reference to False */
	PyObject* getFalse();

	extern PyObject* m_GetArrayClassMethod;
	extern PyObject* m_GetClassMethod;
	extern PyObject* m_JavaArrayClass;
  extern PyObject* m_GetJavaArrayClassMethod;
	extern PyObject* m_JavaExceptionClass;
	extern PyObject* m_ProxyClass;
	extern PyObject* m_PythonJavaObject;
	extern PyObject* m_PythonJavaClass;
	extern PyObject* m_SpecialConstructorKey;
	extern PyObject* m_StringWrapperClass;
	extern PyObject* m_WrapperClass;

	void* gotoExternal();
	void returnExternal(void* state);
};

class JPyBool : public JPyObject
{
	public :
		JPyBool(PyObject* obj) : JPyObject(obj) {}
		JPyBool(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);		
		static JPyBool fromLong(jlong value);

		bool isTrue();		
		bool isFalse();		
};

class JPyCapsule : public JPyObject
{
	public :
		JPyCapsule(PyObject* obj) : JPyObject(obj) {}
		JPyCapsule(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);
		static PyObject* fromVoid(void* data, PyCapsule_Destructor destr);
		static PyObject* fromVoidAndDesc(void* data, const char* name, PyCapsule_Destructor destr);
		void* asVoidPtr();
		const char* getName();
};

class JPyType : public JPyObject
{
	public:
		JPyType(PyObject* obj) : JPyObject(obj) {}
		JPyType(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);
		bool isSubclass(PyObject* o2);
};

// Exception safe handler for PyObject*
class JPyCleaner
{
	public: 
		JPyCleaner();
		~JPyCleaner();
		template <typename T> T& add(T& object)
		{
			refs.push_back((PyObject*)object);
			return object;
		}
		template <typename T> const T& add(const T& object)
		{
			refs.push_back((PyObject*)object);
			return object;
		}

		template <typename T> PyObject* keep(T object)
		{
			PyObject* obj = (PyObject*)object;
			JPyObject(obj).incref();
			return obj;
		}

	private:
		vector<PyObject*> refs;
};

class JPyInt : public JPyObject
{
	public:
		JPyInt(PyObject* obj) : JPyObject(obj) {}
		JPyInt(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);

		static PyObject* fromInt(jint l);
		static PyObject* fromLong(jlong l);
		jint asInt();
};

class JPyLong : public JPyObject
{
	public:
		JPyLong(PyObject* obj) : JPyObject(obj) {}
		JPyLong(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);

		static PyObject* fromLong(jlong l);
		jlong asLong();
};

class JPyFloat : public JPyObject
{
	public:
		JPyFloat(PyObject* obj) : JPyObject(obj) {}
		JPyFloat(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);
		static PyObject* fromDouble(jdouble l);
		jdouble asDouble();
		static PyObject* fromFloat(jfloat l);
		jfloat asFloat();
};

/** This wrapper is problematic because it is oriented toward 
 * Python2.
 */
class JPyString : public JPyObject
{
	public:
		JPyString(PyObject* obj) : JPyObject(obj) {}
		JPyString(const JPyObject &self) : JPyObject(self) {}
		static bool check(PyObject* obj);
		static bool checkStrict(PyObject* obj);
		static bool checkUnicode(PyObject* obj);
		
		string asString();
		jlong asStringAndSize(char** buffer, jlong &length);

		JCharString asJCharString();
		static JPyObject fromUnicode(const jchar* str, int len);
		static JPyObject fromString(const char* str);

		void getRawByteString(char** outBuffer, jlong& outSize);
		void getRawUnicodeString(jchar** outBuffer, jlong& outSize);

		/** Get the size of a unicode character */
		static jlong getUnicodeSize();

		bool isChar();
		jchar asChar();
};

class JPyMemoryView : public JPyObject
{
	public:
		JPyMemoryView(PyObject* obj) : JPyObject(obj) {}
		JPyMemoryView(const JPyObject &self) : JPyObject(self) {}
		static bool check(PyObject* obj);
		void getByteBufferPtr(char** outBuffer, jlong& outSize);
};

class JPyTuple : public JPyObject
{
	public:
		JPyTuple(PyObject* obj) : JPyObject(obj) {}
		JPyTuple(const JPyObject &self) : JPyObject(self) {}

		static JPyTuple newTuple(jlong sz);
		static bool check(PyObject* obj);

		// Note this does not steal a reference
		void setItem(jlong ndx, PyObject* val);
		PyObject* getItem(jlong ndx);
		jlong	size();
};

class JPyList : public JPyObject
{
	public:
		JPyList(PyObject* obj) : JPyObject(obj) {}
		JPyList(const JPyObject &self) : JPyObject(self) {}

		static JPyList newList(jlong sz);
		static bool check(PyObject* obj);
		// Note this does not steal a reference
		void setItem(jlong ndx, PyObject* val);
		PyObject* getItem(jlong ndx);
		jlong size();
};

class JPySequence : public JPyObject
{
	public:
		JPySequence(PyObject* obj) : JPyObject(obj) {}
		JPySequence(const JPyObject &self) : JPyObject(self) {}

		// Note this use to work the same for list, sequence and tuple, but that breaks pypy
		static bool check(PyObject* obj);

		// Note this does not steal a reference
		void setItem(jlong ndx, PyObject* val);
		PyObject* getItem(jlong ndx);

		jlong size();
};

namespace JPyErr
{
	void setString(PyObject* type, const char* str);
	void setObject(PyObject* type, PyObject* str);

	void clear();

	/** Create a runtime error in python */
	void setRuntimeError(const char* msg);

	/** Create a attribute error in python.
	 */
	void setAttributeError(const char* msg);

	/** Create a type error in python
	 * Use when an object is cast to the wrong type.
	 */
	void setTypeError(const char* msg);

	/** Terminate the function.  
	 * Must set the error prior to calling raise.
	 * Call within appropriate try block only. 
	 * msg is not used.
	 *
	 * Produces a PythonException
	 */
	void raise(const char* msg);
}

class JPyDict : public JPyObject
{
	public:
		JPyDict(PyObject* obj) : JPyObject(obj) {}
		JPyDict(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);

		bool contains(PyObject* k);
		PyObject* getItem(PyObject* k);
		PyObject* getKeys();
		PyObject* copy(PyObject* m);
    static JPyDict newInstance();
		void setItemString(PyObject* o, const char* n);
};

/** This serves as a dispatch wrapper for decoding the type
 * of an object.
 */
class JPyAdaptor : public JPyObject
{
	public:
		JPyAdaptor(PyObject* obj) : JPyObject(obj) {}
		JPyAdaptor(const JPyObject &self) : JPyObject(self) {}

		bool isByteString()
		{
			return JPyString::checkStrict(pyobj);
		}

		bool isString()
		{
			return JPyString::check(pyobj);
		}

		bool isByteBuffer()
		{
			return JPyMemoryView::check(pyobj);
		}

		bool isUnicodeString()
		{
			return JPyString::checkUnicode(pyobj);
		}

	// Boolean
		bool isBoolean()
		{ 
			return JPyBool::check(pyobj);
		}

		jboolean asBoolean()
		{
			return JPyBool(pyobj).isTrue();
		}

	// Int
		bool isInt()
		{
			return JPyInt::check(pyobj);
		}

	 JPyInt asInt()
		{
			return JPyInt(pyobj);
		}

	// Long
		bool isLong()
		{
			return JPyLong::check(pyobj);
		}

		JPyLong asLong()
		{
			return JPyLong(pyobj);
		}

	// Float
		bool isFloat()
		{
			return JPyFloat::check(pyobj);
		}

		JPyFloat asFloat()
		{
			return JPyLong(pyobj);
		}


		bool isSequence()
		{
			return JPySequence::check(pyobj) && ! JPyString::check(pyobj);
		}

	// Java
		bool isJavaValue() const;
		const JPValue& asJavaValue();

		bool isJavaClass() const
		{
			return isInstance(JPPyni::m_PythonJavaClass);
		}

		JPClass* asJavaClass();

		JPMethod* asJavaMethod();

	// Array
		bool isArray() const
		{
			return isInstance(JPPyni::m_JavaArrayClass)?true:false;
		}
		JPArray* asArray();

		bool isArrayClass() const
		{
			return (JPyType::check(pyobj)) && JPyType(pyobj).isSubclass(JPPyni::m_JavaArrayClass);
		}
		JPArrayClass* asJavaArrayClass();

	// Proxy
		bool isProxy() const
		{
			return isInstance(JPPyni::m_ProxyClass);
		}
		JPProxy* asProxy();

	// Wrapper
		bool  isWrapper()
		{
			return isInstance(JPPyni::m_WrapperClass);
		}

};

#endif
