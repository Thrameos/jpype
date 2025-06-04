/*
 * PURPOSE: Experimental JPype Object Model with Python-Owned Memory
 *
 * This module experiments with a new JPype object model in which all memory for
 * Java proxy objects is managed entirely within Python's object system.
 *
 * Key Points:
 * - The goal is to derive from many concrete Python types (primitives, boxed types, exceptions)
 *   while preserving the Java inheritance tree.
 * - This is only possible because java.lang.Object and Java interfaces are represented
 *   as empty objects with no members.
 * - However, we must also support creating instances of java.lang.Object and interfaces.
 *   To achieve this, a derived concrete class is created as a substitute whenever
 *   instantiation is required.
 * - Allocation of both the concrete and abstract class occurs when the abstract class is declared.
 * - Each required Python base type (long, float, exception, str) is handled by deriving a
 *   corresponding proxy type, using a Python heap type extension to track the offset of
 *   extra memory.
 * - PyLongObject is a special case: since it is completely concrete and variable-length,
 *   we use a custom allocator to add our extra memory in the item list space after the
 *   normal digits section.
 *
 * Background and Motivation:
 * - Up to Python 3.11, JPype could rely on standard alloc/free slots for memory management.
 *   Python 3.12+ introduced changes that broke these assumptions for performance reasons.
 * - This new approach relocates all memory management into Python's object space, making
 *   the system more robust in terms of Python's awareness, but also requiring several
 *   workarounds ("hacks") to maintain compatibility between the Java and Python type systems.
 * - As a result, while this design is more compliant with Python's evolving internals,
 *   it may be more susceptible to future changes in Python's memory model.
 *
 * For further details on memory extension for variable-length types, see the comment in
 * the "long section" below.
 */
#include <Python.h>
#include <stddef.h>

// java.lang.Object -1     (this is special because we can't change basesize so we must add an extra object)
// +--java.lang.Throwable  offset in PyException
// +--java.lang.Number ?
// |  +--java.lang.Integer offset in PyLong
// |  +--java.lang.Double  offset in PyFloat
// |  +--java.lang.*       same
// +--all other            offset 
// (interfaces)            from the derived class 

// TODO fill out all the required methods of things like comparable
// TODO add subtype checks using fast lookup as testing for class instances and interfaces
// will be outside of the Python system.

// For the header pyjp.h

PyObject *module;
PyObject* PyJPClass_Type = NULL;
PyObject* PyJPObject_Type = NULL;
PyObject* PyJPException_Type = NULL;
PyObject* PyJPLong_Type = NULL;
PyObject* PyJPFloat_Type = NULL;
PyObject* PyJPChar_Type = NULL;
PyObject* _magic = NULL;

static inline size_t align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

typedef struct {
    PyHeapTypeObject ht_object;
    void* jcls;              // Our JClass pointer
    void* extra;             // jvalue for Java
    int offset;              // Location of our custom slot.  (-1 indicates that the slot will need to be bolted on by the allocator)
    PyTypeObject* other;  // an automatically created concrete baseclass 
} PyJPClass;

// PyJPObject and PyJPLong don't exist as they are same layout as Python objects

typedef struct 
{
    PyFloatObject base;  // Base type (int)
    void* extra;          // Additional field
} PyJPFloat;

typedef struct 
{
    PyUnicodeObject base;  // Base type (unicode)
    void* extra; // Additional field
} PyJPChar;

typedef struct
{
    PyBaseExceptionObject base;  // Base type (int)
    void* extra;          // Additional field
} PyJPException;


extern void PyJPValue_free(void* obj);
extern void* PyJPValue_getClass(PyObject* obj);
extern void PyJPValue_setClass(PyObject* obj, void* jcls);
extern void* PyJPValue_getValue(PyObject* obj);
extern void PyJPValue_setValue(PyObject* obj, void* value);
extern PyObject* PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases, int offset);

extern void PyJPClass_initType(PyObject* mod);
extern void PyJPObject_initType(PyObject* mod);
extern void PyJPNumber_initType(PyObject* mod);
extern void PyJPChar_initType(PyObject* mod);
extern void PyJPException_initType(PyObject* mod);

//**************************************************************************
// for pyjp_value.cpp

/** 
 * Free the object, ensuring proper finalization and memory management.
 *
 * This function:
 *  - Calls the type's tp_finalize method if present (for custom cleanup).
 *  - Frees the object using the appropriate memory pool, depending on whether
 *    it participates in Python's garbage collection.
 *
 * In practice, all our objects are heap-allocated, but this covers both GC and
 * non-GC objects for safety and compatibility.
 *
 * @param obj Pointer to the object to free (must not be NULL).
 */
void PyJPValue_free(void* obj)
{
    assert(obj != NULL);
    // Normally finalize is not run on simple classes.
    PyTypeObject *type = Py_TYPE(obj);
    if (type->tp_finalize != NULL)
        type->tp_finalize((PyObject*) obj);
    if (type->tp_flags & Py_TPFLAGS_HAVE_GC)
        PyObject_GC_Del(obj);
    else
        PyObject_Free(obj);  // GCOVR_EXCL_LINE
}

/**
 * Retrieve the Java class (jclass) pointer associated with a Python object or type.
 *
 * This function serves two primary use cases:
 *   1. **Instance Path:** When `self` is an instance of a Java proxy type
 *      (i.e., its type is `PyJPClass_Type`), this function returns the
 *      `jcls` pointer stored in the type object.
 *   2. **Metaclass Path:** When `self` is the type object itself
 *      (i.e., `self == PyJPClass_Type`), this function returns a sentinel
 *      value to represent `java.lang.Class`. This is necessary because
 *      `java.lang.Class` is mapped to the Python metatype.
 *
 * If neither condition is met, the function returns `NULL` to indicate
 * that the object is not a Java proxy.
 *
 * @param self  The Python object or type to probe (must not be NULL).
 * @return      The associated jclass pointer, a sentinel for java.lang.Class,
 *              or NULL if not a Java proxy.
 *
 * @note        This function is on a critical path and should remain minimal.
 *              The dual handling of instance and metaclass is required for
 *              correct mapping of Java's class system to Python's.
 */
void* PyJPValue_getClass(PyObject* self)
{
    assert(self != NULL);
    PyTypeObject *type = Py_TYPE(self);
    if (type == (PyTypeObject*) PyJPClass_Type)
        return ((PyJPClass*)type)->jcls;
    if (self == PyJPClass_Type)
        return (void*) 123; // We are java.lang.Class
    return NULL;
}

/**
 * Set the Java class (jclass) pointer associated with a Python object or type.
 *
 * This function sets the `jcls` field for a Java proxy type instance.
 * It does nothing if the object is not a Java proxy type.
 *
 * @param self  The Python object or type to update (must not be NULL).
 * @param jcls  The Java class pointer to associate.
 *
 * @note        Only updates the jclass for valid Java proxy types.
 */
void PyJPValue_setClass(PyObject* self, void* jcls)
{
    assert(self != NULL);
    PyTypeObject *type = Py_TYPE(self);
    if (type == (PyTypeObject*) PyJPClass_Type)
        ((PyJPClass*)type)->jcls =jcls;
}

/**
 * Retrieve the Java value (jvalue) associated with a Python object or type.
 *
 * This function serves two primary use cases:
 *   1. **Instance Path:** If `self` is an instance of a Java proxy type
 *      (i.e., its type is `PyJPClass_Type`), the function computes the offset
 *      (as stored in the type object) to access the hidden memory slot
 *      containing the associated `jvalue`.
 *   2. **Metaclass Path:** If `self` is the type object itself
 *      (i.e., `self == PyJPClass_Type`), the function returns the `extra`
 *      field from the type object. This is used for the metaclass mapping
 *      to `java.lang.Class`.
 *
 * If neither condition is met, the function returns `NULL` to indicate
 * that the object is not a Java proxy.
 *
 * @param self  The Python object or type to probe (must not be NULL).
 * @return      The associated jvalue pointer, or NULL if not a Java proxy.
 *
 * @note        The offset-based access is required to maintain compatibility
 *              with Python's memory layout for variable-sized objects.
 */
void* PyJPValue_getValue(PyObject* self)
{
    assert(self != NULL);
    PyTypeObject *type = Py_TYPE(self);
    if (type == (PyTypeObject*) PyJPClass_Type)
    {
        int offset = ((PyJPClass*)type)->offset;
        void** extra = (void**)((char*)self + offset);
        return *extra;
    }
    if (self == PyJPClass_Type)
        return ((PyJPClass*)self)->extra;
    return NULL;
}

/**
 * Set the Java value (jvalue) associated with a Python object or type.
 *
 * For variable-length proxy types (such as PyJPLong), the extra memory is
 * always allocated at the maximum size, ensuring that the offset to the
 * extra field is fixed and does not require runtime calculation.
 *
 * This function serves two primary use cases:
 *   1. **Instance Path:** If `self` is an instance of a Java proxy type
 *      (i.e., its type is `PyJPClass_Type`), the function computes the offset
 *      (as stored in the type object) and stores the `value` pointer in the
 *      hidden memory slot.
 *   2. **Metaclass Path:** If `self` is the type object itself
 *      (i.e., `self == PyJPClass_Type`), the function stores the `value`
 *      pointer in the `extra` field of the type object.
 *
 * If neither condition is met, the function does nothing.
 *
 * @param self   The Python object or type to update (must not be NULL).
 * @param value  The Java value pointer to associate.
 *
 * @note        For variable-length objects, the extra memory is always
 *              allocated at the maximum size to guarantee a fixed offset.
 *              This simplifies access logic and avoids runtime checks.
 */
void PyJPValue_setValue(PyObject* self, void* value)
{
    PyTypeObject *type = Py_TYPE(self);
    if (type == (PyTypeObject*) PyJPClass_Type) 
    {
        int offset = ((PyJPClass*)type)->offset;
        void** extra = (void**)((char*)self + offset);
        *extra = value;
    }
    else if (self == PyJPClass_Type)
        ((PyJPClass*)self)->extra = value;
}


/** Prototype testing code.
 */
PyObject* PyJPValue_setSlot(PyObject* self, PyObject* value)
{
    PyTypeObject* type = Py_TYPE(self);
    PyTypeObject* meta = Py_TYPE(type);
    if ( meta != (PyTypeObject*) PyJPClass_Type)
    {
        PyErr_SetString(PyExc_TypeError, "Bad meta type.");
        return NULL;
    }
    long ptr = PyLong_AsLong(value);
    if (PyErr_Occurred())
        return NULL;
    PyJPClass *cls = (PyJPClass*) type;
    if (cls->offset != -1) 
    {
printf("   set slot %d\n", cls->offset);
        void** extra = (void**)((char*)self + cls->offset);
        *extra = (void*) ptr;
    }
    Py_RETURN_NONE;
}

/** Prototype testing code.
 */
PyObject* PyJPValue_getSlot(PyObject* self) 
{
    PyTypeObject* type = Py_TYPE(self);
    PyTypeObject* meta = Py_TYPE(type);
    if ( meta != (PyTypeObject*) PyJPClass_Type)
    {
        PyErr_SetString(PyExc_TypeError, "Bad meta type.");
        return NULL;
    }
    PyJPClass *cls = (PyJPClass*) type;
    if (cls->offset != -1)
    {
printf("   get slot %d\n", cls->offset);
        void** extra = (void**)((char*)self + cls->offset);
        return PyLong_FromLong( (long) *extra);
    }
    Py_RETURN_NONE;
}

/** Clean up proxy references.
 */
void PyJPValue_finalize(PyObject* obj) {
    printf("finalize\n");
  // FIXME release the global reference here.
}

/**
 * Traverse function for Java proxy objects to support Python's garbage collection.
 *
 * Although most Java proxy objects do not directly hold Python objects or resources
 * requiring garbage collection, they must still participate in the GC traversal
 * process. This is essential for detecting and breaking potential reference cycles
 * between Java and Python objects, especially when integrating with Java's own
 * memory management.
 *
 * @param self   The proxy object being traversed.
 * @param visit  The visit function provided by the garbage collector.
 * @param arg    Additional argument for the visit function.
 * @return       0 on success, or a non-zero error code if traversal fails.
 *
 * @note         While having additional GC-tracked objects may seem wasteful,
 *               it is necessary to ensure correct memory management and
 *               reference cycle detection across the Python-Java boundary.
 * @note         Subclasses with actual Python-owned resources must override
 *               this method to ensure proper GC behavior.
 */
int PyJPValue_traverse(PyObject* self, visitproc visit, void* arg) {
    return 0;
}


//**************************************************************************
// for pyjp_class.cpp
// Class

/**
 * Create a concrete subclass for a Java/Python proxy abstract type.
 *
 * This helper function creates a new concrete type that directly inherits
 * from the provided abstract type. It is used to implement the dual type
 * system required for Java/Python integration, where some Java types
 * (such as interfaces or abstract classes) require a concrete subclass
 * for instantiation in Python.
 *
 * The new type is constructed using the custom metaclass (PyJPClass_Type),
 * with the abstract type as its sole base. The function passes a unique
 * sentinel object (`_magic`) as the keyword arguments to the metaclass
 * constructor. This sentinel signals to the metaclass/type initializer
 * that this is a special creation path for a concrete implementation,
 * and enables the metaclass to apply custom initialization logic.
 *
 * @param abstract_type  The abstract base type.
 * @return               0 on success, -1 on failure (with Python exception set).
 */
static int jclass_concrete(PyTypeObject *abstract_type)
{
    PyObject* bases = PyTuple_Pack(1, abstract_type);
    if (bases == NULL)
        return -1;
    PyObject* args = Py_BuildValue("sNN", abstract_type->tp_name, bases, PyDict_New());
    if (args == NULL)
        return -1;
    PyObject* self = ((PyTypeObject*)PyJPClass_Type)->tp_call(PyJPClass_Type, args, _magic);
    Py_DECREF(args);
    if (self == NULL)
        return -1;
    return 0;
}

/**
 * Create a Java Proxy class from a type specification and base classes.
 *
 * This function constructs a new proxy type that bridges Java and Python,
 * using the custom metaclass (`PyJPClass_Type`). It uses Python's
 * `PyType_FromMetaclass` API to create a new heap type with the specified
 * metaclass, module, type specification (`spec`), and base classes (`bases`).
 *
 * After type creation:
 *   - The finalize (`tp_finalize`) and free (`tp_free`) functions are set
 *     to custom implementations for proper resource management.
 *   - The instance dictionary is disabled by setting `tp_dictoffset = 0`.
 *   - For Python 3.13+, the `Py_TPFLAGS_INLINE_VALUES` flag is cleared to
 *     prevent memory layout issues with derived classes.
 *   - The `offset` field is set to indicate where extra per-instance data
 *     (such as a Java value or handle) is stored.
 *
 * If `offset == -1`, indicating an abstract type, a concrete subclass is
 * created using `jclass_concrete`. If this fails, the type is decremented
 * and `NULL` is returned.
 *
 * Note:
 *   - The function does not set a custom allocation function (`tp_alloc`).
 *   - The function does not call type initializers (`tp_init`).
 *   - This function is only used to create base proxy types.
 *
 * @param spec    The PyType_Spec describing the new type.
 * @param bases   Tuple of base classes for the new type.
 * @param offset  Offset for extra per-instance data, or -1 to trigger
 *                automatic concrete type creation.
 * @return        New type object on success, or NULL on failure.
 */
PyObject* PyJPClass_FromSpecWithBases(PyType_Spec *spec, PyObject *bases, int offset)
{
    PyTypeObject *type = (PyTypeObject*) PyType_FromMetaclass((PyTypeObject*) PyJPClass_Type, module, spec, bases);
    if (type == NULL)
        return NULL;

    // Assign custom alloc and finalize functions
    type->tp_finalize = (destructor) PyJPValue_finalize;
    type->tp_free = PyJPValue_free;
    type->tp_dictoffset = 0;  // Disable instance dictionaries

#if PY_VERSION_HEX >= 0x030d0000
    // Python 3.13 adds stuff at the end of layout on derived classes.   We must not let that happen
    type->tp_flags &= ~Py_TPFLAGS_INLINE_VALUES;
#endif
    ((PyJPClass*)type)->offset = offset;

    // If there is no abstract type then we must allocate a concrete type
    if (offset == -1 && jclass_concrete(type)==-1)
    {
       Py_DECREF(type);
       return NULL;
    }

    // This won't call the init.  
    return (PyObject*) type;
}

/**
 * Custom initializer for Java/Python proxy type objects.
 *
 * This function handles both abstract and concrete proxy type initialization,
 * orchestrating the dual-type system required for Java/Python integration.
 *
 * Key responsibilities:
 *   - Calls the base type initializer (`PyType_Type.tp_init`).
 *   - Scans base classes to inherit the memory offset for extra per-instance data,
 *     ensuring correct memory layout and compatibility with the proxy model.
 *   - Validates that the object is of the correct metaclass (`PyJPClass_Type`).
 *   - Disables instance dictionaries and managed dicts to ensure a fixed memory layout.
 *   - Assigns custom finalize and free functions for resource management.
 *   - If the special `_magic` sentinel is present (indicating concrete subclass creation),
 *     calculates and assigns a new offset for the extra memory slot, updates cross-references
 *     between abstract and concrete types, and disables further subclassing.
 *   - If the offset remains unset (`-1`), triggers creation of a concrete subclass.
 *   - (TODO) Placeholder for enforcing required Python dunder methods based on Java class capabilities.
 *
 * This function is critical for maintaining the invariants required by the
 * Java/Python proxy system, especially regarding memory layout, type safety,
 * and the distinction between abstract and concrete proxy types.
 *
 * @param self    The type object being initialized.
 * @param args    Positional arguments (unused).
 * @param kwargs  Keyword arguments; may be the `_magic` sentinel for special paths.
 * @return        0 on success, -1 on failure (with Python exception set).
 */
static int jclass_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    // This path is only used by classes derived in Python
    printf("jinit Magic %d\n", kwargs==_magic);

    // TODO if we are a Java object class (not interface) and 
    // we are not java.lang.Object or java.lang.Number, or a
    // Python concrete class we are safe to add the offset. 
    // Make sure to add the jvalue slot only once.

    // Call the base type initializer
    int result = PyType_Type.tp_init(self, args, kwargs);
    if (result < 0) 
        return -1;

    // We have a new type (may be abstract or concrete currently)
    PyTypeObject *type = (PyTypeObject*) self;

    // Iterate over the bases to find the offset slot
    // watch for concrete bases
    PyObject* bases = type->tp_bases;
    PyObject* abstract_type = NULL;
    int offset = -1;
    Py_ssize_t num_bases = PyTuple_Size(bases);
    for (Py_ssize_t i = 0; i < num_bases; i++)
    {
        PyObject *base = PyTuple_GetItem(bases, i);
        if (PyType_Check(base) && Py_TYPE(base) == (PyTypeObject *)PyJPClass_Type)
        {
            abstract_type = base;
            PyJPClass *base_tclass = (PyJPClass *)base;
            if (offset == -1) 
                offset = base_tclass->offset; // Inherit first offset
        }
    }

    // Ensure the object is of type PyJPClass
    if (Py_TYPE(self) != (PyTypeObject*)PyJPClass_Type)
    {
        PyErr_SetString(PyExc_TypeError, "Object is not of type PyJPClass.");
        return -1;
    }

    // Proxy types will have a number of special restrictions.
    // Python doesn't support all these options directly, so we are going
    // to need to hack the logic to make our heap object disguise itself
    // as static allocation.  We were using real static allocations 
    // but tp_new is no longer available to types with meta classes.
    type->tp_flags &= ~Py_TPFLAGS_MANAGED_DICT; // turn off the managed dict
    type->tp_dictoffset = 0;  // Disable instance dictionaries
    type->tp_finalize = (destructor) PyJPValue_finalize;
    type->tp_free = PyJPValue_free;

    PyJPClass *jtype = (PyJPClass*) self;
    if (kwargs == _magic)
    {
        printf("Add jslot\n");
        offset = align_up(type->tp_basicsize, sizeof(void*));
        type->tp_basicsize = offset + sizeof(void*);
        ((PyJPClass*)abstract_type)->other = type;
        ((PyJPClass*)type)->other = (PyTypeObject*) abstract_type;
        // No extension is possible
        type->tp_flags &= ~Py_TPFLAGS_BASETYPE; // turn off the managed dict
    }
    printf("jclass_init %s offset=%d\n", type->tp_name, offset);
    jtype->offset = offset;

    if (offset <0 )
    {
       // Allocate a new concrete type
       if (jclass_concrete(type) == -1)
          return -1;
    }

    // TODO special checks for required dunder methods go here.
    // if (jcls->isComparable())
    // { // add all the Python compariable dunder }

    return 0;
}

static PyObject* jclass_repr(PyObject* self)
{
    // Get the type name
    PyTypeObject* type = (PyTypeObject*) self;
    PyJPClass *tclass = (PyJPClass*) self;
    const char* type_name = type->tp_name;
    const char* abs = "";
    if (tclass->offset == -1)
        abs = "abstract ";
    return PyUnicode_FromFormat("<java %sclass %s at %p>", abs, type_name, self);
}


/** Traverse for meta class instances.
 *
 * Required for Python's cyclic GC to detect reference cycles involving type objects.
 */
static int jclass_traverse(PyJPClass* obj, visitproc visit, void *arg) 
{
    if (obj->other != NULL)
        Py_VISIT(obj->other);
    return 0;
}

// Define the type slots for the metaclass
static PyType_Slot jclass_slots[] = 
{
    {Py_tp_init, (void *)jclass_init},
    {Py_tp_base, (void*)&PyType_Type},
    {Py_tp_traverse, (void*)&jclass_traverse},
    {Py_tp_repr, (void*)&jclass_repr},
    {0, NULL}  // Sentinel
};

// Define the specification for the metaclass
static PyType_Spec jclass_spec = 
{
    .name = "_JClass",               // Name of the type
    .basicsize = sizeof(PyJPClass),         // Size of the type object
    .itemsize = 0,                      // Item size (for variable-length objects)
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE,  // Flags
    .slots = jclass_slots              // Slots
};

void PyJPClass_initType(PyObject* module)
{
    // Create the metaclass type
    PyTypeObject *baseMetaclass = &PyType_Type;  // Use 'type' as the base metaclass
    PyJPClass_Type = PyType_FromMetaclass(baseMetaclass, NULL, &jclass_spec, NULL);
    if (PyJPClass_Type != NULL)
        PyModule_AddObject(module, "_JClass", (PyObject*) PyJPClass_Type);
}


//**************************************************************************
// Object

/**
 * Allocate a new instance of a Java proxy object.
 *
 * This function serves as the tp_new slot for all types derived from PyJPClass_Type,
 * which represent Java proxy objects in Python. It enforces the following:
 *
 *  - Only types created with the custom metaclass PyJPClass_Type can be instantiated.
 *  - Abstract proxy types (where offset == -1) cannot be directly instantiated;
 *    instead, instantiation is redirected to their associated concrete type.
 *  - Allocates the object using the type's tp_alloc, ensuring proper memory management.
 *  - Initializes the extra memory slot (used for Java-side data) to NULL.
 *
 * Error handling:
 *  - If the type is not governed by PyJPClass_Type, raises TypeError.
 *  - If no concrete implementation exists for an abstract type, raises TypeError.
 *  - If memory allocation fails, returns NULL.
 *
 * Assumptions:
 *  - The type's memory layout and offset have been set up correctly by the type
 *    creation logic.
 *  - The offset is properly aligned for pointer storage.
 *
 * @param type   The Python type object to instantiate.
 * @param args   Arguments passed to the constructor (unused here).
 * @param kwargs Keyword arguments passed to the constructor (unused here).
 * @return       A new proxy object instance, or NULL on error.
 */
static PyObject* jobject_new(PyTypeObject* type, PyObject* args, PyObject* kwargs) 
{
    // Check the metaclass (if not ours abort!)
    if (Py_TYPE(type) != (PyTypeObject*)PyJPClass_Type) {
        PyErr_SetString(PyExc_TypeError, "Object is not of type PyJPClass.");
        return NULL;
    }

    PyJPClass* jtype = (PyJPClass*)type;

    // We are attempting to create an abstract object instance
    if (jtype->offset == -1)
    {
        // Swap to the concrete type
        type = jtype->other;
        jtype = (PyJPClass*) type;
        if (type == NULL || type->offset == -1) {
            PyErr_SetString(PyExc_TypeError, "Concrete implementation missing for abstract type.");
            return NULL;
        }
    }

    // If the type is a subclass of PyJPObject, proceed with normal allocation
    PyObject* self = type->tp_alloc((PyTypeObject*)type, 0);
    if (self == NULL) 
        return NULL; // Memory allocation failed

    // set up the java location
    void** extra = (void**)((char*)self + jtype->offset);
    *extra = 0;
    return self;
}

/**
 * Return the string representation of a Java proxy object.
 *
 * This function implements the tp_repr slot for Java proxy objects, providing
 * a human-readable string that identifies the object's Java type and memory address.
 *
 * Output format:
 *     <java object <type_name> at <address>>
 * where <type_name> is the Python type name (typically mapped from the Java class)
 * and <address> is the object's memory address.
 *
 * This representation aids in debugging and logging by clearly distinguishing
 * Java proxy objects from native Python objects.
 *
 * @param self  The Java proxy object instance.
 * @return      A new Unicode string object describing the proxy, or NULL on error.
 */
static PyObject* jobject_repr(PyObject* self)
{
    // Get the type name
    PyTypeObject* type = Py_TYPE(self);
    const char* type_name = type->tp_name;
    return PyUnicode_FromFormat("<java object %s at %p>", type_name, self);
}


static PyMethodDef jobject_methods[] = 
{
    {"__set_slot__", (PyCFunction) PyJPValue_setSlot, METH_O, ""},
    {"__get_slot__", (PyCFunction) PyJPValue_getSlot, METH_NOARGS, ""},
    {0}
};

static PyType_Slot jobject_slots[] = 
{
    {Py_tp_methods,  (void*) jobject_methods},
    {Py_tp_new,  (void*) &jobject_new},
    {Py_tp_finalize,  (void*) &PyJPValue_finalize},
    {Py_tp_free,  (void*) &PyJPValue_free},
    {Py_tp_traverse, (void*)&PyJPValue_traverse},  
    {Py_tp_repr, (void*)&jobject_repr},  
    {0, NULL}                          // Sentinel
};

static PyType_Spec jobject_spec =
 {
    "helloworld._JObject",
    sizeof(PyObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    jobject_slots
};

void PyJPObject_initType(PyObject* module)
{
    PyJPObject_Type = PyJPClass_FromSpecWithBases(&jobject_spec, NULL, -1);
    if (PyJPObject_Type != NULL)
        PyModule_AddObject(module, "_JObject", (PyObject*) PyJPObject_Type);
}

//**************************************************************************
// Split file to pyjp_number.cpp

/*
 * WHY EXTRA MEMORY IS REQUIRED FOR EXTENDING PyLongObject
 *
 * PyLongObject is a variable-length type with a strict, fixed memory layout
 * defined by CPython. Any attempt to add fields directly to the struct or
 * subclass it with additional members will break compatibility with Python's
 * memory management, leading to crashes or memory corruption.
 *
 * The only safe way to associate extra data with a PyLongObject instance is to
 * allocate extra memory after the object using a custom allocator and store
 * pointers at a known offset. This preserves the expected memory layout for
 * Python internals while allowing per-instance extension.
 *
 * Alternatives (such as wrapper types or struct modification) are not viable
 * for variable-length types due to these constraints.
 */


#define SAFETY ((void*)0x123456789ABCDEFl)

/**
 * Allocate and initialize a new Java proxy integer object (PyJPLong).
 *
 * This function serves as the tp_new slot for the PyJPLong type, which is a
 * subclass of Python's PyLongObject with additional hidden memory for Java integration.
 * It ensures that the memory layout is compatible with both Python's and Java's
 * expectations, and that the extra slot for Java values is always at a predictable,
 * aligned offset.
 *
 * Key responsibilities:
 *   - Allocates a PyLongObject of the correct size, ensuring space for both the
 *     required Python integer digits and the extra pointer for Java data.
 *   - Handles differences in PyLong memory layout between Python versions (e.g.,
 *     use of lv_tag in 3.12+ vs. ob_size in earlier versions).
 *   - Enforces a digit limit (currently 16 bytes) to guarantee that the extra slot
 *     can always be placed at a fixed offset after the digits.
 *   - Aligns the extra slot to pointer boundaries for portability.
 *   - Copies the base PyLongObject’s data into the new allocation, then sets the
 *     type pointer to the custom type.
 *   - Initializes the extra slot to NULL (or a known safety value for debugging).
 *   - Returns the new object, or raises an exception and returns NULL on error.
 *
 * Error handling:
 *   - Raises ValueError if the integer is too large to fit within the digit limit.
 *   - Raises SystemError if memory alignment or copying fails.
 *   - Returns NULL on allocation failure.
 *
 * Design notes:
 *   - This function is tightly coupled to the internals of CPython’s PyLongObject.
 *     Any changes to the core CPython integer implementation may require updates here.
 *   - The digit limit and alignment logic are critical for preventing memory
 *     corruption and ensuring safe interoperability with Java.
 *   - The function intentionally avoids using Python-level dynamic attributes or
 *     dictionaries to preserve the tight memory layout.
 *
 * @param type   The type object (should be PyJPLong_Type or a subclass).
 * @param args   Tuple of arguments passed to the constructor.
 * @param kwargs Dictionary of keyword arguments (unused).
 * @return       A new PyJPLong object, or NULL on error.
 */
static PyObject* PyJPLong_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    PyLongObject* base_value = (PyLongObject*) PyLong_Type.tp_new((PyTypeObject*)&PyLong_Type, args, kwargs);
    if (base_value == NULL)
        return NULL; // Failed to allocate base value

    size_t sz = 0;
#if PY_VERSION_HEX >= 0x030c0000
    // Python 3.12 and later: Use lv_tag to extract size
    PyLongObject* long_obj = (PyLongObject*)base_value;
    sz = (long_obj->long_value.lv_tag) >> 3;  // Extract size from lv_tag
#else
    // Pre-3.12: Use ob_size (correct for negative values)
    Py_ssize_t ob_size = Py_SIZE(base_value);
    sz = (ob_size < 0) ? (size_t)(-ob_size) : (size_t)ob_size;  // Take the absolute value
#endif

    // We will have a digit limit
    if ((1+sz)*sizeof(digit)>16)
    {
        PyErr_SetString(PyExc_ValueError, "Range exceeded");
        return NULL;
    }

    // Allocate memory for the custom type (we always need to select maximum digits so we can add our memory after)
    size_t basicsize = type->tp_basicsize;
    size_t itemsize = type->tp_itemsize;
    size_t desired_size = basicsize + (16/sizeof(digit)-1) * itemsize;

    // Ensure alignment for a pointer
    size_t alignment = sizeof(void*);
    size_t aligned_size = align_up(desired_size + sizeof(void*), alignment);
    Py_ssize_t adjusted_nitems = (aligned_size - basicsize) / itemsize;
    PyLongObject* self = (PyLongObject*)type->tp_alloc(type, adjusted_nitems);
    if (self == NULL)
    {
        Py_DECREF(base_value);
        return NULL; // Memory allocation failed
    }

    // Calculate the total size of the PyLongObject
    size_t total_size = PyLong_Type.tp_basicsize + 16;

    // Compute aligned memory address
    uintptr_t unaligned_address = (uintptr_t)self + total_size;
    void** aligned_address = (void**) align_up(unaligned_address, alignment);

    *aligned_address = SAFETY;

    // Copy over the base memory (erases the type pointer)
#if PY_VERSION_HEX >= 0x030c0000
    // Python 3.12 and later: Do not copy the immortality bit
    if (sz == 0)
        sz = 1;
    self->long_value.lv_tag = base_value->long_value.lv_tag & ~(1<<2);
    for (size_t i = 0; i < sz; i++) {
        self->long_value.ob_digit[i] = base_value->long_value.ob_digit[i];
    }
#else
    memcpy(self, base_value, sizeof(PyLongObject)+type->tp_itemsize*sz);
#endif
    Py_SET_TYPE(self, type);      // Assign the new type manually
    Py_DECREF(base_value);

    // We need to catch this in runtime.  There may be systems
    // with broken things like 16 bit digits we can't easily 
    // test on.  There is no harm in being safe.
    if (*aligned_address != SAFETY)
    {
        PyErr_SetString(PyExc_SystemError, "Memory intrusion");
        return NULL;
    }

#if 0
    for (size_t i=0; i<aligned_size; ++i) {
        if (i ==basicsize)
            printf("| ");
        else if (i%8 ==0)
            printf("^ ");
        printf("%02x ", 0xff&((char*)self)[i]);
    }
    printf("\n");
#endif
    // This will be our jvalue
    *aligned_address = 0;

    return (PyObject*)self;
}


// Function that will be exposed to Python
static PyType_Slot longSlots[] = 
{
    {Py_tp_base, (void*)&PyLong_Type},
    {Py_tp_new, (void*)&PyJPLong_new},
    {Py_tp_traverse, (void*)&PyJPValue_traverse},  
    {Py_tp_finalize,  (void*) &PyJPValue_finalize},
    {Py_tp_free,  (void*) &PyJPValue_free},
    {0, NULL}                          // Sentinel
};

// Define the type specification
static PyType_Spec longSpec = 
{
    .name = "helloworld._JInt",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = longSlots,
};

// Function that will be exposed to Python
static PyType_Slot jfloat_slots[] = 
{
    {Py_tp_base, (void*)&PyFloat_Type},  // Derive from int (PyLong_Type)
    {Py_tp_traverse, (void*)&PyJPValue_traverse},  
    {Py_tp_finalize,  (void*) &PyJPValue_finalize},
    {Py_tp_free,  (void*) &PyJPValue_free},
    {0, NULL}                          // Sentinel
};

// Define the type specification
static PyType_Spec jfloat_spec = 
{
    .name = "helloworld.PyJPFloat",
    .basicsize = sizeof(PyJPFloat),  
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = jfloat_slots,
};

void PyJPNumber_initType(PyObject* module)
{
    PyObject *bases = PyTuple_Pack(2, (PyObject*)&PyLong_Type, (PyObject*) PyJPObject_Type );
    if (bases == NULL)
        return;
    size_t alignment = sizeof(void*);
    size_t total_size = PyLong_Type.tp_basicsize + 16;
    size_t roffset = align_up(total_size, alignment);
    PyJPLong_Type = PyJPClass_FromSpecWithBases(&longSpec, bases, roffset);
    Py_DECREF(bases);
    if (PyJPLong_Type != NULL)
        PyModule_AddObject(module, "_JInt", (PyObject*) PyJPLong_Type);

    bases = PyTuple_Pack(2, (PyObject*)&PyFloat_Type, (PyObject*) PyJPObject_Type);
    if (bases == NULL)
        return;
    PyJPFloat_Type = PyJPClass_FromSpecWithBases(&jfloat_spec, bases, offsetof(PyJPFloat, extra));
    Py_DECREF(bases);
    if (PyJPFloat_Type != NULL)
        PyModule_AddObject(module, "_JFloat", (PyObject*) PyJPFloat_Type);
}

//**************************************************************************
// Split file to pyjp_char.cpp

// Function that will be exposed to Python
static PyType_Slot charSlots[] = {
    {Py_tp_base, (void*)&PyUnicode_Type},  
    {Py_tp_traverse, (void*)&PyJPValue_traverse},  
    {Py_tp_finalize,  (void*) &PyJPValue_finalize},
    {Py_tp_free,  (void*) &PyJPValue_free},
    {0, NULL}                          // Sentinel
};

// Define the type specification
static PyType_Spec charSpec = 
{
    .name = "helloworld.PyJPChar",
    .basicsize = sizeof(PyJPChar),  
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = charSlots,
};

// TODO strings should use the same string wrapper, but we need lazy load 
// support in Python before that can happen.  Target Python 3.15.

void PyJPChar_initType(PyObject* module)
{
    PyObject *bases = PyTuple_Pack(2, (PyObject*)&PyUnicode_Type, (PyObject*) PyJPObject_Type);
    if (bases == NULL)
        return;
    PyJPChar_Type = PyJPClass_FromSpecWithBases(&charSpec, bases, offsetof(PyJPChar, extra));
    Py_DECREF(bases);
    if (PyJPChar_Type != NULL)
        PyModule_AddObject(module, "_JChar", PyJPChar_Type);
}



//**************************************************************************
// Split file to pyjp_exception.cpp

// Exceptions are a special case. To make it so that Java exceptions can be used
// with Python `except` we must mirror them over.  Yes I know it is a hack,
// but every language decides that they must have concrete exceptions for speed
// and don't create an official proxy class, so we all must reinvent the wheel.

static int jexception_traverse(PyObject *self, visitproc visit, void *arg) {
    // Custom GC traversal logic, or just delegate:
    return ((PyTypeObject*)PyExc_Exception)->tp_traverse(self, visit, arg);
}

// Function that will be exposed to Python
static PyType_Slot jexception_slots[] =
{
    {Py_tp_base, (void*)&PyExc_Exception},
    {Py_tp_traverse, (void*)jexception_traverse},
    {0, NULL}                          // Sentinel
};

// Define the type specification
static PyType_Spec jexception_spec =
{
    .name = "helloworld.PyJPException",
    .basicsize = sizeof(PyJPException),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = jexception_slots,
};

void PyJPException_initType(PyObject* module)
{
    PyObject *bases = PyTuple_Pack(2, (PyObject*)PyExc_Exception, (PyObject*) PyJPObject_Type);
    if (bases == NULL)
        return;
    PyJPException_Type = PyJPClass_FromSpecWithBases(&jexception_spec, bases, offsetof(PyJPException, extra));
    Py_DECREF(bases);
    if (PyJPException_Type != NULL)
        PyModule_AddObject(module, "_JException", (PyObject*) PyJPException_Type);
}


//**************************************************************************
// Module

static PyObject* helloworld_say_hello(PyObject* self, PyObject* args)
{ // Define the slots for the derived class
    // Print "Hello, World!" to the console
    printf("Hello, World!\n");

    // Return None to Python
    Py_RETURN_NONE;
}

// Define methods in the module
static PyMethodDef HelloWorldMethods[] = {
    {"say_hello", helloworld_say_hello, METH_VARARGS, "Print 'Hello, World!' to the console."},
    {NULL, NULL, 0, NULL}  // Sentinel to mark the end of the method definitions
};

// Define the module itself
static struct PyModuleDef moduleDef = {
    PyModuleDef_HEAD_INIT,
    "helloworld",  // Name of the module
    "A Hello, World! module written in C.",  // Module documentation
    -1,  // Size of per-interpreter state or -1 if module keeps state in global variables
    HelloWorldMethods
};

/** Create the module.
 *
 * The heart and soul of the module where everything we need will be set up.
 */
PyMODINIT_FUNC PyInit_helloworld(void) {

    // This will serve as a tag for our class
    _magic = PyDict_New();

    // Create the module
    module = PyModule_Create(&moduleDef);
    if (!module) {
        Py_DECREF(PyJPClass_Type);
        return NULL;
    }

    // Create the metaclass type
    PyJPClass_initType(module);
    if (PyJPClass_Type == NULL)
        goto error;

    PyJPObject_initType(module);
    if (PyJPObject_Type == NULL)
        goto error;

    PyJPException_initType(module);
    if (PyJPException_Type == NULL)
        goto error;

    PyJPNumber_initType(module);
    if (PyJPLong_Type == NULL || PyJPFloat_Type == NULL)
        goto error;

    PyJPChar_initType(module);
    if (PyJPChar_Type == NULL)
        goto error;

    return module;

error:
    // NOTE: We DECREF each type object here to clean up our reference if initialization fails.
    // If the object was added to the module, the module owns a reference and will clean up on module destruction.
    // This avoids leaking references if initialization is incomplete.
    if (PyJPClass_Type != NULL)
        Py_DECREF(PyJPClass_Type);
    if (PyJPObject_Type != NULL)
        Py_DECREF(PyJPObject_Type);
    if (PyJPLong_Type != NULL)
        Py_DECREF(PyJPLong_Type);
    if (PyJPFloat_Type != NULL)
        Py_DECREF(PyJPFloat_Type);
    return NULL;
}
