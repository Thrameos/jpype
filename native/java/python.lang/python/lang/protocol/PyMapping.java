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

import org.jpype.python.Types;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyMappingStatic;
import python.lang.PyObject;
import python.lang.exc.PyException;
import python.lang.exc.PyTypeError;

/**
 * PyMapping is less than a Map and implemented by classes such as PyString and
 * PySequence.
 *
 * @author nelson85
 * @param <K>
 * @param <V>
 */
@PyTypeInfo(name = "protocol.mapping", exact = true)
public interface PyMapping<K, V> extends PySized, PyObject
{
  final static PyMappingStatic MAPPING_STATIC = Types.newInstance(PyMappingStatic.class);

  @Override
  default int size()
  {
    return MAPPING_STATIC.size(this);
  }

  /**
   * Return element of o corresponding to the string key or NULL on failure.This
   * is the equivalent of the Python expression o[key].
   *
   * See also PyObject_GetItem().
   *
   * @param key
   * @return the corresponding value or null if not found.
   */
  default V getItem(K key)
  {
    if (key instanceof String)
      return (V) MAPPING_STATIC.getItemString(this, (String) key);
    else
      return (V) MAPPING_STATIC.getItem(this, (String) key);
  }

  /**
   * This is the equivalent of the Python statement o[key] = v.
   * <p>
   * Map the string key to the value v in object o.
   *
   * See also PyObject_SetItem().
   *
   * @param key
   * @param value
   * @throws PyException on failure.
   */
  default void setItem(K key, V value) throws PyTypeError
  {
    if (key instanceof String)
      MAPPING_STATIC.setItemString(this, (String) key, value);
    else
      MAPPING_STATIC.setItem(this, key, value);
  }

  /**
   * This is equivalent to the Python statement del o[key].
   * <p>
   * Remove the mapping for the object key from the object o.
   *
   * This is an alias of PyObject_DelItem().
   *
   * @param key specifies the entry to be deleted.
   * @throws PyException on failure.
   */
  default void delItem(Object key) throws PyTypeError
  {
    if (key instanceof String)
      MAPPING_STATIC.delItemString(this, (String) key);
    else
      MAPPING_STATIC.delItem(this, key);
  }

  /**
   * Return 1 if the mapping object has the key key and 0 otherwise. This is
   * equivalent to the Python expression key in o. This function always
   * succeeds.
   *
   * Note that exceptions which occur while calling the __getitem__() method
   * will get suppressed. To get error reporting use PyObject_GetItem() instead.
   *
   * @param key
   * @return
   */
  default boolean hasKey(Object key)
  {
    if (key instanceof String)
      return MAPPING_STATIC.hasKeyString(this, (String) key);
    return MAPPING_STATIC.hasKey(this, key);
  }

  /**
   * Return value: New reference.
   *
   * On success, return a list of the keys in object o. On failure, return NULL.
   *
   * Changed in version 3.7: Previously, the function returned a list or a
   * tuple.
   *
   * @return
   */
  default PySequence keys()
  {
    return (PySequence) MAPPING_STATIC.keys(this);
  }

  /**
   * Return value: New reference.On success, return a list of the values in
   * object o.
   *
   * On failure, return NULL.
   *
   * Changed in version 3.7: Previously, the function returned a list or a
   * tuple.
   *
   * @return
   */
  default PySequence values()
  {
    return (PySequence) MAPPING_STATIC.values(this);
  }

  /**
   * Return value: New reference.On success, return a list of the items in
   * object o, where each item is a tuple containing a key-value pair.
   *
   * On failure, return NULL.
   *
   * Changed in version 3.7: Previously, the function returned a list or a tuple
   *
   * @return
   */
  default PySequence items()
  {
    return (PySequence) MAPPING_STATIC.items(this);
  }

}
