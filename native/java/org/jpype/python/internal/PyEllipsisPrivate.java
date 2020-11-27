/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.python.internal;

import python.lang.PyBuiltins;
import python.lang.PyEllipsis;
import python.lang.PyNone;

/**
 *
 * @author nelson85
 */
public class PyEllipsisPrivate extends PyBaseObject
{
  public PyEllipsisPrivate(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  protected static Object _allocate(long instance)
  {
    // No need to reference as PyNone is immortal
    if (PyBuiltins.Ellipsis == null)
      PyBuiltins.Ellipsis = (PyEllipsis) new PyEllipsisPrivate(PyConstructor.CONSTRUCTOR, instance);
    return PyBuiltins.Ellipsis;
  }

}
