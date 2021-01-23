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
package python.lang.exc;

import org.jpype.manager.TypeInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;
import python.lang.PyBuiltins;
import python.lang.PyObject;

/**
 * A container for holding Python exceptions.
 *
 * Java does not have an interface for throwable, so we have to throw a wrapper
 * class that holds the actual Python Exception.
 */
@PyTypeInfo(name = "BaseException", exact = true)
public class PyBaseException extends RuntimeException implements PyObject
{
  long _self;

  public PyBaseException()
  {
    super();
  }

  PyBaseException(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyBaseException(ALLOCATOR, inst);
  }

  @Override
  public String getMessage()
  {
    System.out.println("Get message " + PyBuiltins.str(this));
    return "IMPLEMENT ME"; //PyBuiltins.str(this).toString();
  }

}
