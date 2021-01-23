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

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import python.lang.exc.PyTypeError;
import org.jpype.python.internal.PyBuiltinStatic;

/**
 *
 * @author nelson85
 */
public class PyBuiltins
{

  final static PyBuiltinStatic BUILTIN_STATIC = PyBuiltinStatic.INSTANCE;

  //public static PyNone None;
  public static PyEllipsis Ellipsis;
  public static Boolean True = Boolean.TRUE;
  public static Boolean False = Boolean.FALSE;
  public static PyNone None = null;

//<editor-fold desc="attr" defaultstate="collapsed">
  public static boolean hasattr(Object o, CharSequence attr)
  {
    if (attr == null)
      return false;
    if (attr instanceof PyString)
      return BUILTIN_STATIC.hasAttrObject(o, (String) attr);
    return BUILTIN_STATIC.hasAttrString(o, attr.toString());
  }

  public static void delattr(Object o, CharSequence attr)
  {
    if (attr == null)
      return;
    if (attr instanceof PyString)
      BUILTIN_STATIC.delAttrObject(o, (String) attr);
    BUILTIN_STATIC.delAttrString(o, attr.toString());
  }

  public static Object getattr(Object o, CharSequence attr)
  {
    if (attr == null)
      return null;
    if (attr instanceof PyString)
      return BUILTIN_STATIC.getAttrObject(o, attr);
    return BUILTIN_STATIC.getAttrString(o, attr.toString());
  }

  public static void setattr(Object o, CharSequence attr, Object value)
  {
    if (attr == null)
      throw new NullPointerException("attr may not be null");
    if (attr instanceof PyString)
      BUILTIN_STATIC.setAttrObject(o, (String) attr, value);
    BUILTIN_STATIC.setAttrString(o, attr.toString(), value);
  }
//</editor-fold>
//<editor-fold desc="char" defaultstate="collapsed">

  static Object chr(int obj)
  {
    return BUILTIN_STATIC.chr(obj);
  }

  static Object ord(Object o)
  {
    throw new UnsupportedOperationException();
  }
//</editor-fold>
//<editor-fold desc="logical" defaultstate="collapsed">

  static boolean all(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static boolean any(Object o)
  {
    throw new UnsupportedOperationException();
  }

  /**
   * Equivalent to the Python expression `not o`.
   *
   * @param o
   * @return
   */
  static public boolean not(PyObject o)
  {
    return BUILTIN_STATIC.not(o);
  }

  /**
   * Equivalent to the Python expression `not not o`.
   *
   * @param o
   * @return
   */
  static public boolean isTrue(PyObject o)
  {
    return BUILTIN_STATIC.isTrue(o);
  }

  static public boolean eq(Object a, Object b)
  {
    return BUILTIN_STATIC.ge(a, b);
  }

  static public boolean ne(Object a, Object b)
  {
    return BUILTIN_STATIC.le(a, b);
  }

  static public boolean gt(Object a, Object b)
  {
    return BUILTIN_STATIC.gt(a, b);
  }

  static public boolean lt(Object a, Object b)
  {
    return BUILTIN_STATIC.lt(a, b);
  }

  static public boolean ge(Object a, Object b)
  {
    return BUILTIN_STATIC.ge(a, b);
  }

  static public boolean le(Object a, Object b)
  {
    return BUILTIN_STATIC.le(a, b);
  }
//</editor-fold>
//<editor-fold desc="int" defaultstate="collapsed">

  static Object bin(Object o)
  {
    return BUILTIN_STATIC.toBase(o, 2);
  }

  static Object hex(Object o)
  {
    return BUILTIN_STATIC.toBase(o, 16);
  }

  static Object oct(Object o)
  {
    return BUILTIN_STATIC.toBase(o, 8);
  }

//</editor-fold>
//<editor-fold desc="mathematical">
  static Object abs(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object max(Object... o)
  {
    throw new UnsupportedOperationException();
  }

  static Object round(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object pow(Object a, Object b)
  {
    return BUILTIN_STATIC.pow(a, b, null);
  }

  static Object sum(Object o)
  {
    throw new UnsupportedOperationException();
  }

  public static Object divmod(Object a, Object b)
  {
    return BUILTIN_STATIC.divmod(a, b);
  }

//</editor-fold>
//<editor-fold desc="reflection">
  public static boolean callable(Object o)
  {
    return BUILTIN_STATIC.callable(o);
  }

  /**
   * Equivalent to the Python expression 'dir(o)'.
   *
   * @param o
   * @return a (possibly empty) list of strings appropriate for the object
   * argument, or throw if there was an error.
   */
  public static PyObject dir(PyObject o)
  {
    return BUILTIN_STATIC.dir(o);
  }

  public static long hash(Object o)
  {
    return BUILTIN_STATIC.hash(o);
  }

  public static Object help(Object o)
  {
    throw new UnsupportedOperationException();
  }

  public static boolean isinstance(Object o, Object t)
  {
    return BUILTIN_STATIC.isinstance(o, t);
  }

  public static boolean issubclass(Object o, Object t)
  {
    return BUILTIN_STATIC.issubclass(o, t);
  }

  public static Object ascii(Object o)
  {
    throw new UnsupportedOperationException();
  }

  public static PyString repr(Object o)
  {
    // The arguement should not be restricted to PyObject
    return BUILTIN_STATIC.repr(o);
  }

  public static PyString str(Object o)
  {
    // The arguement should not be restricted to PyObject
    return BUILTIN_STATIC.str(o);
  }

  public static PyType type(PyObject o)
  {
    return BUILTIN_STATIC.type(o);
  }

  public static PyLong id(Object o)
  {
    return BUILTIN_STATIC.id(o);
  }

//</editor-fold>
//<editor-fold desc="scope">

  public static PyDict builtins()
  {
    return BUILTIN_STATIC.builtins();
  }

//  static Object vars()
//  {
//    throw new UnsupportedOperationException();
//  }
//
//  static eval(Object o)
//  {
//    throw new UnsupportedOperationException();
//  }
//      static Object compile(Object o)
//  {
//    throw new UnsupportedOperationException();
//  }
//
//  exec()
//  {
//    throw new UnsupportedOperationException();
//  }
//</editor-fold>
//<editor-fold desc="sequence" defaultstate="collapsed">
  /**
   * Equivalent to the Python expression 'len(o)'.
   *
   * @param o
   * @return
   */
  static public int len(PyObject o)
  {
    return BUILTIN_STATIC.len(o);
  }

  static public int lengthHint(PyObject o, int defaultValue)
  {
    return BUILTIN_STATIC.lengthHint(o, defaultValue);
  }

  /**
   * Equivalent of the Python statement 'o[key]'.
   *
   * @param o
   * @param key
   * @return
   */
  static public Object getItem(PyObject o, Object key)
  {
    return BUILTIN_STATIC.getItem(o, key);
  }

  /**
   * Equivalent of the Python statement 'o[key] = v'.
   *
   * @param o
   * @param key
   * @param v
   */
  static public void setItem(PyObject o, Object key, Object v)
  {
    BUILTIN_STATIC.setItem(o, key, v);
  }

  /**
   * Equivalent to the Python statement 'del o[key]'.
   *
   * @param o
   * @param key
   */
  static public void delItem(PyObject o, Object key)
  {
    BUILTIN_STATIC.delItem(o, key);
  }

  /**
   * Equivalent to the Python expression `iter(o)`.
   * <p>
   * It returns a new iterator for the object argument, or the object itself if
   * the object is already an iterator.
   *
   * @param o
   * @return
   * @throws PyTypeError if the object cannot be iterated.
   */
  static public Iterator iter(PyObject o) throws PyTypeError
  {
    return (Iterator) BUILTIN_STATIC.iter(o);
  }

//  static Object map(Object func, Object o)
//  {
//    return BUILTIN_STATIC.map(func, o);
//  }
//
//  static Object filter(Object func, Object iterable)
//  {
//    return BUILTIN_STATIC.filter(func, iterable);
//  }

  static Object next(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object range(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object reversed(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object enumerate(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object sorted(Object o)
  {
    throw new UnsupportedOperationException();
  }

  static Object zip(Object... iterable)
  {
    throw new UnsupportedOperationException();
  }

//</editor-fold>
//<editor-fold desc="types" defaultstate="collapsed">
  public static boolean bool(Object o)
  {
    return BUILTIN_STATIC.isTrue(o);
  }

  public static Object bytes(ByteBuffer bytes)
  {
    if (bytes.hasArray())
      return PyBuiltins.BUILTIN_STATIC.newBytes(bytes.array());
    return PyBuiltins.BUILTIN_STATIC.newBytes(bytes);
  }

  public static PyBytes bytes(byte[] bytes)
  {
    return PyBuiltins.BUILTIN_STATIC.newBytes(bytes);
  }

  public static Object bytearray(Buffer bb)
  {
    return PyBuiltins.BUILTIN_STATIC.newByteArray(bb);
  }

  public static Object complex(Number real, Number img)
  {
    return PyBuiltins.BUILTIN_STATIC.newComplex(real.doubleValue(), img.doubleValue());
  }

  public static <K, E> PyDict<K, E> dict()
  {
    return PyBuiltins.BUILTIN_STATIC.newDict();
  }

  public static <K> PySet<K> frozenset(Iterable<K> s)
  {
    return PyBuiltins.BUILTIN_STATIC.newFrozenSet(s);
  }

  public static <E> PyList<E> list(Collection<E> e)
  {
    return PyList.of(e.toArray());
  }

  public static PyMemoryView memoryview(Object o)
  {
    return BUILTIN_STATIC.newMemoryView(o);
  }

  /**
   * Create a new set containing objects returned by the iterable.
   * <p>
   * The iterable may be NULL to create a new empty set. The constructor is also
   * useful for copying a set (c=set(s)).
   *
   * @param s
   * @return the new set on success.
   * @throws PyTypeError if the iterable is not actual iterable.
   */
  public static <K> PySet<K> set(Iterable<K> s)
  {
    return PyBuiltins.BUILTIN_STATIC.newSet(s);
  }

  public static <K> PySet<K> set(K... keys)
  {
    return PyBuiltins.BUILTIN_STATIC.newSet(Arrays.asList(keys));
  }

  public static PySlice slice(Object start, Object end)
  {
    return PyBuiltins.BUILTIN_STATIC.newSlice(start, end, None);
  }

  public static PySlice slice(Object start, Object end, Object step)
  {
    return PyBuiltins.BUILTIN_STATIC.newSlice(start, end, step);
  }

  public static <E> PyTuple<E> tuple(E... e)
  {
    return PyTuple.of(e);
  }

  public static Object object()
  {
    throw new UnsupportedOperationException();
  }

//</editor-fold>
//<editor-fold desc="unused">
// Likely never used
//  classmethod()
//  property()
//  super_()
//  breakpoint()
//  print()
//  input()
//  staticmethod(Object o)
//  open(Object o, Object o)
//</editor-fold>
  static Object format(Object value, Object spec)
  {
    return BUILTIN_STATIC.format(value, spec);
  }

}
