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
#include <jp_primitive_util.h>

PyObject* JPFloatType::asHostObject(jvalue val) 
{
	return JPyFloat::fromFloat(val.f);
}

PyObject* JPFloatType::asHostObjectFromObject(jobject val)
{
	// FIXME this is odd
	jdouble v = JPJni::doubleValue(val);
	return JPyFloat::fromDouble(v);
} 

EMatchType JPFloatType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_float))
	{
		return _exact;
	}

	if (obj.isFloat())
	{
		if (obj.isJavaValue())
		{
			return _implicit;
		}
		return _implicit;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPFloatType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_float))
	{
		return obj.asJavaValue();
	}
	else if (obj.isInt())
	{
		res.d = JPyInt(obj).asInt();
	}
	else if (obj.isLong())
	{
		res.d = JPyLong(obj).asLong();
	}
	else
	{
		jdouble l = JPyFloat(obj).asDouble();
		if (l > 0 && (l < JPJni::s_minFloat || l > JPJni::s_maxFloat))
		{
			JPyErr::setTypeError("Cannot convert value to Java float");
			JPyErr::raise("JPFloatType::convertToJava");
		}
		else if (l < 0 && (l > -JPJni::s_minFloat || l < -JPJni::s_maxFloat))
		{
			JPyErr::setTypeError("Cannot convert value to Java float");
			JPyErr::raise("JPFloatType::convertToJava");
		}
		res.f = (jfloat)l;
	}
	return res;
}

jarray JPFloatType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewFloatArray(sz);
}

PyObject* JPFloatType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.f = JPEnv::getJava()->GetStaticFloatField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPFloatType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.f = JPEnv::getJava()->GetFloatField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPFloatType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.f = JPEnv::getJava()->CallStaticFloatMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPFloatType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.f = JPEnv::getJava()->CallNonvirtualFloatMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPFloatType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jfloat val = convertToJava(obj).f;
    JPEnv::getJava()->SetStaticFloatField(clazz->getNativeClass(), fid, val);
}

void JPFloatType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jfloat val = convertToJava(obj).f;
    JPEnv::getJava()->SetFloatField(c, fid, val);
}

vector<PyObject*> JPFloatType::getArrayRange(jarray a, int start, int length)
{
    jfloatArray array = (jfloatArray)a;    
    jfloat* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetFloatArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.f = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseFloatArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseFloatArrayElements(array, val, JNI_ABORT); } );
}

void JPFloatType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jfloatArray array = (jfloatArray)a;    
    jfloat* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetFloatArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).f;            
        }
        JPEnv::getJava()->ReleaseFloatArrayElements(array, val, 0);        
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseFloatArrayElements(array, val, JNI_ABORT); } );
}

void JPFloatType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jfloatArray, jfloat>(a, start, length, sequence,
            &JPJavaEnv::SetFloatArrayRegion))
        return;

    jfloatArray array = (jfloatArray)a;
    jfloat* val = NULL;
    jboolean isCopy;
    try {
        val = JPEnv::getJava()->GetFloatArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            jfloat v = (jfloat) PyFloat_AsDouble(o);
            Py_DecRef(o);
            if (v == -1.) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = v;
        }
        JPEnv::getJava()->ReleaseFloatArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseFloatArrayElements(array, val, JNI_ABORT); } );
}

PyObject* JPFloatType::getArrayItem(jarray a, int ndx)
{
    jfloatArray array = (jfloatArray)a;
    jfloat val;
    
    try {
        JPEnv::getJava()->GetFloatArrayRegion(array, ndx, 1, &val);
        
        jvalue v;
        v.f = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPFloatType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jfloatArray array = (jfloatArray)a;
    jfloat val;
    
    try {
        val = convertToJava(obj).f;
        JPEnv::getJava()->SetFloatArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPFloatType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jfloat>(a, lo, hi, NPY_FLOAT32, PyFloat_FromDouble);
}

