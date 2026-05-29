// In order to safely shutdown a interpreter we need to wipe out every single stale
// Java reference.  We avoid this when we shut down a JVM but 
#include <Python.h>
#include <jni.h>
#include "pyjp.h"
#include "jpype.h"

// 1. The clean visitproc callback matching CPython's functional signature
static int jpype_eject_visitor(PyObject* obj, void* arg)
{
    // The void* argument acts as our closure payload holding the JNIEnv pointer
    JNIEnv* env = (JNIEnv*)arg;

    if (obj == NULL)
        return 0;

    // Your safe-by-design lookup screens out native objects instantly
    JPValue* value = PyJPValue_getJavaSlot(obj);
    if (value == NULL)
        return 0; // Return 0 to continue the heap sweep traversal

    JPClass* cls = value->getClass();
    if (cls == NULL || cls->isPrimitive())
        return 0;

    jobject global_ref = (jobject)value->getValue().l;
    if (global_ref == NULL)
        return 0;

    // Safely disconnect the reference from the JVM
    env->DeleteGlobalRef(global_ref);

    // Invalidate the slot structure so that if CPython hits a normal 
    // destructor cycle later during finalization, it becomes a safe no-op.
    *value = JPValue();

    return 0; // Standard Py_VISIT success signal to keep going
}

int jpype_module_clear(PyObject *module)
{
#if PY_VERSION_HEX>=0x030c0000
    PyJPModuleState *st = (PyJPModuleState*)PyModule_GetState(module);
    if (st == nullptr) return 0;

    // Establish your clean JNI downcall frame boundary or pull the active environment
    JPJavaFrame frame = JPJavaFrame::outer(st->context);
    JNIEnv* env = frame.getEnv();

    // Execute the sweep across the entire internal subinterpreter heap
    PyUnstable_GC_VisitObjects(jpype_eject_visitor, (void*)env);
#endif
    return 0;
}

