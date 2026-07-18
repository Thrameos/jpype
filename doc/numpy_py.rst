.. _working_with_numpy:

Working with NumPy
********************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) NumPy story --
``python.io``/``python.collections``/``python.datetime`` don't expose a
NumPy-array front end, so no ``numpy_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

enabling efficient data exchange and manipulation across both ecosystems. By
leveraging JPype's ability to transfer arrays bidirectionally, users can combine
NumPy's powerful numerical computing capabilities with Java's robust libraries
for machine learning, scientific computing, and enterprise applications. Whether
transferring data to NumPy for analysis or sending arrays to Java for processing,
JPype ensures high performance and compatibility with minimal overhead. This
integration is particularly useful for applications requiring large-scale
numerical computations or interoperability between Python and Java-based systems.


.. _working_with_numpy_transferring_arrays_between_python_and_java:

Transferring Arrays Between Python and Java
===========================================

JPype supports bidirectional transfers of arrays between Python (NumPy) and Java.
This lets numerical libraries on either side operate on the same underlying data.



.. _working_with_numpy_transferring_arrays_to_numpy:

Transferring Arrays to NumPy
----------------------------

Java arrays can be transferred into NumPy arrays using Python's `memoryview`.
This enables efficient bulk data transfer for rectangular arrays.

**Example: Transferring a Java Array to NumPy**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a Java array
   java_array = jpype.JDouble[:]([1.1, 2.2, 3.3])

   # Transfer the Java array to NumPy
   numpy_array = np.array(memoryview(java_array))

   print(numpy_array)  # Output: [1.1 2.2 3.3]

**Constraints**:
- The Java array must be rectangular. Jagged arrays are not supported.
- Only primitive types (e.g., `double`, `int`) are supported for direct transfer.



.. _working_with_numpy_transferring_arrays_to_java:

Transferring Arrays to Java
---------------------------

NumPy arrays can be transferred to Java using the `JArray.of` function. This maps
the structure of a NumPy array to a Java multidimensional array.

**Example: Transferring a NumPy Array to Java**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array
   numpy_array = np.zeros((5, 10, 20))  # 5x10x20 array filled with zeros

   # Transfer the array to Java
   java_array = jpype.JArray.of(numpy_array)

   print(java_array[0][0][0])  # Output: 0.0

**Constraints**:
- The NumPy array must be rectangular. Jagged arrays are not supported.
- Data types must be compatible with Java primitives (e.g., `np.float64` → `double`).



.. _working_with_numpy_requirements_and_constraints:

Requirements and Constraints
----------------------------

1. **Rectangular Arrays**:

   - Both NumPy and Java arrays must be rectangular for direct transfer.

2. **Data Type Compatibility**:

   - NumPy types must map to Java primitives (e.g., `np.int32` → `int`).

3. **Error Handling**:

   - Jagged arrays or incompatible types will raise a `TypeError`.



.. _working_with_numpy_best_practices_with_numpy:

Best Practices with NumPy
-------------------------

1. **Validate Array Structure**:
   - Ensure arrays are rectangular before transferring.

2. **Optimize Data Types**:
   - Use NumPy types that map directly to Java primitives for efficiency.

3. **Monitor Memory Usage**:
   - Large arrays can consume significant memory. Monitor resources carefully.



.. _working_with_numpy_summary_of_numpy:

Summary of NumPy
----------------

JPype provides efficient bidirectional array transfers between Python and Java.
Following the outlined constraints and best practices keeps numerical and
scientific transfers working correctly.


.. _working_with_numpy_buffer_backed_numpy_arrays:

Buffer Backed NumPy Arrays
==========================

Java direct buffers provide a mechanism for shared memory between Java and
Python, enabling high-speed data exchange by bypassing the JNI layer. These
buffers are particularly useful for applications requiring efficient handling
of large datasets, such as scientific computing or memory-mapped files.

Direct buffers are part of the Java ``nio`` package and can be accessed using
the ``jpype.nio`` module. NumPy arrays can be backed by Java direct buffers,
allowing Python and Java to operate on the same memory space. However, direct
buffers are not managed by the garbage collector, so improper use may lead to
memory leaks or crashes.


.. _working_with_numpy_creating_buffer_backed_arrays:

Creating Buffer Backed Arrays
-----------------------------

To create a buffer-backed NumPy array, you can either originate the buffer in
Java or Python. The following examples demonstrate both approaches:

**Example 1: Creating a Buffer in Java**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Allocate a direct buffer in Java
   jb = java.nio.ByteBuffer.allocateDirect(80)  # Allocates 80 bytes
   db = jb.asDoubleBuffer()                     # Converts to a double buffer

   # Convert the buffer to a NumPy array
   np_array = np.asarray(db)                    # NumPy array backed by Java buffer
   print(np_array)

**Example 2: Creating a Buffer in Python**

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a Python bytearray
   py_buffer = bytearray(80)                    # Allocates 80 bytes
   jb = jpype.nio.convertToDirectBuffer(py_buffer)  # Maps the bytearray to Java
   db = jb.asDoubleBuffer()                     # Converts to a double buffer

   # Convert the buffer to a NumPy array
   np_array = np.asarray(db)                    # NumPy array backed by Python buffer
   print(np_array)


.. _working_with_numpy_important_considerations_for_buffer_backed_arrays:

Important Considerations for Buffer Backed Arrays
-------------------------------------------------

1. **Buffer Lifetime**:

   - Python and NumPy cannot detect when a Java buffer becomes invalid. Once the
     JVM is shut down, all buffers originating from Java become invalid, and any
     access to them may result in crashes.

   - To avoid this, create buffers in Python and pass them to Java, ensuring
     Python retains control over the memory.

2. **JVM Shutdown**:

   - If buffers are created in Java, consider using ``java.lang.Runtime.exit``
     to terminate both the Java and Python processes simultaneously. This
     prevents accidental access to dangling buffers.

3. **Applications**:

   - Buffer-backed memory is not limited to NumPy. It can be used for shared
     memory between processes, memory-mapped files, or any application requiring
     efficient data exchange.


.. _working_with_numpy_summary_of_buffer_backed_arrays:

Summary of Buffer Backed Arrays
-------------------------------

Buffer-backed NumPy arrays provide a powerful mechanism for high-speed data
exchange between Python and Java. However, users must carefully manage buffer
lifetimes and ensure proper handling during JVM shutdown to avoid crashes or
memory leaks.


.. _working_with_numpy_numpy_primitives:

Both Python and Java have a notion of readonly memory (bytes vs bytearray in
Python). ConvertToDirectBuffer will honor the the writability of the passed
object and return a readonly Java ByteBuffer if the source object is readonly.


NumPy Primitives
================

JPype integrates with NumPy directly, allowing efficient data transfers
between Python and Java. NumPy arrays can be mapped to Java boxed
types or primitive arrays. However, certain types, such as `np.float16`, are
converted to compatible Java types during transfer.

.. _working_with_numpy_supported_numpy_types:

Supported NumPy Types
---------------------

The following table summarizes how NumPy types are mapped to Java boxed types
and primitive arrays:

=================  ============================
NumPy Type         Java Type (Boxed/Primitive)
=================  ============================
np.int8            java.lang.Byte / byte[]
np.int16           java.lang.Short / short[]
np.int32           java.lang.Integer / int[]
np.int64           java.lang.Long / long[]
np.float16         java.lang.Float / float[] (*)
np.float32         java.lang.Float / float[]
np.float64         java.lang.Double / double[]
=================  ============================

(*) `np.float16` will be automatically converted to `float32` (`java.lang.Float`
or `float[]`) during the transfer to Java.

.. note::
   `np.float16` can be transferred to Java, but it will be automatically
   converted to `float32` (`java.lang.Float` for boxed types or `float[]` for
   primitive arrays) on the Java side. This is because Java does not natively
   support `float16`. If precise handling of `float16` is required, consider
   converting the data to `float32` or `float64` explicitly in Python before
   transferring it.

.. _working_with_numpy_examples:

Examples
--------

The following examples demonstrate how to transfer `np.float16` data to Java
as boxed types or primitive arrays.

.. _working_with_numpy_example_1_transferring_float16_to_a_boxed_type:

Example 1: Transferring `float16` to a Boxed Type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array with float16 data
   float16_array = np.array([1.1, 2.2, 3.3], dtype=np.float16)

   # Transfer the array to a Java boxed type (java.util.ArrayList)
   java_list = jpype.java.util.ArrayList()
   for value in float16_array:
       java_list.add(jpype.JFloat(value))  # Automatically converted to float32

   print(java_list)  # Output: [1.1, 2.2, 3.3] (as float32)

.. _working_with_numpy_example_2_transferring_float16_to_a_primitive_array:

Example 2: Transferring `float16` to a Primitive Array
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   import jpype
   import numpy as np

   # Start the JVM
   jpype.startJVM()

   # Create a NumPy array with float16 data
   float16_array = np.array([1.1, 2.2, 3.3], dtype=np.float16)

   # Transfer the array to a Java primitive array
   java_primitive_array = jpype.JArray(jpype.JFloat)(float16_array)  # Converted
                                                                     # to float32[]

   print(java_primitive_array)  # Output: [1.1, 2.2, 3.3] (as float32[])

.. _working_with_numpy_summary_of_numpy_automatic_conversions:

Summary of NumPy Automatic Conversions
---------------------------------------

JPype supports transferring NumPy arrays to Java, with automatic conversions
for certain types. While `np.float16` can be transferred, it is converted to
`float32` on the Java side for compatibility. Users should be aware of this
behavior and plan accordingly when working with `float16` data.
