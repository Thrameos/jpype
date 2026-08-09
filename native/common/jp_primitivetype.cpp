/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include <vector>
#include "jpype.h"
#include "pyjp.h"

JPPrimitiveType::JPPrimitiveType(const string& name)
: JPClass(name, 0x411)
{
}

JPPrimitiveType::~JPPrimitiveType()
= default;

bool JPPrimitiveType::isPrimitive() const
{
	return true;
}

PyObject *JPPrimitiveType::convertLong(PyTypeObject* wrapper, PyLongObject* tmp)
{
	if (wrapper == nullptr)
		JP_RAISE(PyExc_SystemError, "bad wrapper");

	// PyLong_AsLongLong can't fail/overflow here -- tmp always represents a
	// genuine Java primitive (byte/short/int/long), which always fits
	// within 64 bits.
	long long value = PyLong_AsLongLong((PyObject*) tmp);
	return PyJPNumber_longFromLongLong(wrapper, value);
}

JPPyObject JPPrimitiveType::getArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize step, jsize len)
{
	JPPyObject list = JPPyObject::call(PyList_New(len));
	if (len == 0)
		return list;

	Py_ssize_t itemsize = getItemSize();
	char typeCode = getTypeCode();

	// A GetPrimitiveArrayCritical pin held across this whole per-element
	// PyObject-allocation loop is the wrong tool even though it measures
	// no faster than the alternative below (verified: same numbers at
	// every size) -- the loop's duration scales with len and with
	// whatever CPython's own allocator does per element, which is
	// exactly the kind of unbounded hold time the JNI critical-section
	// contract warns against. Prefer a plain Get<Type>ArrayRegion copy
	// into a local buffer, released immediately, then convert from that
	// local memory with no JNI/GC interaction at all during boxing.
	// Region calls have no stride support, so this only applies to
	// step == 1 (an unsliced array, the overwhelmingly common case);
	// step != 1 falls back to the critical-section path below.
	if (step == 1)
	{
		std::vector<char> buf((size_t) itemsize * len);
		copyElements(frame, a, start, len, buf.data(), 0);
		const char *base = buf.data();
		for (jsize i = 0; i < len; ++i)
		{
			const char *src = base + (jlong) i * itemsize;
			jvalue v;
			switch (typeCode)
			{
				case 'Z': v.z = *(const jboolean*) src; break;
				case 'B': v.b = *(const jbyte*) src; break;
				case 'C': v.c = *(const jchar*) src; break;
				case 'S': v.s = *(const jshort*) src; break;
				case 'I': v.i = *(const jint*) src; break;
				case 'J': v.j = *(const jlong*) src; break;
				case 'F': v.f = *(const jfloat*) src; break;
				default: v.d = *(const jdouble*) src; break; // 'D'
			}
			PyList_SET_ITEM(list.get(), i, convertToPythonObject(frame, v, false).keep());
		}
		return list;
	}

	jboolean isCopy;
	void *mem = frame.getEnv()->GetPrimitiveArrayCritical(a, &isCopy);
	JP_TRACE_JAVA("GetPrimitiveArrayCritical", mem);
	const char *base = (const char*) mem;

	for (jsize i = 0; i < len; ++i)
	{
		const char *src = base + (start + (jlong) i * step) * itemsize;
		jvalue v;
		switch (typeCode)
		{
			case 'Z': v.z = *(const jboolean*) src; break;
			case 'B': v.b = *(const jbyte*) src; break;
			case 'C': v.c = *(const jchar*) src; break;
			case 'S': v.s = *(const jshort*) src; break;
			case 'I': v.i = *(const jint*) src; break;
			case 'J': v.j = *(const jlong*) src; break;
			case 'F': v.f = *(const jfloat*) src; break;
			default: v.d = *(const jdouble*) src; break; // 'D'
		}
		PyList_SET_ITEM(list.get(), i, convertToPythonObject(frame, v, false).keep());
	}

	JP_TRACE_JAVA("ReleasePrimitiveArrayCritical", mem);
	frame.getEnv()->ReleasePrimitiveArrayCritical(a, mem, JNI_ABORT);
	return list;
}

