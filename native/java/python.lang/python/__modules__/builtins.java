package python.__modules__;

import java.util.HashMap;
import java.util.Map;
import org.jpype.python.PyModuleIndex;

/**
 * Index for Python builtins module.
 *
 * This scans for defined wrappers for Python objects. Other modules can have
 * their own wrapper interfaces if they implement an index in
 * python.__modules__. Otherwise, they will fall back to the builtin wrappers.
 *
 * Currently this looks for wrappers in python.lang, python.lang.exc, and
 * python.lang.protocol.
 *
 * @author nelson85
 */
public class builtins implements PyModuleIndex
{

  final static Map<String, Class> ENTRIES = new HashMap<>();

  static
  {
    ENTRIES.putAll(PyModuleIndex.scanPackage("python.lang"));
    ENTRIES.putAll(PyModuleIndex.scanPackage("python.lang.exc"));
    ENTRIES.putAll(PyModuleIndex.scanPackage("python.lang.protocol"));
  }

  @Override
  public Class getWrapper(String name)
  {
    return ENTRIES.get(name);
  }

}
