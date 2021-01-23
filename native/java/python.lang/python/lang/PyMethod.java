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
package python.lang;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;

@PyTypeInfo(name = "method")
public interface PyMethod extends PyObject
{

  /**
   * Return a new method object, with func being any callable object and self
   * the instance the method should be bound .
   *
   * @param func is the function that will be called when the method is called.
   * @param self must not be null.
   * @return
   */
  static PyMethod of(Object func, Object self)
  {
    return PyBuiltins.BUILTIN_STATIC.newMethod(func, self);
  }

  /**
   * Get the function object associated with the method meth.
   *
   * @return
   */
  @PyMethodInfo(name = "PyMethod_Function", invoke = PyInvocation.Unary, method = true,  flags = PyMethodInfo.BORROWED)
  Object getFunction();

  /**
   * Get the instance associated with the method meth.
   *
   * @return
   */
  @PyMethodInfo(name = "PyMethod_Self", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getSelf();

}
