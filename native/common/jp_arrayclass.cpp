/*****************************************************************************
   Copyright 2004 Steve M�nard

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



JPArrayClass::JPArrayClass(jclass c) :
	JPClass(c)
{
	m_ComponentType = JPTypeManager::findClass(JPJni::getComponentType(c));
}

JPArrayClass::~JPArrayClass()
{
}

bool JPArrayClass::isObjectType() const
{
	return true;
}

EMatchType JPArrayClass::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	TRACE_IN("JPArrayClass::canConvertToJava");
	JPLocalFrame frame;
	
	if (obj.isNone())
	{
		return _implicit;
	}
	
	if (obj.isArray())
	{
		TRACE1("array");
		JPArray* a = obj.asArray();
		
		JPArrayClass* ca = a->getClass();
		
		if (ca == this)
		{
			return _exact;
		}
		
		if (JPEnv::getJava()->IsAssignableFrom(ca->getNativeClass(), getNativeClass()))
		{
			return _implicit;
		}
	}
	else if (obj.isUnicodeString() && m_ComponentType==JPTypeManager::_char)
	{
		TRACE1("char[]");
		// Strings are also char[]
		return _implicit;
	}
	else if (obj.isByteString() && m_ComponentType==JPTypeManager::_byte)
	{
		TRACE1("char[]");
		// Strings are also char[]
		return _implicit;
	}
	else if (obj.isSequence() && !obj.isJavaObject())
	{
		JPyCleaner cleaner;
		TRACE1("Sequence");
		EMatchType match = _implicit;
		JPySequence sequence(obj);
		int length = sequence.size();
		for (int i = 0; i < length && match > _none; i++)
		{
			PyObject* pyobj = cleaner.add(sequence.getItem(i));
			EMatchType newMatch = m_ComponentType->canConvertToJava(pyobj);
			if (newMatch < match)
			{
				match = newMatch;
			}
		}
		return match;
	}
	
	return _none;
	TRACE_OUT;
}

PyObject* JPArrayClass::asHostObject(jvalue val)
{
	TRACE_IN("JPArrayClass::asHostObject")
	if (val.l == NULL)
	{
		return JPPyni::getNone();
	}
	return JPPyni::newArray(new JPArray(this, (jarray)val.l));
	TRACE_OUT;
}

jvalue JPArrayClass::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	TRACE_IN("JPArrayClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;
	res.l = NULL;
	
	if (obj.isArray())
	{
		TRACE1("direct");
		JPArray* a = obj.asArray();
		res = a->getValue();		
	}
	else if (obj.isByteString() && m_ComponentType==JPTypeManager::_byte && sizeof(char) == sizeof(jbyte))
	{
		TRACE1("char[]");
		char* rawData;
		long size;
		JPyString(obj).getRawByteString(&rawData, size);
		
		jbyteArray array = JPEnv::getJava()->NewByteArray(size);

		jboolean isCopy;
		jbyte* contents = JPEnv::getJava()->GetByteArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jbyte));
		JPEnv::getJava()->ReleaseByteArrayElements(array, contents, 0);
		
		res.l = array;
	}
	else if (obj.isUnicodeString() && m_ComponentType==JPTypeManager::_char && JPyString::getUnicodeSize() == sizeof(jchar))
	{
		TRACE1("uchar[]");
		jchar* rawData;
		long size;
		JPyString(obj).getRawUnicodeString(&rawData, size);
		
		jcharArray array = JPEnv::getJava()->NewCharArray(size);

		jboolean isCopy;
		jchar* contents = JPEnv::getJava()->GetCharArrayElements(array, &isCopy);
		memcpy(contents, rawData, size*sizeof(jchar));
		JPEnv::getJava()->ReleaseCharArrayElements(array, contents, 0);
		
		res.l = array;
	}
	else if (obj.isSequence())
	{
		JPyCleaner cleaner;
		TRACE1("sequence");
		JPySequence sequence(obj);
		int length = sequence.size();
		
		jarray array = m_ComponentType->newArrayInstance(length);
		
		for (int i = 0; i < length ; i++)
		{
			PyObject* obj2 = cleaner.add(sequence.getItem(i));
			m_ComponentType->setArrayItem(array, i, obj2);
		}
		res.l = array;
	}

	res.l = frame.keep(res.l);
	return res;
	TRACE_OUT;
}

jvalue JPArrayClass::convertToJavaVector(vector<PyObject*>& refs, size_t start, size_t end)
{
	JPLocalFrame frame;
	TRACE_IN("JPArrayClass::convertToJavaVector");
	size_t length = end-start;
	// FIXME java used jint for jsize, which means we have signed 32 to unsigned 64 issues.

	jarray array = m_ComponentType->newArrayInstance((int)length);
	jvalue res;
		
	for (size_t i = start; i < end ; i++)
	{
		m_ComponentType->setArrayItem(array, (int)(i-start), refs[i]);
	}
	res.l = frame.keep(array);
	return res;
	TRACE_OUT;
}

JPArray* JPArrayClass::newInstance(int length)
{	
	JPLocalFrame frame;
	jarray array = m_ComponentType->newArrayInstance(length);
	return  new JPArray(this, array);
}

