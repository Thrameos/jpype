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

PyObject* JPIntType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.i);
}

PyObject* JPIntType::asHostObjectFromObject(jobject val)
{
	long v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPIntType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_int))
	{
		return _exact;
	}

	if (obj.isInt())
	{
		if (obj.isJavaValue())
		{
			return _implicit;
		}
		return _exact;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPIntType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_int))
	{
		return obj.asJavaValue();
	}
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPyErr::setTypeError("Cannot convert value to Java int");
			JPyErr::raise("JPIntType::convertToJava");
		}

		res.i = (jint)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();;
		if (l < JPJni::s_minInt || l > JPJni::s_maxInt)
		{
			JPyErr::setTypeError("Cannot convert value to Java int");
			JPyErr::raise("JPIntType::convertToJava");
		}
		res.i = (jint)l;
	}
	return res;
}

jarray JPIntType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewIntArray(sz);
}

PyObject* JPIntType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.i = JPEnv::getJava()->GetStaticIntField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPIntType::getInstanceValue(jobject obj, jfieldID fid) 
{
    jvalue v;
    v.i = JPEnv::getJava()->GetIntField(obj, fid);
    
    return asHostObject(v);
}

PyObject* JPIntType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.i = JPEnv::getJava()->CallStaticIntMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPIntType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.i = JPEnv::getJava()->CallNonvirtualIntMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPIntType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jint val = convertToJava(obj).i;
    JPEnv::getJava()->SetStaticIntField(clazz->getNativeClass(), fid, val);
}

void JPIntType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jint val = convertToJava(obj).i;
    JPEnv::getJava()->SetIntField(c, fid, val);
}

vector<PyObject*> JPIntType::getArrayRange(jarray a, int start, int length)
{
    jintArray array = (jintArray)a;
    jint* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetIntArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.i = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseIntArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseIntArrayElements(array, val, JNI_ABORT); } );
}

void JPIntType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jintArray array = (jintArray)a;    
    jint* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetIntArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).i;            
        }
        JPEnv::getJava()->ReleaseIntArrayElements(array, val, 0);        
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseIntArrayElements(array, val, JNI_ABORT); } );
}

void JPIntType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jintArray, jint>(a, start, length, sequence,
            &JPJavaEnv::SetIntArrayRegion))
        return;

    jintArray array = (jintArray)a;
    jint* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetIntArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            jint v = (jint) PyInt_AsLong(o);
            Py_DecRef(o);
            if (v == -1) { CONVERSION_ERROR_HANDLE }
            val[start+i] = v;
        }
        JPEnv::getJava()->ReleaseIntArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseIntArrayElements(array, val, JNI_ABORT); } );
}

PyObject* JPIntType::getArrayItem(jarray a, int ndx)
{
    jintArray array = (jintArray)a;    
    jint val;
    
    try {
        JPEnv::getJava()->GetIntArrayRegion(array, ndx, 1, &val);
        jvalue v;
        v.i = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPIntType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jintArray array = (jintArray)a;
    
    try {
        jint val = convertToJava(obj).i;
        JPEnv::getJava()->SetIntArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPIntType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jint>(a, lo, hi, NPY_INT, PyInt_FromLong);
}

