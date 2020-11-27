/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.jpype.python.internal;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.exc.PyException;
import python.lang.exc.PyIndexError;

/**
 *
 * @author nelson85
 */
public interface PyTupleStatic
{

  @PyMethodInfo(name = "PyTuple_Size", invoke = PyInvocation.AsInt, method = false)
  int size(Object self);

  @PyMethodInfo(name = "PyTuple_GetItemB", invoke = PyInvocation.BinaryInt, method = false)
  Object get(Object self, int i) throws PyIndexError;

  @PyMethodInfo(name = "PyTuple_GetSlice", invoke = PyInvocation.GetSlice, method = false)
  Object getSlice(Object self, int i1, int i2) throws PyException;

}
