package org.jpype.python.internal;

import static org.jpype.python.internal.PyConstructor.ALLOCATOR;
import python.lang.PyBuiltins;
import python.lang.PyObject;

/**
 * Base implementation.
 */
public class PyBaseObject implements PyObject
{
  long _self;

  @SuppressWarnings("LeakingThisInConstructor")
  public PyBaseObject(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  @Override
  public String toString()
  {
    CharSequence c = PyBuiltins.str(this);
    return c.toString();
  }

  @Override
  public int hashCode()
  {
    long l = PyBuiltins.hash(this);
    int u = (int) (l >> 32);
    return u ^ ((int) l);
  }

  // Stub
  @Override
  public boolean equals(Object obj)
  {
    if (this == obj)
      return true;
    return PyBuiltins.eq(this, obj);
  }

  static Object _allocate(long inst)
  {
    return new PyBaseObject(ALLOCATOR, inst);
  }

}
