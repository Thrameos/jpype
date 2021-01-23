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

import python.lang.protocol.PyMapping;
import java.lang.reflect.InvocationTargetException;
import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import org.jpype.python.Types;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;
import org.jpype.python.internal.PyDictStatic;
import python.lang.exc.PyException;
import python.lang.exc.PyTypeError;

@PyTypeInfo(name = "dict")
public class PyDict<K, V> extends PyBaseObject implements Map<K, V>, PyMapping<K, V>
{

  final static PyDictStatic DICT_STATIC = Types.newInstance(PyDictStatic.class);

  protected PyDict(PyConstructor key, long instance)
  {
    super(key, instance);
    HashMap h;
  }

  public PyDict()
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(null));
  }

  public PyDict(Map<? extends K, ? extends V> m)
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(m));
  }

  static Object _allocate(long instance)
  {
    return new PyDict(PyConstructor.ALLOCATOR, instance);
  }

  /**
   * MappingProxyType object for a mapping which enforces read-only behavior.
   * <p>
   * This is normally used to create a view to prevent modification of the
   * dictionary for non-dynamic class types.
   *
   * @return
   */
  public PyDict asReadOnly()
  {
    return DICT_STATIC.asReadOnly(this);
  }

  /**
   * Empty an existing dictionary of all key-value pairs.
   *
   */
  @Override
  public void clear()
  {
    DICT_STATIC.clear(this);
  }

  /**
   * Determine if dictionary p contains key.If an item in p is matches key,
   * return true, otherwise return false.
   *
   *
   * @param key
   * @return
   * @throws PyException on error.
   */
  @Override
  public boolean containsKey(Object key) throws PyException
  {
    return DICT_STATIC.contains(this, key);
  }

  @Override
  public boolean containsValue(Object value)
  {
    return this.values().contains(value);
  }

  /**
   *
   * Return a new dictionary that contains the same key-value pairs as p.
   *
   * @return a copy of the dictionary.
   */
  @Override
  public PyDict<K, V> clone() throws CloneNotSupportedException
  {
    return DICT_STATIC.clone(this);
  }

  /**
   * Insert val into the dictionary p with a key of key.
   *
   * @param key
   * @param value
   * @return the previous value associated with {@code key}, or {@code null} if
   * there was no mapping for {@code key}. (A {@code null} return can also
   * indicate that the map previously associated {@code null} with {@code key},
   * if the implementation supports {@code null} values.)
   * @throws PyTypeError if the key is not hashable.
   */
  @Override
  public V put(K key, V value) throws PyTypeError
  {
    V out = get(key);
    setItem(key, value);
    return out;
  }

  /**
   * Insert val into the dictionary p using key as a key.
   * <p>
   * The key object is created using PyUnicode_FromString(key).
   *
   * Return 0 on success or -1 on failure. This function does not steal a
   * reference to val.
   *
   * @param key
   * @param value
   */
  @Override
  public void setItem(K key, V value) throws PyTypeError
  {
    if (key instanceof String)
      DICT_STATIC.setItemString(this, (String) key, value);
    else
      DICT_STATIC.setItem(this, key, value);
  }

  /**
   * Remove the entry in dictionary p with key key.
   *
   * @param key
   * @throws PyTypeError if the key is not hashable.
   */
  @Override
  public void delItem(Object key) throws PyTypeError
  {
    if (key instanceof String)
      DICT_STATIC.delItemString(this, (String) key);
    else
      DICT_STATIC.delItem(this, key);
  }

  @Override
  public V remove(Object key)
  {
    V out = get(key);
    delItem(key);
    return out;
  }

  /**
   * Return the object from dictionary p which has a key key.
   * <p>
   * This does not set an exception if the key is not found.
   * <p>
   * Note that exceptions which occur while calling __hash__() and __eq__()
   * methods will get suppressed.
   *
   * To get error reporting use PyDict_GetItemWithError() instead.
   *
   * @param key is the key to match in the dictionary.
   * @return the value associated with the key or null if key is not present.
   */
  @Override
  public V get(Object key)
  {
    if (key instanceof String)
      return (V) DICT_STATIC.getItemString(this, (String) key);
    else
      return (V) DICT_STATIC.getItem(this, (String) key);
  }

  /**
   * Variant of PyDict_GetItem() that does not suppress exceptions.
   *
   * @param key is the key to match in the dictionary.
   * @return the value associated with the key.
   * @throws PyException is the key is not found.
   */
  public Object getItemWithError(Object key) throws PyException
  {
    return DICT_STATIC.getItemWithError(this, key);
  }

  /**
   * This is the same as the Python-level dict.setdefault().
   *
   * If present, it returns the value corresponding to key from the dictionary
   * p. If the key is not in the dict, it is inserted with value defaultobj and
   * defaultobj is returned. This function evaluates the hash function of key
   * only once, instead of evaluating it independently for the lookup and the
   * insertion.
   *
   * New in version 3.4.
   *
   * @param key
   * @param defaultobj
   * @return
   */
  public V setDefault(K key, V defaultobj)
  {
    return (V) DICT_STATIC.setDefault(this, key, defaultobj);
  }

  /**
   * Get a list containing all the items from the dictionary.
   *
   * @return
   */
  @Override
  public PyList items()
  {
    return DICT_STATIC.items(this);
  }

  @Override
  public Set<Entry<K, V>> entrySet()
  {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  /**
   * Get a list containing all the keys from the dictionary.
   *
   * @return
   */
  @Override
  public PyList<K> keys()
  {
    return DICT_STATIC.keys(this);
  }

  @Override
  public Set<K> keySet()  // FIXME this should be a view
  {
    return new PyDictKeySet();
  }

  /**
   * Return a PyList containing all the values from the dictionary p.
   *
   * @return a new list containing the items the dictionary.
   */
  @Override
  public PyList<V> values()
  {
    return DICT_STATIC.values(this);
  }

  /**
   * Return the number of items in the dictionary.
   * <p>
   * This is equivalent to len(p) on a dictionary.
   *
   * @return the number of items in the dictionary.
   */
  @Override
  public int size()
  {
    return DICT_STATIC.size(this);
  }

  /**
   * Iterate over mapping object b adding key-value pairs to dictionary.
   * <p>
   * dict may be a dictionary, or any object supporting PyMapping_Keys() and
   * PyObject_GetItem().
   * <p>
   * If override is true, existing pairs in a will be replaced if a matching key
   * is found in b, otherwise pairs will only be added if there is not a
   * matching key in a.
   *
   * Return 0 on success or -1 if an exception was raised.
   *
   * @param dict
   * @param override
   */
  public void merge(Object dict, boolean override) throws PyException
  {
    DICT_STATIC.merge(this, dict, override);
  }

  @Override
  public void putAll(Map<? extends K, ? extends V> m)
  {
    merge(m, true);
  }

  /**
   * This is the same as PyDict_Merge(a, b, 1) in C, and is similar to
   * a.update(b) in Python except that PyDict_Update() doesn’t fall back to the
   * iterating over a sequence of key value pairs if the second argument has no
   * “keys” attribute.
   *
   * @param b
   * @throws PyException is the key is not found.
   */
  public void update(Object b) throws PyException
  {
    DICT_STATIC.update(this, b);
  }

  /**
   * Update or merge into dictionary a, from the key-value pairs in seq2.
   * <p>
   * seq2 must be an iterable object producing iterable objects of length 2,
   * viewed as key-value pairs.
   *
   * In case of duplicate keys, the last wins if override is true, else the
   * first wins.
   *
   * @throws PyException is the key is not found.
   */
  public void mergeFromSeq2(Object seq2, boolean override) throws PyException
  {
    DICT_STATIC.mergeFromSeq2(this, seq2, override);
  }

  @Override
  public boolean isEmpty()
  {
    return size() == 0;
  }

  class PyDictKeySet<K> implements Set<K>
  {

    @Override
    public int size()
    {
      return PyDict.this.size();
    }

    @Override
    public boolean isEmpty()
    {
      return PyDict.this.isEmpty();
    }

    @Override
    public boolean contains(Object o)
    {
      return PyDict.this.containsKey(o);
    }

    @Override
    public Iterator<K> iterator()
    {
      return PyBuiltins.iter(PyDict.this);
    }

    @Override
    public Object[] toArray()
    {
      Object[] out = new Object[size()];
      int i = 0;
      for (Object s : PyDict.this.keys())
      {
        out[i++] = s;
      }
      return out;
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
      for (Object s : PyDict.this.keys())
      {
        a[i++] = (T) s;
      }
      return a;
    }

    @Override
    public boolean add(K e)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean remove(Object o)
    {
      return PyDict.this.remove(o) != null;
    }

    @Override
    public boolean containsAll(Collection<?> c)
    {
      for (Object u : c)
      {
        if (!PyDict.this.containsKey(u))
          return false;
      }
      return true;
    }

    @Override
    public boolean addAll(Collection<? extends K> c)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean retainAll(Collection<?> c)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean removeAll(Collection<?> c)
    {
      boolean changed = false;
      for (Object u : c)
      {
        if (remove(u))
          changed = true;
      }
      return changed;
    }

    @Override
    public void clear()
    {
      PyDict.this.clear();
    }

  }

  class PyDictEntrySet implements Set<Entry<K, V>>
  {

    @Override
    public int size()
    {
      return PyDict.this.size();
    }

    @Override
    public boolean isEmpty()
    {
      return PyDict.this.isEmpty();
    }

    @Override
    public boolean contains(Object o)
    {
      return false;
    }

    @Override
    public Iterator<Entry<K, V>> iterator()
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public Object[] toArray()
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public <T> T[] toArray(T[] a)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean add(Entry<K, V> e)
    {
      return PyDict.this.put((K) e.getKey(), (V) e.getValue()) != null;
    }

    @Override
    public boolean remove(Object o)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean containsAll(Collection<?> c)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean addAll(Collection<? extends Entry<K, V>> c)
    {
      boolean changed = false;
      for (Entry<K, V> e : c)
      {
        if (this.add(e))
          changed = true;
      }
      return changed;
    }

    @Override
    public boolean retainAll(Collection<?> c)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public boolean removeAll(Collection<?> c)
    {
      throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
    }

    @Override
    public void clear()
    {
      PyDict.this.clear();
    }

  }

  private static native long _ctor(Object o);

}
