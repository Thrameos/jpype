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
#include <jpype_python.h>

PyObject* JPPyni::m_GetClassMethod = NULL;
PyObject* JPPyni::m_GetArrayClassMethod = NULL;
PyObject* JPPyni::m_GetJavaArrayClassMethod = NULL;
PyObject* JPPyni::m_JavaArrayClass = NULL;
PyObject* JPPyni::m_JavaExceptionClass = NULL;
PyObject* JPPyni::m_ProxyClass = NULL;
PyObject* JPPyni::m_PythonJavaObject = NULL;
PyObject* JPPyni::m_PythonJavaClass = NULL;
PyObject* JPPyni::m_SpecialConstructorKey = NULL;
PyObject* JPPyni::m_StringWrapperClass = NULL;
PyObject* JPPyni::m_WrapperClass = NULL;

//=====================================================================
// JPyObject
jlong JPyObject::length() 
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

bool JPyObject::isNone() const
{
  return pyobj == Py_None;
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

PyObject* JPyObject::getAttrString(const char* k)
{
  PY_CHECK( PyObject* res = PyObject_GetAttrString(pyobj, (char*)k) );
  return res;
}

void JPyObject::setAttrString(const char* k, PyObject *v)
{
  PY_CHECK( PyObject_SetAttrString(pyobj, (char*)k, v ) );
}

PyObject* JPyObject::call(PyObject* a, PyObject* w)
{
  PY_CHECK( PyObject* res = PyObject_Call(pyobj, a, w) );
  return res;
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

//=====================================================================
// JPyBool

bool JPyBool::check(PyObject* obj)    
{
  return PyBool_Check(obj);
}

JPyBool JPyBool::fromLong(jlong value)
{
  return PyBool_FromLong(value?1:0);
}

bool JPyBool::isTrue()
{
  return pyobj==Py_True;
}

bool JPyBool::isFalse()
{
  return pyobj==Py_False;
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
//
PyObject* JPyInt::fromInt(jint l)
{
#if PY_MAJOR_VERSION >= 3 
  PY_CHECK( PyObject* res = PyLong_FromLong(l) );
#else
  PY_CHECK( PyObject* res = PyInt_FromLong(l) );
#endif
  return res; 
}

PyObject* JPyInt::fromLong(jlong l)
{
#if PY_MAJOR_VERSION >= 3 
  PY_CHECK( PyObject* res = PyLong_FromLongLong(l) );
#else
  PY_CHECK( PyObject* res = PyInt_FromLong(l) );
#endif
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
  PY_CHECK( jint res = PyInt_AsLong(pyobj) );
  return res;
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
    return PyInt_Check(obj) || PyLong_Check(obj);
#else
    return PyLong_Check(obj);
#endif
}

jlong JPyLong::asLong()
{
  jlong res;
#if PY_MAJOR_VERSION >= 3
  PY_CHECK( res = PyLong_AsLongLong(pyobj) );
#elif LONG_MAX > 2147483647
  PY_CHECK( res = PyInt_Check(pyobj) ? PyInt_asLong(pyobj) : PyLong_asLongLong(pyobj) );
#else
  PY_CHECK( res = PyLong_asLongLong(pyobj); );
#endif
  return res;
}

//=====================================================================
// JPyFloat
PyObject* JPyFloat::fromFloat(jfloat l)
{
  PY_CHECK( PyObject* res = PyFloat_FromDouble(l) );
  return res; 
}

PyObject* JPyFloat::fromDouble(jdouble l)
{
  PY_CHECK( PyObject* res = PyFloat_FromDouble(l) );
  return res; 
}

bool JPyFloat::check(PyObject* obj)
{
  return PyFloat_Check(obj);
}

jdouble JPyFloat::asDouble()
{
  PY_CHECK( jdouble res = PyFloat_AsDouble(pyobj) );
  return res;
}

//=====================================================================
// JPyTuple

JPyTuple JPyTuple::newTuple(jlong sz)
{
  PY_CHECK( PyObject* res = PyTuple_New(sz););
  return JPyTuple(res);
}

bool JPyTuple::check(PyObject* obj)
{
  return (PyTuple_Check(obj))?true:false;
}

void JPyTuple::setItem(jlong ndx, PyObject* val)
{
  Py_XINCREF(val);
  PY_CHECK( PyTuple_SetItem(pyobj, ndx, val) );
}

PyObject* JPyTuple::getItem(jlong ndx) 
{
  PY_CHECK( PyObject* res = PyTuple_GetItem(pyobj, ndx) );
  return res;
}

jlong JPyTuple::size() 
{
  PY_CHECK( jlong res = PyTuple_Size(pyobj) );
  return res;
}

//=====================================================================
// JPyList

JPyList JPyList::newList(jlong sz)
{
  PY_CHECK( PyObject* res = PyList_New(sz););
  return res;
}

bool JPyList::check(PyObject* obj)
{
  return (PyList_Check(obj))?true:false;
}

void JPyList::setItem(jlong ndx, PyObject* val)
{
  Py_XINCREF(val);
  PY_CHECK( PyList_SetItem(pyobj, ndx, val) );
}

PyObject* JPyList::getItem(jlong ndx) 
{
  PY_CHECK( PyObject* res = PyList_GetItem(pyobj, ndx) );
  return res;
}

//=====================================================================
// JPySequence


bool JPySequence::check(PyObject* obj)
{
  return (PySequence_Check(obj))?true:false;
}

jlong JPySequence::size()
{
  return PySequence_Size(pyobj);
}

void JPySequence::setItem(jlong ndx, PyObject* val)
{
  Py_XINCREF(val);
  PY_CHECK( PySequence_SetItem(pyobj, ndx, val) );
}

PyObject* JPySequence::getItem(jlong ndx) 
{
  PY_CHECK( PyObject* res = PySequence_GetItem(pyobj, ndx) );
  return res;
}

//=====================================================================

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

string JPyString::asString() 
{  
  TRACE_IN("JPyString::asString");
#if PY_MAJOR_VERSION < 3
  PY_CHECK( string res = string(PyBytes_AsString(pyobj)) );
#else
  PyObject* val;
  bool needs_decref = false;
  if(PyUnicode_Check(pyobj)) {
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
  Py_ssize_t len = length();
  JCharString res(len);
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

JPyObject JPyString::fromUnicode(const jchar* str, int len) 
{
  Py_UNICODE* value = new Py_UNICODE[len+1];
  // FIXME the encoding for jni is not the same as standard.  Need special handling for 4 byte codes
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

void JPyString::getRawByteString(char** buffer, jlong& outSize)
{
  Py_ssize_t tempSize = 0;
  PY_CHECK( PyBytes_AsStringAndSize(pyobj, buffer, &tempSize) );
  outSize = (long)tempSize;
}

void JPyString::getRawUnicodeString(jchar** outBuffer, jlong& outSize)
{
  // FIXME jni uses a different encoding than is standard, thus we may need conversion here.
  outSize = length();
  *outBuffer = (jchar*)PyUnicode_AsUnicode(pyobj);
}

jlong JPyString::getUnicodeSize()
{
  return sizeof(Py_UNICODE);
}

bool JPyString::isChar()
{
#if PY_MAJOR_VERSION < 3
  if (PyUnicode_Check(pyobj))
    return PyUnicode_GetSize(pyobj)==1;
  if (PyString_Check(pyobj))
    return PyString_Size(pyobj)==1;
#else
  if (PyUnicode_Check(pyobj))
    return PyUnicode_GET_LENGTH(pyobj)==1;
  if (PyBytes_Check(pyobj))
    return PyBytes_Size(pyobj)==1;
#endif
  return false;
}

jchar JPyString::asChar()
{
#if PY_MAJOR_VERSION < 3
  if (PyString_Check(pyobj))
    return PyBytes_AsAtring(pyobj)[0];
  if (PyUnicode_Chec:k(pyobj))
  {
    wchar_t buffer;
    PyUnicode_AsWideChar(pyobj, &buffer, 1);
    return buffer;
  }
#else
  if (PyBytes_Check(pyobj))
    return PyBytes_AsString(pyobj)[0];
  if (PyUnicode_Check(pyobj))
  {
    Py_UCS4 value = PyUnicode_ReadChar(pyobj, 0);
    if (value>0xffff)
    {
      PyErr_SetString(PyExc_ValueError, "Unable to pack 4 byte unicode into java char");
      JPyErr::raise("unicode");
    }
    return value;
  }
#endif
  JPyErr::setRuntimeError("error converting string to char");
  JPyErr::raise("asChar");
  return 0;
}


//=====================================================================
// JPyMemoryView

bool JPyMemoryView::check(PyObject* obj) 
{
  PY_CHECK( int res = PyMemoryView_Check(obj) );
  return res != 0;
}

void JPyMemoryView::getByteBufferPtr(char** buffer, jlong& length)
{
    PY_CHECK( Py_buffer* py_buf = PyMemoryView_GET_BUFFER(pyobj) );
    *buffer = (char*)py_buf->buf;
    length = py_buf->len;
}


//=====================================================================
// JPyErr
void JPyErr::setString(PyObject* type, const char* str)
{
  PyErr_SetString(type, str);
}

void JPyErr::setObject(PyObject* type, PyObject* str)
{
  PyErr_SetObject(type, str);
}

void JPyErr::clear()
{
  PyErr_Clear();
}

void JPyErr::setRuntimeError(const char* msg)
{
  PyErr_SetString( PyExc_RuntimeError, msg);
}

void JPyErr::setAttributeError(const char* msg)
{
  PyErr_SetString( PyExc_AttributeError, msg);
}

void JPyErr::setTypeError(const char* msg)
{
  PyErr_SetString( PyExc_TypeError, msg);
}

void JPyErr::raise(const char* msg)
{
  throw PythonException();
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
  return JPyDict(res);
}

void JPyDict::setItemString( PyObject* o, const char* n)
{
  PY_CHECK( PyDict_SetItemString(pyobj, n, o) );
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
  if (JPySequence::check(m_ExceptionValue) && JPySequence(m_ExceptionValue).size() == 1)
  {
    PyObject* v0 = JPySequence(m_ExceptionValue).getItem(0);
    if (JPySequence::check(v0) && JPySequence(v0).size() == 2)
    {
      PyObject* v00 = JPySequence(v0).getItem(0);
      PyObject* v01 = JPySequence(v0).getItem(1);

      if (v00 == JPPyni::m_SpecialConstructorKey)
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
   PyObject* className = JPyObject(m_ExceptionClass).getAttrString("__name__");
   message += JPyString(className).asString();
   Py_DECREF(className);

   // Exception value
   if(m_ExceptionValue)
   {
      // Convert the exception value to string
      PyObject* pyStrValue = PyObject_Str(m_ExceptionValue);
      if(pyStrValue)
      {
         message += ": " + JPyString(pyStrValue).asString();
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

void* JPyCapsule::asVoidPtr()
{
  PY_CHECK( void* res = PyCapsule_GetPointer(pyobj, PyCapsule_GetName(pyobj)) );
  return res;
}

const char* JPyCapsule::getName()
{
  PY_CHECK( const char* res = (const char*)PyCapsule_GetName(pyobj) );
  return res;
}

bool JPyCapsule::check(PyObject* obj)
{
  return PyCapsule_CheckExact(obj);
}

//void JPyHelper::dumpSequenceRefs(PyObject* seq, const char* comment)
//{
//  cerr << "Dumping sequence state at " << comment << endl;
//  cerr << "   sequence has " << (long)seq->ob_refcnt << " reference(s)" << endl;
//  Py_ssize_t dx = PySequence_Length(seq);
//  for (Py_ssize_t i = 0; i < dx; i++)
//  {
//    PyObject* el = PySequence_GetItem(seq, i);
//    Py_XDECREF(el); // PySequence_GetItem return a new ref
//    cerr << "   item[" << (long)i << "] has " << (long)el->ob_refcnt << " references" << endl;
//  }
//}


// ======================================================================
// JPyAdaptor

// This accepts either the capsule or the python object
bool JPyAdaptor::isJavaValue() const
{
	return PyObject_HasAttrString(pyobj, "__javavalue__");
}

const JPValue& JPyAdaptor::asJavaValue()
{
  JPyCleaner cleaner;
  PyObject* javaObject = cleaner.add(getAttrString("__javavalue__"));
	if (!PyJPValue::check(javaObject))
	{
		JPyErr::setRuntimeError("invalid __javavalue__");
		JPyErr::raise("asJavaValue");
	}
  return PyJPValue::getValue(javaObject);
}

JPClass* JPyAdaptor::asJavaClass()
{
  JPyCleaner cleaner;
  PyObject* claz = cleaner.add(getAttrString("__javaclass__"));
 	if (!PyJPClass::check(claz))
	{
		JPyErr::setRuntimeError("invalid __javaclass__");
		JPyErr::raise("asJavaClass");
	}
  return ((PyJPClass*)claz)->m_Class;
}

JPProxy* JPyAdaptor::asProxy()
{
  JPyCleaner cleaner;
  PyObject* jproxy = cleaner.add(getAttrString("__javaproxy__"));
  return (JPProxy*)JPyCapsule(jproxy).asVoidPtr();
}

JPArray* JPyAdaptor::asArray()
{
  JPyCleaner cleaner;
  PyObject* javaObject = cleaner.add(getAttrString("__javaobject__"));  
  return ((PyJPArray*)javaObject)->m_Object;
}

//========================================================
// JPPyni

PyObject* JPPyni::getNone()
{
  Py_RETURN_NONE;
}

PyObject* JPPyni::getTrue()
{
  Py_RETURN_TRUE;
}

PyObject* JPPyni::getFalse()
{
  Py_RETURN_FALSE;
}

void JPPyni::assertInitialized()
{
  if (! JPEnv::isInitialized())
  {
    JPyErr::setRuntimeError("Java Subsystem not started");
    JPyErr::raise("assert");
  }
}

JPyObject JPPyni::newArrayClass(JPArrayClass* m)
{
  JPyCleaner cleaner;
  JPyTuple args = cleaner.add(JPyTuple::newTuple(1));
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
  PyObject* joHolder = (PyObject*)cleaner.add(PyJPArray::alloc(m));
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
  PY_CHECK( PyObject* callable = PyObject_CallMethodObjArgs(ref, mname, pname, NULL); )
  return callable;
}

void JPPyni::printError()
{
  PyErr_Print();
  PyErr_Clear();
}

//bool JPPyni::isJavaException(PythonException* ex)
//{
//  return JPyObject(ex->m_ExceptionClass).isSubclass(m_JavaExceptionClass);
//}

//PyObject* JPPyni::getJavaException(PythonException* ex)
//{
//  PyObject* obj = ex->getJavaException();
//  return JPyObject(obj).getAttrString("__javaobject__");
//}

PyObject* JPPyni::newStringWrapper(jstring jstr)
{
  JPyCleaner cleaner;
  TRACE_IN("JPPyni::newStringWrapper");
  jvalue v;
  v.l = JPEnv::getJava()->NewGlobalRef(jstr);

  // Create a new jvalue wrapper
  PyObject* value = cleaner.add(PyJPValue::alloc(JPTypeManager::_java_lang_String, v));

  // Set up arguments
  JPyTuple args = cleaner.add(JPyTuple::newTuple(1));
  args.setItem(0, Py_None);

  // Call a python method
  PyObject* res;
  PY_CHECK( res = cleaner.add(PyObject_Call(m_StringWrapperClass, args, Py_None)) );

  // Push into resource
  PY_CHECK( PyObject_SetAttrString(res, "__javavalue__", cleaner.keep(value) ));

  // Return the resource
  return cleaner.keep(res);
  TRACE_OUT;
}

JPyObject JPPyni::newClass(JPObjectClass* m)
{
  JPyCleaner cleaner;
  // Allocated a new module PyJPClass
  PyObject* co = cleaner.add((PyObject*)PyJPClass::alloc(m));

  // call jpype._jclass._getClassFor()
  JPyTuple args = cleaner.add(JPyTuple::newTuple(1));
  args.setItem(0, co);
  return JPyObject(m_GetClassMethod).call(args, NULL);
}

PyObject* JPPyni::newObject(const JPValue& value)
{
	JPObjectClass* cls = (JPObjectClass*)value.getClass();
	jvalue v = value.getValue();
  JPyCleaner cleaner;
  TRACE_IN("JPPyni::newObject");
  TRACE2("classname", cls->getSimpleName());

  // Convert to a capsule
  JPyObject pyClass = cleaner.add(newClass(cls));
  PyObject* joHolder = cleaner.add((PyObject*)PyJPValue::alloc(cls, v));

  // Call the python class constructor
  JPyTuple args = cleaner.add(JPyTuple::newTuple(2));
  args.setItem(0, m_SpecialConstructorKey);
  args.setItem(1, joHolder);

  JPyTuple arg2 = cleaner.add(JPyTuple::newTuple(1));
  arg2.setItem(0, args);
  return pyClass.call((PyObject*)arg2, NULL);
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

void* JPPyni::gotoExternal()
{  
  PyThreadState *_save; 
  _save = PyEval_SaveThread();
  return (void*)_save;
}

void JPPyni::returnExternal(void* state)
{
  PyThreadState *_save = (PyThreadState *)state;
  PyEval_RestoreThread(_save);
}

// Convert a java exception to python
static void errorOccurred()
{
  TRACE_IN("PyJPModule::errorOccurred");
  JPLocalFrame frame(8);
  JPyCleaner cleaner;

  // Get the throwable (object instance)
  jthrowable th = JPEnv::getJava()->ExceptionOccurred();
	jvalue val;
	val.l = th;

  // Tell java we have it covered
  JPEnv::getJava()->ExceptionClear();

  // Find the class for the throwable
  jclass ec = JPJni::getClass(th);
  JPObjectClass* jpclass = dynamic_cast<JPObjectClass*>(JPTypeManager::findClass(ec));
  if (jpclass==NULL)
  {
    JPyErr::setRuntimeError("Unable to find java exception type");
  }

  // Create an exception object 
  JPyObject jexclass = cleaner.add(JPPyni::newClass(jpclass));

  // Convert the throwable to a python object
  PyObject* pyth = cleaner.add(JPPyni::newObject(JPValue(jpclass, val)));

  // Arguments to construct the python instance
  JPyTuple args = cleaner.add(JPyTuple::newTuple(2));
  args.setItem( 0, JPPyni::m_SpecialConstructorKey);
  args.setItem( 1, pyth);

  JPyTuple arg2 = cleaner.add(JPyTuple::newTuple(1));
  arg2.setItem( 0, args);

  // Tell python about it
  PyObject* pyexclass = cleaner.add(jexclass.getAttrString("PYEXC"));
  JPyErr::setObject(pyexclass, arg2);
  TRACE_OUT;
}


void JPPyni::handleCatch()
{
  try 
  {
    throw;
  }
  catch(JavaException& ex) 
  { 
    try { 
      errorOccurred(); 
    } 
    catch(...) 
    { 
      JPyErr::setRuntimeError("An unknown error occured while handling a Java Exception");
    }
  }
  catch(JPypeException& ex)
  {
    try { 
      JPyErr::setRuntimeError(ex.getMsg()); 
    } 
    catch(...) 
    { 
      JPyErr::setRuntimeError("An unknown error occured while handling a JPype Exception");
    }
  }
  catch(PythonException& ex)
  {
    // Python error is already set up
  }
  catch(...)
  {
    JPyErr::setRuntimeError("Unknown Exception");
  }
} 

