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
#include <jp_primitive_utils.h>

PyObject* JPByteType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.b);
}

PyObject* JPByteType::asHostObjectFromObject(jobject val)
{
	jint v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPByteType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	if (checkValue(obj, JPTypeManager::_byte))
	{
		return _exact;
	}

	return _none;
}

jvalue JPByteType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_byte))
	{
		return obj.asJavaValue();
	}
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPyErr::setTypeError("Cannot convert value to Java byte");
			JPyErr::raise("JPByteType::convertToJava");
		}
		res.b = (jbyte)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();
		if (l < JPJni::s_minByte || l > JPJni::s_maxByte)
		{
			JPyErr::setTypeError("Cannot convert value to Java byte");
			JPyErr::raise("JPByteType::convertToJava");
		}
		res.b = (jbyte)l;
	}
	return res;
}

PyObject* JPByteType::convertToDirectBuffer(PyObject* pysrc)
{
	JPyObject src(pysrc);
	JPLocalFrame frame;
	TRACE_IN("JPByteType::convertToDirectBuffer");
	if (src.isByteBuffer())
	{

		char* rawData;
		jlong size;
		JPyMemoryView(pysrc).getByteBufferPtr(&rawData, size);

		jobject obj = JPEnv::getJava()->NewDirectByteBuffer(rawData, size);
		JPReference::registerRef(obj, pysrc);

		jvalue v;
		v.l = obj;
		jclass cls = JPJni::getClass(obj);
		JPClass* type = JPTypeManager::findClass(cls);
		return type->asHostObject(v);
	}

	RAISE(JPypeException, "Unable to convert to Direct Buffer");
	TRACE_OUT;
}

jarray JPByteType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewByteArray(sz);
}

PyObject* JPByteType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.b = JPEnv::getJava()->GetStaticByteField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPByteType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.b = JPEnv::getJava()->GetByteField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPByteType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.b = JPEnv::getJava()->CallStaticByteMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPByteType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.b = JPEnv::getJava()->CallNonvirtualByteMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPByteType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jbyte val = convertToJava(obj).b;
    JPEnv::getJava()->SetStaticByteField(clazz->getNativeClass(), fid, val);
}

void JPByteType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jbyte val = convertToJava(obj).b;
    JPEnv::getJava()->SetByteField(c, fid, val);
}

vector<PyObject*> JPByteType::getArrayRange(jarray a, int start, int length)
{
    jbyteArray array = (jbyteArray)a;
    jbyte* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.b = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseByteArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseByteArrayElements(array, val, JNI_ABORT); } );
}

void JPByteType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jbyteArray array = (jbyteArray)a;
    jbyte* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).b;            
        }
        JPEnv::getJava()->ReleaseByteArrayElements(array, val, 0);        
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseByteArrayElements(array, val, JNI_ABORT); } );
}

void JPByteType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jbyteArray, jbyte>(a, start, length, sequence,
            &JPJavaEnv::SetByteArrayRegion))
        return;

    jbyteArray array = (jbyteArray)a;
    jbyte* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            jbyte l = (jbyte) PyInt_AS_LONG(o);
            Py_DECREF(o);
            if(l == -1) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = l;
        }
        JPEnv::getJava()->ReleaseByteArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseByteArrayElements(array, val, JNI_ABORT); } );
}

PyObject* JPByteType::getArrayItem(jarray a, int ndx)
{
    jbyteArray array = (jbyteArray)a;
    jbyte val;
    
    try {
        jvalue v;
        JPEnv::getJava()->GetByteArrayRegion(array, ndx, 1, &val);
        v.b = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPByteType::setArrayItem(jarray a, int ndx, PyObject* obj)
{
    jbyteArray array = (jbyteArray)a;
    
    try {
        jbyte val = convertToJava(obj).b;
        JPEnv::getJava()->SetByteArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPByteType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jbyte>(a, lo, hi, NPY_BYTE, PyInt_FromLong);
}


