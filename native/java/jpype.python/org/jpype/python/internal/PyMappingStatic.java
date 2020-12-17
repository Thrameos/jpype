package org.jpype.python.internal;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.exc.PyException;

/**
 *
 * @author nelson85
 */
public interface PyMappingStatic
{

  @PyMethodInfo(name = "PyMapping_SetItemString", invoke = PyInvocation.SetStr, method = false)
  void setItemString(Object self, String key, Object value);

  @PyMethodInfo(name = "PyObject_SetItem", invoke = PyInvocation.SetObj, method = false)
  void setItem(Object self, Object key, Object value);

  @PyMethodInfo(name = "PyMapping_GetItemString", invoke = PyInvocation.Binary, method = false)
  Object getItemString(Object self, String key);

  @PyMethodInfo(name = "PyMapping_GetItemString", invoke = PyInvocation.Binary, method = false)
  Object getItem(Object self, Object key);

  @PyMethodInfo(name = "PyObject_DelItem", invoke = PyInvocation.BinaryToInt, method = false)
  void delItem(Object self, Object key) throws PyException;

  @PyMethodInfo(name = "PyObject_DelItemString", invoke = PyInvocation.BinaryToInt, method = false)
  void delItemString(Object self, String key) throws PyException;

  @PyMethodInfo(name = "PyMapping_HasKey", invoke = PyInvocation.BinaryToInt, method = false)
  boolean hasKey(Object self, Object key);

  @PyMethodInfo(name = "PyMapping_HasKeyString", invoke = PyInvocation.DelStr, method = false)
  boolean hasKeyString(Object self, String key);

  @PyMethodInfo(name = "PyMapping_Keys", invoke = PyInvocation.Unary, method = false)
  Object keys(Object self);

  @PyMethodInfo(name = "PyMapping_Values", invoke = PyInvocation.Unary, method = false)
  Object values(Object self);

  @PyMethodInfo(name = "PyMapping_Items", invoke = PyInvocation.Unary, method = false)
  Object items(Object self);

  @PyMethodInfo(name = "PyMapping_Size", invoke = PyInvocation.AsLong, method = false)
  int size(Object self);
}
