package python.lang.protocol;

/**
 * Package holding protocols for Python.
 *
 * The Python type system is a bit like a buffet. You start by grabbing a plate
 * (concrete type) and walking down the line sampling food (protocols) until you
 * have your meal. Python looks like it has multiple inheritance, but in fact it
 * is purely single inheritance as if you try to mix two types that have
 * different concrete types it will fail. We have represented that here as a set
 * of concrete types and protocols (Java interfaces). When retrieving an entity
 * from Python, it may either a Python object defined as a concrete type plus a
 * set of protocols, or as a Java type. Therefore, most methods return a generic
 * Object which must be cast to the proper type to use. Methods similarly take
 * any generic Object rather than a specific type unless the specific type is
 * required. There is no difference between Python types that have the same
 * protocols regardless of how it was derived. They will all share the same Java
 * class as they all implement the same interface.
 *
 * The type system will also make its best guess as to the most appropriate type
 * to use for a wrapper. If if can't find an appropriate type it will use an
 * protocols as a mixin to represent the capabilities of the class. Even if the
 * wrapper is not exactly the correct type, all functionality is available
 * through the generic object interface though the attributes and call
 * mechanisms.
 *
 * In some cases the same slot is shared for multiple concepts. Is something
 * with __getitem__, __len__, and __iter__ a sequence, a mapping, or a string?
 * In such a case, we check with Python typing to see what Python would consider
 * it to be.
 *
 */
