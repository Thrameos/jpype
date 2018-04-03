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

PyObject* JPCharType::asHostObject(jvalue val)   
{
	return JPyString::fromCharUTF16(val.c);
}

PyObject* JPCharType::asHostObjectFromObject(jobject val)
{
	return JPyString::fromCharUTF16(JPJni::charValue(val));
} 

EMatchType JPCharType::canConvertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	if (obj.isNone())
	{
		return _none;
	}

	if (checkValue(obj, JPTypeManager::_char))
	{
		return _exact;
	}

	if (obj.isString() && JPyString(obj).isChar())
	{
		return _implicit;
	}

	return _none;
}

jvalue JPCharType::convertToJava(PyObject* pyobj)
{
	JPyObject obj(pyobj);

	jvalue res;

	if (checkValue(obj, JPTypeManager::_char))
	{
		return obj.asJavaValue();
	}
	else
	{
		res.c = JPyString(obj).asChar();
	}
	return res;
}

jarray JPCharType::newArrayInstance(int sz)
{
    return JPEnv::getJava()->NewCharArray(sz);
}

PyObject* JPCharType::getStaticValue(JPClass* clazz, jfieldID fid) 
{
    jvalue v;
    v.c = JPEnv::getJava()->GetStaticCharField(clazz->getNativeClass(), fid);
    
    return asHostObject(v);
}

PyObject* JPCharType::getInstanceValue(jobject c, jfieldID fid) 
{
    jvalue v;
    v.c = JPEnv::getJava()->GetCharField(c, fid);
    
    return asHostObject(v);
}

PyObject* JPCharType::invokeStatic(JPClass* claz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.c = JPEnv::getJava()->CallStaticCharMethodA(claz->getNativeClass(), mth, val);
    return asHostObject(v);
}

PyObject* JPCharType::invoke(jobject obj, JPClass* clazz, jmethodID mth, jvalue* val)
{
    jvalue v;
    v.c = JPEnv::getJava()->CallNonvirtualCharMethodA(obj, clazz->getNativeClass(), mth, val);
    return asHostObject(v);
}

void JPCharType::setStaticValue(JPClass* clazz, jfieldID fid, PyObject* obj) 
{
    jchar val = convertToJava(obj).c;
    JPEnv::getJava()->SetStaticCharField(clazz->getNativeClass(), fid, val);
}

void JPCharType::setInstanceValue(jobject c, jfieldID fid, PyObject* obj) 
{
    jchar val = convertToJava(obj).c;
    JPEnv::getJava()->SetCharField(c, fid, val);
}

vector<PyObject*> JPCharType::getArrayRange(jarray a, int start, int length)
{
    jcharArray array = (jcharArray)a;    
    jchar* val = NULL;
    jboolean isCopy;
    
    try {
        val = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
        vector<PyObject*> res;
        
        jvalue v;
        for (int i = 0; i < length; i++)
        {
            v.c = val[i+start];
            PyObject* pv = asHostObject(v);
            res.push_back(pv);
        }
        JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT);
        
        return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT); } );
}

void JPCharType::setArrayRange(jarray a, int start, int length, vector<PyObject*>& vals)
{
    jcharArray array = (jcharArray)a;    
    jchar* val = NULL;
    jboolean isCopy;

    try {
        val = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
        
        for (int i = 0; i < length; i++)
        {
            val[start+i] = convertToJava(vals[i]).c;            
        }
        JPEnv::getJava()->ReleaseCharArrayElements(array, val, 0);        
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT); } );
}

void JPCharType::setArrayRange(jarray a, int start, int length, PyObject* sequence)
{
    if (setViaBuffer<jcharArray, jchar>(a, start, length, sequence,
            &JPJavaEnv::SetCharArrayRegion))
        return;

    jcharArray array = (jcharArray)a;
    jchar* val = NULL;
    jboolean isCopy;
    long c;

    try {
        val = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
        for (Py_ssize_t i = 0; i < length; ++i) {
            PyObject* o = PySequence_GetItem(sequence, i);
            c = PyInt_AsLong(o);
            Py_DecRef(o);
            if(c == -1) { CONVERSION_ERROR_HANDLE; }
            val[start+i] = (jchar) c;
        }
        JPEnv::getJava()->ReleaseCharArrayElements(array, val, 0);
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT); } );
}

PyObject* JPCharType::getArrayItem(jarray a, int ndx)
{
    jcharArray array = (jcharArray)a;
    jchar val;

    try {
        JPEnv::getJava()->GetCharArrayRegion(array, ndx, 1, &val);
        jvalue v;
        v.c = val;

        return asHostObject(v);
    }
    RETHROW_CATCH();
}

void JPCharType::setArrayItem(jarray a, int ndx , PyObject* obj)
{
    jcharArray array = (jcharArray)a;
    jchar val;
    
    try {
        val = convertToJava(obj).c;
        JPEnv::getJava()->SetCharArrayRegion(array, ndx, 1, &val);
    }
    RETHROW_CATCH();
}

PyObject* JPCharType::getArrayRangeToSequence(jarray a, int start, int length) {
    jcharArray array = (jcharArray)a;
    jchar* val = NULL;
    jboolean isCopy;
    PyObject* res = NULL;
    try {
       val = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
       if (sizeof(Py_UNICODE) == sizeof(jchar))
       {
           res = PyUnicode_FromUnicode((const Py_UNICODE *) val + start,
                                        length);
       }
       else
       {
           res = PyUnicode_FromUnicode(NULL, length);
           Py_UNICODE *pchars = PyUnicode_AS_UNICODE(res);

           for (Py_ssize_t i = start; i < length; i++)
               pchars[i] = (Py_UNICODE) val[i];
       }

       JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT);
       return res;
    }
    RETHROW_CATCH( if (val != NULL) { JPEnv::getJava()->ReleaseCharArrayElements(array, val, JNI_ABORT); } );
}

//----------------------------------------------------------
