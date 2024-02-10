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
package python.lang.protocol;

import java.util.ListIterator;

/**
 *
 * @author nelson85
 */
public class PySequenceListIterator<E> implements ListIterator<E>
{

  PySequence<E> outer;
  int index;
  int last = -1;

  PySequenceListIterator(PySequence<E> outer, int index)
  {
    this.outer = outer;
    this.index = index;
  }

  @Override
  public boolean hasNext()
  {
    return index < outer.size();
  }

  @Override
  public E next()
  {
    E out = outer.get(index);
    last = index;
    index++;
    return out;
  }

  @Override
  public boolean hasPrevious()
  {
    return (index > 0);
  }

  @Override
  public E previous()
  {
    index--;
    last = index;
    E out = outer.get(index);
    return out;
  }

  @Override
  public int nextIndex()
  {
    return index;
  }

  @Override
  public int previousIndex()
  {
    return index - 1;
  }

  @Override
  public void remove()
  {
    if (last == -1)
      throw new IllegalStateException();
    outer.remove(last);
    last = -1;
  }

  @Override
  public void set(E arg0)
  {
    if (last == -1)
      throw new IllegalStateException();
    outer.set(last, arg0);
    last = -1;
  }

  @Override
  public void add(E arg0)
  {
    outer.add(index, arg0);
  }

}
