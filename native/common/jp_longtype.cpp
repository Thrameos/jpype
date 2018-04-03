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

PyObject* JPLongType::asHostObject(jvalue val) 
{
	TRACE_IN("JPLongType::asHostObject");
	return JPyLong::fromLong(val.j);
	TRACE_OUT;
}

PyObject* JPLongType::asHostObjectFromObject(jobject val)
{
	jlong v = JPJni::longValue(val);
	return JPyLong::fromLong(v);
} 

EMatchType JPLongType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_long))
	{
		return _exact;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		if (obj.isJavaValue())
		{
			return _implicit;
		}
		return _exact;
	}

	return _none;
}

jvalue JPLongType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_long))
	{
		return obj.asJavaValue();
	}
	if (obj.isInt())
	{
		res.j = (jlong)JPyInt(obj).asInt();
	}
	else if (obj.isLong())
	{
		res.j = (jlong)JPyLong(obj).asLong();
	}
	else
	{
		JPyErr::setTypeError("Cannot convert value to Java long");
		JPyErr::raise("JPLongType::convertToJava");
		res.j = 0; // never reached
	}
	return res;
}

jarray JPLongType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewLongArray(sz);
}

PyObject* JPLongType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.j = JPEnv::getJava()->GetStaticLongField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPLongType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.j = JPEnv::getJava()->GetLongField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPLongType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.j = JPEnv::getJava()->CallStaticLongMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPLongType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.j = JPEnv::getJava()->CallNonvirtualLongMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPLongType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jlong val = convertToJava(obj).j;
    JPEnv::getJava()->SetStaticLongField(clazz->getNativeClass(), fid, val);
}

void JPLongType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jlong val = convertToJava(obj).j;
    JPEnv::getJava()->SetLongField(c, fid, val);
}

vector<PyObject*> JPLongType::getArrayRange(jarray a, int start, int length)
{
    jlongArray array = (jlongArray)a;    
    jlong* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetLongArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.j = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseLongArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseLongArrayElements(array, val, JNI_ABORT); } );
}

void JPLongType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jlongArray array = (jlongArray)a;    
    jlong* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetLongArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).j;            
        }
        JPEnv::getJava()->ReleaseLongArrayElements(array, val, 0);        
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseLongArrayElements(array, val, JNI_ABORT); } );
}

void JPLongType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jlongArray, jlong>(a, start, length, sequence,
            &JPJavaEnv::SetLongArrayRegion))
        return;

    jlongArray array = (jlongArray)a;
    jlong* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetLongArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            jlong l = (jlong) PyLong_AsLong(o);
            Py_DECREF(o);
            if(l == -1) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = l;
        }
        JPEnv::getJava()->ReleaseLongArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseLongArrayElements(array, val, JNI_ABORT); } );
}



PyObject* JPLongType::getArrayItem(jarray a, int ndx)
{
    jlongArray array = (jlongArray)a;
    jlong val;
    
    try {
        JPEnv::getJava()->GetLongArrayRegion(array, ndx, 1, &val);
        jvalue v;
        v.j = val;
        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPLongType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jlongArray array = (jlongArray)a;
    jlong val;
    
    try {
        val = convertToJava(obj).j;
        JPEnv::getJava()->SetLongArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPLongType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jlong>(a, lo, hi, NPY_INT64, PyLong_FromLong);
}


//----------------------------------------------------------
