.. _serialization_with_jpickler:

Serialization with JPickler
****************************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) pickling story --
``python.io``/``python.collections``/``python.datetime`` don't expose a
JPickler equivalent, so no ``pickling_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

JPype provides the **JPickler** utility for serializing (`pickling`) Java objects
into Python-compatible byte streams. This is particularly useful for saving Java
objects to disk, transferring them between systems, or debugging their state.



.. _serialization_with_jpickler_why_use_jpickler:

Why Use JPickler?
=================

When working with Java objects in Python, serialization is often required for:

1. **Persistence**: Saving Java objects to files for later use.
2. **Data Exchange**: Transferring Java objects between Python applications or
   systems.
3. **Debugging**: Capturing the state of Java objects during execution for
   offline analysis.

However, Python's default `pickle` module does not support Java objects.
JPickler bridges this gap by encoding Java objects into a format compatible
with Python's serialization tools.



.. _serialization_with_jpickler_how_jpickler_works:

How JPickler Works
------------------

JPickler uses Java's `Serializable` interface to serialize Java objects into a
byte stream that can be stored or transferred. It also provides a companion
utility, **JUnpickler**, for deserializing these byte streams back into Java
objects.



.. _serialization_with_jpickler_example_1_basic_serialization:

Example 1: Basic Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following example demonstrates how to serialize and deserialize Java
objects using JPickler and JUnpickler.

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    # Start the JVM
    jpype.startJVM()

    # Create a Java object
    java_list = jpype.java.util.ArrayList()
    java_list.add("Hello")
    java_list.add("World")

    # Serialize the Java object to a file
    with open("serialized_java_list.pkl", "wb") as f:
        JPickler(f).dump(java_list)

    print("Java object serialized successfully!")

    # Deserialize the Java object from the file
    with open("serialized_java_list.pkl", "rb") as f:
        deserialized_list = JUnpickler(f).load()

    print("Deserialized Java object:", deserialized_list)
    # Output: [Hello, World]



.. _serialization_with_jpickler_example_2_serializing_complex_java_objects:

Example 2: Serializing Complex Java Objects
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

JPickler can handle any Java object that implements `java.io.Serializable`.
Here's an example with a custom Java class:

.. code-block:: java

    // Save this as MySerializableClass.java and compile it
    import java.io.Serializable;

    public class MySerializableClass implements Serializable {
        private String name;
        private int value;

        public MySerializableClass(String name, int value) {
            this.name = name;
            this.value = value;
        }

        @Override
        public String toString() {
            return "MySerializableClass{name='" + name + "', value=" + value + "}";
        }
    }

.. code-block:: python

    import jpype
    import jpype.imports
    from jpype.pickle import JPickler, JUnpickler

    # Start the JVM
    jpype.startJVM(classpath=["."])

    # Create an instance of the custom Java class
    MySerializableClass = jpype.JClass("MySerializableClass")
    java_object = MySerializableClass("TestObject", 42)

    # Serialize the Java object to a file
    with open("serialized_java_object.pkl", "wb") as f:
        JPickler(f).dump(java_object)

    print("Custom Java object serialized successfully!")

    # Deserialize the Java object from the file
    with open("serialized_java_object.pkl", "rb") as f:
        deserialized_object = JUnpickler(f).load()

    print("Deserialized Java object:", deserialized_object)
    # Output: MySerializableClass{name='TestObject', value=42}



.. _serialization_with_jpickler_best_practices_with_jpicker:

Best Practices with JPicker
===========================

1. **Ensure Objects Are Serializable**:

   - Only Java objects that implement `java.io.Serializable` can be serialized.
     Ensure your custom Java classes implement this interface.

2. **Validate Serialization**:

   - Test serialization and deserialization to ensure data integrity.

3. **Handle Non-Serializable Fields**:

   - If a Java object contains non-serializable fields, mark them as `transient`
     to exclude them during serialization.

4. **Avoid Reference Loops**:

   - Break reference loops between Python and Java objects to prevent memory
     leaks.


.. _serialization_with_jpickler_limitations_of_jpickler:

Limitations of JPickler
=======================

1. **Non-Serializable Objects**:

   - Java objects that do not implement `java.io.Serializable` cannot be
     serialized with JPickler.

2. **Cross-Version Compatibility**:

   - Serialized Java objects may not be compatible across different JVM
     versions.

3. **Performance**:

   - Serialization and deserialization can be resource-intensive for large or
     complex objects.


.. _serialization_with_jpickler_use_cases_of_jpickler:

Use Cases of JPickler
=====================

1. **Persistence**:

   - Save Java objects to disk for later use.

   - Example: Storing application state or configuration.

2. **Data Exchange**:

   - Transfer Java objects between Python applications or systems.

   - Example: Network communication or distributed systems.

3. **Debugging**:

   - Capture the state of Java objects during execution for offline analysis.

   - Example: Serialize problematic objects for inspection after a crash.


.. _serialization_with_jpickler_conclusion_for_jpicker:

Conclusion for JPicker
======================

JPickler simplifies serialization of Java objects in Python. By following best practices and
understanding its limitations, you can use JPickler effectively for
persistence, data exchange, and debugging tasks.

