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
import python.lang.PyTuple;
import python.lang.exc.PyException;
import python.lang.exc.PyIndexError;

public interface PyListStatic
{

  @PyMethodInfo(name = "PyList_GetItem", invoke = PyInvocation.BinaryInt, method = false, flags = PyMethodInfo.BORROWED)
  Object getItem(Object o, int index) throws PyIndexError;

  @PyMethodInfo(name = "PyList_SetItemS", invoke = PyInvocation.SetInt, method = false)
  void setItem(Object o, int index, Object item) throws PyIndexError;

  @PyMethodInfo(name = "PyList_Insert", invoke = PyInvocation.SetInt, method = false)
  void insert(Object o, int index, Object item) throws PyException;

  @PyMethodInfo(name = "PyList_Append", invoke = PyInvocation.BinaryToInt, method = false)
  void append(Object o, Object item) throws PyException;

  @PyMethodInfo(name = "PyList_GetSlice", invoke = PyInvocation.GetSlice, method = false)
  Object getSlice(Object o, int low, int high) throws PyException;

  @PyMethodInfo(name = "PyList_SetSlice", invoke = PyInvocation.SetSlice, method = false)
  void setSlice(Object o, int low, int high, Object item) throws PyException;

  @PyMethodInfo(name = "PyList_Sort", invoke = PyInvocation.AsInt, method = false)
  void sort(Object o) throws PyException;

  @PyMethodInfo(name = "PyList_Reverse", invoke = PyInvocation.AsInt, method = false)
  void reverse(Object o) throws PyException;

  @PyMethodInfo(name = "PyList_AsTuple", invoke = PyInvocation.Unary, method = false)
  PyTuple asTuple(Object o);

}
