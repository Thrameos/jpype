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

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * Customizer for Iterator to have Java semantics.
 *
 * @author nelson85
 */
public abstract class PyIteratorPrivate implements Iterator
{

  public Object element_ = null;

  @Override
  public boolean hasNext()
  {
    if (element_ == null)
      element_ = PyBuiltinStatic.INSTANCE.next(this);
    return element_ != null;
  }

  @Override
  public Object next()
  {
    if (element_ == null)
      element_ = PyBuiltinStatic.INSTANCE.next(this);
    if (element_ == null)
    {
      throw new NoSuchElementException();
    }
    Object out = element_;
    element_ = null;
    return out;
  }

}
