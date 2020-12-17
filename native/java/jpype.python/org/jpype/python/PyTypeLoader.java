package org.jpype.python;

import java.util.HashMap;
import org.jpype.JPypeContext;

/**
 * Class load for dynamic classes.
 *
 * @author nelson85
 */
class PyTypeLoader extends ClassLoader
{

  final HashMap<Integer, Class> cache = new HashMap<>();
  final PyTypeBuilder builder;

  PyTypeLoader(PyTypeBuilder builder)
  {
    super(JPypeContext.getInstance().getClassLoader());
    this.builder = builder;
  }

  /**
   * Find a class with the specified interfaces.
   *
   * @param name
   * @param concrete
   * @param interfaces
   * @return
   */
  public Class findClass(String name, Class concrete, Class[] interfaces)
  {
    int hash = 0;
    for (int i = 0; i < interfaces.length; i++)
    {
      hash = hash * 0xa2060073 + interfaces[i].getName().hashCode();
    }
    if (concrete != null)
      hash = hash * 0xa2060073 + concrete.getName().hashCode();
    Class cls = cache.get(hash);
    if (cls == null)
    {
      String className = String.format("%s$%08x", name, hash);
      byte[] byteCode = builder.newClass(className, concrete, interfaces);
      cls = defineClass(className, byteCode, 0, byteCode.length);
      cache.put(hash, cls);
    }
    return cls;
  }

}
