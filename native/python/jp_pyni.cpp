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
#include <jpype.h>

#define PY_CHECK(op) op; {if (PyErr_Occured()) throw PythonException();}};


PyObject* JPPyni::m_PythonJavaObject = NULL;
PyObject* JPPyni::m_PythonJavaClass = NULL;
PyObject* JPPyni::m_ProxyClass = NULL;
PyObject* JPPyni::m_GetClassMethod = NULL;
PyObject* JPPyni::m_JavaArrayClass = NULL;
PyObject* JPPyni::m_StringWrapperClass = NULL;
PyObject* JPPyni::m_GetArrayClassMethod = NULL;

//=====================================================================
// JPyObject
Py_ssize_t JPyObject::length(PyObject* obj) 
{
	PY_CHECK( Py_ssize_t res = PyObject_Length(pyobj) );
	return res;
}

void JPyObject::incref()
{
	Py_XINCREF(pyobj);
}

void JPyObject::decref()
{
	Py_XDECREF(pyobj);
}

bool JPyObject::hasAttr(PyObject* k)
{
	PY_CHECK( int res = PyObject_HasAttr(pyobj, k) );
	if (res) 
		return true;
	return false;
}

PyObject* JPyObject::getAttr(PyObject* k)
{
	PY_CHECK( PyObject* res = PyObject_GetAttr(pyobj, k) );
	return res;
}

JPyObject JPyObject::getAttrString(const char* k)
{
	PY_CHECK( PyObject* res = PyObject_GetAttrString(pyobj, (char*)k) );
	return JPyObject(res);
}

void JPyObject::setAttrString(const char* k, PyObject *v)
{
	PY_CHECK( PyObject_SetAttrString(pyobj, (char*)k, v ) );
}

JPyObject JPyObject::call(PyObject* a, PyObject* w)
{
	PY_CHECK( PyObject* res = PyObject_Call(pyobj, a, w) );
	return JPyObject(res);
}

bool JPyObject::isInstance(PyObject* t) const
{
	PY_CHECK( int res = PyObject_IsInstance(pyobj, t) );
	return res != 0;
}

bool JPyObject::isSubclass(PyObject* t) const
{
	int res = PyObject_IsSubclass(pyobj, t);
	return res != 0;
}

bool JPyObject::isMemoryView() const
{
	PY_CHECK( int res = PyMemoryView_Check(pyobj) );
	return res != 0;
}

void JPyObject::AsPtrAndSize(char **buffer, Py_ssize_t *length)
{
		TRACE_IN("JPyObject::AsPtrAndSize");
		PY_CHECK( Py_buffer* py_buf = PyMemoryView_GET_BUFFER(pyobj) );
		buffer = (char*)py_buf->buf;
		length = py_buf->len;
		TRACE_OUT;
}

//=====================================================================
// JPyType
bool JPyType::check(PyObject* obj)
{
	return PyType_Check(obj);
}

bool JPyType::isSubclass(PyObject* o2)
{
	return PyType_IsSubtype((PyTypeObject*)pyobj, (PyTypeObject*)o2);
}


//=====================================================================
// JPyCleaner
JPyCleaner::JPyCleaner()
{
}

JPyCleaner::~JPyCleaner()
{
	for (vector<PyObject*>::iterator iter=refs.begin(); iter!=refs.end(); ++iter)
		Py_XDECREF(*iter);
}


//=====================================================================
// JPyInt

PyObject* JPyInt::fromLong(long l)
{
	PY_CHECK( PyObject* res = PyInt_FromLong(l) );
	return res; 
}

bool JPyInt::check(PyObject* obj)
{
#if PY_MAJOR_VERSION >= 3 || LONG_MAX > 2147483647
		return false;
#else
		return PyInt_Check(pyobj);
#endif
}

jint JPyInt::asInt()
{
	return PyInt_AsLong(pyobj);
}

//=====================================================================
// JPLong

PyObject* JPyLong::fromLong(jlong l)
{
	PY_CHECK( PyObject* res = PyLong_FromLongLong(l) );
	return res; 
}

bool JPyLong::check(PyObject* obj)
{
#if PY_MAJOR_VERSION < 3 || LONG_MAX > 2147483647
		return PyInt_Check(pyobj) || PyLong_Check(pyobj);
#else
		return PyLong_Check(pyobj);
#endif
}

jlong JPyLong::asLong()
{
#if PY_MAJOR_VERSION >= 3
	return PyLong_AsLongLong(pyobj);
#elif LONG_MAX > 2147483647
	return PyInt_Check(pyobj) ? PyInt_asLong(pyobj) : PyLong_asLongLong(pyobj);
#else
	return PyLong_asLongLong(pyobj);
#endif
}

//=====================================================================
// JPyFloat

PyObject* JPyFloat::fromDouble(double l)
{
	PY_CHECK( PyObject* res = PyFloat_FromDouble(l) );
	return res; 
}

bool JPyFloat::check(PyObject* obj)
{
	return PyFloat_Check(obj);
}

double JPyFloat::asDouble()
{
	return PyFloat_AsDouble(pyobj);
}

//=====================================================================
// JPyTuple

JPyTuple JPyTuple::newTuple(Py_ssize_t sz)
{
	PY_CHECK( PyObject* res = PyTuple_New(sz););
	return JPyTuple(res);
}

bool JPyTyple::check(PyObject* obj)
{
	return (PyTuple_Check(obj))?true:false;
}

void JPyTyple::setItem(Py_ssize_t ndx, PyObject* val)
{
	Py_XINCREF(val);
	PY_CHECK( PyTuple_SetItem(pyobj, ndx, val) );
}

PyObject* JPyTyple::getItem(Py_ssize_t ndx) 
{
	PY_CHECK( PyObject* res = PyTuple_GetItem(pyobj, ndx) );
	return res;
}

//=====================================================================
// JPyList

PyObject* JPyList::newList(Py_ssize_t sz)
{
	PY_CHECK( PyObject* res = PyList_New(sz););
	return res;
}

bool JPyList::check(PyObject* obj)
{
	return (PyList_Check(obj))?true:false;
}

void JPyList::setItem(Py_ssize_t ndx, PyObject* val)
{
	Py_XINCREF(val);
	PY_CHECK( PyList_SetItem(pyobj, ndx, val) );
}

PyObject* JPyList::getItem(Py_ssize_t ndx) 
{
	PY_CHECK( PyObject* res = PyList_GetItem(pyobj, ndx) );
	return res;
}

//=====================================================================
// JPySequence

bool JPSequenceList::check(PyObject* obj)
{
	return (PySequence_Check(obj))?true:false;
}

void JPyList::setItem(Py_ssize_t ndx, PyObject* val)
{
	Py_XINCREF(val);
	PY_CHECK( PySequence_SetItem(pyobj, ndx, val) );
}

PyObject* JPyList::getItem(Py_ssize_t ndx) 
{
	PY_CHECK( PyObject* res = PySequence_GetItem(pyobj, ndx) );
	return res;
}

/=====================================================================

bool JPyString::check(PyObject* obj)
{
	return PyBytes_Check(obj) || PyUnicode_Check(obj);
}

bool JPyString::checkStrict(PyObject* obj)
{
	return PyBytes_Check(obj);
}

bool JPyString::checkUnicode(PyObject* obj)
{
	return PyUnicode_Check(obj);
}

Py_UNICODE* JPyString::asUnicode()
{
	return PyUnicode_AsUnicode(pyobj);
}

string JPyString::asString() 
{	
	TRACE_IN("JPyString::asString");
#if PY_MAJOR_VERSION < 3
	PY_CHECK( string res = string(PyBytes_AsString(pyobj)) );
#else
	PyObject* val;
	bool needs_decref = false;
	if(PyUnicode_Check(obj)) {
		 val = PyUnicode_AsEncodedString(pyobj, "UTF-8", "strict");
		 needs_decref = true;
	} else {
		val = pyobj;
	}

	PY_CHECK( string res = string(PyBytes_AsString(val)) );

	if(needs_decref) {
		Py_DECREF(val);
	}
#endif
	return res;
	TRACE_OUT;
}

JCharString JPyString::asJCharString() 
{	
	PyObject* torelease = NULL;
	TRACE_IN("JPyString::asJCharString");
	
	if (PyBytes_Check(pyobj))
	{
		PY_CHECK( pyobj = PyUnicode_FromObject(pyobj) );	
		torelease = pyobj;
	}

	Py_UNICODE* val = PyUnicode_AS_UNICODE(pyobj);	
	Py_ssize_t length = JPyObject::length(pyobj);
	JCharString res(length);
	for (int i = 0; val[i] != 0; i++)
	{
		res[i] = (jchar)val[i];
	}

	if (torelease != NULL)
	{
		Py_DECREF(torelease);
	}

	return res;
	TRACE_OUT;
}

JPyObject JPyString::fromUnicode(const char* str, int len) 
{
	Py_UNICODE* value = new Py_UNICODE[len+1];
	value[len] = 0;
	for (int i = 0; i < len; i++)
	{
		value[i] = (Py_UNICODE)str[i];
	}
	PY_CHECK( PyObject* obj = PyUnicode_FromUnicode(value, len) );
	delete[] value;
	return JPyObject(obj);
}

JPyObject JPyString::fromString(const char* str) 
{
#if PY_MAJOR_VERSION < 3
	PY_CHECK( PyObject* obj = PyString_FromString(str) );
	return JPyObject(obj);
#else
	PY_CHECK( PyObject* bytes = PyBytes_FromString(str) );
	PY_CHECK( PyObject* unicode = PyUnicode_FromEncodedObject(bytes, "UTF-8", "strict") );
	Py_DECREF(bytes);
	return JPyObject(unicode);
#endif
}

void JPyString::getRawByteString(char** outBuffer, long& outSize)
{
	Py_ssize_t tempSize = 0;
	PY_CHECK( Py_ssize_t res = PyBytes_AsStringAndSize(pyobj, buffer, tempSize) );
	outSize = (long)tempSize;
}

void JPyString::getByteBufferPtr(char** outBuffer, long& outSize)
{
	Py_ssize_t tempSize = 0;
	JPyObject::AsPtrAndSize(pyobj, outBuffer, &tempSize);
	outSize = (long)tempSize;
}


//=====================================================================
// JPyErr
void JPyErr::setString(const char* str)
{
	PyErr_SetString(pyobj, str);
}

void JPyErr::setObject(PyObject* str)
{
	PyErr_SetObject(pyobj, str);
}

void JPyErr::clearError();
{
	PyErr_Clear();
}

//=====================================================================
// JPyDict

bool JPyDict::contains(PyObject* k)
{
	PY_CHECK( int res = PyMapping_HasKey(pyobj, k) );
	if (res) 
		return true;
	return false;
}

PyObject* JPyDict::getItem(PyObject* k)
{
	PY_CHECK( PyObject* res = PyDict_GetItem(pyobj, k) );
	Py_XINCREF(res); // FIXME Seems fishy
	return res;
}

bool JPyDict::check(PyObject* obj)
{
	return PyDict_Check(obj);
}

PyObject* JPyDict::getKeys()
{
	PY_CHECK( PyObject* res = PyDict_Keys(pyobj) );
	return res;
}

PyObject* JPyDict::copy(PyObject* m)
{
	PY_CHECK( PyObject* res = PyDict_Copy(pyobj) );
	return res;
}

JPyDict JPyDict::newInstance()
{
	PY_CHECK( PyObject* res = PyDict_New() );
	return PyDict(res);
}

void JPyDict::setItemString(PyObject* d, PyObject* o, const char* n)
{
	PY_CHECK( PyDict_SetItemString(d, n, o) );
}


// =====================================================
// PythonException

PythonException::PythonException()
{
	JPyCleaner cleaner;
	TRACE_IN("PythonException::PythonException");
	PyObject* traceback;
	PyErr_Fetch(&m_ExceptionClass, &m_ExceptionValue, &traceback);
	Py_INCREF(m_ExceptionClass);
	Py_XINCREF(m_ExceptionValue);

	JPyObject name = cleaner.add(JPyObject(m_ExceptionClass).getAttrString("__name__"));
	string ascname = JPyString(name).asString();
	TRACE1(ascname);
	TRACE1(m_ExceptionValue->ob_type->tp_name);

	PyErr_Restore(m_ExceptionClass, m_ExceptionValue, traceback);
	TRACE_OUT;
}

PythonException::PythonException(const PythonException& ex)
{
	m_ExceptionClass = ex.m_ExceptionClass;
	Py_INCREF(m_ExceptionClass);
	m_ExceptionValue = ex.m_ExceptionValue;
	Py_INCREF(m_ExceptionValue);
}

PythonException::~PythonException()
{
	Py_XDECREF(m_ExceptionClass);
	Py_XDECREF(m_ExceptionValue);
}

PyObject* PythonException::getJavaException()
{
	PyObject* retVal = NULL;

	// If the exception was caught further down ...
	if (JPySequence::check(m_ExceptionValue) && JPyObject::length(m_ExceptionValue) == 1)
	{
		PyObject* v0 = JPySequence::getItem(m_ExceptionValue, 0);
		if (JPySequence::check(v0) && JPyObject::length(v0) == 2)
		{
			PyObject* v00 = JPySequence::getItem(v0, 0);
			PyObject* v01 = JPySequence::getItem(v0, 1);

			if (v00 == hostEnv->getSpecialConstructorKey())
			{
				retVal = v01;
			}
			else
			{
				Py_DECREF(v01);
			}

			Py_DECREF(v00);
		}
		else
		{
			Py_DECREF(v0);
		}
	}
	else
	{
		Py_XINCREF(m_ExceptionValue);
		retVal = m_ExceptionValue;
	}
	return retVal;
}

string PythonException::getMessage()
{
     string message = "";

     // Exception class name
     PyObject* className = JPyObject::getAttrString(m_ExceptionClass, "__name__");
     message += JPyString::asString(className);
     Py_DECREF(className);

     // Exception value
     if(m_ExceptionValue)
     {
          // Convert the exception value to string
          PyObject* pyStrValue = PyObject_Str(m_ExceptionValue);
          if(pyStrValue)
          {
               message += ": " + JPyString::asString(pyStrValue);
               Py_DECREF(pyStrValue);
          }
     }

     return message;
}

// =============================================================
// Capsule

PyObject* JPyCapsule::fromVoid(void* data, PyCapsule_Destructor destr)
{
	PY_CHECK( PyObject* res = PyCapsule_New(data, (char *)NULL, destr) );
	return res;
}

PyObject* JPyCapsule::fromVoidAndDesc(void* data, const char* desc, PyCapsule_Destructor destr)
{
	PY_CHECK( PyObject* res = PyCapsule_New(data, desc, destr) );
	return res;
}

void* JPyCapsule::asVoidPtr(PyObject* obj)
{
	PY_CHECK( void* res = PyCapsule_GetPointer(obj, PyCapsule_GetName(obj)) );
	return res;
}

void* JPyCapsule::getDesc(PyObject* obj)
{
	PY_CHECK( void* res = (void*)PyCapsule_GetName(obj) );
	return res;
}

bool JPyCapsule::check(PyObject* obj)
{
	return PyCapsule_CheckExact(obj);
}

//void JPyHelper::dumpSequenceRefs(PyObject* seq, const char* comment)
//{
//	cerr << "Dumping sequence state at " << comment << endl;
//	cerr << "   sequence has " << (long)seq->ob_refcnt << " reference(s)" << endl;
//	Py_ssize_t dx = PySequence_Length(seq);
//	for (Py_ssize_t i = 0; i < dx; i++)
//	{
//		PyObject* el = PySequence_GetItem(seq, i);
//		Py_XDECREF(el); // PySequence_GetItem return a new ref
//		cerr << "   item[" << (long)i << "] has " << (long)el->ob_refcnt << " references" << endl;
//	}
//}


// ======================================================================
// JPyAdaptor
bool JPyAdaptor::isNone()
{
	return pyobj == Py_None;
}

bool JPyAdaptor::isBoolean()
{
	return pyobj==Py_True || pyobj==Py_False;
}

jboolean JPyAdaptor::asBoolean()
{
	return (pyobj==Py_True)?true:false;
}

JPObject* JPyAdaptor::asJavaObject()
{
	JPyCleaner cleaner;
	if (JPyCObject::check(pyobj))
	{
		return (JPObject*)JPyCObject::asVoidPtr(pyobj);
	}
	PyObject* javaObject = cleaner.add(JPyObject::getAttrString(pyobj, "__javaobject__"));
	JPObject* res = (JPObject*)JPyCObject::asVoidPtr(javaObject);
	return res;
}

JPClass* JPyAdaptor::asJavaClass()
{
	JPyCleaner cleaner;
	PyObject* claz = cleaner.add(getAttrString("__javaclass__"));
	PyJPClass* res = (PyJPClass*)claz;
	return res->m_Class;
}

JPClass* JPyAdaptor::getWrapperClass()
{
	return asJavaClass();
//	PyObject* pyTName = JPyObject::getAttrString(pyobj, "typeName");
//	string tname = JPyString::asString(pyTName);
//	Py_DECREF(pyTName);
//	return JPTypeManager::findClass(JPJni::findClass(tname));
}

jvalue JPyAdaptor::getWrapperValue()
{
	JPyCleaner cleaner;
	JPClass* name = getWrapperClass(obj);
	PyObject* value = cleaner.add(getAttrString("__javavalue__"));
	jvalue* v = (jvalue*)JPyCObject::asVoidPtr(value);
	Py_DECREF(value);

	if (name->isObjectType())
	{
		jvalue res;
		res.l = JPEnv::getJava()->NewLocalRef(v->l); 
		return res;
	}
	return *v;
}

JPProxy* JPAdaptor::asProxy()
{
	JPyCleaner cleaner;
	PyObject* jproxy = cleaner.add(getAttrString("__javaproxy__"));
  return (JPProxy*)JPyCObject::asVoidPtr(jproxy);
}

JPArray* JPyAdaptor::asArray()
{
	JPyCleaner cleaner;
	PyObject* javaObject = cleaner.add(JPyObject::getAttrString(pyobj, "__javaobject__"));	
	return (JPArray*)JPyCObject::asVoidPtr(javaObject);
}

//========================================================
// JPPyni

JPyCapsule JPyCapsule::fromMethod(JPMethod* m)
{
	// Methods are owned by the JPObjectClass*
	return JPyCapule::fromVoidAndDesc(m, "JPMethod", NULL);
}

static void deleteJPArrayDestructor(CAPSULE_DESTRUCTOR_ARG_TYPE data)
{
  delete (JPArray*)CAPSULE_EXTRACT(data);
}

JPyCapsule JPyCapsule::fromArray(JPArray* v)
{
	return JPyCObject::fromVoidAndDesc(v, "JPArray", PythonHostEnvironment::deleteJPArrayDestructor);
}

JPyObject JPPyni::newClass(JPObjectClass* m)
{
	JPyCleaner cleaner;
	PyTuple args = cleaner.add(JPyTuple::newTuple(1));
	PyJPClass* co = cleaner.add(PyJPClass::alloc(m));
	args.setItem(0, (PyObject*)co);
	return JPyObject(m_GetClassMethod).call(args, NULL);
}

JPyObject JPPyni::newArrayClass(JPArrayClass* m)
{
	JPyCleaner cleaner;
	PyTuple args = cleaner.add(JPyTuple::newTuple(1));
	PyObject* cname = cleaner.add(JPyString::fromString(m->getSimpleName().c_str()));
	args.setItem(0, cname);
	return JPyObject(m_GetArrayClassMethod).call(args, NULL);
}

JPyObject JPPyni::newArray(JPArray* m)
{
	JPyCleaner cleaner;
	JPArrayClass* jc = m->getClass();

	// Get the array class 
	JPyTuple args = cleaner.add(JPyTuple::newTuple(1));
	JPyObject pyClass = cleaner.add(newArrayClass(jc));

	// Call python ctor
	PyObject* joHolder = cleaner.add(JPyCapule::fromVoidAndDesc((void*)m, "JPArray", &deleteJPArrayDestructor));
	args = cleaner.add(JPyTuple::newTuple(2));
	args.setItem(0, m_SpecialConstructorKey);
	args.setItem(1, joHolder);
	return pyClass.call(args, NULL);
}

JPyObject JPPyni::getCallableFrom(PyObject* ref, string& name)
{
	JPyCleaner cleaner;
	PyObject* pname = cleaner.add(JPyString::fromString(name.c_str()));
	PyObject* mname = cleaner.add(JPyString::fromString("getCallable"));
	PyObject* callable = PyObject_CallMethodObjArgs(ref, mname, pname, NULL);
	JPyErr::check();
	return callable;
}

//bool PythonHostEnvironment::isString(Host_Ref* ref)
//{
//	return JPyString::check(UNWRAP(ref));
//}

//jsize PythonHostEnvironment::getStringLength(Host_Ref* ref)
//{
//	return (jsize)JPyObject::length(UNWRAP(ref));
//}

//string PythonHostEnvironment::stringAsString(Host_Ref* ref)
//{
//	return JPyString::asString(UNWRAP(ref));
//}

//JCharString PythonHostEnvironment::stringAsJCharString(Host_Ref* ref)
//{
//	return JPyString::asJCharString(UNWRAP(ref));
//}

void JPPyni::printError()
{
	PyErr_Print();
	PyErr_Clear();
}

bool JPPyni::isJavaException(HostException* ex)
{
	PythonException* pe = (PythonException*)ex;
	return JPyObject::isSubclass(pe->m_ExceptionClass, m_JavaExceptionClass);
}

JPyObject* JPPyni::getJavaException(HostException* ex)
{
	PythonException* pe = (PythonException*)ex;
	PyObject* obj = pe->getJavaException();
	return JPyObject::getAttrString(obj, "__javaobject__");
}

//void PythonHostEnvironment::getRawUnicodeString(Host_Ref* obj, jchar** outBuffer, long& outSize)
//{
//	PyObject* objRef = UNWRAP(obj);
//	outSize = (long)JPyObject::length(objRef);
//	*outBuffer = (jchar*)JPyString::AsUnicode(objRef);
//}

//size_t PythonHostEnvironment::getUnicodeSize()
//{
//	return sizeof(Py_UNICODE);
//}

JPyObject JPPyni::newStringWrapper(jstring jstr)
{
	JPyCleaner cleaner;
	TRACE_IN("JPPyni::newStringWrapper");
	jvalue* v = new jvalue;
	v->l = JPEnv::getJava()->NewGlobalRef(jstr);

	// Create a new jvalue wrapper
	PyObject* value = cleaner.add(JPyCapule::fromVoidAndDesc((void*)v, "object jvalue", deleteObjectJValueDestructor));

	// Set up arguments
	JPyTuple args = cleaner.add(JPyTuple::newTuple(1));
	args.setItem(0, Py_None);

	// Call a python method
	PY_CHECK( PyObject* res = cleaner.add(PyObject_Call(m_StringWrapperClass, args, Py_None)) );

	// Push into resource
	PY_CHECK( PyObject_SetAttrString(res, "_value", cleaner.keep(value) ));

	// Return the resource
	return JPyObject(cleaner.keep(res));
	TRACE_OUT;
}

JPyObject JPPyni::newObject(JPObject* obj)
{
	JPyCleaner cleaner;
	TRACE_IN("JPPyni::newObject");
	TRACE2("classname", obj->getClass()->getSimpleName());
	JPObjectClass* jc = obj->getClass();

	// Convert to a capsule
	JPyObject pyClass = cleaner.add(newClass(jc));

	// Call the python constructor
	JPyTuple args = cleaner.add(JPyTuple::newTuple(2));
	JPyTuple arg2 = cleaner.add(JPyTuple::newTuple(1));
	arg2.setItem(0, args);
	PyObject* joHolder = cleaner.add(JPyCapsule::fromVoidAndDesc((void*)obj, "JPObject", &deleteJPObjectDestructor));
	args.setItem(0, m_SpecialConstructorKey);
	args.setItem(1, joHolder);
	return JPyObject::call(pyClass, arg2, NULL);
	TRACE_OUT;
}

void* JPPyni::prepareCallbackBegin()
{
	PyGILState_STATE state = PyGILState_Ensure();;
	return (void*)new PyGILState_STATE(state);
}

void JPPyni::prepareCallbackFinish(void* state)
{
	PyGILState_STATE* state2 = (PyGILState_STATE*)state;
	PyGILState_Release(*state2);
	delete state2;
}

