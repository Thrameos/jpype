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
#include "jpype.h"
#include "pyjp.h"
#include "jp_array.h"
#include "jp_arrayclass.h"
#include "jp_classhints.h"
#include "jp_primitive_accessor.h"
#include "jp_inttype.h"

JPIntType::JPIntType()
: JPPrimitiveType("int")
{
}

JPIntType::~JPIntType()
= default;

JPClass* JPIntType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Integer;
}

JPPyObject JPIntType::convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast)
{
	if (getHost() == nullptr)
		return JPPyObject::call(PyLong_FromLong(field(val)));
	JPPyObject out = JPPyObject::call(convertLong(getHost(), field(val)));
	PyJPValue_assignJavaSlot(frame, out.get(), JPValue(this, val));
	return out;
}

JPValue JPIntType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	jobject jo = obj.getValue().l;
	auto* jb = dynamic_cast<JPBoxedType*>( frame.findClassForObject(jo));
	field(v) = (type_t) frame.CallIntMethodA(jo, jb->m_IntValueID, nullptr);
	return JPValue(this, v);
}

JPConversionLong<JPIntType> intConversion;
JPConversionLongNumber<JPIntType> intNumberConversion;
JPConversionLongWiden<JPIntType> intWidenConversion;

class JPConversionJInt : public JPConversionJavaValue
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (match.getJPClass() == nullptr)
			return JPMatch::_none;
		match.type = JPMatch::_none;

		// Implied conversion from boxed to primitive (JLS 5.1.8)
		if (javaValueConversion->matches(cls, match)
				|| unboxConversion->matches(cls, match))
			return match.type;

		// Consider widening
		JPClass *cls2 = match.getJPClass();
		if (cls2->isPrimitive())
		{
			// https://docs.oracle.com/javase/specs/jls/se7/html/jls-5.html#jls-5.1.2
			auto *prim = dynamic_cast<JPPrimitiveType*>( cls2);
			switch (prim->getTypeCode())
			{
				case 'C':
				case 'S':
				case 'B':
					match.conversion = &intWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					break;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit;  //short cut further checks
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = JPContext_global;
		PyList_Append(info.exact, (PyObject*) context->_int->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_byte->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_char->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_short->getHost());
		unboxConversion->getInfo(cls, info);
	}

} jintConversion;

JPMatch::Type JPIntType::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPIntType::findJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	if (jintConversion.matches(this, match)
			|| intConversion.matches(this, match)
			|| intNumberConversion.matches(this, match))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

bool JPIntType::fastElementCheck(PyObject* obj, JPMatch::Type& quality) const
{
	// Matches intConversion's (JPConversionLong<JPIntType>) own test
	// exactly -- PyLong_CheckExact/PyIndex_Check are themselves Py_TYPE
	// slot checks, so this is exactly as correct as the general path for
	// this case, just without constructing a JPMatch or going through the
	// cache. A tagged Java value (e.g. an actual JInt) could in principle
	// resolve to a higher quality than _implicit via jintConversion
	// (checked ahead of intConversion in findJavaConversionImpl), but
	// JPConversionSequence::matches() clamps the whole sequence's result
	// to _implicit regardless (its running match.type starts at
	// _implicit and can only be lowered), so that distinction is
	// unobservable here -- _implicit is always the correct answer for
	// any element that would not have been rejected outright.
	if (!PyLong_CheckExact(obj) && !PyIndex_Check(obj))
		return false;
	quality = JPMatch::_implicit;
	return true;
}

bool JPIntType::fastSequenceCheck(JPMatch& match, JPPySequence& seq, jlong length)
{
	// Whole-sequence fast path for JPConversionSequence (jp_classhints.cpp).
	// A single {PyTypeObject*} slot for the whole scan, checked first via
	// a bare Py_TYPE(item) == cachedType pointer compare -- intConversion's
	// own matches() (JPConversionLong<JPIntType>, jp_primitive_accessor.h)
	// determines quality purely from PyLong_CheckExact/PyIndex_Check, never
	// from the object's value, so caching by Py_TYPE alone is exactly as
	// correct as re-probing every element of that type. Only one slot is
	// needed (not one per quality level) because both the exact and
	// index-check branches resolve to the same _implicit floor.
	//
	// A tagged Java value (e.g. an actual JInt) could in principle resolve
	// to a higher quality than _implicit via jintConversion (checked ahead
	// of intConversion in findJavaConversionImpl), but
	// JPConversionSequence::matches()'s running match.type starts at
	// _implicit and can only be lowered, never raised, so that distinction
	// is unobservable at the sequence level -- _implicit is always the
	// correct answer for any element this fast check accepts.
	//
	// Any element that isn't exact-int or index-like (a string, a float,
	// something needing a real widening/unboxing decision, ...) is
	// resolved on the spot via the general findJavaConversion path for
	// just that one element -- there is no "bail out, caller reprocesses
	// the whole sequence" case, so this always returns true.
	match.type = JPMatch::_implicit;
	PyTypeObject *cachedType = nullptr;
	for (jlong i = 0; i < length && match.type > JPMatch::_none; i++)
	{
		JPPyObject item = seq[i];
		PyObject *obj = item.get();
		PyTypeObject *itemType = Py_TYPE(obj);

		if (itemType == cachedType)
			continue;

		if (PyLong_CheckExact(obj) || PyIndex_Check(obj))
		{
			cachedType = itemType;
			continue;
		}

		JPMatch imatch(match.frame, obj);
		findJavaConversion(imatch);
		if (imatch.type < match.type)
			match.type = imatch.type;
	}
	return true;
}

void JPIntType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	jintConversion.getInfo(this, info);
	intConversion.getInfo(this, info);
	intNumberConversion.getInfo(this, info);
	PyList_Append(info.ret, (PyObject*) JPContext_global->_int->getHost());
}

jarray JPIntType::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewIntArray(sz);
}

JPPyObject JPIntType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticIntField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPIntType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetIntField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPIntType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticIntMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPIntType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue* val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == nullptr)
			field(v) = frame.CallIntMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualIntMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPIntType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetStaticIntField(c, fid, val);
}

void JPIntType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetIntField(c, fid, val);
}

void JPIntType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPIntType::setArrayRange");
	if (tryFastBufferPush(frame, this, a, start, step, length, sequence))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetIntArrayElements, &JPJavaFrame::ReleaseIntArrayElements);

	type_t* val = accessor.get();
	// First check if assigning sequence supports buffer API
	if (PyObject_CheckBuffer(sequence))
	{
		JPPyBuffer buffer(sequence, PyBUF_FULL_RO);
		if (buffer.valid())
		{
			Py_buffer& view = buffer.getView();
			if (view.ndim != 1)
				JP_RAISE(PyExc_TypeError, "buffer dims incorrect");
			Py_ssize_t vshape = view.shape[0];
			Py_ssize_t vstep = view.strides[0];
			if (vshape != length)
				JP_RAISE(PyExc_ValueError, "mismatched size");

			char* memory = (char*) view.buf;
			if (view.suboffsets && view.suboffsets[0] >= 0)
				memory = *((char**) memory) + view.suboffsets[0];
			jsize index = start;
			jconverter conv = getConverter(view.format, (int) view.itemsize, "i");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.i;
				memory += vstep;
			}
			accessor.commit();
			return;
		} else
		{
			PyErr_Clear();
		}
	}

	jsize index = start;
	Py_ssize_t i = 0;

	// Fast path: a plain list of exact ints, avoiding PySequence_GetItem's
	// generic protocol dispatch (a real function call through the type's
	// sequence-protocol slot) in favor of PyList_GET_ITEM (a raw, borrowed-
	// reference index into the list's backing array), and PyLong_AsLong
	// instead of the more general PyIndex_Check + PyLong_AsLongLong.
	// Breaks out to the general path below on the first element that
	// isn't an exact int (a bool, a numpy scalar, a custom __index__
	// object, ...) -- matches() already guaranteed every element converts
	// *somehow*, just not necessarily via this cheap check.
	if (PyList_CheckExact(sequence))
	{
		for (; i < length; ++i, index += step)
		{
			PyObject *item = PyList_GET_ITEM(sequence, i);
			if (!PyLong_CheckExact(item))
				break;
			long v = PyLong_AsLong(item);
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) assertRange(v);
		}
	}

	if (i < length)
	{
		// General sequence API, continuing from wherever the fast path
		// above left off (i==0/index==start if it never ran at all, e.g.
		// sequence isn't a list) -- indices before i are already correctly
		// populated, so there's no redundant work either way.
		JPPySequence seq = JPPySequence::use(sequence);
		for (; i < length; ++i, index += step)
		{
			PyObject *item = seq[i].get();
			if (!PyIndex_Check(item))
			{
				PyErr_Format(PyExc_TypeError, "Unable to implicitly convert '%s' to int", Py_TYPE(item)->tp_name);
				JP_RAISE_PYTHON();
			}
			jlong v = PyLong_AsLongLong(item);
			if (v == -1)
				JP_PY_CHECK();
			val[index] = (type_t) assertRange(v);
		}
	}

	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPIntType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	auto array = (array_t) a;
	type_t val;
	frame.GetIntArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPIntType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java int");
	type_t val = field(match.convert());
	frame.SetIntArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPIntType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// Inlines convertToPythonObject directly rather than calling it,
	// because that would require a real JPJavaFrame& just to satisfy the
	// signature -- and PyJPValue_assignJavaSlot is a *guaranteed* no-op
	// for this family: it's rooted at a JValueFn-registered base type
	// (PyJPNumberLong_Type -- see PyJPClass_GetJValueFn's tp_base walk,
	// which every subclass, including a customized host, must inherit
	// from), so its early "nothing to write" return always fires here.
	// No frame is ever genuinely needed on this path.
	auto array = (array_t) a;
	type_t val;
	frame.GetIntArrayRegion(array, ndx, 1, &val);
	if (getHost() == nullptr)
		return JPPyObject::call(PyLong_FromLong(val));
	return JPPyObject::call(convertLong(getHost(), val));
}

JPArray* JPIntType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayInt(value);
}

JPArrayClass* JPIntType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassInt(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassInt::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassInt::findJavaConversion");
	if (nullConversion->matches(this, match)
			|| objectConversion->matches(this, match)
			|| bufferConversion->matches(this, match)
			|| sequenceConversion->matches(this, match)
			|| hintsConversion->matches(this, match)
			)
		return match.type;
	JP_TRACE("None");
	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

void JPArrayClassInt::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	bufferConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayInt::JPArrayInt(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPIntType*>(m_Class->getComponentType()))
{
}

JPArrayInt::JPArrayInt(JPArrayInt* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayInt::getItem(jsize ndx)
{
	ndx = checkIndex(ndx);
	JPJavaAccess frame;
	return m_CompType->getFastArrayItem(frame, m_Object.get(), m_Start + ndx * m_Step);
}

JPArray* JPArrayInt::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayInt(this, start, stop, step);
}

void JPIntType::getView(JPArrayView& view)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	view.m_IsCopy = false;
	view.m_Memory = (void*) frame.GetIntArrayElements(
			(jintArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "=i";
	view.m_Buffer.itemsize = sizeof (jint);
}

void JPIntType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		frame.ReleaseIntArrayElements((jintArray) view.m_Array->getJava(),
				(jint*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPIntType::getBufferFormat()
{
	return "=i";
}

Py_ssize_t JPIntType::getItemSize()
{
	return sizeof (jint);
}

void JPIntType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	jint* b = (jint*) ((char*) memory + offset);
	frame.GetIntArrayRegion((jintArray) a, start, len, b);
}

void JPIntType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jint*) ((const char*) memory + offset);
	frame.SetIntArrayRegion((jintArray) a, start, len, const_cast<jint*>(b));
}

static void pack(jint* d, jvalue v)
{
	*d = v.i;
}

PyObject *JPIntType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPIntType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "i",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}

jobject JPIntType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPIntType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
