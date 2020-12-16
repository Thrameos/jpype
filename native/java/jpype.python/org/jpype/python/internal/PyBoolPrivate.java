package org.jpype.python.internal;

/**
 *
 * @author nelson85
 */
public class PyBoolPrivate
{

  static long _True;

  public static Object _allocate(long instance)
  {
    // First call defines TRUE during bootup.
    if (_True == 0)
      _True = instance;
    return instance == _True;
  }

}
