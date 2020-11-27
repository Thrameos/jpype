package org.jpype.python.internal;

import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

/**
 * Template for objects that derive from a concrete object
 *
 * @author nelson85
 */
public class PyBaseExtension extends PyBaseObject
{

  public PyBaseExtension(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyBaseExtension(ALLOCATOR, inst);
  }

}
