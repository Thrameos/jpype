package org.jpype.python;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import org.jpype.JPypeContext;
import org.jpype.manager.ClassDescriptor;
import org.jpype.manager.TypeManager;
import org.jpype.manager.TypeManagerExtension;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import python.lang.PyFloat;
import python.lang.PyLong;
import python.lang.PyObject;
import python.lang.PyString;
import python.lang.exc.PyBaseException;

/**
 * TypeManager holds all of the wrapper classes that have been created.
 */
public class PyTypeManager implements TypeManagerExtension
{

  static final private PyTypeManager instance = new PyTypeManager();
  final PyTypeLoader loader;

  static public PyTypeManager getInstance()
  {
    if (JPypeContext.getInstance() == null)
      throw new RuntimeException("Python environment is not started");
    return instance;
  }

  private PyTypeManager()
  {
      
    // Make sure that all native entry points are loaded.
    // This happens during the bootup sequence and we don't yet have the
    // ability to print stacktraces in Python, so we have to do it here.
    try
    {
      Class.forName("org.jpype.python.enums.PyInvocation", true, JPypeContext.getInstance().getClassLoader());
    } catch (ClassNotFoundException ex)
    {
      ex.printStackTrace();
      throw new RuntimeException(ex);
    } catch (RuntimeException | Error ex)
    {
      ex.printStackTrace();
      throw ex;
    }

    PyTypeBuilder builder = new PyTypeBuilder();
    loader = new PyTypeLoader(builder);
    

    // Install wrappers in C++ layer
    TypeManager typeManager = JPypeContext.getInstance().getTypeManager();
      // Note that order is very important when creating these initial wrapper
      // types. If something inherits from another type then the super class
      // will be created without the special flag and the type system won't
      // be able to handle the duplicate type properly.
      Class[] cls =
      {
        PyBaseObject.class, PyBaseException.class,
        PyString.class, PyLong.class, PyFloat.class
      };
      for (Class c : cls)
      {
        createClass(typeManager, c);
      }

  }

  private void collectBases(ArrayList<Class> interfaces, Class cls)
  {
    if (cls.getAnnotation(PyTypeInfo.class) != null)
    {
      if (interfaces.contains(cls))
        return;
      interfaces.add(cls);
      return;
    }

    for (Class intf : cls.getInterfaces())
    {
      collectBases(interfaces, intf);
    }
  }

  /**
   * Get the wrapper for a class.
   *
   * This operates to create the customized wrapper for a Python class. First it
   * checks the index to see if there is a defined wrapper type. If the wrapper
   * type is complete then its work is done. Otherwise, it consults the dynamic
   * classloader to construct a new class wrapper.
   *
   * @param moduleName is the name of the module in Python.
   * @param className is the name of the class in Python.
   * @param bases is a list of protocols that apply to this object.
   * @return
   */
  public Class getWrapper(String moduleName, String className, Class[] bases)
  {
    System.out.println("get wrapper " + className);
    try
    {
      ArrayList<Class> interfaces = new ArrayList<>();
      if (bases == null)
        bases = new Class[0];

      interfaces.ensureCapacity(bases.length + 5);
      Class wrapper;
      Class concrete = null;
      try
      {
        // Search the index for the best fit wrapper
        Class module = Class.forName("python.__modules__." + moduleName);
        PyModuleIndex index = (PyModuleIndex) module.getConstructor().newInstance();
        wrapper = index.getWrapper(className);

        // If we get a wrapper then see how it is to be used.
        if (wrapper != null)
        {
          // If it is concrete then we should include its interfaces
          if (!Modifier.isAbstract(wrapper.getModifiers()))
          {
            concrete = wrapper;
            interfaces.addAll(Arrays.asList(wrapper.getInterfaces()));
          } else
            interfaces.add(wrapper);
          PyTypeInfo annotation = (PyTypeInfo) wrapper.getAnnotation(PyTypeInfo.class);
          if (annotation.exact())
            return wrapper;
        }
      } catch (ClassNotFoundException | NoSuchMethodException | SecurityException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
      {
      }

      for (Class base : bases)
      {
        collectBases(interfaces, base);
      }

      // Pass 1 find a concrete type if it exists
      Iterator<Class> iter = interfaces.iterator();
      while (iter.hasNext())
      {
        Class base = iter.next();
        if (!base.isInterface())
        {
          if (concrete != null && concrete != base && !base.isAssignableFrom(concrete))
          {

            System.out.println("concrete " + concrete + " " + base.isAssignableFrom(concrete));
            throw new RuntimeException(String.format("Base conflict between '%s' and '%s' while creating wrapper for %s",
                    concrete.getName(), base.getName(), className));
          }
          concrete = base;
          iter.remove();
        }
      }

      // Pass 2 remove an interfaces already implemented by the concrete type
      if (concrete != null)
      {
        iter = interfaces.iterator();
        while (iter.hasNext())
        {
          Class base = iter.next();
          if (base.isAssignableFrom(concrete))
            iter.remove();
        }
      }

      // Construct the class using the loader
      return loader.findClass(className, concrete, interfaces.toArray(Class[]::new));
    } catch (Exception ex)
    {
      ex.printStackTrace();
      throw ex;
    }
  }

  /**
   * 
   * @param <T>
   * @param cls
   * @return 
   */
  public <T> T createStaticInstance(Class<T> cls)
  {
    try
    {
      System.out.println("Create static instance " + cls);
      Class<T> c = loader.findClass(cls.getSimpleName(), null, new Class[]
      {
        cls
      });
      return c.getConstructor().newInstance();
    } catch (NoSuchMethodException | SecurityException | InstantiationException
            | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
    {
      // There is no error recovery as this is called from static initializer
      ex.printStackTrace();
      throw new RuntimeException(ex);
    }
  }

//<editor-fold desc="extension">  
    @Override
  public ClassDescriptor createClass(TypeManager typeManager, Class<?> cls)
  {
    // Figure out the base class to apply
    ClassDescriptor out = null;
    Class base = null;
    if (PyBaseObject.class.isAssignableFrom(cls))
      base = PyBaseObject.class;
    else if (PyBaseException.class.isAssignableFrom(cls))
      base = PyBaseException.class;
    else if (PyString.class.isAssignableFrom(cls))
      base = PyString.class;
    else if (PyLong.class.isAssignableFrom(cls))
      base = PyLong.class;
    else if (PyFloat.class.isAssignableFrom(cls))
      base = PyFloat.class;
    else
      throw new RuntimeException("No known base for " + cls.getName() + " " + cls.getSuperclass());

    out = typeManager.classMap.get(base);
    if (out != null)
    {
      // This class will shared the same object wrapper.
      typeManager.classMap.put(cls, out);
      return out;
    }
    out = typeManager.createOrdinaryClass(base, true, false);
    typeManager.classMap.put(cls, out);
    return out;
  }

  @Override
  public Class getManagedClass()
  {
    return PyObject.class;
  }
//</editor-fold>
}
