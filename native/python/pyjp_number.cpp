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
#include "jp_boxedtype.h"
#include <cstddef>

static inline size_t PyJPNumber_alignUp(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

// Long/Boolean are variable-length PyLongObject subtypes.  Rather than a
// compile-time struct (as Exception/Float use), we reserve a FIXED maximum
// digit budget -- enough for any 64-bit Java boxed value -- so the appended
// JPValue's offset never depends on which value is being boxed.  See
// PyJPNumber_longFromLongLong's header comment for why we can't just let CPython's own
// long_subtype_new (used by default for any int subtype) handle this.
#if PYLONG_BITS_IN_DIGIT == 30
#define JLONG_MAX_DIGITS 3 /* 3*30 = 90 >= 64 bits */
#elif PYLONG_BITS_IN_DIGIT == 15
#define JLONG_MAX_DIGITS 5 /* 5*15 = 75 >= 64 bits */
#else
#error "Unexpected PYLONG_BITS_IN_DIGIT"
#endif

// Number of extra digit-slots (nitems) that must be passed to tp_alloc for
// every Long/Boolean instance so the fixed offset computed in
// PyJPNumber_initType always lands inside the actual allocation, regardless
// of alignment padding. Computed once at module init (PyJPNumber_initType).
static Py_ssize_t gLongReserveItems = 0;

// Float has a genuine compile-time-known C layout (like Exception), so it
// gets a real struct and a direct offsetof-based concrete offset -- no
// digit-budget reasoning needed since PyFloatObject is always fixed-size.
struct PyJPFloat
{
	PyFloatObject base;
	JPValue extra;
};

/*
 * WHY THESE DON'T CALL INTO PyLong_Type.tp_new/PyFloat_Type.tp_new WITH THE
 * ACTUAL SUBTYPE
 *
 * CPython's own tp_new for int/float SUBTYPES (long_subtype_new/
 * float_subtype_new, Objects/longobject.c, Objects/floatobject.c) build a
 * throwaway base-type instance first and then allocate+copy into the real
 * subtype using however many digits (int) that specific value needs --
 * NOT our fixed maximum budget. Since our type's slot offset is now a fixed
 * constant (computed once, independent of the boxed value), any allocation
 * that doesn't reserve the full fixed budget would place the appended
 * JPValue past the end of a too-small allocation. So every construction
 * path for Long/Boolean must go through PyJPNumber_longFromLongLong (which always
 * allocates gLongReserveItems digit-slots, regardless of the actual value's
 * magnitude); Float doesn't have this variable-length concern at all, since
 * PyFloatObject's size never depends on the value it stores.
 *
 * Digit layout is duplicated per CPython version boundary (confirmed
 * against the CPython source tree directly, tags v3.10.0 through v3.14.0 --
 * see jpype_boxed_long_antipattern_fix memory for the full derivation):
 *   - <=3.11: struct _longobject { PyObject_VAR_HEAD; digit ob_digit[1]; };
 *     sign is the sign of ob_size.
 *   - >=3.12: struct _longobject { PyObject_HEAD; _PyLongValue long_value; }
 *     where long_value = { uintptr_t lv_tag; digit ob_digit[1]; }. lv_tag's
 *     low 2 bits are sign (0=positive,1=zero,2=negative), bit 2 is the
 *     immortal-object flag (0 for our freshly allocated objects), lv_tag>>3
 *     is the digit count.
 */
PyObject* PyJPNumber_longFromLongLong(PyTypeObject* type, long long value)
{
	// Magnitude via unsigned negation so INT64_MIN doesn't overflow.
	unsigned long long mag = (value < 0)
			? (0ULL - (unsigned long long) value)
			: (unsigned long long) value;

	digit digits[JLONG_MAX_DIGITS];
	unsigned long long m = mag;
	for (int i = 0; i < JLONG_MAX_DIGITS; i++)
	{
		digits[i] = (digit) (m & PyLong_MASK);
		m >>= PyLong_SHIFT;
	}
	int ndigits = JLONG_MAX_DIGITS;
	while (ndigits > 0 && digits[ndigits - 1] == 0)
		ndigits--;

	// Always allocate the full reserved budget, regardless of how many
	// digits this particular value actually needs, so the appended slot's
	// offset never moves.
	auto* self = (PyLongObject*) type->tp_alloc(type, gLongReserveItems);
	if (self == nullptr)
		return nullptr;

#if PY_VERSION_HEX >= 0x030c0000
	int sign_code = (mag == 0) ? 1 : (value < 0 ? 2 : 0);
	self->long_value.lv_tag = ((uintptr_t) ndigits << 3) | (uintptr_t) sign_code;
	for (int i = 0; i < JLONG_MAX_DIGITS; i++)
		self->long_value.ob_digit[i] = digits[i];
#else
	Py_SET_SIZE(self, (value < 0) ? -ndigits : ndigits);
	for (int i = 0; i < JLONG_MAX_DIGITS; i++)
		self->ob_digit[i] = digits[i];
#endif
	return (PyObject*) self;
}

static PyObject* newFloatFixed(PyTypeObject* type, double value)
{
	auto* self = (PyFloatObject*) type->tp_alloc(type, 0);
	if (self == nullptr)
		return nullptr;
	self->ob_fval = value;
	return (PyObject*) self;
}

static bool isNull(PyObject *self)
{
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot != nullptr
			&& !javaSlot->getClass()->isPrimitive()
			&& javaSlot->getValue().l == nullptr)
		return true;
	return false;
}

#ifdef __cplusplus
extern "C"
{
#endif

static PyObject *PyJPNumber_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPNumber_new", type);
	auto *cls = (JPClass*) PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
		JP_RAISE(PyExc_TypeError, "Class type incorrect");

	JPJavaFrame frame = JPJavaFrame::outer();
	jvalue val;
	// One argument tries Java conversion first
	if (PyTuple_Size(args) == 1)
	{
		PyObject *arg = PyTuple_GetItem(args, 0);
		JPMatch match(&frame, arg);
		cls->findJavaConversion(match);
		if (match.type >= JPMatch::_implicit)
		{
			// Disable OverrangeError
			match.type = JPMatch::_exact;
			val = match.convert();
			PyObject *obj = cls->convertToPythonObject(frame, val, true).keep();
			return obj;
		}
	}

	if (PyObject_IsSubclass((PyObject*) type, (PyObject*) & PyLong_Type))
	{
		JPPyObject self = JPPyObject::call(PyLong_Type.tp_new(&PyLong_Type, args, kwargs));
		JPMatch match(&frame, self.get());
		cls->findJavaConversion(match);
		match.type = JPMatch::_exact;
		val = match.convert();
		return cls->convertToPythonObject(frame, val, true).keep();
	} else if (PyObject_IsSubclass((PyObject*) type, (PyObject*) & PyFloat_Type))
	{
		JPPyObject self = JPPyObject::call(PyFloat_Type.tp_new(&PyFloat_Type, args, kwargs));
		JPMatch match(&frame, self.get());
		cls->findJavaConversion(match);
		match.type = JPMatch::_exact;
		val = match.convert();
		return cls->convertToPythonObject(frame, val, true).keep();
	} else
	{
		PyErr_Format(PyExc_TypeError, "Type '%s' is not a number class", type->tp_name);
		return nullptr;
	}
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_int(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_int");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_float");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyLong_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_int(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_int");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_int(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-int");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_float(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_float");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (!isNull(self))
		return PyFloat_Type.tp_as_number->nb_float(self);
	PyErr_SetString(PyExc_TypeError, "cast of null pointer would return non-float");
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_str");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_str(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_str");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyFloat_Type.tp_str(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberLong_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_repr");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyLong_Type.tp_repr(self);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_repr(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_repr");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	return PyFloat_Type.tp_repr(self);
	JP_PY_CATCH(nullptr);
}

static const char* op_names[] = {
	"<", "<=", "==", "!=", ">", ">="
};

static PyObject *PyJPNumberLong_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPNumberLong_compare");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
	{
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None);
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyErr_Format(PyExc_TypeError, "'%s' not supported with null pointer", op_names[op]);
		JP_RAISE_PYTHON();
	}
	if (!PyNumber_Check(other))
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}
	return PyLong_Type.tp_richcompare(self, other, op);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumberFloat_compare(PyObject *self, PyObject *other, int op)
{
	JP_PY_TRY("PyJPNumberFloat_compare");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (isNull(self))
	{
		if (op == Py_EQ)
			return PyBool_FromLong(other == Py_None);
		if (op == Py_NE)
			return PyBool_FromLong(other != Py_None);
		PyErr_Format(PyExc_TypeError, "'%s' not supported with null pointer", op_names[op]);
		JP_RAISE_PYTHON();
	}
	if (!PyNumber_Check(other)) // || Py_TYPE(other) == (PyTypeObject*) _JChar)
	{
		PyObject *out = Py_NotImplemented;
		Py_INCREF(out);
		return out;
	}
	return PyFloat_Type.tp_richcompare(self, other, op);
	JP_PY_CATCH(nullptr);
}

static Py_hash_t PyJPNumberLong_hash(PyObject *self)
{
	JP_PY_TRY("PyJPNumberLong_hash");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	if (!javaSlot->getClass()->isPrimitive())
	{
		jobject o = javaSlot->getJavaObject();
		if (o == nullptr)
			return Py_TYPE(Py_None)->tp_hash(Py_None);
	}
	return PyLong_Type.tp_hash(self);
	JP_PY_CATCH(0);
}

static Py_hash_t PyJPNumberFloat_hash(PyObject *self)
{
	JP_PY_TRY("PyJPNumberFloat_hash");
	JPJavaFrame frame = JPJavaFrame::outer();
	JPValue *javaSlot = PyJPValue_getJavaSlot(self);
	if (javaSlot == nullptr)
		return Py_TYPE(Py_None)->tp_hash(Py_None);
	if (!javaSlot->getClass()->isPrimitive())
	{
		jobject o = javaSlot->getJavaObject();
		if (o == nullptr)
			return Py_TYPE(Py_None)->tp_hash(Py_None);
	}
	return PyFloat_Type.tp_hash(self);
	JP_PY_CATCH(0);
}

static PyObject *PyJPBoolean_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	JP_PY_TRY("PyJPBoolean_new", type);
	if (PyTuple_Size(args) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Requires one argument");
		return nullptr;
	}
	int i = PyObject_IsTrue(PyTuple_GetItem(args, 0));
	JPClass *cls = PyJPClass_getJPClass((PyObject*) type);
	if (cls == nullptr)
	{
		PyErr_SetString(PyExc_TypeError, "Class type incorrect");
		return nullptr;
	}
	// Must use our own fixed-budget allocator, not PyLong_Type.tp_new(type,
	// ...) -- calling it with the actual subtype would dispatch to CPython's
	// own long_subtype_new, which allocates only as many digits as the
	// value 0/1 needs, not our fixed reserved budget, corrupting the
	// appended JPValue slot. See PyJPNumber_longFromLongLong's header comment.
	JPPyObject self = JPPyObject::call(PyJPNumber_longFromLongLong(type, i));
	JP_PY_CHECK();
	JPJavaFrame frame = JPJavaFrame::outer();
	JPMatch match(&frame, self.get());
	cls->findJavaConversion(match);
	jvalue val = match.convert();
	PyJPValue_assignJavaSlot(frame, self.get(), JPValue(cls, val));
	JP_TRACE("new", self.get());
	return self.keep();
	JP_PY_CATCH(nullptr);
}

static PyObject* PyJPBoolean_str(PyObject* self)
{
	JP_PY_TRY("PyJPBoolean_str", self);
	if (isNull(self))
		return Py_TYPE(Py_None)->tp_str(Py_None);
	if (PyLong_AsLong(self) == 0)
		return Py_TYPE(Py_False)->tp_str(Py_False);
	return Py_TYPE(Py_True)->tp_str(Py_True);
	JP_PY_CATCH(nullptr);
}

static PyObject *PyJPNumber_initSubclass(PyObject *cls, PyObject* args, PyObject *kwargs)
{
        Py_RETURN_NONE;
}

static PyMethodDef numberMethods[] = {
    {"__init_subclass__", (PyCFunction) PyJPNumber_initSubclass, METH_CLASS | METH_VARARGS | METH_KEYWORDS, ""},
    {0}
};


static PyType_Slot numberLongSlots[] = {
	{Py_tp_new,      (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,      (void*) &PyJPNumberLong_int},
	{Py_nb_float,    (void*) &PyJPNumberLong_float},
	{Py_tp_str,      (void*) &PyJPNumberLong_str},
	{Py_tp_repr,     (void*) &PyJPNumberLong_repr},
	{Py_tp_hash,     (void*) &PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberLong_Type = nullptr;
PyType_Spec numberLongSpec = {
	"_jpype._JNumberLong",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberLongSlots
};

static PyType_Slot numberFloatSlots[] = {
	{Py_tp_new,      (void*) &PyJPNumber_new},
	{Py_tp_getattro, (void*) &PyJPValue_getattro},
	{Py_tp_setattro, (void*) &PyJPValue_setattro},
	{Py_nb_int,      (void*) &PyJPNumberFloat_int},
	{Py_nb_float,    (void*) &PyJPNumberFloat_float},
	{Py_tp_str,      (void*) &PyJPNumberFloat_str},
	{Py_tp_repr,     (void*) &PyJPNumberFloat_repr},
	{Py_tp_hash,     (void*) &PyJPNumberFloat_hash},
	{Py_tp_richcompare, (void*) &PyJPNumberFloat_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberFloat_Type = nullptr;
PyType_Spec numberFloatSpec = {
	"_jpype._JNumberFloat",
	sizeof (struct PyJPFloat),
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberFloatSlots
};

static PyType_Slot numberBooleanSlots[] = {
	{Py_tp_new,      (void*) PyJPBoolean_new},
	{Py_tp_getattro, (void*) PyJPValue_getattro},
	{Py_tp_setattro, (void*) PyJPValue_setattro},
	{Py_tp_str,      (void*) PyJPBoolean_str},
	{Py_tp_repr,     (void*) PyJPBoolean_str},
	{Py_nb_int,      (void*) PyJPNumberLong_int},
	{Py_nb_float,    (void*) PyJPNumberLong_float},
	{Py_tp_hash,     (void*) PyJPNumberLong_hash},
	{Py_tp_richcompare, (void*) PyJPNumberLong_compare},
	{Py_tp_methods,  (void*) numberMethods},
	{0}
};

PyTypeObject *PyJPNumberBool_Type = nullptr;
PyType_Spec numberBooleanSpec = {
	"_jpype._JBoolean",
	0,
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	numberBooleanSlots
};

#ifdef __cplusplus
}
#endif

void PyJPNumber_initType(PyObject* module)
{
	// Compute the fixed Long/Boolean digit-slot layout once: enough room for
	// JLONG_MAX_DIGITS digits (the offset), plus however many whole
	// digit-slots (nitems, gLongReserveItems) are needed for the actual
	// allocation to always cover offset+sizeof(JPValue), regardless of
	// alignment padding. See PyJPNumber_longFromLongLong's header comment.
	//
	// EXTRA MARGIN, WHY: jpype.JLong/JInt/JShort/JByte/JBoolean (jpype/types.py)
	// are ordinary Python subclasses of _jpype._JNumberLong/_JBoolean (`class
	// JLong(_jpype._JNumberLong, internal=True): pass`, no __slots__). Since
	// _jpype._JNumberLong itself has tp_dictoffset==0, CPython's type_new_
	// descriptors (Objects/typeobject.c) adds an implicit instance __dict__
	// to that first subclass -- and because the base has nonzero tp_itemsize
	// (it's a variable-length int), CPython does NOT grow tp_basicsize by a
	// dict *pointed to by a fixed offset*; instead it sets
	// tp_dictoffset = -sizeof(PyObject*) (a NEGATIVE/dynamic offset) so the
	// dict pointer resolves at runtime to
	// (this_type's grown tp_basicsize) + Py_SIZE(obj)*itemsize - sizeof(PyObject*)
	// == PyLong_Type.tp_basicsize + Py_SIZE(obj)*itemsize
	// i.e. immediately after however many digits THIS PARTICULAR boxed value
	// actually has (Py_SIZE, 0..JLONG_MAX_DIGITS). We always allocate/reserve
	// a fixed JLONG_MAX_DIGITS-digit budget regardless of the real value, so
	// this dynamic dict slot can land anywhere in
	// [PyLong_Type.tp_basicsize, PyLong_Type.tp_basicsize + JLONG_MAX_DIGITS*itemsize + sizeof(PyObject*))
	// -- our own appended JPValue must start at or past the far end of that
	// whole window, not just past the digit budget itself, or assignJavaSlot
	// silently overwrites live bytes of that dict pointer (a rare, version-
	// sensitive CPython heap-layout hazard: newer CPython's managed-dict
	// scheme, Py_TPFLAGS_MANAGED_DICT, avoids this entirely by not touching
	// tp_basicsize/tp_dictoffset at all -- confirmed no reproduction on
	// 3.12 -- but pre-3.11-style dict slots hit it whenever a boxed value
	// needs the full digit budget, e.g. JLong(2**61)).
	auto base = (size_t) PyLong_Type.tp_basicsize;
	auto itemsize = (size_t) PyLong_Type.tp_itemsize;
	size_t rawNeeded = base + (size_t) JLONG_MAX_DIGITS * itemsize + sizeof (PyObject*);
	size_t longOffset = PyJPNumber_alignUp(rawNeeded, alignof (JPValue));
	size_t totalNeeded = longOffset + sizeof (JPValue);
	size_t extraBytes = totalNeeded - base;
	gLongReserveItems = (Py_ssize_t) ((extraBytes + itemsize - 1) / itemsize);

	JPPyObject bases = JPPyTuple_Pack(&PyLong_Type, PyJPObject_Type);
	PyJPNumberLong_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberLongSpec, bases.get(), (Py_ssize_t) longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JNumberLong", (PyObject*) PyJPNumberLong_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	bases = JPPyTuple_Pack(&PyFloat_Type, PyJPObject_Type);
	PyJPNumberFloat_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberFloatSpec, bases.get(),
			offsetof (struct PyJPFloat, extra));
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JNumberFloat", (PyObject*) PyJPNumberFloat_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE

	// Boolean is its own family root (not a subclass of PyJPNumberLong_Type)
	// but shares the identical PyLong_Type-based layout, so it reuses the
	// same offset/reserve-items values computed above.
	bases = JPPyTuple_Pack(&PyLong_Type, PyJPObject_Type);
	PyJPNumberBool_Type = (PyTypeObject*) PyJPClass_FromSpecWithBases(&numberBooleanSpec, bases.get(), (Py_ssize_t) longOffset);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
	PyModule_AddObject(module, "_JBoolean", (PyObject*) PyJPNumberBool_Type);
	JP_PY_CHECK(); // GCOVR_EXCL_LINE
}

JPPyObject PyJPNumber_create(JPJavaFrame &frame, JPPyObject& wrapper, const JPValue& value)
{
	JPContext *context = PyJPModule_getContext();
	// Bools are not numbers in Java
	if (value.getClass() == context->_java_lang_Boolean)
	{
		jlong l = 0;
		if (value.getValue().l != nullptr)
			l = frame.CallBooleanMethodA(value.getJavaObject(), context->_java_lang_Boolean->m_BooleanValueID, nullptr);
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyLong_Type))
	{
		jlong l = 0;
		if (value.getValue().l != nullptr)
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallLongMethodA(value.getJavaObject(), jb->m_LongValueID, nullptr);
		}
		return JPPyObject::call(PyJPNumber_longFromLongLong((PyTypeObject*) wrapper.get(), l));
	}
	if (PyObject_IsSubclass(wrapper.get(), (PyObject*) & PyFloat_Type))
	{
		jdouble l = 0;
		if (value.getValue().l != nullptr)
		{
			auto* jb = dynamic_cast<JPBoxedType*>( value.getClass());
			l = frame.CallDoubleMethodA(value.getJavaObject(), jb->m_DoubleValueID, nullptr);
		}
		return JPPyObject::call(newFloatFixed((PyTypeObject*) wrapper.get(), l));
	}
	JP_RAISE(PyExc_TypeError, "unable to convert");  //GCOVR_EXCL_LINE
}
