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

PyObject* JPBooleanType::asHostObject(jvalue val) 
{
	if (val.z)
	{
		return JPPyni::getTrue();
	}
	return JPPyni::getFalse();
}

PyObject* JPBooleanType::asHostObjectFromObject(jobject val)
{
	if (JPJni::booleanValue(val))
	{
		return JPPyni::getTrue();
	}
	return JPPyni::getFalse();
} 

EMatchType JPBooleanType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (checkValue(obj, JPTypeManager::_boolean))
	{
		return _exact;
	}

	if (obj.isInt() || obj.isLong())
	{
		return _implicit;
	}

	// FIXME what about isTrue and isFalse? Those should be exact

	return _none;
}

jvalue JPBooleanType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);
	jvalue res;
	if (checkValue(obj, JPTypeManager::_boolean))
	{
		return obj.asJavaValue();
	}
	else if (obj.isLong())
	{
		res.z = (jboolean)JPyLong(obj).asLong();
	}
	else
	{
		res.z = (jboolean)JPyInt(obj).asInt();
	}
	return res;
}

jarray JPBooleanType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewBooleanArray(sz);
}

PyObject* JPBooleanType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.z = JPEnv::getJava()->GetStaticBooleanField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPBooleanType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.z = JPEnv::getJava()->GetBooleanField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPBooleanType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.z = JPEnv::getJava()->CallStaticBooleanMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPBooleanType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.z = JPEnv::getJava()->CallNonvirtualBooleanMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPBooleanType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jboolean val = convertToJava(obj).z;
    JPEnv::getJava()->SetStaticBooleanField(clazz->getNativeClass(), fid, val);
}

void JPBooleanType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jboolean val = convertToJava(obj).z;
    JPEnv::getJava()->SetBooleanField(c, fid, val);
}

vector<PyObject*> JPBooleanType::getArrayRange(jarray a, int start, int length)
{
    jbooleanArray array = (jbooleanArray)a;    
    jboolean* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetBooleanArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.z = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, JNI_ABORT); } );
}

void JPBooleanType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jbooleanArray array = (jbooleanArray)a;
    jboolean* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetBooleanArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).z;
        }
        JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, JNI_ABORT); } );
}

void JPBooleanType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jbooleanArray, jboolean>(a, start, length, sequence,
            &JPJavaEnv::SetBooleanArrayRegion))
        return;

    jbooleanArray array = (jbooleanArray) a;
    jboolean isCopy;
    jboolean* val = NULL;
    long c;

    try {
        val = JPEnv::getJava()->GetBooleanArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            c = PyInt_AsLong(o);
            Py_DecRef(o);
            if(c == -1) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = (jboolean) c;
        }
        JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseBooleanArrayElements(array, val, JNI_ABORT); } );

}

PyObject* JPBooleanType::getArrayItem(jarray a, int ndx)
{
    jbooleanArray array = (jbooleanArray)a;
    jboolean val;
    
    try {
        JPEnv::getJava()->GetBooleanArrayRegion(array, ndx, 1, &val);
        jvalue v;
        v.z = val;
        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPBooleanType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jbooleanArray array = (jbooleanArray)a;
    
    try {
        jboolean val = convertToJava(obj).z;
        JPEnv::getJava()->SetBooleanArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPBooleanType::getArrayRangeToSequence(jarray a, int start, int length) {
    return getSlice<jboolean>(a, start, length, NPY_BOOL, PyBool_FromLong);
}

