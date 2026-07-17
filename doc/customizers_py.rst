.. _customization:

Customization
*************

JPype supports customization to enhance the integration between Java and Python.
This allows users to modify Java classes and type conversions to better suit
their needs, making Java APIs more Pythonic or enabling seamless interaction
with Python data structures.

This chapter covers the Python-calling-Java direction. If you are instead
embedding Python in a Java application, see :doc:`customizers_java` for the
matching Java-side extension points (SPI providers and the ``$``-mangled
direct-dispatch mechanism).

There are two primary types of customizations available:

1. **Class Customizers**: Add Python methods and properties to Java classes to
   make them behave like native Python classes.
2. **Type Conversion Customizers**: Define implicit conversions between Python
   types and Java types for seamless interoperability.

.. _customization_class_customizers:

Class Customizers
=================

Customizers are applied to JPype wrapper classes to enhance their Pythonic
interface. By adding Python methods and properties to Java classes, customizers
make Java objects behave like native Python objects. These customizations are
applied to wrappers, whether they encapsulate a proxy or a Java reference.

Java wrappers can be customized to better match the expected behavior in Python.
Customizers are defined using decorators. Applying the annotations
``@JImplementationFor`` and ``@JOverride`` to a regular Python class will
transfer methods and properties to a Java class.

``@JImplementationFor`` requires the class name as a string, a Java class
wrapper, or a Java class instance. Only a string can be used prior to starting
the JVM. ``@JOverride``, when applied to a Python method, will hide the Java
implementation, allowing the Python method to replace the Java implementation.
When a Java method is overridden, it is renamed with a preceding underscore to
appear as a private method. Optional arguments to ``@JOverride`` can be used to
control the renaming and force the method override to apply to all classes that
derive from a base class ("sticky").

Generally speaking, a customizer should be defined before the first instance of
a given class is created so that the class wrapper and all instances will have
the customization.

.. _customization_example_customizing_javautilmap:

Example: Customizing ``java.util.Map``
--------------------------------------

The following example demonstrates how to customize the ``java.util.Map`` class
to behave like a Python dictionary:

.. code-block:: python

   @_jcustomizer.JImplementationFor('java.util.Map')
   class _JMap:
       def __jclass_init__(self):
           Mapping.register(self)

       def __len__(self):
           return self.size()

       def __iter__(self):
           return self.keySet().iterator()

       def __delitem__(self, i):
           return self.remove(i)

The name of the class does not matter for the purposes of the customizer,
though it should be a private class so that it does not get used accidentally.
The customizer code will steal from the prototype class rather than acting as a
base class, ensuring that the methods will appear on the most derived Python
class and are not hidden by the Java implementations.

The customizer copies methods, callable objects, ``__new__``, class member
strings, and properties.


.. _customization_type_conversion_customizers:

Type Conversion Customizers
===========================

JPype allows users to define custom conversion methods that are called whenever
a specified Python type is passed to a particular Java type. To specify a
conversion method, add ``@JConversion`` to an ordinary Python function with the
name of the Java class to be converted to and one keyword of ``exact`` or
``instanceof``. The keyword controls how strictly the conversion will be
applied:

- ``exact``: Restricted to Python objects whose type exactly matches the
  specified type.
- ``instanceof``: Accepts anything that matches ``isinstance`` to the specified
  type or protocol.

In some cases, the existing protocol definition will be overly broad. Adding
the keyword argument ``excludes`` with a type or tuple of types can be used to
prevent the conversion from being applied. Exclusions always apply first.

User-supplied conversions are tested after all internal conversions have been
exhausted and are always considered to be an implicit conversion.


.. _customization_example_converting_python_sequences_to_java_collections:

Example: Converting Python Sequences to Java Collections
--------------------------------------------------------

The following example demonstrates how to convert Python sequences into Java
collections:

.. code-block:: python

   @JConversion("java.util.Collection", instanceof=Sequence,
                             excludes=str)
   def _JSequenceConvert(jcls, obj):
       return _jclass.JClass('java.util.Arrays').asList(obj)

JPype supplies customizers for certain Python classes by default. These include:

========================= ==============================
Python class              Implicit Java Class
========================= ==============================
pathlib.Path              java.io.File
pathlib.Path              java.nio.file.Path
datetime.datetime         java.time.Instant
collections.abc.Sequence  java.util.Collection
collections.abc.Mapping   java.util.Map
========================= ==============================


.. _customization_jpype_beans_module:

JPype Beans Module
==================

.. _customization_overview_of_jpype_beans:

Overview of JPype Beans
-----------------------

The `jpype.beans` module is an optional feature that converts Java Bean-style
getter and setter methods into Python properties. This customization is
particularly useful for interactive programming or when working with Java
classes that follow the Bean pattern.

However, this behavior is not enabled by default because it can lead to
confusion about whether a class is exposing a variable or a property added by
JPype. Additionally, it violates Python's principle of *"There should be one--
and preferably only one --obvious way to do it."* and the C++ principle of
*"You only pay for what you use."*

If you find this feature useful, you can enable it explicitly by importing the
`jpype.beans` module.

.. _customization_enabling_beans_as_properties:

Enabling Beans as Properties
----------------------------

To enable the `jpype.beans` module, simply import it into your Python program:

.. code-block:: python

  import jpype.beans

Once enabled, the module applies globally to all Java classes that have already
been loaded, as well as any classes loaded afterward. This behavior cannot be
undone after the module is imported.

.. _customization_how_it_jpype_beans_works:

How It JPype beans Works
------------------------

The `jpype.beans` module scans Java classes for methods that follow the Bean
naming conventions:

- **Getter methods**: Methods prefixed with `get` (e.g., `getName`) are treated
  as property accessors.
- **Setter methods**: Methods prefixed with `set` (e.g., `setName`) are treated
  as property mutators.

For example, a Java class with the following methods:

.. code-block:: java

  public class Person {
      private String name;

      public String getName() {
          return name;
      }

      public void setName(String name) {
          this.name = name;
      }
  }

Will automatically expose the `name` field as a Python property:

.. code-block:: python

  import jpype
  import jpype.beans

  jpype.startJVM()

  Person = jpype.JClass("Person")
  person = Person()
  person.name = "Alice"  # Calls setName("Alice")
  print(person.name)     # Calls getName(), Output: Alice

.. _customization_implementation_details_of_jpype_beans:

Implementation Details of JPype beans
-------------------------------------

The module works by:

1. Identifying getter and setter methods in Java classes using the
   `_isBeanAccessor()` and `_isBeanMutator()` methods.
2. Creating Python properties for these methods.
3. Adding the properties to the class dynamically.

The customization applies retroactively to all classes currently loaded and
globally to all future classes.

.. _customization_limitations_of_jpype_beans:

Limitations of JPype beans
--------------------------

1. **Global Behavior**: Once enabled, the customization applies to all Java
   classes globally. It cannot be undone.
2. **Confusion with Existing Members**: If a Java class already has a Python
   member with the same name as a property, the property will not be added to
   avoid conflicts.
3. **Ambiguity**: This feature can make it unclear whether a field is a true
   Java variable or a property added by JPype.

.. _customization_best_practices_for_jpype_beans:

Best Practices for JPype beans
------------------------------

- Use this module only when working with Java classes that heavily rely on the
  Bean pattern.
- Avoid enabling this module in large projects unless absolutely necessary, as
  the global behavior may lead to unintended consequences.
- Document its usage clearly in your codebase to avoid confusion for other
  developers.

.. _customization_summary_of_jpype_beans:

Summary of JPype beans
----------------------

The `jpype.beans` module provides a convenient way to work with Java Bean-style
classes in Python by exposing getter and setter methods as Python properties.
While useful in certain scenarios, it is an optional feature that must be
explicitly enabled and should be used with caution due to its global and
irreversible behavior.

.. _customization_resolving_method_name_conflicts_with_customizers:

Resolving Method Name Conflicts with Customizers
================================================

.. _customization_overview_of_conflict_resolution:

Overview of conflict resolution
-------------------------------

When working with Java classes in Python, conflicts can arise between public
fields and methods that share the same name. JPype provides tools to resolve
these conflicts using customizers, allowing you to rename fields or methods
dynamically and expose them in a Pythonic way.

This section demonstrates how to use a customizer to resolve such conflicts by
renaming fields or methods and exposing them as Python properties.

.. _customization_example_renaming_conflicting_fields_and_methods:

Example: Renaming Conflicting Fields and Methods
------------------------------------------------

Consider a Java class with a field and a method that share the same name.
Without customization, JPype will expose the method, and the field will be
hidden. To resolve this, you can use a customizer to rename the conflicting
field or method and expose it as a Python property.

Here’s an example:

.. code-block:: python

    def asProperty(field):
        def get(E):
            return field.get(E)
        def set(E, V):
            field.set(E, V)
        return property(get, set)

    @jpype.JImplementationFor("java.lang.Object")  # Use your base class.
    class MyCustomizer(object):

        # This is applied to every class that derives from the type
        def __jclass_init__(cls):
            # Traverse the fields
            for field in cls.class_.getDeclaredFields():
                name = str(field.getName())
                tp = type(cls.__dict__.get(str(field.getName()), None))

                # Watch for private methods
                if tp is type(None):
                    continue

                # Resolve conflicts between public fields and methods
                if tp is jpype.JMethod:
                    cls._customize("%s_" % name, asProperty(field))

.. _customization_how_it_conflict_resolution_works:

How It Conflict Resolution Works
--------------------------------

1. **Field Traversal**: The customizer iterates over all declared fields in the
   class using `getDeclaredFields()`.
2. **Conflict Detection**: For each field, it checks whether a public method
   with the same name exists.
3. **Renaming**: If a conflict is detected, the field is renamed by appending
   an underscore (`_`) to its name.
4. **Property Creation**: The renamed field is exposed as a Python property
   using the `property()` function.

.. _customization_example_usage_of_conflict_resolution:

Example Usage of Conflict Resolution
------------------------------------

Suppose you have a Java class `A` with a field `mean` and a method `mean`.
Without customization, the field would be inaccessible. Using the customizer
above, you can expose the field as `mean_`:

.. code-block:: python

    A = jpype.JClass("A")
    a = A()
    print(a.mean_)  # Access the renamed field
    a.mean_ = 2      # Modify the field
    print(a.mean_)   # Verify the updated value

.. _customization_notes_on_global_customizers:

Notes on Global Customizers
---------------------------

- The customizer is applied globally to all classes that derive from the
  specified base class (`java.lang.Object` in this example). You can replace
  the base class with a more specific class to limit the scope of the
  customization.
- This approach is particularly useful for resolving conflicts in large Java
  libraries or frameworks where method and field names overlap frequently.

.. _customization_best_practices_regarding_name_resolution_customizers:

Best Practices Regarding Name Resolution Customizers
----------------------------------------------------

- Use meaningful naming conventions when renaming fields or methods to avoid
  confusion.
- Document customizations clearly in your codebase to help other developers
  understand the changes.
- Test the customizer thoroughly to ensure it behaves as expected across all
  relevant classes.

.. _customization_summary_of_naming_conflict_resolution:

Summary of Naming Conflict Resolution
-------------------------------------

This example demonstrates how to use JPype customizers to resolve conflicts
between fields and methods in Java classes. By renaming conflicting fields or
methods and exposing them as Python properties, you can create a more Pythonic
interface for interacting with Java classes.


.. _customization_best_practices_for_class_customization:

Best Practices For Class Customization
======================================

To ensure effective use of customizations, follow these best practices:

1. **Define Customizers Early**: Always define customizers before the first
   instance of the class is created to ensure proper initialization.

2. **Test Customizations Thoroughly**: Verify that the customized behavior
   works as expected, especially for complex or heavily-used classes.

3. **Avoid Conflicts**: Ensure that customizers do not introduce conflicting
   methods or properties, especially when customizing multiple interfaces.

4. **Monitor Performance**: Be mindful of performance implications when adding
   extensive customizations.

5. **Document Customizations**: Clearly document the purpose and behavior of
   customizations to assist other developers working on the codebase.

By leveraging class and type conversion customizers, JPype users can create
seamless integrations between Python and Java, making Java APIs feel native to
Python programmers.
