package python.lang;

/**
 * Package holding basic types for Python.
 *
 * These are the concrete (or stubs for concrete) types that Python implements.
 *
 * The type system will also make its best guess as to the most appropriate type
 * to use for a wrapper. If if can't find an appropriate type it will use an
 * protocols as a mixin to represent the capabilities of the class. Even if the
 * wrapper is not exactly the correct type, all functionality is available
 * through the generic object interface though the attributes and call
 * mechanisms.
 *
 * Naming of classes is Py followed by the name as it appears in Python a few
 * exceptions.
 *
 * Builtin functions are held in the PyBuiltin class which has static methods
 * for each of the types.
 *
 */
