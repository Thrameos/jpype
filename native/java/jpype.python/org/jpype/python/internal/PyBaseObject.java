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

import static org.jpype.python.internal.PyConstructor.ALLOCATOR;
import python.lang.PyBuiltins;
import python.lang.PyObject;

/**
 * Base implementation.
 */
public class PyBaseObject implements PyObject
{
  long _self;

  @SuppressWarnings("LeakingThisInConstructor")
  public PyBaseObject(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  @Override
  public String toString()
  {
    CharSequence c = PyBuiltins.str(this);
    return c.toString();
  }

  @Override
  public int hashCode()
  {
    long l = PyBuiltins.hash(this);
    int u = (int) (l >> 32);
    return u ^ ((int) l);
  }

  // Stub
  @Override
  public boolean equals(Object obj)
  {
    if (this == obj)
      return true;
    return PyBuiltins.eq(this, obj);
  }

  static Object _allocate(long inst)
  {
    return new PyBaseObject(ALLOCATOR, inst);
  }
  
  /** 
   * Get the internal pointer to PyObject.
   * 
   * @param o
   * @return 
   */
  public static long _getSelf(PyBaseObject o)
  {
    return o._self;
  }

}
