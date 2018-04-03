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

PyObject* JPDoubleType::asHostObject(jvalue val) 
{
	return JPyFloat::fromFloat(val.d);
}

PyObject* JPDoubleType::asHostObjectFromObject(jobject val)
{
	jdouble v = JPJni::doubleValue(val);
	return JPyFloat::fromDouble(v);
} 

EMatchType JPDoubleType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_double))
	{
		return _exact;
	}

	if (obj.isFloat())
	{
	  if (obj.isJavaValue())
		{
			return _implicit;
		}
		return _exact;
	}

	// Java allows conversion to any type with a longer range even if lossy
	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPDoubleType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;
	if (checkValue(obj, JPTypeManager::_double))
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
		res.d = JPyFloat(obj).asDouble();
	}
	return res;
}

jarray JPDoubleType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewDoubleArray(sz);
}

PyObject* JPDoubleType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.d = JPEnv::getJava()->GetStaticDoubleField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPDoubleType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.d = JPEnv::getJava()->GetDoubleField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPDoubleType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.d = JPEnv::getJava()->CallStaticDoubleMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPDoubleType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.d = JPEnv::getJava()->CallNonvirtualDoubleMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPDoubleType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jdouble val = convertToJava(obj).d;
    JPEnv::getJava()->SetStaticDoubleField(clazz->getNativeClass(), fid, val);
}

void JPDoubleType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jdouble val = convertToJava(obj).d;
    JPEnv::getJava()->SetDoubleField(c, fid, val);
}

vector<PyObject*> JPDoubleType::getArrayRange(jarray a, int start, int length)
{
    jdoubleArray array = (jdoubleArray)a;    
    jdouble* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetDoubleArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.d = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseDoubleArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseDoubleArrayElements(array, val, JNI_ABORT); } );
}

void JPDoubleType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jdoubleArray array = (jdoubleArray)a;
    jdouble* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetDoubleArrayElements(array, &isCopy);

        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).f;
        }
        JPEnv::getJava()->ReleaseDoubleArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseDoubleArrayElements(array, val, JNI_ABORT); })
}

void JPDoubleType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jdoubleArray, jdouble>(a, start, length, sequence,
            &JPJavaEnv::SetDoubleArrayRegion))
        return;

    jdoubleArray array = (jdoubleArray)a;
    vector<jdouble> val;
    val.resize(length);
    // fill temporary array
    for (Py_ssize_t i = 0; i < length; ++i) {
        PyObject* o = PySequence_GetItem(sequence, i);
        jdouble d = (jdouble) PyFloat_AsDouble(o);
        Py_DecRef(o);
        if (d == -1.) { CONVERSION_ERROR_HANDLE; }
        val[i] = d;
    }

    // set java array
    try {
        JPEnv::getJava()->SetDoubleArrayRegion(array, start, length, &val.front());
    } RETHROW_CATCH();
}

PyObject* JPDoubleType::getArrayItem(jarray a, int ndx)
{
    jdoubleArray array = (jdoubleArray)a;
    jdouble val;
    
    try {
        JPEnv::getJava()->GetDoubleArrayRegion(array,ndx, 1, &val);
        jvalue v;
        v.d = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPDoubleType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jdoubleArray array = (jdoubleArray)a;
    jdouble val;
    
    try {
        val = convertToJava(obj).d;
        JPEnv::getJava()->SetDoubleArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPDoubleType::getArrayRangeToSequence(jarray a, int lo, int hi) {
    return getSlice<jdouble>(a, lo, hi, NPY_FLOAT64, PyFloat_FromDouble);
}

