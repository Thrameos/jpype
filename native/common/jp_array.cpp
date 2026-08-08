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
#include <cctype>
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_primitive_accessor.h"

// Note: java represents arrays of zero length as null, thus we
// need to be careful to handle these properly.  We need to
// carry them around so that we can match types.

JPArray::JPArray(const JPValue &value)
: m_Object((jarray) value.getValue().l)
{
	m_Class = dynamic_cast<JPArrayClass*>( value.getClass());
	JPJavaFrame frame = JPJavaFrame::outer();
	JP_TRACE_IN("JPArray::JPArray");
	ASSERT_NOT_NULL(m_Class);
	JP_TRACE(m_Class->toString());

	// We will use this during range checks, so cache it
	if (m_Object.get() == nullptr)
		m_Length = 0;  // GCOVR_EXCL_LINE
	else
		m_Length = frame.GetArrayLength(m_Object.get());

	m_Step = 1;
	m_Start = 0;
	m_Slice = false;

	JP_TRACE_OUT;
}

JPArray::JPArray(JPArray* instance, jsize start, jsize stop, jsize step)
: m_Object((jarray) instance->getJava())
{
	JP_TRACE_IN("JPArray::JPArraySlice");
	m_Class = instance->m_Class;
	m_Step = step * instance->m_Step;
	m_Start = instance->m_Start + instance->m_Step*start;
	if (step > 0)
		m_Length =  (stop - start - 1 + step) / step;
	else
		m_Length =  (stop - start + 1 + step) / step;
	if (m_Length < 0)
		m_Length = 0;  // GCOVR_EXCL_LINE
	m_Slice = true;
	JP_TRACE_OUT;
}

JPArray::~JPArray()
= default;

jsize JPArray::getLength() const
{
	return m_Length;
}

void JPArray::setRange(jsize start, jsize length, jsize step, PyObject* val)
{
	JP_TRACE_IN("JPArray::setRange");

	// Make sure it is an iterable before we start
	if (!PySequence_Check(val))
		JP_RAISE(PyExc_TypeError, "can only assign a sequence");

	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();
	JPPySequence seq = JPPySequence::use(val);
	long plength = (long) seq.size();

	JP_TRACE("Verify lengths", length, plength);
	if ((long) length != plength)
	{
		// Python would allow mismatching size by growing or shrinking
		// the length of the array.  But java arrays are immutable in length.
		std::stringstream out;
		out << "Slice assignment must be of equal lengths : " << length << " != " << plength;
		JP_RAISE(PyExc_ValueError, out.str());
	}

	JP_TRACE("Call component set range");
	jsize i0 = m_Start + m_Step*start;
	compType->setArrayRange(frame, m_Object.get(), i0, length, m_Step*step, val);
	JP_TRACE_OUT;
}

void JPArray::setItem(jsize ndx, PyObject* val)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
		JP_RAISE(PyExc_IndexError, "java array assignment out of bounds");

	compType->setArrayItem(frame, m_Object.get(), m_Start + ndx*m_Step, val);
}

JPPyObject JPArray::getItem(jsize ndx)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	JPClass* compType = m_Class->getComponentType();

	if (ndx < 0)
		ndx += m_Length;

	if (ndx >= m_Length || ndx < 0)
	{
		JP_RAISE(PyExc_IndexError, "array index out of bounds");
	}

	return compType->getArrayItem(frame, m_Object.get(), m_Start + ndx * m_Step);
}

jarray JPArray::clone(JPJavaFrame& frame, PyObject* obj)
{
	JPValue value = m_Class->newArray(frame, m_Length);
	JPClass* compType = m_Class->getComponentType();
	auto out = (jarray) value.getValue().l;
	compType->setArrayRange(frame, out, 0, m_Length, 1, obj);
	return out;
}

void JPArray::pullTo(PyObject* dest)
{
	JP_TRACE_IN("JPArray::pullTo");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	if (compType == nullptr)
		JP_RAISE(PyExc_TypeError, "pullTo requires a primitive array");

	JPJavaFrame frame = JPJavaFrame::outer();
	JPPyBuffer buffer(dest, PyBUF_WRITABLE | PyBUF_STRIDES | PyBUF_FORMAT);
	JP_PY_CHECK();
	Py_buffer& view = buffer.getView();

	Py_ssize_t total = 1;
	for (int i = 0; i < view.ndim; ++i)
		total *= view.shape[i];
	if (total != m_Length)
		JP_RAISE(PyExc_ValueError, "mismatched size");
	if (view.itemsize != compType->getItemSize())
		JP_RAISE(PyExc_TypeError, "mismatched item size");

	// Fast path: a single Get<Type>ArrayRegion call straight into the
	// destination memory. Requires a unit-step source (no sliced array)
	// and a C-contiguous destination (any number of dims, so long as the
	// whole thing is one contiguous run).
	if (m_Step == 1 && view.suboffsets == nullptr && PyBuffer_IsContiguous(&view, 'C'))
	{
		compType->copyElements(frame, m_Object.get(), m_Start, m_Length, view.buf, 0);
	} else
	{
		// General path: stepped source and/or non-contiguous/N-D destination.
		copyArrayToBuffer(frame, m_Object.get(), m_Start, m_Step, m_Length,
				compType->getItemSize(), buffer);
	}
	JP_TRACE_OUT;
}

void JPArray::pushFrom(PyObject* src)
{
	JP_TRACE_IN("JPArray::pushFrom");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	if (compType == nullptr)
		JP_RAISE(PyExc_TypeError, "pushFrom requires a primitive array");

	JPJavaFrame frame = JPJavaFrame::outer();
	JPPyBuffer buffer(src, PyBUF_STRIDES | PyBUF_FORMAT);
	JP_PY_CHECK();
	Py_buffer& view = buffer.getView();

	Py_ssize_t total = 1;
	for (int i = 0; i < view.ndim; ++i)
		total *= view.shape[i];
	if (total != m_Length)
		JP_RAISE(PyExc_ValueError, "mismatched size");

	char code[2] = {(char) tolower(compType->getTypeCode()), 0};
	const char *format = view.format != nullptr ? view.format : "B";
	jconverter converter = getConverter(format, (int) view.itemsize, code);
	if (converter == nullptr)
		JP_RAISE(PyExc_TypeError, "No type converter found");

	// Fast path: source needs no per-element conversion at all (matching
	// dtype, native byte order) -- a single Set<Type>ArrayRegion call
	// straight from the source memory. Requires a unit-step destination
	// (no sliced array) and a C-contiguous source (any number of dims, so
	// long as the whole thing is one contiguous run). Unlike the
	// multi-dim push path (JPConversionMultiArrayBuffer), JPArray is
	// always a single flat Java array, so there is no per-row pinning
	// cost to dodge for RAW_SWAPPED/RAW_HALF_* here -- the general path
	// below already pins the whole destination array exactly once
	// (copyBufferToArray), so only the strictly-cheapest case (no
	// conversion at all) earns a dedicated fast path.
	JPRawTransferMode mode = classifyRawTransfer(converter, compType, format, (int) view.itemsize, code);
	if (mode == RAW_NATIVE && m_Step == 1 && view.suboffsets == nullptr && PyBuffer_IsContiguous(&view, 'C'))
	{
		compType->setElements(frame, m_Object.get(), m_Start, m_Length, view.buf, 0);
	} else
	{
		// General path: real value conversion (dtype coercion, byte swap,
		// half-precision) and/or stepped destination and/or
		// non-contiguous/N-D source. Single JNI critical section for the
		// whole destination array.
		copyBufferToArray(frame, m_Object.get(), m_Start, m_Step, m_Length,
				compType->getItemSize(), converter, buffer);
	}
	JP_TRACE_OUT;
}

JPPyObject JPArray::toList()
{
	JP_TRACE_IN("JPArray::toList");
	auto *compType = dynamic_cast<JPPrimitiveType*>(m_Class->getComponentType());
	if (compType != nullptr)
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		return compType->getArrayRange(frame, m_Object.get(), m_Start, m_Step, m_Length);
	}

	// Object[] or a nested array class -- no bulk read possible (each
	// element can be a distinct runtime type), but recurse into any
	// nested Java array so multi-dim primitive arrays still come out as
	// genuinely nested Python lists.
	JPPyObject list = JPPyObject::call(PyList_New(m_Length));
	for (jsize i = 0; i < m_Length; ++i)
	{
		JPPyObject item = getItem(i);
		if (item.get() != nullptr && PyObject_IsInstance(item.get(), (PyObject*) PyJPArray_Type))
			item = ((PyJPArray*) item.get())->m_Array->toList();
		PyList_SET_ITEM(list.get(), i, item.keep());
	}
	return list;
	JP_TRACE_OUT;
}

JPArrayView::JPArrayView(JPArray* array)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;
	m_RefCount = 0;
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = 1;
	m_Buffer.suboffsets = nullptr;
	auto *type = dynamic_cast<JPPrimitiveType*>( array->getClass()->getComponentType());
	type->getView(*this);
	m_Strides[0] = m_Buffer.itemsize * array->m_Step;
	m_Shape[0] = array->m_Length;
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = array->m_Length * m_Buffer.itemsize;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	m_Owned = false;
}

JPArrayView::JPArrayView(JPArray* array, jobject collection)
{
	JP_TRACE_IN("JPArrayView::JPArrayView");
	// All of the work has already been done by org.jpype.Utilities
	JPJavaFrame frame = JPJavaFrame::outer();
	m_Array = array;

	jobject item0 = frame.GetObjectArrayElement((jobjectArray) collection, 0);
	jobject item1 = frame.GetObjectArrayElement((jobjectArray) collection, 1);

	// First element is the primitive type that we are packing the array from
	auto *componentType = dynamic_cast<JPPrimitiveType*>(
			frame.findClass((jclass) item0));

	// Second element is the shape of the array from which we compute the
	// memory size, the shape, and strides
	int dims;
	Py_ssize_t itemsize;
	Py_ssize_t sz;
	{
		JPPrimitiveArrayAccessor<jintArray, jint*> accessor(frame, (jintArray) item1,
				&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);
		jint* shape2 = accessor.get();
		dims = frame.GetArrayLength((jarray) item1);
		itemsize = componentType->getItemSize();
		sz = itemsize;
		for (int i = 0; i < dims; ++i)
		{
			m_Shape[i] = shape2[i];
			sz *= m_Shape[i];
		}
		accessor.abort();
	}
	Py_ssize_t stride = itemsize;
	for (int i = 0; i < dims; ++i)
	{
		int n = dims - 1 - i;
		m_Strides[n] = stride;
		stride *= m_Shape[n];
	}

	m_RefCount = 0;
	m_Memory = new char[sz];
	m_Owned = true;

	// Phase 3.6 (plan/ArrayTransferPhase3.md): a single JNI entry into
	// Support.collectToBuffer instead of one reflective
	// GetObjectArrayElement plus one Get<Type>ArrayRegion (via
	// copyElements) per leaf array -- the whole remaining-elements walk
	// and bulk write into m_Memory happens in pure Java (including the
	// serial-vs-parallel decision -- see Support.leafRange), wrapped as a
	// direct buffer so no further JNI calls are needed at all.
	jobject directBuf = frame.NewDirectByteBuffer(m_Memory, sz);
	frame.collectMultiArrayToBuffer(componentType->getTypeCode(), collection, directBuf);

	// Copy values into Python buffer for consumption
	m_Buffer.obj = nullptr;
	m_Buffer.ndim = dims;
	m_Buffer.suboffsets = nullptr;
	m_Buffer.itemsize = itemsize;
	m_Buffer.format = const_cast<char*> (componentType->getBufferFormat());
	m_Buffer.buf = (char*) m_Memory + m_Buffer.itemsize * array->m_Start;
	m_Buffer.len = sz;
	m_Buffer.shape = m_Shape;
	m_Buffer.strides = m_Strides;
	m_Buffer.readonly = 1;
	JP_TRACE_OUT;  // GCOVR_EXCL_LINE
}

JPArrayView::~JPArrayView()
{
	if (m_Owned)
		delete [] (char*) m_Memory;
}

void JPArrayView::reference()
{
	m_RefCount++;
}

bool JPArrayView::unreference()
{
	m_RefCount--;
	auto *type = dynamic_cast<JPPrimitiveType*>( m_Array->getClass()->getComponentType());
	if (m_RefCount == 0 && !m_Owned)
		type->releaseView(*this);
	return m_RefCount == 0;
}
