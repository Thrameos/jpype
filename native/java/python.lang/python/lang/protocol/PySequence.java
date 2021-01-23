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

import java.lang.reflect.InvocationTargetException;
import java.util.Collection;
import java.util.List;
import java.util.ListIterator;
import java.util.RandomAccess;
import org.jpype.python.PyTypeManager;
import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;
import org.jpype.python.internal.PyBuiltinStatic;
import org.jpype.python.internal.PySequenceStatic;
import python.lang.PyList;
import python.lang.PyTuple;
import python.lang.exc.PyException;

@PyTypeInfo(name = "protocol.sequence", exact = true)
public interface PySequence<E> extends PyCollection<E>, List<E>, RandomAccess
{

  final static PySequenceStatic SEQUENCE_STATIC = PyTypeManager.getInstance()
          .createStaticInstance(PySequenceStatic.class);

  /**
   * Get the length of the sequence.
   * <p>
   * If both sequence and mapping are implemented, then sequence size is used.
   */
  @PyMethodInfo(name = "PyObject_Size", invoke = PyInvocation.AsLong, method = true)
  @Override
  default int size()
  {
    // PyMethodInfo is used to ensure that derived classes will override if both
    // sequence and mapping are inherited.
    return PyBuiltinStatic.INSTANCE.len(this);
  }

  @Override
  default boolean isEmpty()
  {
    return size() == 0;
  }

  @Override
  default PyIterator<E> iterator()
  {
    return (PyIterator<E>) PyBuiltinStatic.INSTANCE.iter(this);
  }

  /**
   * Get the concatenation of o1 and o2 on success, and NULL on failure.
   * <p>
   * This is the equivalent of the Python expression o1 + o2.
   *
   * @param o2
   * @return
   */
  default Object concat(Object o2)
  {
    return SEQUENCE_STATIC.concat(this, o2);
  }

  /**
   * Get the result of repeating sequence object o count times .
   * <p>
   * This is the equivalent of the Python expression o * count.
   *
   * @param count
   * @return
   */
  default Object repeat(int count)
  {
    return SEQUENCE_STATIC.repeat(this, count);
  }

  /**
   *
   * Get the concatenation of o1 and o2 on success.
   * <p>
   * The operation is done in-place when supported. This is the equivalent of
   * the Python expression o1 += o2.
   *
   * @param o2
   * @return
   */
  default Object assignConcat(Object o2)
  {
    return SEQUENCE_STATIC.assignConcat(this, o2);
  }

  /**
   * Get the result of repeating sequence object o count times.
   * <p>
   * The operation is done in-place when o supports it. This is the equivalent
   * of the Python expression o *= count.
   *
   * @param count
   * @return
   */
  default Object assignRepeat(int count)
  {
    return SEQUENCE_STATIC.assignRepeat(this, count);
  }

  /**
   * Get an element of o.
   *
   * <p>
   * This is the equivalent of the Python expression o[i].
   *
   * @throws PyException on failure.
   */
  @Override
  default E get(int index) throws PyException
  {
    return (E) SEQUENCE_STATIC.get(this, index);
  }

  /**
   * Get the slice of sequence object o between i1 and i2.
   * <p>
   * This is the equivalent of the Python expression o[i1:i2].
   *
   * @param i1
   * @param i2
   * @return
   * @throws PyException on failure.
   */
  default Object getSlice(int i1, int i2) throws PyException
  {
    return SEQUENCE_STATIC.getSlice(this, i1, i2);
  }

  /**
   * Assign a value to a position.
   * <p>
   * This is the equivalent of the Python statement o[i] = v.
   *
   * @param index
   * @param value
   * @throws PyException on failure.
   */
  @Override
  default E set(int index, E value) throws PyException
  {
    E out = get(index);
    setItem(index, value);
    return out;
  }

  default void setItem(int index, E value) throws PyException
  {
    SEQUENCE_STATIC.setItem(this, index, value);
  }

  /**
   * Delete the ith element.
   * <p>
   * This is the equivalent of the Python statement del o[i].
   *
   * @param index
   * @throws PyException on failure.
   */
  default void delItem(int index) throws PyException
  {
    SEQUENCE_STATIC.delItem(this, index);
  }

  /**
   * Assign the sequence object v to the slice in sequence object o from i1 to
   * i2.
   * <p>
   * This is the equivalent of the Python statement o[i1:i2] = v.
   *
   * @param start is the start of the assignment.
   * @param end is the end of the assignment (exclusive).
   * @param value
   * @throws PyException on failure.
   *
   */
  default void setSlice(int start, int end, Object value)
  {
    SEQUENCE_STATIC.setSlice(this, start, end, value);
  }

  /**
   * Delete the slice in sequence.
   * <p>
   * This is the equivalent of the Python statement del o[i1:i2].
   *
   * @param start is the start of the deletion.
   * @param end is the end of the deletion (exclusive).
   * @throws PyException on failure.
   *
   */
  default void delSlice(int start, int end) throws PyException
  {
    SEQUENCE_STATIC.delSlice(this, start, end);
  }

  /**
   * Count the number of occurrences of a value.
   * <p>
   * This is equivalent to the Python expression o.count(value).
   *
   * @param value
   * @return the number of keys for which o[key] == value.
   * @throws PyException on failure.
   *
   */
  default int count(E value) throws PyException
  {
    return SEQUENCE_STATIC.count(this, value);
  }

  /**
   * Determine if o contains value.
   * <p>
   * This is equivalent to the Python expression value in o.
   *
   * @param value
   * @return true if an item in o is equal to value, otherwise false.
   * @throws PyException on failure.
   */
  @Override
  default boolean contains(Object value) throws PyException
  {
    return SEQUENCE_STATIC.contains(this, value);
  }

  /**
   * Get the index the first index i for which o[i] == value.
   *
   * @param value
   * @return the index or -1 if not found.
   * @throws PyException on failure.
   */
  @Override
  default int indexOf(Object value) throws PyException
  {
    return SEQUENCE_STATIC.indexOf(this, value);
  }

  /**
   *
   * Get a list object with the same contents as the sequence or iterable.
   * <p>
   * The returned list is guaranteed to be new.
   * <p>
   * This is equivalent to the Python expression list(o).
   *
   * @return
   * @throws PyException on failure.
   */
  default PyList asList() throws PyException
  {
    return SEQUENCE_STATIC.asList(this);
  }

  /**
   *
   * Get a tuple object with the same contents as the sequence or iterable.
   * <p>
   * If this is a tuple, a new reference will be returned, otherwise a tuple
   * will be constructed with the appropriate contents.
   * <p>
   * This is equivalent to the Python expression tuple(o).
   *
   * @return a tuple with the same values.
   */
  default PyTuple asTuple()
  {
    return SEQUENCE_STATIC.asTuple(this);
  }

  @Override
  default Object[] toArray()
  {
    int n = this.size();
    Object[] out = new Object[this.size()];
    for (int i = 0; i < n; ++i)
      out[i] = this.get(i);
    return out;
  }

  @Override
  default <T> T[] toArray(T[] a)
  {
    int n = this.size();
    if (a.length != n)
      try
    {
      a = (T[]) a.getClass().getConstructor(int.class).newInstance(n);
    } catch (NoSuchMethodException | SecurityException | InstantiationException | IllegalAccessException | IllegalArgumentException | InvocationTargetException ex)
    {
      throw new RuntimeException(ex);
    }
    for (int i = 0; i < n; ++i)
      a[i] = (T) this.get(i);
    return a;
  }

  @Override
  default boolean add(E e)
  {
    int size = this.size();
    SEQUENCE_STATIC.setSlice(this, size, size, PyTuple.of(e));
    return true;
  }
  
  @Override
  default boolean remove(Object o)
  {
    int i = this.indexOf(o);
    if (i==-1)
      return false;
    SEQUENCE_STATIC.delItem(o, i);
    return true;
  }

  @Override
  default boolean containsAll(Collection<?> c)
  {
    for (Object v : c)
    {
      if (!this.contains(v))
        return false;
    }
    return true;
  }

  @Override
  default boolean addAll(Collection<? extends E> c)
  {
    SEQUENCE_STATIC.assignConcat(this, c);
    return true;
  }

  @Override
  default boolean addAll(int index, Collection<? extends E> c)
  {
    if (c.isEmpty())
      return false;
    int size = this.size();
    SEQUENCE_STATIC.setSlice(this, size, size, new PyTuple(c));
    return true;
  }

  @Override
  @SuppressWarnings("element-type-mismatch")
  default boolean removeAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("Not supported");
  }

  @Override
  default boolean retainAll(Collection<?> c)
  {
    throw new UnsupportedOperationException("Not supported");
  }

  @Override
  default void clear()
  {
    SEQUENCE_STATIC.delSlice(this, 0, this.size());
  }

  @Override
  default void add(int index, E element)
  {
    SEQUENCE_STATIC.setSlice(this, index, index, PyTuple.of(element));
  }

  @Override
  default E remove(int index)
  {
    Object out = SEQUENCE_STATIC.get(this, index);
    SEQUENCE_STATIC.delItem(this, index);
    return (E) out;
  }

  @Override
  default int lastIndexOf(Object o)
  {
    return SEQUENCE_STATIC.indexOf(this, o);
  }

  @Override
  default ListIterator<E> listIterator()
  {
    return new PySequenceListIterator<>(this, 0);
  }

  @Override
  default ListIterator<E> listIterator(int index)
  {
    return new PySequenceListIterator<>(this, index);
  }

  @Override
  default List<E> subList(int fromIndex, int toIndex)
  {
    // It is not clear if this will actually work because the slice may
    // be a view or copy.  We may have to reimplement in Java. 
    return (PySequence<E>) SEQUENCE_STATIC.getSlice(this, fromIndex, toIndex);
  }

}
