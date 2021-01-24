/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.python.internal;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.PyDict;
import python.lang.PyList;
import python.lang.exc.PyException;

/**
 *
 * @author nelson85
 */
public interface PyDictStatic
{

  @PyMethodInfo(name = "PyDict_SetItemString", invoke = PyInvocation.SetStr, method = false)
  public void setItemString(Object self, String key, Object value);

  @PyMethodInfo(name = "PyDict_SetItem", invoke = PyInvocation.SetObj, method = false)
  public void setItem(Object self, Object key, Object value);

  @PyMethodInfo(name = "PyDict_GetItem", invoke = PyInvocation.Binary,
          method = false, flags = PyMethodInfo.ACCEPT|PyMethodInfo.BORROWED)
  Object getItem(Object self, Object key);

  @PyMethodInfo(name = "PyDict_GetItemString", invoke = PyInvocation.GetStr,
          method = false, flags = PyMethodInfo.ACCEPT|PyMethodInfo.BORROWED)
  Object getItemString(Object self, String key);

  @PyMethodInfo(name = "PyDict_DelItem", invoke = PyInvocation.BinaryToInt, method = false)
  void delItem(Object self, Object key) throws PyException;

  @PyMethodInfo(name = "PyDict_DelItemString", invoke = PyInvocation.DelStr, method = false)
  void delItemString(Object self, String key) throws PyException;

  @PyMethodInfo(name = "PyDictProxy_New", invoke = PyInvocation.Unary, method = false)
  PyDict asReadOnly(Object self);

  @PyMethodInfo(name = "PyDict_Clear", invoke = PyInvocation.Unary, method = false)
  void clear(Object self);

  @PyMethodInfo(name = "PyDict_Contains", invoke = PyInvocation.BinaryToInt, method = false)
  boolean contains(Object self, Object key) throws PyException;

  @PyMethodInfo(name = "PyDict_Copy", invoke = PyInvocation.Unary, method = false)
  PyDict clone(Object self);

  @PyMethodInfo(name = "PyDict_GetItemWithError", invoke = PyInvocation.Binary, method = false, flags = PyMethodInfo.BORROWED)
  Object getItemWithError(Object self, Object key) throws PyException;

  @PyMethodInfo(name = "PyDict_SetDefault", invoke = PyInvocation.Ternary, method = false)
  Object setDefault(Object self, Object key, Object defaultobj);

  @PyMethodInfo(name = "PyDict_Items", invoke = PyInvocation.Unary, method = false)
  PyList items(Object self);

  @PyMethodInfo(name = "PyDict_Keys", invoke = PyInvocation.Unary, method = false)
  PyList keys(Object self);

  @PyMethodInfo(name = "PyDict_Values", invoke = PyInvocation.Unary, method = false)
  PyList values(Object self);

  @PyMethodInfo(name = "PyDict_Size", invoke = PyInvocation.AsInt, method = false)
  int size(Object self);

  @PyMethodInfo(name = "PyDict_Merge", invoke = PyInvocation.IntOperator2, method = false)
  void merge(Object self, Object dict, boolean override) throws PyException;

  @PyMethodInfo(name = "PyDict_Update", invoke = PyInvocation.Binary, method = false)
  void update(Object self, Object b) throws PyException;

  @PyMethodInfo(name = "PyDict_MergeFromSeq2", invoke = PyInvocation.IntOperator2, method = false)
  void mergeFromSeq2(Object self, Object seq2, boolean override) throws PyException;
}