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

PyObject* JPShortType::asHostObject(jvalue val) 
{
	return JPyInt::fromInt(val.s);
}

PyObject* JPShortType::asHostObjectFromObject(jobject val)
{
	jint v = JPJni::intValue(val);
	return JPyInt::fromInt(v);
} 

EMatchType JPShortType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_short))
	{
		return _exact;
	}

	if (obj.isInt())
	{
		return _implicit;
	}

	if (obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPShortType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_short))
	{
		return obj.asJavaValue();
	}
	if (obj.isInt())
	{
		jint l = JPyInt(obj).asInt();;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPyErr::setTypeError("Cannot convert value to Java short");
			JPyErr::raise("JPShortType::convertToJava");
		}

		res.s = (jshort)l;
	}
	else if (obj.isLong())
	{
		jlong l = JPyLong(obj).asLong();;
		if (l < JPJni::s_minShort || l > JPJni::s_maxShort)
		{
			JPyErr::setTypeError("Cannot convert value to Java short");
			JPyErr::raise("JPShortType::convertToJava");
		}
		res.s = (jshort)l;
	}
	return res;
}

jarray JPShortType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewShortArray(sz);
}

PyObject* JPShortType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.s = JPEnv::getJava()->GetStaticShortField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPShortType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.s = JPEnv::getJava()->GetShortField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPShortType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.s = JPEnv::getJava()->CallStaticShortMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPShortType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.s = JPEnv::getJava()->CallNonvirtualShortMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPShortType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jshort val = convertToJava(obj).s;
    JPEnv::getJava()->SetStaticShortField(clazz->getNativeClass(), fid, val);
}

void JPShortType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jshort val = convertToJava(obj).s;
    JPEnv::getJava()->SetShortField(c, fid, val);
}

vector<PyObject*> JPShortType::getArrayRange(jarray a, int start, int length)
{
    jshortArray array = (jshortArray)a;    
    jshort* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetShortArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.s = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseShortArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseShortArrayElements(array, val, JNI_ABORT); } );
}

void JPShortType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jshortArray array = (jshortArray)a;
    jshort* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetShortArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).s;
        }
        JPEnv::getJava()->ReleaseShortArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseShortArrayElements(array, val, JNI_ABORT); } );
}

void JPShortType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jshortArray, jshort>(a, start, length, sequence,
            &JPJavaEnv::SetShortArrayRegion))
        return;

    jshortArray array = (jshortArray)a;
    jshort* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetShortArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            jshort l = (jshort) PyInt_AsLong(o);
            Py_DECREF(o);
            if(l == -1) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = l;
        }
        JPEnv::getJava()->ReleaseShortArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseShortArrayElements(array, val, JNI_ABORT); } );
}

PyObject* JPShortType::getArrayItem(jarray a, int ndx)
{
    jshortArray array = (jshortArray)a;
    jshort val;
    
    try {
        JPEnv::getJava()->GetShortArrayRegion(array, ndx, 1, &val);
        jvalue v;
        v.s = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPShortType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jshortArray array = (jshortArray)a;
    
    try {
        jshort val = convertToJava(obj).s;
        JPEnv::getJava()->SetShortArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPShortType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jshort>(a, lo, hi, NPY_SHORT, PyInt_FromLong);
}

