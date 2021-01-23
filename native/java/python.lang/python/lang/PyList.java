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

import python.lang.protocol.PySequence;
import java.util.Collection;
import java.util.List;
import org.jpype.python.Types;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;
import org.jpype.python.internal.PyListStatic;
import python.lang.exc.PyException;
import python.lang.exc.PyIndexError;

@PyTypeInfo(name = "list")
public class PyList<E> extends PyBaseObject implements PySequence<E>, List<E>
{

  final static PyListStatic LIST_STATIC = Types.newInstance(PyListStatic.class);

  protected PyList(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  public PyList()
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(null));
  }

  public PyList(Collection<? extends E> c)
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(c.toArray()));
  }

  static <E> PyList<E> of(Object... elements)
  {
    return new PyList(PyConstructor.CONSTRUCTOR, _ctor(elements));
  }

  /**
   * Return the object at position index in the list pointed to by list.
   * <p>
   * The position must be non-negative; indexing from the end of the list is not
   * supported.
   *
   * @param index
   * @return
   * @throws PyIndexError if index is out of bounds.
   *
   */
  @Override
  public E get(int index) throws PyIndexError
  {
    return (E) LIST_STATIC.getItem(this, index);
  }

  @Override
  public void setItem(int index, E item) throws PyIndexError
  {
    if (item == null)
      throw new NullPointerException();
    LIST_STATIC.setItem(this, index, item);
  }

  /**
   * Analogous to list.insert(index, item).
   * <p>
   * Insert the item item into list list in front of index index.
   *
   * @param index
   * @param item
   * @throws PyException on failure.
   */
  public void insert(int index, Object item) throws PyException
  {
    if (item == null)
      throw new NullPointerException();
    LIST_STATIC.insert(this, index, item);
  }

  /**
   * Append the object item at the end of list.
   * <p>
   * Analogous to list.append(item).
   *
   * @param item
   * @throws PyException on failure.
   */
  public void append(E item) throws PyException
  {
    if (item == null)
      throw new NullPointerException();
    LIST_STATIC.append(this, item);
  }

  /**
   * Return a list of the objects in list containing the objects between low and
   * high.
   * <p>
   * Analogous to list[low:high]. Indexing from the end of the list is not
   * supported
   *
   * @param low
   * @param high
   * @return
   * @throws PyException on failure.
   */
  @Override
  public Object getSlice(int low, int high) throws PyException
  {
    return LIST_STATIC.getSlice(this, low, high);
  }

  /**
   * Set the slice of list between low and high to the contents of itemlist.
   * <p>
   * Analogous to list[low:high] = itemlist. The itemlist may be NULL,
   * indicating the assignment of an empty list (slice deletion).
   * <p>
   * Indexing from the end of the list is not supported.
   *
   * @param low
   * @param high
   * @param item
   * @throws PyException on failure.
   */
  @Override
  public void setSlice(int low, int high, Object item) throws PyException
  {
    LIST_STATIC.setSlice(this, low, high, item);
  }

  /**
   * Sort the items of list in place.
   * <p>
   * This is equivalent to list.sort().
   *
   * @throws PyException on failure.
   */
  public void sort() throws PyException
  {
    LIST_STATIC.sort(this);
  }

  /**
   * This is the equivalent of list.reverse().
   *
   * Reverse the items of list in place.
   *
   * @throws PyException on failure.
   */
  public void reverse() throws PyException
  {
    LIST_STATIC.reverse(this);
  }

  /**
   * Return a new tuple object containing the contents of list.
   * <p>
   * Equivalent to tuple(list).
   *
   * @return
   */
  @Override
  public PyTuple asTuple()
  {
    return LIST_STATIC.asTuple(this);
  }

  private native static long _ctor(Object[] o);

  static Object _allocate(long instance)
  {
    return new PyList(null, instance);
  }

}
