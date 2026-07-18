.. _Proxies:

Calling Python Code from Java
*****************************
Proxies in JPype enable Python objects to implement Java interfaces directly,
so Java code can call into Python without knowing it. These proxies are
specifically designed to implement Java interfaces, acting as wrapper classes
that disguise the Python nature of the object in a Java type-safe manner.

Don't confuse this with :doc:`types_java`: this chapter is about a *Python*
object (defined with ``@JImplements``/``JProxy``) playing a *Java* role --
Python code written to satisfy a Java interface, called from Java code that
doesn't know it's talking to Python underneath. ``python.lang`` (covered in
:doc:`types_java`) is the opposite relationship: Java code directly
manipulating a live Python object through a typed Java-side handle
(``PyObject`` and its subtypes), with no pretense of being a Java type at
all. Both directions involve Java calling into Python, but the object
being passed around plays a different role in each.

In Java, proxies are foreign elements that pretend to implement a Java
interface. JPype uses this proxy API to allow Python code to implement any
Java interface. While proxies allow Python objects to fulfill the contract of a
Java interface, they are not equivalent to subclassing Java classes in Python.

Fortunately, many Java APIs are designed to minimize the need for subclassing.
For example, frameworks like AWT and SWING allow developers to create complete
user interfaces without requiring a single subclass. Subclassing is typically
reserved for more advanced features or specialized use cases.

For those cases where sub-classing is absolutely necessary (i.e. using Java's
SAXP classes), it is necessary to create an interface and a simple
subclass in Java that delegates the calls to that interface.  The interface
can then be implemented in Python using a proxy.

There are three APIs for supporting of Java proxies. The direct method is to
pass a Python function, method, bound method, or lambda to any Java method
that accepts a FunctionInterface or other SAM.  If more complex behaviors
need to be exchanged Python can implement a Java interface. Implementation of an
interface uses decorators which features strong error checking and easy
notation.  The older low-level interface allows any Python object or dictionary
to act as a proxy even if it does not provide the required methods for the
interface.

.. _calling_python_code_from_java_passing_python_callables_to_java_functional_interfaces:

Passing Python Callables to Java Functional Interfaces
=======================================================

JPype supports passing Python functions, methods, and bound methods directly to
Java methods or fields that implement `FunctionalInterface`. This allows Python
code to be used directly as Java's functional programming constructs, such
as lambdas and method references, without requiring a proxy or explicit
implementation of the interface.

### Supported Use Cases

This feature works with any Java method or field that expects a
`FunctionalInterface`. Common examples include:
- Java Streams (`java.util.stream`)
- Java Executors (`java.util.concurrent`)
- Custom functional interfaces defined in Java code

### Example: Passing a Python Function to a Java Method

Suppose you have a Java method that expects a `java.util.function.Function`:

.. code-block:: java

    import java.util.function.Function;

    public class Example {
        public static String applyFunction(Function<String, String> func, String input) {
         return func.apply(input);
        }
    }

You can pass a Python function directly to this method:

.. code-block:: python

    import jpype import jpype.imports
    jpype.startJVM()

    from java.util.function import Function from Example import Example

    # Define a Python function
    def to_uppercase(s):
        return s.upper()

    # Pass the Python function to the Java method
    result = Example.applyFunction(to_uppercase, "hello")
    print(result)  #Output: HELLO

### Example: Using a Lambda Expression

Python lambdas can also be passed to Java methods:

.. code-block:: python

    # Pass a lambda expression
    result = Example.applyFunction(lambda s: s[::-1], "hello")
    print(result)  #Output: olleh

### Example: Using a Bound Method

Bound methods of Python objects can be passed as well:

.. code-block:: python

    class StringManipulator:
        def reverse(self, s):
            return s[::-1]

    manipulator = StringManipulator()
    result = Example.applyFunction(manipulator.reverse, "hello")
    print(result)  # Output: olleh

### Notes and Best Practices

1. **Performance**: While using Python callables is convenient, it may not be
as performant as implementing a full Java proxy for high-frequency calls. Use
proxies for performance-critical applications.

2. **Error Handling**: If an exception occurs within the Python callable, it
will be wrapped in a `RuntimeException` when passed back to Java.

3. **Type Matching**: Ensure that the Python callable returns a type compatible
with the expected Java return type. Implicit conversions will be applied where
possible.

By leveraging this feature, you can simplify integration between Python and
Java, especially when working with Java's functional programming APIs.


.. _@JImplements:

Implements
==========

The newer style of proxy works by decorating any ordinary Python class to
designate it as a proxy.  This is most effective when you
control the Python class definition.  If you don't control the class definition
you either need to encapsulate the Python object in another object or
use the older style.

Implementing a proxy is simple.  First construct an ordinary Python class with
method names that match the Java interface to be implemented.  Then add
the ``@JImplements`` decorator to the class definition.  The first argument
to the decorator is the interface to implement.  Then mark each
method corresponding to a Java method in the interface with ``@JOverride``.
When the proxy class is declared, the methods will be checked against the Java
interface.  Any missing method will result in JPype raising an exception.

High-level proxies have one other important behavior.  When a proxy created
using the high-level API returns from Java it unpacks back to the original
Python object complete with all of its attributes.  This occurs whether the
proxy is the ``self`` argument for a method or
proxy is returned from a Java container such as a list.  This is accomplished
because the actually proxy is a temporary Java object with no substance,
thus rather than returning a useless object, JPype unpacks the proxy
to its original Python object.

.. _calling_python_code_from_java_proxy_method_overloading:

Proxy Method Overloading
------------------------

Overloaded methods will issue to a single method with the matching name.  If
they take different numbers of arguments then it is best to implement a method
dispatch:

.. code-block:: python

    @JImplements(JavaInterface)
    class MyImpl:
        @JOverride
        def callOverloaded(self, *args):
            # always use the wild card args when implementing a dispatch
            if len(args)==2:
                return self.callMethod1(*args)
            if len(args)==1 and isinstance(args[0], JString):
                return self.callMethod2(*args)
            raise RuntimeError("Incorrect arguments")

       def callMethod1(self, a1, a2):
            # ...
       def callMethod2(self, jstr):
            # ...

.. _calling_python_code_from_java_multiple_interfaces:

Multiple interfaces
-------------------

Proxies can implement multiple interfaces as long as none of those interfaces
have conflicting methods.  To implement more than one interface, use a
list as the argument to the JImplements decorator.  Each interface must be
implemented completely.

.. _calling_python_code_from_java_deferred_realization:

Deferred realization
--------------------

Sometimes it is useful to implement proxies before the JVM is started.  To
achieve this, specify the interface using a string and add the keyword argument
``deferred`` with a value of ``True`` to the decorator.

.. code-block:: python

    @JImplements("org.foo.JavaInterface", deferred=True)
    class MyImpl:
        # ...


Deferred proxies are not checked at declaration time, but instead at the time
for the first usage.  Because of this, when uses an deferred proxy the code
must be able to handle initialization errors wherever the proxy is created.

Other than the raising of exceptions on creation, there is no penalty to
deferring a proxy class. The implementation is checked once on the first
usage and cached for the remaining life of the class.

.. _calling_python_code_from_java_proxy_factory:

Proxy Factory
=============

When a foreign object from another module for which you do not control the class
implementation needs to be passed into Java, the low level API is appropriate.
In this API you manually create a JProxy object.  The proxy object must either
be a Python object instance or a Python dictionary.  Low-level proxies use the
JProxy API.

.. _calling_python_code_from_java_jproxy:

JProxy
------

The ``JProxy`` allows Python code to "implement" any number of Java interfaces,
so as to receive callbacks through them.  The JProxy factory has the signature::

   JProxy(int, [dict=obj | inst=obj] [, deferred=False])

The first argument is the interface to be implemented.  This may be either
a string with the name of the interface, a Java class, or a Java class instance.
If multiple interfaces are to be implemented the first argument is
replaced by a Python sequence.  The next argument is a keyword argument
specifying the object to receive methods.  This can either be a dictionary
``dict`` which names the methods as keys or an object instance ``inst`` which
will receive method calls.  If more than one option is selected, a ``TypeError``
is raised.  When Java calls the proxy the method is looked up in either
the dictionary or the instance and the resulting method is called.  Any
exceptions generated in the proxy will be wrapped as a ``RuntimeException``
in Java.  If that exception reaches back to Python it is unpacked to return
the original Python exception.

Assume a Java interface like:

.. code-block:: java

  public interface ITestInterface2
  {
          int testMethod();
          String testMethod2();
  }

You can create a proxy *implementing* this interface in two ways.
First, with an object:

.. code-block:: python

  class C:
      def testMethod(self):
          return 42

      def testMethod2(self):
          return "Bar"

  c = C()  # create an instance
  proxy = JProxy("ITestInterface2", inst=c)  # Convert it into a proxy

or you can use a dictionary.

.. code-block:: python

    def _testMethod():
        return 32

    def _testMethod2():
        return "Fooo!"

    d = { 'testMethod': _testMethod, 'testMethod2': _testMethod2, }
    proxy = JProxy("ITestInterface2", dict=d)

.. _calling_python_code_from_java_wrapping_a_python_instance_with_new_behaviors_for_java:

Wrapping a Python instance with new behaviors for Java
======================================================

JPype allows a JProxy to implement Java interfaces using a combination of a
dictionary (dict) and an object instance (inst). This feature enables arbitrary
Python objects to dynamically define methods via a dictionary while also
providing methods from the object's class. The combined approach is
particularly useful for cases where some methods are predefined in a Python
class and others need to be dynamically added or overridden.  This is useful when
the names and functionality of a Python object need to be made to conform to
Java's expected behaviors.

.. _calling_python_code_from_java_syntax:

Syntax
------
The JProxy factory supports both dict and inst as keyword arguments. When both are provided:

 * Methods in the dictionary take precedence.
 * The inst object is passed as the self argument to methods defined in the dictionary.
 * If a method is not found in the dictionary, JPype will fall back to the default method implementation in Java.

.. code-block:: python

    JProxy(interface, dict=my_dict, inst=my_instance)

Example: Combining dict and inst
Suppose you have a Java interface:

.. code-block:: java

    public interface MyInterface {
        String method1();
        String method2();
    }

You can implement this interface using both a dictionary and an object instance:

.. code-block:: python

    public interface MyInterface {
        String method1();
        default String method2() {
            return "hello";
        }
    }
    You can implement this interface using both a dictionary and an object instance:

    .. code-block:: python

    from jpype import JProxy

    class MyClass:
        def __init__(self, name):
            self.name = name

    # Define a dictionary with methods
    my_dict = {
        "method1": lambda self: f"Hello, {self.name} from method1"
    }

    # Create an instance of the class
    my_instance = MyClass("Alice")

    # Combine the dictionary and instance in a JProxy
    proxy = JProxy("MyInterface", dict=my_dict, inst=my_instance)

    # Use the proxy in Java
    print(proxy.method1())  # Output: Hello, Alice from method1
    print(proxy.method2())  # Falls back to Java's default method implementation


.. _calling_python_code_from_java_notes_and_best_practices:

Notes and Best Practices
------------------------
Method Resolution:

 * Methods in the dictionary take precedence over methods in the instance.

 * If a method is not found in the dictionary, JPype will attempt to resolve it in the
   instance.

Error Handling:

 * If neither the dictionary nor the instance provides the required method, a NotImplementedError will be raised.

Flexibility:

This approach allows dynamic addition or overriding of methods via the dictionary while retaining the benefits of object-oriented programming with the instance.

Example: Dynamic Overrides
You can dynamically override methods in the instance using the dictionary:

.. code-block:: python

    class MyClass:
        def __init__(self, name):
            self.name = name

    # Define a dictionary to override methods
    my_dict = {
        "method1": lambda self: f"Overridden method1 for {self.name}"
    }

    my_instance = MyClass("Bob")

    proxy = JProxy("MyInterface", dict=my_dict, inst=my_instance)

    print(proxy.method1())  # Output: Overridden method1 for Bob
    print(proxy.method2())  # Falls back to Java's default method implementation


.. _calling_python_code_from_java_proxying_python_objects:

Proxying Python objects
=======================

Sometimes it is necessary to push a Python object into Java memory space as an
opaque object.  This can be achieved using be implementing a proxy for
an interface which has no methods.  For example, ``java.io.Serializable`` has
no arguments and little functionality beyond declaring that an object can be
serialized. As low-level proxies to not automatically convert back to Python
upon returning to Java, the special keyword argument ``convert`` should be set
to True.

For example, let's place a generic Python object such as NumPy array into Java.

.. code-block:: python

    import numpy as np
    u = np.array([[1,2],[3,4]])
    ls = java.util.ArrayList()
    ls.add(jpype.JProxy(java.io.Serializable, inst=u, convert=True))
    u2 = ls.get(0)
    print(u is u2)  # True!

We get the expected result of ``True``.  The Python has passed through Java
unharmed.  In future versions of JPype, this method will be extended to provide
access to Python methods from within Java by implementing a Java interface that
points to back to Python objects.


.. _calling_python_code_from_java_reference_loops:

Reference Loops
===============

It is strongly recommended that object used in proxies must never hold a
reference to a Java container.  If a Java container is asked to hold a Python
object and the Python object holds a reference to the container, then a
reference loop is formed.  Both the Python and Java garbage collectors are
aware of reference loops within themselves and have appropriate handling for
them.  But the memory space of the other machine is opaque and neither Java nor
Python is aware of the reference loop.  Therefore, unless you manually break
the loop by either clearing the container, or removing the Java reference from
Python these objects can never be collected.  Once you lose the handle they
will both become immortal.

Ordinarily the proxy by itself would form a reference loop.  The Python
object points to a Java invocation handler and the invocation handler points
back to Python object to prevent the Python object from going away as long
as Java is holding onto the proxy.  This is resolved internally by making
the Python weak reference the Java portion.  If Java ever garbage collects
the Java half, it is recreated again when the proxy is next used.

This does have some consequences for the use of proxies.  Proxies must never
be used as synchronization objects.  Whenever
they are garbage collected, they lose their identity.  In addition, their
hashCode and system id both are reissued whenever they are refreshed.
Therefore, using a proxy as a Java map key can be problematic.  So long as
it remains in the Java map, it will maintain the same identify.  But once
it is removed, it is free to switch identities every time it is garbage
collected.
