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

HostRef* convertObjectToHostRef(jobject obj)
{
	if (obj == NULL)
		return JPEnv::getHost()->getNone();

	jvalue v;
	v.l = obj;
	
	jclass cls = JPJni::getClass(v.l);
	JPType* type = JPTypeManager::findClass(cls);
	return type->asHostObject(v);
}


HostRef* JPClass::getStaticValue(jclass c, jfieldID fid) 
{
	TRACE_IN("JPClass::getStaticValue");
	JPLocalFrame frame;

	jobject r = JPEnv::getJava()->GetStaticObjectField(c, fid);
	return convertObjectToHostRef(r);

	TRACE_OUT;
}

HostRef* JPClass::getInstanceValue(jobject c, jfieldID fid) 
{
	TRACE_IN("JPClass::getInstanceValue");
	JPLocalFrame frame;
	jobject r = JPEnv::getJava()->GetObjectField(c, fid);
	return convertObjectToHostRef(r);

	TRACE_OUT;
}

HostRef* JPClass::invokeStatic(jclass claz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invokeStatic");
	JPLocalFrame frame;
	
	jobject res = JPEnv::getJava()->CallStaticObjectMethodA(claz, mth, val);
	return convertObjectToHostRef(res);

	TRACE_OUT;
}

HostRef* JPClass::invoke(jobject claz, jclass clazz, jmethodID mth, jvalue* val)
{
	TRACE_IN("JPClass::invoke");
	JPLocalFrame frame;

	// Call method
	jobject res = JPEnv::getJava()->CallNonvirtualObjectMethodA(claz, clazz, mth, val);
	return convertObjectToHostRef(res);
	
	TRACE_OUT;
}

void JPClass::setStaticValue(jclass c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPClass::setStaticValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetStaticObjectField(c, fid, val);
	TRACE_OUT;
}

void JPClass::setInstanceValue(jobject c, jfieldID fid, HostRef* obj) 
{
	TRACE_IN("JPClass::setInstanceValue");
	JPLocalFrame frame;

	jobject val = convertToJava(obj).l;

	JPEnv::getJava()->SetObjectField(c, fid, val);
	TRACE_OUT;
}

jarray JPClass::newArrayInstance(int sz)
{
	return JPEnv::getJava()->NewObjectArray(sz, getNativeClass(), NULL);
}

vector<HostRef*> JPClass::getArrayRange(jarray a, int start, int length)
{
	jobjectArray array = (jobjectArray)a;	
	JPLocalFrame frame;
	
	vector<HostRef*> res;
	
	for (int i = 0; i < length; i++)
	{
		res.push_back(convertObjectToHostRef( JPEnv::getJava()->GetObjectArrayElement(array, i+start) ));
	}
	
	return res;  
}

void JPClass::setArrayRange(jarray a, int start, int length, vector<HostRef*>& vals)
{
	JPLocalFrame frame(8+length);
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v;
	for (int i = 0; i < length; i++)
	{
		HostRef* pv = vals[i];
		
		v = convertToJava(pv);

		JPEnv::getJava()->SetObjectArrayElement(array, i+start, v.l);
	}
}

void JPClass::setArrayItem(jarray a, int ndx, HostRef* val)
{
	JPLocalFrame frame;
	jobjectArray array = (jobjectArray)a;	
	
	jvalue v = convertToJava(val);
	
	JPEnv::getJava()->SetObjectArrayElement(array, ndx, v.l);		
}

HostRef* JPClass::getArrayItem(jarray a, int ndx)
{
	JPLocalFrame frame;
	TRACE_IN("JPClass::getArrayItem");
	jobjectArray array = (jobjectArray)a;	
	
	jobject obj = JPEnv::getJava()->GetObjectArrayElement(array, ndx);
	return convertObjectToHostRef(obj);
	TRACE_OUT;
}

jobject JPClass::convertToJavaObject(HostRef* obj)
{
	jvalue v = convertToJava(obj);
	return v.l;
}

HostRef* JPClass::asHostObjectFromObject(jvalue val)
{
	return asHostObject(val);
}

HostRef* JPClass::convertToDirectBuffer(HostRef* src)
{
	RAISE(JPypeException, "Unable to convert to Direct Buffer");
}

bool JPClass::isSubTypeOf(const JPType& other) const
{
	const JPClass* otherObjectType = dynamic_cast<const JPClass*>(&other);
	if (!otherObjectType)
	{
		return false;
	}
	JPLocalFrame frame;
	jclass ourClass = getNativeClass();
	jclass otherClass = otherObjectType->getNativeClass();
	// IsAssignableFrom is a jni method and the order of parameters is counterintuitive
	bool otherIsSuperType = JPEnv::getJava()->IsAssignableFrom(ourClass, otherClass);
	//std::cout << other.getName().getSimpleName() << " isSuperType of " << getName().getSimpleName() << " " << otherIsSuperType << std::endl;
	return otherIsSuperType;
}

PyObject* JPClass::getArrayRangeToSequence(jarray, int start, int length)
{
	RAISE(JPypeException, "not impled for void*");
}
	
void JPClass::setArrayRange(jarray, int start, int len, PyObject*) 
{
	RAISE(JPypeException, "not impled for void*");
}



//-------------------------------------------------------------------------------

JPStringType::JPStringType():
	JPClass(JPJni::s_StringClass)
{
}

JPStringType::~JPStringType()
{}

HostRef* JPStringType::asHostObject(jvalue val) 
{
	TRACE_IN("JPStringType::asHostObject");
	
	if (val.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}
	
	jstring v = (jstring)val.l;

	if (JPEnv::getJava()->getConvertStringObjects())
	{
		TRACE1(" Performing conversion");
		jsize len = JPEnv::getJava()->GetStringLength(v);

		jboolean isCopy;
		const jchar* str = JPEnv::getJava()->GetStringChars(v, &isCopy);

		HostRef* res = JPEnv::getHost()->newStringFromUnicode(str, len);
		
		JPEnv::getJava()->ReleaseStringChars(v, str);

		return res;
	}
	else
	{
		TRACE1(" Performing wrapping");
		HostRef* res = JPEnv::getHost()->newStringWrapper(v);
		TRACE1(" Wrapping successfull");
		return res;
	}
	TRACE_OUT;
}

EMatchType JPStringType::canConvertToJava(HostRef* obj)
{
	JPLocalFrame frame;

	if (obj == NULL || JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isString(obj))
	{
		return _exact;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPTypeName name = JPEnv::getHost()->getWrapperTypeName(obj);

		if (name.getType() == JPTypeName::_string)
		{
			return _exact;
		}
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			return _exact;
		}
	}
	return _none;
}

jvalue JPStringType::convertToJava(HostRef* obj)
{
	TRACE_IN("JPStringType::convertToJava");
	jvalue v;
	
	if (JPEnv::getHost()->isNone(obj))
	{
		v.l = NULL;
		return v;
	}
	
	if (JPEnv::getHost()->isWrapper(obj))
	{
		return JPEnv::getHost()->getWrapperValue(obj);
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);

		JPClass* oc = o->getClass();
		if (oc->getName().getSimpleName() == "java.lang.String")
		{
			v.l = o->getObject(); 
			return v;
		}
	}

	JCharString wstr = JPEnv::getHost()->stringAsJCharString(obj);

	jchar* jstr = new jchar[wstr.length()+1];
	jstr[wstr.length()] = 0;
	for (size_t i = 0; i < wstr.length(); i++)
	{
		jstr[i] = (jchar)wstr[i];  
	}
	jstring res = JPEnv::getJava()->NewString(jstr, (jint)wstr.length());
	delete[] jstr;
	
	v.l = res;
	
	return v;
	TRACE_OUT;
}

