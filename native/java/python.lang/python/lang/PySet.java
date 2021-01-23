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

import org.jpype.python.internal.PySetStatic;
import java.lang.reflect.InvocationTargetException;
import java.util.Collection;
import java.util.Set;
import org.jpype.python.Types;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;
import org.jpype.python.internal.PyNumberStatic;
import python.lang.exc.PyKeyError;
import python.lang.exc.PyMemoryError;
import python.lang.exc.PySystemError;
import python.lang.exc.PyTypeError;
import python.lang.protocol.PyCollection;
import python.lang.protocol.PyIterator;

@PyTypeInfo(name = "set")
public class PySet<E> extends PyBaseObject implements Set<E>, PyCollection<E>
{

  final static PySetStatic SET_STATIC = Types.newInstance(PySetStatic.class);

  public PySet()
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(null));
  }

  public PySet(Collection<? extends E> c)
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(c.toArray()));
  }

  protected PySet(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  @Override
  public PyIterator<E> iterator()
  {
    return (PyIterator<E>) PyBuiltins.iter(this);
  }

  @Override
  public int size()
  {
    return SET_STATIC.size(this);
  }

  @Override
  public boolean isEmpty()
  {
    return size() == 0;
  }

  /**
   * Returns true if this set contains the specified element.
   * <p>
   * Unlike the Python __contains__() method, this function does not
   * automatically convert unhashable sets into temporary frozensets.
   *
   * @param key
   * @throws PyTypeError if the key is unhashable.
   * @throws PySystemError if anyset is not a set, frozenset, or an instance of
   * a subtype.
   */
  @Override
  public boolean contains(Object key)
  {
    return SET_STATIC.contains(this, key);
  }

  /**
   * Add key to a set instance.
   * <p>
   * Also works with frozenset instances (like PyTuple_SetItem() it can be used
   * to fill-in the values of brand new frozensets before they are exposed to
   * other code). Return 0 on success or -1 on failure.
   *
   * @param key
   * @throws PyTypeError if the key is unhashable.
   * @throws PyMemoryError if there is no room to grow.
   * @throws PySystemError if set is not an instance of set or its subtype.
   *
   *
   */
  @Override
  public boolean add(E key) throws PyTypeError, PyMemoryError, PySystemError
  {
    boolean b = this.contains(key);
    addItem(key);
    return b;
  }

  public void addItem(E key) throws PyTypeError, PyMemoryError, PySystemError
  {
    SET_STATIC.addItem(this, key);
  }

  /**
   * Remove a key from the set.
   *
   * <p>
   * Does not raise KeyError for missing keys. Unlike the Python discard()
   * method, this function does not automatically convert unhashable sets into
   * temporary frozensets.
   *
   * @param key
   * @return true if found and removed, 0 if not found (no action taken).
   * @throws PyTypeError if the key is unhashable.
   * @throws PySystemError if set is not an instance of set or its subtype.
   *
   */
  @Override
  public boolean remove(Object key) throws PySystemError
  {
    return SET_STATIC.remove(this, key);
  }

  /**
   * Return a new reference to an arbitrary object in the set, and removes the
   * object from the set.
   *
   * <p>
   * @return the object removed.
   * @throws PyKeyError if the set is empty.
   * @throws PySystemError if set is not an instance of set or its subtype.
   *
   */
  public E pop() throws PyKeyError, PySystemError
  {
    return (E) SET_STATIC.pop(this);
  }

  /**
   * Empty an existing set of all elements.
   */
  @Override
  public void clear()
  {
    SET_STATIC.clear(this);
  }

  @Override
  public Object[] toArray()
  {
    Object[] o = new Object[this.size()];
    int i = 0;
    for (E item : this)
    {
      o[i++] = item;
    }
    return o;
  }

  @Override
  public <T> T[] toArray(T[] a)
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
    int i = 0;
    for (E item : this)
    {
      a[i++] = (T) item;
    }
    return a;
  }

  @Override
  public boolean containsAll(Collection<?> c)
  {
    PySet out = (PySet) PyNumberStatic.INSTANCE.and(this, c);
    return out.size() == c.size();
  }

  @Override
  public boolean addAll(Collection<? extends E> c)
  {
    PyNumberStatic.INSTANCE.orAssign(this, c);
    return true; // We assume something was changed.
  }

  @Override
  public boolean retainAll(Collection<?> c)
  {
    PyNumberStatic.INSTANCE.andAssign(this, c);
    return true; // We assume something was changed.
  }

  @Override
  public boolean removeAll(Collection<?> c)
  {
    PyNumberStatic.INSTANCE.subtractAssign(this, c);
    return true; // We assume something was changed
  }

  static Object _allocate(long instance)
  {
    return new PySet(PyConstructor.ALLOCATOR, instance);
  }

  private native static long _ctor(Object[] o);
}
