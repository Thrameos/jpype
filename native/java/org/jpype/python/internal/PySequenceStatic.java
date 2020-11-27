package org.jpype.python.internal;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.PyList;
import python.lang.PyTuple;
import python.lang.exc.PyException;

public interface PySequenceStatic
{

  @PyMethodInfo(name = "PySequence_Concat", invoke = PyInvocation.Binary, method = false)
  Object concat(Object self, Object o2);

  @PyMethodInfo(name = "PySequence_Repeat", invoke = PyInvocation.BinaryInt, method = false)
  Object repeat(Object self, int count);

  @PyMethodInfo(name = "PySequence_InPlaceConcat", invoke = PyInvocation.Binary, method = false)
  Object assignConcat(Object self, Object o2);

  @PyMethodInfo(name = "PySequence_InPlaceRepeat", invoke = PyInvocation.BinaryInt, method = false)
  Object assignRepeat(Object self, int count);

  @PyMethodInfo(name = "PySequence_GetItem", invoke = PyInvocation.BinaryInt, method = false)
  Object get(Object self, int index) throws PyException;

  @PyMethodInfo(name = "PySequence_GetSlice", invoke = PyInvocation.GetSlice, method = false)
  Object getSlice(Object self, int i1, int i2) throws PyException;

  @PyMethodInfo(name = "PySequence_SetItem", invoke = PyInvocation.SetInt, method = false)
  void setItem(Object self, int index, Object value) throws PyException;

  @PyMethodInfo(name = "PySequence_DelItem", invoke = PyInvocation.IntOperator1, method = false)
  void delItem(Object self, int index) throws PyException;

  @PyMethodInfo(name = "PySequence_SetSlice", invoke = PyInvocation.SetSlice, method = false)
  void setSlice(Object self, int start, int end, Object value);

  @PyMethodInfo(name = "PySequence_DelSlice", invoke = PyInvocation.DelSlice, method = false)
  void delSlice(Object self, int start, int end) throws PyException;

  @PyMethodInfo(name = "PySequence_Count", invoke = PyInvocation.BinaryToInt, method = false)
  int count(Object self, Object value) throws PyException;

  @PyMethodInfo(name = "PySequence_Contains", invoke = PyInvocation.BinaryToInt, method = false)
  boolean contains(Object self, Object value) throws PyException;

  @PyMethodInfo(name = "PySequence_Index", invoke = PyInvocation.BinaryToInt, method = false)
  int indexOf(Object self, Object value) throws PyException;

  @PyMethodInfo(name = "PySequence_List", invoke = PyInvocation.Unary, method = false)
  PyList asList(Object self) throws PyException;

  @PyMethodInfo(name = "PySequence_Tuple", invoke = PyInvocation.Unary, method = false)
  PyTuple asTuple(Object self);

}
