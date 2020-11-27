#include "jpype.h"
#include "pyjp.h"
#include "epypj.h"
#include "jp_classloader.h"
#include "jp_reference_queue.h"
#include "jp_boxedtype.h"
#include <object.h>

// Global resources
jclass ejp_managerClass;
jobject ejp_manager;
jmethodID ejp_getWrapper;
PyObject* ejp_typedict = NULL;

// Protocols (not currently expandable)
jobject ejp_numberProtocol;
jobject ejp_sequenceProtocol;
jobject ejp_mappingProtocol;
jobject ejp_iterableProtocol;
jobject ejp_iteratorProtocol;
jobject ejp_bufferProtocol;
jobject ejp_descriptorProtocol;
jobject ejp_callableProtocol;
jobject ejp_containerProtocol;
jobject ejp_sizedProtocol;

JPPyObject ejp_mappingType;

jclass EJP_CreateJavaWrapper(JPJavaFrame &frame, const string& moduleName, const char* typeName, jobjectArray a);

EJPClass::EJPClass(JPJavaFrame& frame, jclass wrapper)
{
	m_Wrapper = wrapper;
	m_Allocator = frame.GetStaticMethodID(wrapper, "_allocate", "(J)Ljava/lang/Object;");
}

EJPClass::~EJPClass()
{
	JPContext *context = JPContext_global;
	if (!context->isRunning())
		return;
	JPJavaFrame frame = JPJavaFrame::outer(context);
	frame.DeleteGlobalRef(m_Wrapper);
}

void EJPClass::destroy(PyObject *wrapper)
{
	EJPClass *cls = (EJPClass*) PyCapsule_GetPointer(wrapper, NULL);
	delete cls;
}

/** Set up resource required for Java to handle Python objects.
 *
 * This is called as part of the _jpype start up sequence.
 */
void EJP_Init(JPJavaFrame &frame)
{
	// FIXME allocate weak dict
	ejp_managerClass = frame.getContext()->getClassLoader()->findClass(frame, "org.jpype.python.PyTypeManager");
	ejp_managerClass = (jclass) frame.NewGlobalRef((jobject) ejp_managerClass);
	jmethodID get = frame.GetStaticMethodID(ejp_managerClass, "getInstance", "()Lorg/jpype/python/PyTypeManager;");
	ejp_manager = frame.CallStaticObjectMethodA(ejp_managerClass, get, NULL);
	ejp_manager = frame.NewGlobalRef( ejp_manager);
	ejp_getWrapper = frame.GetMethodID(ejp_managerClass, "getWrapper",
			"(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/Class;)Ljava/lang/Class;");

	// Define the protocols
	ejp_numberProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.number", NULL));
	ejp_sequenceProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.sequence", NULL));
	ejp_mappingProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.mapping", NULL));
	ejp_iterableProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.iterable", NULL));
	ejp_iteratorProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.iterator", NULL));
	ejp_bufferProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.buffer", NULL));
	ejp_descriptorProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.descriptor", NULL));
	ejp_callableProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.callable", NULL));
	ejp_containerProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.container", NULL));
	ejp_sizedProtocol = frame.NewGlobalRef(EJP_CreateJavaWrapper(frame, "builtins", "protocol.sized", NULL));

	PyObject *typing = PyImport_AddModule("jpype.protocol");
	ejp_mappingType = JPPyObject::call(PyObject_GetAttrString(typing, "Mapping"));

	// Create a weak dictionary for holding wrappers.
	//   This must be a weak dict so that we don't cause dynamic types to
	//   leak in Python.
	PyObject *weakref = PyImport_AddModule("weakref");
	JPPyObject dict = JPPyObject::call(PyObject_GetAttrString(weakref, "WeakKeyDictionary"));
	JPPyObject args = JPPyObject::call(PyTuple_New(0));
	ejp_typedict = PyObject_Call(dict.get(), args.get(), NULL);

	// Push singletons over to Java
	EJP_ToJava(frame, Py_None, 0);
	EJP_ToJava(frame, Py_True, 0);
	EJP_ToJava(frame, Py_False, 0);
	EJP_ToJava(frame, Py_Ellipsis, 0);
}

jclass EJP_CreateJavaWrapper(JPJavaFrame &frame, const string& moduleName, const char* typeName, jobjectArray a)
{
	jvalue v[3];
	v[0].l = frame.NewStringUTF(moduleName.c_str());
	v[1].l = frame.NewStringUTF(typeName);
	v[2].l = (jobject) a;
	return (jclass) frame.NewGlobalRef(frame.CallObjectMethodA(ejp_manager, ejp_getWrapper, v));
}

bool EJP_HasPyType(PyTypeObject *type)
{
	return PyObject_GetItem(ejp_typedict, (PyObject*) type) != NULL;
}

EJPClass *EJP_GetClass(JPJavaFrame &frame, PyTypeObject *type)
{
	printf("EJP_GetClass %s\n", type->tp_name);
	// Check for an existing wrapper using the cache.
	PyObject *capsule = PyObject_GetItem(ejp_typedict, (PyObject*) type);
	if (capsule != NULL)
	{
		EJPClass *cls = (EJPClass*) PyCapsule_GetPointer(capsule, NULL);
		JP_PY_CHECK();
		return cls;
	}
	PyErr_Clear();

	// Check the mro to make sure all base classes have wrappers
	Py_ssize_t sz = PyTuple_Size(type->tp_mro);
	for (Py_ssize_t i = 0; i < sz; ++i)
	{
		PyObject *item = PyTuple_GetItem(type->tp_mro, i);
		if (item != (PyObject*) type && PyDict_GetItem(ejp_typedict, item) == NULL)
		{
			// Not found, then construct it first.
			EJP_GetClass(frame, (PyTypeObject*) item);
		}
	}

	// Check for protocols
	vector<jobject> protocols;
	bool isDict =  PyType_FastSubclass(type, Py_TPFLAGS_DICT_SUBCLASS);
	if (type->tp_as_number && (type->tp_as_number->nb_index || type->tp_as_number->nb_int
			|| type->tp_as_number->nb_float))
		protocols.push_back(ejp_numberProtocol);
	if (!isDict && type->tp_as_sequence && type->tp_as_sequence->sq_item != NULL)
		protocols.push_back(ejp_sequenceProtocol);
	if (type->tp_as_mapping && type->tp_as_mapping->mp_subscript)
	{
		// it may be a mapping, but check with Python
		if (PyObject_IsSubclass((PyObject*) type, ejp_mappingType.get()))
			protocols.push_back(ejp_mappingProtocol);
	}
	// These two share a slot so we look for next to see if iterable vs iterator
	if (type->tp_iternext && type->tp_iternext != &_PyObject_NextNotImplemented)
		protocols.push_back(ejp_iteratorProtocol);
	else if (type->tp_iter)
		protocols.push_back(ejp_iterableProtocol);
	if (type->tp_as_buffer && type->tp_as_buffer->bf_getbuffer)
		protocols.push_back(ejp_bufferProtocol);
	if (type->tp_descr_get || type->tp_descr_set != NULL)
		protocols.push_back(ejp_descriptorProtocol);
	if (type->tp_call)
		protocols.push_back(ejp_callableProtocol);
	if (type->tp_as_sequence && type->tp_as_sequence->sq_contains)
		protocols.push_back(ejp_containerProtocol);
	if ((type->tp_as_sequence && type->tp_as_sequence->sq_length) ||
			(type->tp_as_mapping && type->tp_as_mapping->mp_length))
		protocols.push_back(ejp_sizedProtocol);

	// Convert the types to a Java array
	jobjectArray  jprotocols = frame.NewObjectArray(sz - 1 + protocols.size(),
			frame.getContext()->_java_lang_Class->getJavaClass(), NULL);
	jsize j = 0;
	for (Py_ssize_t i = 0; i < sz; ++i)
	{
		PyObject *item = PyTuple_GetItem(type->tp_mro, i);
		if (item == (PyObject*) type)
			continue;

		// Get the wrapper
		PyObject *wrapper = PyObject_GetItem(ejp_typedict, item);
		EJPClass *cls = (EJPClass*) PyCapsule_GetPointer(wrapper, NULL);

		// Copy to Java
		frame.SetObjectArrayElement(jprotocols, j++, (jobject) cls->m_Wrapper);
	}

	// Add the protocols
	for (size_t k = 0; k < protocols.size(); ++k)
	{
		frame.SetObjectArrayElement(jprotocols, j++, protocols[k]);
	}

	// Create a Java wrapper class
	JPPyObject pyModuleName = JPPyObject::call(PyObject_GetAttrString((PyObject*) type, "__module__"));
	string moduleName = JPPyString::asStringUTF8(pyModuleName.get());
	jclass wrapper = EJP_CreateJavaWrapper(frame, moduleName, type->tp_name, jprotocols);
	EJPClass *cls = new EJPClass(frame, wrapper);

	// Place it in a capsule for reuse.
	JPPyObject ecapsule = JPPyObject::call(PyCapsule_New(cls, NULL, EJPClass::destroy));
	if (PyObject_SetItem(ejp_typedict, (PyObject*) type, ecapsule.get()) == -1)
		JP_PY_CHECK();

	// Return it to Python
	return cls;
}
/**
 *
 * @param frame
 * @param obj
 * @return
 */
jobject EJP_ToJava(JPJavaFrame& frame, PyObject *obj, int flags)
{
	if (obj == NULL)
	{
		if (flags & 1)
			return (jobject) 0;
		JP_RAISE_PYTHON();
	}

	JPValue *value = PyJPValue_getJavaSlot(obj);
	if (value != NULL)
	{
		JPClass *cls = value->getClass();
		if (cls->isPrimitive())
		{
			JPBoxedType *boxed =  (JPBoxedType *) ((JPPrimitiveType*) cls)
					->getBoxedClass(frame.getContext());
			return boxed->box(frame, value->getValue());
		}
		return value->getValue().l;
	}

	// Create a wrapper for Python object.
	EJPClass *cls = EJP_GetClass(frame, Py_TYPE(obj));
	jvalue val;
	val.j = (jlong) obj;
	jobject ret = frame.CallStaticObjectMethodA(cls->m_Wrapper, cls->m_Allocator, &val);
	return ret;
}

/**
 *
 * @param frame
 * @param obj
 * @return
 */
JPPyObject EJP_ToPython(JPJavaFrame& frame, jobject obj)
{
	if (obj == NULL)
		return JPPyObject::getNone();
	JPClass *cls = frame.findClassForObject(obj);
	jvalue value;
	value.l = obj;
	return cls->convertToPythonObject(frame, value, true);
}
