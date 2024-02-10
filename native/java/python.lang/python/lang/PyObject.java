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

import org.jpype.python.annotation.PyTypeInfo;

/**
 * Python Object protocol.
 *
 * If it isn't here then it needs to be cast to the appropriate type to access.
 * Use 'obj instanceOf type` for type checks.
 */
@PyTypeInfo(name = "object")
public interface PyObject
{

  /**
   * Equivalent to the Python expression 'hasattr(o, attr_name)'.
   *
   * This function always succeeds.
   *
   * @param name
   * @return
   */
  default boolean hasAttr(String name)
  {
    return PyBuiltins.hasattr(this, name);
  }

  /**
   * Equivalent of the Python expression 'o.attr_name'.
   *
   * @param name
   * @return
   */
  default Object getAttr(CharSequence name)
  {
    return PyBuiltins.getattr(this, name);
  }

  /**
   * Equivalent of the Python statement 'o.attr_name = v'.
   *
   * @param name
   * @param value
   */
  default void setAttr(CharSequence name, Object value)
  {
    PyBuiltins.setattr(this, name, value);
  }

  /**
   * Equivalent of the Python statement 'del o'.
   *
   * @param name
   */
  default void delAttr(CharSequence name)
  {
    PyBuiltins.delattr(this, name);
  }

}
