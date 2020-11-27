package org.jpype.python;

import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import org.jpype.JPypeContext;
import org.jpype.pkg.JPypePackageManager;
import org.jpype.python.annotation.PyTypeInfo;

/**
 * Index used to define the interfaces for a Python Type.
 * <p>
 * Implementations of this class must have the package python.__modules__.
 *
 * @author nelson85
 */
public interface PyModuleIndex
{

  Class getWrapper(String name);

  /**
   * Function for constructing an index of wrappers by scanning a Python package
   * for annotations.
   *
   * @param pkg
   * @return
   */
  public static Map<String, Class> scanPackage(String pkg)
  {
    HashMap<String, Class> out = new HashMap<>();
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
    Map<String, URI> content = JPypePackageManager.getContentMap(pkg);
    for (Map.Entry<String, URI> entry : content.entrySet())
    {
      Class c;
      try
      {
        c = Class.forName(pkg + "." + entry.getKey(), false, cl);
        PyTypeInfo ti = (PyTypeInfo) c.getAnnotation(PyTypeInfo.class);
        if (ti == null)
          continue;
        out.put(ti.name(), c);
      } catch (ClassNotFoundException ex)
      {
      }
    }
    return out;
  }
}
