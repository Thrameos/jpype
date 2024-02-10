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

import python.lang.PyBuiltins;
import python.lang.PyEllipsis;
import python.lang.PyNone;

/**
 *
 * @author nelson85
 */
public class PyEllipsisPrivate extends PyBaseObject
{
  public PyEllipsisPrivate(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  protected static Object _allocate(long instance)
  {
    // No need to reference as PyNone is immortal
    if (PyBuiltins.Ellipsis == null)
      PyBuiltins.Ellipsis = (PyEllipsis) new PyEllipsisPrivate(PyConstructor.CONSTRUCTOR, instance);
    return PyBuiltins.Ellipsis;
  }

}
