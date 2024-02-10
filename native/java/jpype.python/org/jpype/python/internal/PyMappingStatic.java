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
