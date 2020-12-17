package org.jpype.python.internal;

import python.lang.PyBuiltins;
import python.lang.PyNone;

/**
 *
 * @author nelson85
 */
public class PyNonePrivate extends PyBaseObject
{

  public PyNonePrivate(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  protected static Object _allocate(long instance)
  {
    // No need to reference as PyNone is immortal
    if (PyBuiltins.None == null)
      PyBuiltins.None = (PyNone) new PyNonePrivate(PyConstructor.CONSTRUCTOR, instance);
    return PyBuiltins.None;
  }

}
