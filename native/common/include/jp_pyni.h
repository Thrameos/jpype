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

struct PyJClass;


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
		JPyObject getAttr(PyObject* k);
		JPyObject getAttrString(const char* k);
		void setAttrString(const char* k, PyObject *v);
		JPyObject call(PyObject* a, PyObject* w);

		bool isNull() const 
		{
			return pyobj == NULL;
		}

		bool isInstance(PyObject* t) const;
		bool isSubclass(PyObject* t) const;
		bool isMemoryView() const;
		void asPtrAndSize(char** buffer, jlong &length);

		operator PyObject*() const
		{
			return pyobj;
		}

	protected:
		PyObject* pyobj;
};

namespace JPPyni
{
	JPyObject newMethod(JPMethod* m);

	JPyObject newClass(JPObjectClass* m);

	/** Create a new python object of type JavaArray
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
	bool isJavaException(HostException* ex);
	JPyObject getJavaException(HostException* ex);
	JPyObject newStringWrapper(jstring jstr);
	JPyObject newObject(JPObject* obj);

	void* prepareCallbackBegin();
	void prepareCallbackFinish(void* state);

	JPyObject getNone();
	JPyObject getTrue();
	JPyObject getFalse();

public:
	PyObject* m_PythonJavaObject;
	PyObject* m_PythonJavaClass;
	PyObject* m_ProxyClass;
	PyObject* m_GetClassMethod;
	PyObject* m_JavaArrayClass;
	PyObject* m_StringWrapperClass;
	PyObject* m_GetArrayClassMethod;
	PyObject* m_WrapperClass;

	void setRuntimeException(const char* msg);
//	{
//		JyErr::setString(PyExc_RuntimeError, msg);
//	}

	void setAttributeError(const char* msg);
//	{
//		JPyErr::setString(PyExc_AttributeError, msg);
//	}

	void setTypeError(const char* msg);
//	{
//		JPyErr::setString(PyExc_TypeError, msg);
//	}

	void raise(const char* msg);
//	{
//		throw PythonException();
//	}


	void* gotoExternal();
//	{  
//		PyThreadState *_save; 
//		_save = PyEval_SaveThread();
//		return (void*)_save;
//	}

	void returnExternal(void* state);
//	{
//		PyThreadState *_save = (PyThreadState *)state;
//		PyEval_RestoreThread(_save);
//	}

};

class JPyBool : public JPyObject
{
	public :
		JPyBool(PyObject* obj) : JPyObject(obj) {}
		JPyBool(const JPyObject &self) : JPyObject(self) {}

		static bool check(JPyObject* obj);		
		static JPyBool fromLong(jlong value);
};

class JPyCapsule : public JPyObject
{
	public :
		JPyCapsule(PyObject* obj) : JPyObject(obj) {}
		JPyCapsule(const JPyObject &self) : JPyObject(self) {}

		static bool check(PyObject* obj);
		static JPyCapsule fromVoid(void* data, PyCapsule_Destructor destr);
		static JPyCapsule fromVoidAndDesc(void* data, const char* name, PyCapsule_Destructor destr);
		void* asVoidPtr();
		const char* getDesc();

		static JPyCapsule fromMethod(JPMethod* method);
		static JPyCapsule fromObject(JPObject* object);
		static JPyCapsule fromClass(JPClass* cls);
		static JPyCapsule fromArrayClass(JPArrayClass* cls);
		static JPyCapsule fromArray(JPArray* cls);
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
PyObject* jpy_cleaner_unwrap(PyJClass* cls)
{
	return (PyObject*)cls;
}

PyObject* jpy_cleaner_unwrap(PyObject* obj)
{
	return obj;
}

PyObject* jpy_cleaner_unwrap(const JPyObject& obj)
{
	return (PyObject*)obj;
}

class JPyCleaner
{
	public: 
		JPyCleaner();
		~JPyCleaner();
		template <typename T> T& add(T& object)
		{
			refs.push_back(jpy_cleaner_unwrap(object));
			return object;
		}
		template <typename T> const T& add(const T& object)
		{
			refs.push_back(jpy_cleaner_unwrap(object));
			return object;
		}


		template <typename T> PyObject* keep(T object)
		{
			PyObject* obj = jpy_cleaner_unwrap(object);
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

class JPyString : public JPyObject
{
	public:
		JPyString(PyObject* obj) : JPyObject(obj) {}
		JPyString(const JPyObject &self) : JPyObject(self) {}
		static bool check(PyObject* obj);
		static bool checkStrict(PyObject* obj);
		static bool checkUnicode(PyObject* obj);
		//Py_UNICODE* asUnicode();
		
		jlong size();

		string asString();
		jlong asStringAndSize(char** buffer, jlong &length);

		JCharString asJCharString();
		static JPyObject fromUnicode(const jchar* str, int len);
		static JPyObject fromString(const char* str);

		void getRawByteString(char** outBuffer, long& outSize);
		void getByteBufferPtr(char** outBuffer, long& outSize);

		void getRawUnicodeString(jchar** outBuffer, long& outSize);
//		{
//			PyObject* objRef = UNWRAP(obj);
//			outSize = (long)JPyObject::length(objRef);
//			*outBuffer = (jchar*)JPyString::AsUnicode(objRef);
//		}


		static size_t getUnicodeSize();
//		{
//			return sizeof(Py_UNICODE);
//		}

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
		jlong size();
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

class JPyErr : public JPyObject
{
	public:
		JPyErr(PyObject* obj) : JPyObject(obj) {}
		JPyErr(const JPyObject &self) : JPyObject(self) {}

		void setString(const char* str);
		void setObject(PyObject* str);
		static void clearError();
};

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
		void setItemString(PyObject* o, const jchar* n);
};

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
			return isMemoryView();
		}

		bool isUnicodeString()
		{
			return JPyString::checkUnicode(pyobj);
		}

		bool isNone();
//		{
//			return pyobj == Py_None;
//		}

	// Boolean
		bool isBoolean();
//		{
//			return pyobj==Py_True || pyobj==Py_False;
//		}

		jboolean asBoolean();
//		{
//			return (pyobj==Py_True)?true:false;
//		}

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
		bool isJavaObject() const
		{
			return isInstance(JPPyni::m_PythonJavaObject);
		}

		JPObject* asJavaObject();

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

		JPClass* getWrapperClass();
		jvalue getWrapperValue();

};

#endif
