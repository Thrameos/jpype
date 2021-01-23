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
import python.lang.exc.PyKeyError;
import python.lang.exc.PyMemoryError;
import python.lang.exc.PySystemError;
import python.lang.exc.PyTypeError;

public interface PySetStatic
{

  @PyMethodInfo(name = "PyObject_Size", invoke = PyInvocation.AsLong, method = false)
  int size(Object self);

  @PyMethodInfo(name = "PySet_Contains", invoke = PyInvocation.AsInt, method = false)
  boolean contains(Object self, Object key);

  @PyMethodInfo(name = "PySet_Add", invoke = PyInvocation.Binary, method = false)
  void addItem(Object self, Object key) throws PyTypeError, PyMemoryError, PySystemError;

  @PyMethodInfo(name = "PySet_Discard", invoke = PyInvocation.BinaryToInt, method = false)
  boolean remove(Object self, Object key) throws PySystemError;

  @PyMethodInfo(name = "PySet_Pop", invoke = PyInvocation.Unary, method = false)
  Object pop(Object self) throws PyKeyError, PySystemError;

  @PyMethodInfo(name = "PySet_Clear", invoke = PyInvocation.Unary, method = false)
  void clear(Object self);
}
