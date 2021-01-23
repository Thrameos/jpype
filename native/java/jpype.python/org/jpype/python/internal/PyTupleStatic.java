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
import python.lang.exc.PyIndexError;

/**
 *
 * @author nelson85
 */
public interface PyTupleStatic
{

  @PyMethodInfo(name = "PyTuple_Size", 
          invoke = PyInvocation.AsInt, 
          method = false)
  int size(Object self);

  @PyMethodInfo(name = "PyTuple_GetItem", 
          invoke = PyInvocation.BinaryInt, 
          method = false, 
          flags = PyMethodInfo.BORROWED)
  Object get(Object self, int i) throws PyIndexError;

  @PyMethodInfo(name = "PyTuple_GetSlice", 
          invoke = PyInvocation.GetSlice, 
          method = false)
  Object getSlice(Object self, int i1, int i2) throws PyException;

}
