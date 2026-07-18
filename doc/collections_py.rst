.. _collections:

Collections
***********

JPype uses customizers to augment Java collection classes to operate like Python
collections. Enhanced objects include ``java.util.List``, ``java.util.Set``,
``java.util.Map``, and ``java.util.Iterator``. These classes generally comply
with the Python API except in cases where there is a significant name conflict.
This section details the integration of Java collections with Python constructs.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`collections_java` for the
matching Java-side ``python.collections`` front end.

.. _collections_specialized_collection_wrappers:

Specialized Collection Wrappers
===============================

JPype customizes Java collection classes to behave like Python collections,
making them intuitive for Python developers. This includes support for iteration,
indexing, and key-value access. Below are the key behaviors of specific Java
collection types.

.. _collections_iterable:

Iterable
--------

Java classes that implement ``java.util.Iterable`` are customized to support
Python's iteration constructs, so they work directly in Python `for` loops
and list comprehensions. For example, a Java ``ArrayList`` can be iterated
directly:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    for item in jlist:
        print(item)

This integration ensures that Java collections behave like Python sequences,
providing a natural experience for Python developers.

.. _collections_iterators:

Iterators
---------

Java classes that implement ``java.util.Iterator`` act as Python iterators.
This means they can be used in Python `for` loops and list comprehensions
without requiring additional conversion. For example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    iterator = jvector.iterator()
    for item in iterator:
        print(item)

.. _collections_collection:

Collection
----------

Java classes that inherit from ``java.util.Collection`` work directly with
Python's collection constructs. They support operations such as length
retrieval, iteration, and implicit conversion of Python sequences into Java
collections. For example:

.. code-block:: python

    from java.util import ArrayList

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java collection

    print(len(jlist))  # Output: 3
    for item in jlist:
        print(item)

Methods that accept Java collections can automatically convert Python sequences
if all elements are compatible with Java types. Otherwise, a ``TypeError`` is
raised.

.. _java.util.List:

Lists
-----

Java `List` classes, such as ``ArrayList`` and ``LinkedList``, can be used in
Python `for` loops and list comprehensions. They also support indexing and
deletion, making them behave like Python lists. For example:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    print(jlist[0])  # Output: apple
    del jlist[1]     # Remove "orange"
    print(jlist)     # Output: [apple, banana]

Java lists can also be converted to Python lists and vice versa using the copy
constructor. For example:

.. code-block:: python

    pylist = ["apple", "orange", "banana"]
    jlist = ArrayList(pylist)  # Convert Python list to Java list
    pylist2 = list(jlist)      # Convert Java list back to Python list

Note that individual elements remain Java objects when converted to Python.
Converting to Java will attempt to convert each argument
individually to Java.  If there is no conversion it will produce a
``TypeError``.  The conversion can be forced by casting to the appropriate
Java type with a list comprehension or by defining a new conversion
customizer.

Lists also have iterable, length, item deletion, and indexing.  Note that
indexing of ``java.util.LinkedList`` is supported but can have a large
performance penalty for large lists.  Use of iteration is much for efficient.


.. _java.util.Map:

Maps
----

Java classes that implement ``java.util.Map`` behave like Python dictionaries.
They support key-value access, iteration, and deletion. 
Here is a summary of their capabilities:

=========================== ================================
Action                       Python
=========================== ================================
Place a value in the map     ``jmap[key]=value``
Delete an entry              ``del jmap[key]``
Get the length               ``len(jmap)``
Lookup the value             ``v=jmap[key]``
Get the entries              ``jmap.items()``
Fetch the keys               ``jmap.key()``
Check for a key              ``key in jmap``
=========================== ================================

Example using Java HashMap with Pythonic interface:

.. code-block:: python

    from java.util import HashMap

    jmap = HashMap()
    jmap.put("key1", "value1")
    jmap.put("key2", "value2")

    print(jmap["key1"])  # Output: value1
    del jmap["key2"]     # Remove "key2"
    print(len(jmap))     # Output: 1

Maps also support iteration over keys and values:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

Methods that accept Java maps can implicitly convert Python dictionaries if
all keys and values are compatible with Java types. Otherwise, a ``TypeError``
is raised.

.. _collections_map_entries:

Map Entries
-----------

Java map entries unpack into key-value pairs, allowing easy iteration in Python
loops. For example:

.. code-block:: python

    for key, value in jmap.items():
        print(f"{key}: {value}")

.. _collections_sets:

Sets
----

Java classes that implement ``java.util.Set`` behave like Python sets. They
support operations such as item deletion, iteration, and length retrieval. For
example:

.. code-block:: python

    from java.util import HashSet

    jset = HashSet()
    jset.add("apple")
    jset.add("orange")

    print(len(jset))  # Output: 2
    jset.remove("orange")
    print(jset)       # Output: [apple]

.. _collections_enumeration:

Enumeration
-----------

Java classes that implement ``java.util.Enumeration`` act as Python iterators.
This allows them to be used in Python `for` loops and list comprehensions. For
example:

.. code-block:: python

    from java.util import Vector

    jvector = Vector()
    jvector.add("apple")
    jvector.add("orange")

    enumeration = jvector.elements()
    for item in enumeration:
        print(item)


.. _collections_integrating_pythonic_constructs_with_java_collections:

Integrating Pythonic Constructs with Java Collections
======================================================

JPype enables Python developers to interact with Java collections and streams
while leveraging Python's idiomatic constructs, such as list comprehensions and
generator expressions. This section explores how Pythonic constructs and Java
methods can be used interchangeably or combined for efficient manipulation of
data structures.



.. _collections_using_pythonic_constructs_with_java_collections:

Using Pythonic Constructs with Java Collections
------------------------------------------------
JPype enables Python developers to interact with Java collections while
leveraging Python's idiomatic constructs, such as list comprehensions and
generator expressions. For example:

.. code-block:: python

    from java.util import ArrayList

    jlist = ArrayList()
    jlist.add("apple")
    jlist.add("orange")
    jlist.add("banana")

    filtered = [item.upper() for item in jlist if item.startswith("a")]
    print(filtered)  # Output: ['APPLE']

Combining Pythonic constructs with Java methods allows developers to use the
best tools for the task, whether they prefer Python's simplicity or Java's
robustness.


.. _collections_using_java_streams_for_functional_operations:

Using Java Streams for Functional Operations
---------------------------------------------

Java's `Stream` API provides powerful functional programming constructs, such
as `filter`, `map`, and `reduce`. JPype allows Python developers to use these
methods with Java collections directly.



.. _collections_example_filtering_and_mapping_with_java_streams:

Example: Filtering and Mapping with Java Streams
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   from java.util.stream import Collectors

   # Use Java Stream API for filtering and mapping
   filtered = jlist.stream().filter(lambda s: s.startswith("a")).map(
       lambda s: s.upper()).collect(Collectors.toList())
   print(filtered)  # Output: [APPLE]

Advantages of Java Streams:
- Integration with Java's enterprise libraries.
- Parallel processing capabilities (e.g., `.parallelStream()`).
- Type-safe operations with Java's generics.



.. _collections_comparison_pythonic_constructs_vs_java_methods:

Comparison: Pythonic Constructs vs Java Methods
------------------------------------------------

+---------------------------+---------------------------------------+
| **Feature**               | **Pythonic Constructs**               |
|                           | **(List Comprehensions)**             |
+---------------------------+---------------------------------------+
| Syntax                    | Concise and readable                  |
+---------------------------+---------------------------------------+
| Performance               | Python interpreter overhead           |
+---------------------------+---------------------------------------+
| Parallel Processing       | Requires external libraries           |
|                           | (e.g., `multiprocessing`)             |
+---------------------------+---------------------------------------+
| Type Safety               | Dynamic typing                        |
+---------------------------+---------------------------------------+
| Ease of Use               | Familiar to Python developers         |
+---------------------------+---------------------------------------+


.. _collections_combining_pythonic_constructs_and_java_methods:

Combining Pythonic Constructs and Java Methods
----------------------------------------------

JPype allows developers to mix Pythonic constructs and Java methods for maximum
flexibility. For example, you can use Java streams for complex operations and
Pythonic constructs for post-processing.


.. _collections_example_combining_streams_and_list_comprehensions:

Example: Combining Streams and List Comprehensions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

   # Use Java Stream API for filtering
   filtered_stream = jlist.stream().filter(lambda s: s.startswith("a")).collect(
       Collectors.toList())

   # Use Pythonic list comprehension for further processing
   final_result = [item.lower() for item in filtered_stream]
   print(final_result)  # Output: ['apple']


.. _collections_when_to_use_each_approach:

When to Use Each Approach
-------------------------

+-------------------------------------------+---------------------------+
| **Scenario**                              | **Recommended Approach**  |
+-------------------------------------------+---------------------------+
| Simple filtering or mapping               | Pythonic constructs       |
|                                           | (list comprehensions)     |
+-------------------------------------------+---------------------------+
| Complex operations (e.g., grouping,       | Java Streams              |
| reducing)                                 |                           |
+-------------------------------------------+---------------------------+
| Integration with Java enterprise          | Java Streams              |
| libraries                                 |                           |
+-------------------------------------------+---------------------------+
| Quick prototyping or debugging            | Pythonic constructs       |
+-------------------------------------------+---------------------------+
| Parallel processing                       | Java Streams              |
|                                           | (`parallelStream`)        |
+-------------------------------------------+---------------------------+



.. _collections_best_practices_for_collection_processing:

Best Practices for Collection Processing
----------------------------------------

1. **Choose the Right Tool for the Job**:
   - Use Pythonic constructs for simplicity and readability.
   - Use Java streams for performance-critical or enterprise applications.

2. **Mix Pythonic and Java-native calls freely**:
   - Combine Pythonic constructs and Java methods on the same object as needed.

3. **Optimize for Performance**:
   - Avoid frequent back-and-forth calls between Python and Java. Cache results when possible.



.. _collections_conclusion_on_collection_processing:

Conclusion on Collection Processing
-----------------------------------

JPype enables Python developers to work with Java collections using both
Pythonic constructs and Java methods. Whether you prefer Python's simplicity or
Java's robustness, JPype provides the flexibility to choose the paradigm that
best fits your workflow.
