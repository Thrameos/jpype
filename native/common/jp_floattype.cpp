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
#include "jp_floattype.h"
#include "jp_boxedtype.h"

JPFloatType::JPFloatType()
: JPPrimitiveType("float")
{
}

JPFloatType::~JPFloatType()
= default;

JPClass* JPFloatType::getBoxedClass(JPJavaFrame& frame) const
{
	return frame.getContext()->_java_lang_Float;
}

JPPyObject JPFloatType::convertToPythonObject(JPJavaFrame& frame, jvalue value, bool cast)
{
	PyTypeObject * wrapper = getHost();
	JPPyObject obj = JPPyObject::call(wrapper->tp_alloc(wrapper, 0));
	((PyFloatObject*) obj.get())->ob_fval = value.f;
	PyJPValue_assignJavaSlot(frame, obj.get(), JPValue(this, value));
	return obj;
}

JPValue JPFloatType::getValueFromObject(JPJavaFrame& frame, const JPValue& obj)
{
	jvalue v;
	jobject jo = obj.getValue().l;
	auto* jb = dynamic_cast<JPBoxedType*>( frame.findClassForObject(jo));
	field(v) = (type_t) frame.CallFloatMethodA(jo, jb->m_FloatValueID, nullptr);
	return JPValue(this, v);
}

static JPConversionAsFloat<JPFloatType> asFloatConversion;
static JPConversionLongAsFloat<JPFloatType> asFloatLongConversion;
static JPConversionFloatWiden<JPFloatType> floatWidenConversion;

class JPConversionAsJFloat : public JPConversionJavaValue
{
public:

	JPMatch::Type matches(JPClass *cls, JPMatch &match) override
	{
		if (match.getJPClass() == nullptr)
			return match.type = JPMatch::_none;
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
				case 'B':
				case 'S':
				case 'C':
				case 'I':
				case 'J':
					match.conversion = &floatWidenConversion;
					return match.type = JPMatch::_implicit;
				default:
					break;
			}
		}

		// Unboxing must be to the from the exact boxed type (JLS 5.1.8)
		return JPMatch::_implicit; // stop search
	}

	void getInfo(JPClass *cls, JPConversionInfo &info) override
	{
		JPContext *context = JPContext_global;
		PyList_Append(info.exact, (PyObject*) context->_float->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_byte->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_char->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_short->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_int->getHost());
		PyList_Append(info.implicit, (PyObject*) context->_long->getHost());
		unboxConversion->getInfo(cls, info);
	}

} asJFloatConversion;

JPMatch::Type JPFloatType::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPFloatType::findJavaConversion");

	if (match.object == Py_None)
		return match.type = JPMatch::_none;

	if (asJFloatConversion.matches(this, match)
			|| asFloatLongConversion.matches(this, match)
			|| asFloatConversion.matches(this, match))
		return match.type;

	return match.type = JPMatch::_none;
	JP_TRACE_OUT;
}

bool JPFloatType::fastElementCheck(PyObject* obj, JPMatch::Type& quality) const
{
	// Matches asFloatConversion's (JPConversionAsFloat<JPFloatType>)
	// PyNumber_Check branch for an exact Python float -- see
	// JPIntType::fastElementCheck for the same pattern. Unlike double,
	// float has no dedicated exact-match conversion (narrowing a 64-bit
	// Python float into a 32-bit Java float is always lossy), so this is
	// _implicit, not _exact.
	if (!PyFloat_CheckExact(obj))
		return false;
	quality = JPMatch::_implicit;
	return true;
}

void JPFloatType::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	asJFloatConversion.getInfo(this, info);
	asFloatLongConversion.getInfo(this, info);
	asFloatConversion.getInfo(this, info);
	PyList_Append(info.ret, (PyObject*) JPContext_global->_float->getHost());
}

jarray JPFloatType::newArrayOf(JPJavaFrame& frame, jsize sz)
{
	return frame.NewFloatArray(sz);
}

JPPyObject JPFloatType::getStaticField(JPJavaFrame& frame, jclass c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetStaticFloatField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPFloatType::getField(JPJavaFrame& frame, jobject c, jfieldID fid)
{
	jvalue v;
	field(v) = frame.GetFloatField(c, fid);
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPFloatType::invokeStatic(JPJavaFrame& frame, jclass claz, jmethodID mth, jvalue *val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		field(v) = frame.CallStaticFloatMethodA(claz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

JPPyObject JPFloatType::invoke(JPJavaFrame& frame, jobject obj, jclass clazz, jmethodID mth, jvalue *val)
{
	jvalue v;
	{
		JPPyCallRelease call;
		if (clazz == nullptr)
			field(v) = frame.CallFloatMethodA(obj, mth, val);
		else
			field(v) = frame.CallNonvirtualFloatMethodA(obj, clazz, mth, val);
	}
	return convertToPythonObject(frame, v, false);
}

void JPFloatType::setStaticField(JPJavaFrame& frame, jclass c, jfieldID fid, PyObject *obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java float");
	type_t val = field(match.convert());
	frame.SetStaticFloatField(c, fid, val);
}

void JPFloatType::setField(JPJavaFrame& frame, jobject c, jfieldID fid, PyObject *obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java float");
	type_t val = field(match.convert());
	frame.SetFloatField(c, fid, val);
}

void JPFloatType::setArrayRange(JPJavaFrame& frame, jarray a,
		jsize start, jsize length, jsize step,
		PyObject* sequence)
{
	JP_TRACE_IN("JPFloatType::setArrayRange");
	if (tryFastBufferPush(frame, this, a, start, step, length, sequence))
		return;

	JPPrimitiveArrayAccessor<array_t, type_t*> accessor(frame, a,
			&JPJavaFrame::GetFloatArrayElements, &JPJavaFrame::ReleaseFloatArrayElements);

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
			jconverter conv = getConverter(view.format, (int) view.itemsize, "f");
			for (Py_ssize_t i = 0; i < length; ++i, index += step)
			{
				jvalue r = conv(memory);
				val[index] = r.f;
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

	// Fast path: a plain list of exact floats, avoiding PySequence_GetItem's
	// generic protocol dispatch in favor of PyList_GET_ITEM. See
	// JPIntType::setArrayRange for the same pattern.
	if (PyList_CheckExact(sequence))
	{
		for (; i < length; ++i, index += step)
		{
			PyObject *item = PyList_GET_ITEM(sequence, i);
			if (!PyFloat_CheckExact(item))
				break;
			double v = PyFloat_AS_DOUBLE(item);
			val[index] = (type_t) v;
		}
	}

	if (i < length)
	{
		// General sequence API, continuing from wherever the fast path
		// above left off.
		JPPySequence seq = JPPySequence::use(sequence);
		for (; i < length; ++i, index += step)
		{
			double v =  PyFloat_AsDouble(seq[i].get());
			if (v == -1.)
				JP_PY_CHECK();
			val[index] = (type_t) v;
		}
	}
	accessor.commit();
	JP_TRACE_OUT;
}

JPPyObject JPFloatType::getArrayItem(JPJavaFrame& frame, jarray a, jsize ndx)
{
	auto array = (array_t) a;
	type_t val;
	frame.GetFloatArrayRegion(array, ndx, 1, &val);
	jvalue v;
	field(v) = val;
	return convertToPythonObject(frame, v, false);
}

void JPFloatType::setArrayItem(JPJavaFrame& frame, jarray a, jsize ndx, PyObject* obj)
{
	JPMatch match(&frame, obj);
	if (findJavaConversion(match) < JPMatch::_implicit)
		JP_RAISE(PyExc_TypeError, "Unable to convert to Java float");
	type_t val = field(match.convert());
	frame.SetFloatArrayRegion((array_t) a, ndx, 1, &val);
}

JPPyObject JPFloatType::getFastArrayItem(JPJavaAccess& frame, jarray a, jsize ndx)
{
	// Inlines convertToPythonObject directly rather than calling it,
	// because that would require a real JPJavaFrame& just to satisfy the
	// signature. Unlike byte/short/int/long/char (see
	// JPIntType::getFastArrayItem), float has no JValueFn registered, so
	// PyJPValue_assignJavaSlot's real effect -- a plain offset-based
	// jvalue write, no JNI involved -- does need replicating here, but
	// that's pure Python-level type metadata (PyJPValue_getJavaSlotOffset)
	// plus a raw memory write, not anything a frame is needed for.
	auto array = (array_t) a;
	type_t val;
	frame.GetFloatArrayRegion(array, ndx, 1, &val);
	PyTypeObject* wrapper = getHost();
	JPPyObject obj = JPPyObject::call(wrapper->tp_alloc(wrapper, 0));
	((PyFloatObject*) obj.get())->ob_fval = val;
	Py_ssize_t offset = PyJPValue_getJavaSlotOffset(obj.get());
	auto* slot = (jvalue*) (((char*) obj.get()) + offset);
	slot->f = val;
	return obj;
}

JPArray* JPFloatType::createArrayWrapper(const JPValue& value)
{
	return new JPArrayFloat(value);
}

JPArrayClass* JPFloatType::createArrayClass(JPJavaFrame& frame, jclass cls,
		const string& name, JPClass* superClass, jint modifiers)
{
	return new JPArrayClassFloat(frame, cls, name, superClass, this, modifiers);
}

JPMatch::Type JPArrayClassFloat::findJavaConversionImpl(JPMatch &match)
{
	JP_TRACE_IN("JPArrayClassFloat::findJavaConversion");
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

void JPArrayClassFloat::getConversionInfo(JPConversionInfo &info)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	objectConversion->getInfo(this, info);
	bufferConversion->getInfo(this, info);
	sequenceConversion->getInfo(this, info);
	hintsConversion->getInfo(this, info);
	PyList_Append(info.ret, PyJPClass_create(frame, this).get());
}

JPArrayFloat::JPArrayFloat(const JPValue& array)
: JPArray(array), m_CompType(dynamic_cast<JPFloatType*>(m_Class->getComponentType()))
{
}

JPArrayFloat::JPArrayFloat(JPArrayFloat* src, jsize start, jsize stop, jsize step)
: JPArray(src, start, stop, step), m_CompType(src->m_CompType)
{
}

JPPyObject JPArrayFloat::getItem(jsize ndx)
{
	ndx = checkIndex(ndx);
	JPJavaAccess frame;
	return m_CompType->getFastArrayItem(frame, m_Object.get(), m_Start + ndx * m_Step);
}

JPArray* JPArrayFloat::slice(jsize start, jsize stop, jsize step)
{
	return new JPArrayFloat(this, start, stop, step);
}

void JPFloatType::getView(JPArrayView& view)
{
	JPJavaFrame frame = JPJavaFrame::outer();
	view.m_Memory = (void*) frame.GetFloatArrayElements(
			(jfloatArray) view.m_Array->getJava(), &view.m_IsCopy);
	view.m_Buffer.format = "f";
	view.m_Buffer.itemsize = sizeof (jfloat);
}

void JPFloatType::releaseView(JPArrayView& view)
{
	try
	{
		JPJavaFrame frame = JPJavaFrame::outer();
		frame.ReleaseFloatArrayElements((jfloatArray) view.m_Array->getJava(),
				(jfloat*) view.m_Memory, view.m_Buffer.readonly ? JNI_ABORT : 0);
	}	catch (...)
	{
		// This is called as part of the cleanup routine and exceptions
		// are not permitted
	}
}

const char* JPFloatType::getBufferFormat()
{
	return "f";
}

Py_ssize_t JPFloatType::getItemSize()
{
	return sizeof (jfloat);
}

void JPFloatType::copyElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		void* memory, int offset)
{
	auto* b = (jfloat*) ((char*) memory + offset);
	frame.GetFloatArrayRegion((jfloatArray) a, start, len, b);
}

void JPFloatType::setElements(JPJavaFrame &frame, jarray a, jsize start, jsize len,
		const void* memory, int offset)
{
	auto* b = (jfloat*) ((const char*) memory + offset);
	frame.SetFloatArrayRegion((jfloatArray) a, start, len, const_cast<jfloat*>(b));
}

static void pack(jfloat* d, jvalue v)
{
	*d = v.f;
}

PyObject *JPFloatType::newMultiArray(JPJavaFrame &frame, JPPyBuffer &buffer, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPFloatType::newMultiArray");
	return convertMultiArray<type_t>(
			frame, this, &pack, "f",
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}

jobject JPFloatType::newMultiArrayObject(JPJavaFrame &frame, JPPyBuffer &buffer, jconverter converter, int subs, int base, jobject dims)
{
	JP_TRACE_IN("JPFloatType::newMultiArrayObject");
	return convertMultiArrayObject<type_t>(
			frame, this, &pack, converter,
			buffer, subs, base, dims);
	JP_TRACE_OUT;
}
