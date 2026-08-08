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
#ifndef _JPARRAY_H_
#define _JPARRAY_H_

#include "jp_javaframe.h"

class JPArray;

class JPArrayView
{
public:
	explicit JPArrayView(JPArray* array);
	JPArrayView(JPArray* array, jobject collection);
	~JPArrayView();
	void reference();
	bool unreference();
public:
	JPArray *m_Array;
	void *m_Memory{};
	Py_buffer m_Buffer{};
	int m_RefCount;
	Py_ssize_t m_Shape[5]{};
	Py_ssize_t m_Strides[5]{};
	jboolean m_IsCopy{};
	jboolean m_Owned{};
} ;

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPArray
{
	friend class JPArrayView;
public:
	explicit JPArray(const JPValue& array);
	JPArray(JPArray* cls, jsize start, jsize stop, jsize step);
	virtual~ JPArray();

	JPArrayClass* getClass()
	{
		return m_Class;
	}

	jsize     getLength() const;
	void       setRange(jsize start, jsize length, jsize step, PyObject* val);
	JPPyObject getItem(jsize ndx);
	void       setItem(jsize ndx, PyObject*);

	/**
	 *  Create a shallow copy of an array.
	 *
	 * This is used to extract a slice before calling or casting operations.
	 *
	 * @param frame
	 * @param obj
	 * @return
	 */
	jarray     clone(JPJavaFrame& frame, PyObject* obj);

	/**
	 * Bulk-copy this array's elements into a caller-supplied writable
	 * Python buffer (JArray.copyInto). Primitive arrays only -- requires a
	 * matching element count and item size, but not a matching shape
	 * (dest may be any number of dimensions, so long as the total element
	 * count lines up).
	 *
	 * @param dest a writable buffer-protocol object.
	 */
	void       copyInto(PyObject* dest);

	/**
	 * Bulk-convert this array into a genuine Python list (JArray.tolist()).
	 *
	 * For a primitive array, reads the whole range in a single JNI
	 * critical section (JPPrimitiveType::getArrayRange) instead of one JNI
	 * call per element. For an Object[]/nested-array component type, boxes
	 * each element individually (each element can be a distinct runtime
	 * type, so there is no bulk read to do) but recurses into any nested
	 * Java array so a multi-dim primitive array produces genuinely nested
	 * Python lists rather than a list of JArray wrapper objects.
	 *
	 * @return a new Python list.
	 */
	JPPyObject toList();

	bool       isSlice() const
	{
		return m_Slice;
	}

	jarray     getJava()
	{
		return m_Object.get();
	}

private:
	JPArrayClass* m_Class;
	JPArrayRef    m_Object;
	jsize         m_Start;
	jsize         m_Step;
	jsize         m_Length;
	bool          m_Slice;
} ;

#endif // _JPARRAY_H_
