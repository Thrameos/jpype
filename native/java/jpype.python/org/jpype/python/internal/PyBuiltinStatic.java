package org.jpype.python.internal;

import org.jpype.python.PyTypeManager;
import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.PyByteArray;
import python.lang.PyBytes;
import python.lang.PyDict;
import python.lang.PyLong;
import python.lang.PyMemoryView;
import python.lang.PyMethod;
import python.lang.PyModule;
import python.lang.PyObject;
import python.lang.PySet;
import python.lang.PySlice;
import python.lang.PyString;
import python.lang.PyTuple;
import python.lang.PyType;
import python.lang.exc.PyTypeError;
import python.lang.protocol.PyNumber;

/**
 *
 * @author nelson85
 */
public interface PyBuiltinStatic
{

  final static PyBuiltinStatic INSTANCE = PyTypeManager.getInstance()
          .createStaticInstance(PyBuiltinStatic.class);

//<editor-fold desc="sequence" defaultstate="collapsed">
  /**
   * Equivalent to the Python expression 'len(o)'.
   *
   * @return
   */
  @PyMethodInfo(name = "PyObject_Length", invoke = PyInvocation.AsLong, method = false)
  int len(Object o);

  @PyMethodInfo(name = "PyObject_LengthHint", invoke = PyInvocation.IntOperator1, method = false)
  int lengthHint(Object o, int defaultValue);

  /**
   * Equivalent of the Python statement 'o[key]'.
   *
   * @param key
   * @return
   */
  @PyMethodInfo(name = "PyObject_GetItem", invoke = PyInvocation.Binary, method = false)
  Object getItem(Object o, Object key);

  /**
   * Equivalent of the Python statement 'o[key] = v'.
   *
   * @param key
   * @param v
   */
  @PyMethodInfo(name = "PyObject_SetItem", invoke = PyInvocation.Ternary, method = false)
  void setItem(Object o, Object key, Object v);

  /**
   * Equivalent to the Python statement 'del o[key]'.
   *
   * @param key
   */
  @PyMethodInfo(name = "PyObject_DelItem", invoke = PyInvocation.BinaryToInt, method = false)
  void delItem(Object o, Object key);

  /**
   * Equivalent to the Python expression `iter(o)`.
   *
   * It returns a new iterator for the object argument, or the object itself if
   * the object is already an iterator.
   *
   * @return
   * @throws PyTypeError if the object cannot be iterated.
   */
  @PyMethodInfo(name = "PyObject_GetIter", invoke = PyInvocation.Unary, method = false)
  Object iter(Object o) throws PyTypeError;

  //</editor-fold>
  @PyMethodInfo(name = "PyObject_Type", invoke = PyInvocation.Unary, method = false)
  PyType type(Object o);

  @PyMethodInfo(name = "PyObject_Hash", invoke = PyInvocation.AsLong, method = false)
  long hash(Object o);

  @PyMethodInfo(name = "PyObject_Str", invoke = PyInvocation.Unary, method = false)
  PyString str(Object o);

  @PyMethodInfo(name = "PyObject_Repr", invoke = PyInvocation.Unary, method = false)
  PyString repr(Object o);

  /**
   * Equivalent to the Python expression 'dir(o)'.
   *
   * @param o
   * @return a (possibly empty) list of strings appropriate for the object
   * argument, or throw if there was an error.
   */
  @PyMethodInfo(name = "PyObject_Dir", invoke = PyInvocation.Unary, method = false)
  PyObject dir(Object o);

  /**
   * Equivalent to the Python expression `not o`.
   *
   * @param o
   * @return
   */
  @PyMethodInfo(name = "PyObject_Not", invoke = PyInvocation.AsBoolean, method = false)
  boolean not(Object o);

  /**
   * Equivalent to the Python expression `not not o`.
   *
   * @param o
   * @return
   */
  @PyMethodInfo(name = "PyObject_IsTrue", invoke = PyInvocation.AsBoolean, method = false)
  boolean isTrue(Object o);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 2, method = false)
  boolean eq(Object a, Object b);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 3, method = false)
  boolean ne(Object a, Object b);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 4, method = false)
  boolean gt(Object a, Object b);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 0, method = false)
  boolean lt(Object a, Object b);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 5, method = false)
  boolean ge(Object a, Object b);

  @PyMethodInfo(name = "PyObject_RichCompareBool", invoke = PyInvocation.IntOperator2, op = 1, method = false)
  boolean le(Object a, Object b);

  @PyMethodInfo(name = "PyObject_HasAttrString", invoke = PyInvocation.DelStr, method = false)
  boolean hasAttrString(Object o, String name);

  @PyMethodInfo(name = "PyObject_HasAttr", invoke = PyInvocation.BinaryToInt, method = false)
  boolean hasAttrObject(Object o, Object name);

  @PyMethodInfo(name = "PyObject_GetAttrString", invoke = PyInvocation.GetStr, method = false)
  Object getAttrString(Object o, String name);

  @PyMethodInfo(name = "PyObject_GetAttrString", invoke = PyInvocation.Binary, method = false)
  Object getAttr(Object o, Object name);

  @PyMethodInfo(name = "PyObject_SetAttr", invoke = PyInvocation.Ternary, method = false)
  void setAttrObject(Object o, Object name, Object value);

  @PyMethodInfo(name = "PyObject_SetAttrString", invoke = PyInvocation.SetStr, method = false)
  void setAttrString(Object o, String name, Object value);

  @PyMethodInfo(name = "PyObject_DelAttrE", invoke = PyInvocation.Binary, method = false)
  void delAttrObject(Object o, Object name);

  @PyMethodInfo(name = "PyObject_DelAttrStringE", invoke = PyInvocation.GetStr, method = false)
  void delAttrString(Object o, String name);

  @PyMethodInfo(name = "PyObject_GetAttr", invoke = PyInvocation.Binary, method = false)
  Object getAttrObject(Object o, Object string);

  @PyMethodInfo(name = "PyDict_New", invoke = PyInvocation.NoArgs, method = false)
  PyDict newDict();

  @PyMethodInfo(name = "PySet_New", invoke = PyInvocation.Unary, method = false)
  <K> PySet<K> newSet(Iterable<K> s);

  @PyMethodInfo(name = "PyFrozenSet_New", invoke = PyInvocation.Unary, method = false)
  <K> PySet<K> newFrozenSet(Iterable<K> s);

  @PyMethodInfo(name = "PyBytes_FromStringAndSizeE", invoke = PyInvocation.FromJObject, method = false)
  PyBytes newBytes(Object bytes);

  @PyMethodInfo(name = "PyByteArray_FromObject", invoke = PyInvocation.Unary, method = false)
  PyByteArray newByteArray(Object bytes);

  @PyMethodInfo(name = "PyFloat_FromDouble", invoke = PyInvocation.FromDouble, method = false)
  PyNumber newFloat(double d);

  @PyMethodInfo(name = "PySlice_New", invoke = PyInvocation.Ternary, method = false)
  PySlice newSlice(Object start, Object end, Object step);

  @PyMethodInfo(name = "PyMemoryView_FromObject", invoke = PyInvocation.Unary, method = false)
  PyMemoryView newMemoryView(Object obj);

  @PyMethodInfo(name = "PyUnicode_FromOrdinal", invoke = PyInvocation.FromInt, method = false)
  Object chr(int obj);

  @PyMethodInfo(name = "PyObject_Format", invoke = PyInvocation.Binary, method = false)
  Object format(Object value, Object spec);

  @PyMethodInfo(name = "PyNumber_Divmod", invoke = PyInvocation.Binary, method = false)
  Object divmod(Object a, Object b);

  @PyMethodInfo(name = "PyLong_FromVoidPtr", invoke = PyInvocation.Unary, method = false)
  PyLong id(Object o);

  @PyMethodInfo(name = "PyNumber_ToBase", invoke = PyInvocation.BinaryInt, method = false)
  Object toBase(Object o, int i);

  @PyMethodInfo(name = "PyCallable_Check", invoke = PyInvocation.AsInt, method = false)
  boolean callable(Object o);

  @PyMethodInfo(name = "PyEval_GetLocals", invoke = PyInvocation.NoArgs, method = false, flags = PyMethodInfo.BORROWED)
  PyDict locals();

  @PyMethodInfo(name = "PyEval_GetGlobals", invoke = PyInvocation.NoArgs, method = false, flags = PyMethodInfo.BORROWED)
  PyDict globals();

  @PyMethodInfo(name = "PyEval_GetBuiltins", invoke = PyInvocation.NoArgs, method = false, flags = PyMethodInfo.BORROWED)
  PyDict builtins();

  @PyMethodInfo(name = "PyNumber_Power", invoke = PyInvocation.Ternary, method = false)
  Object pow(Object a, Object b, Object object);

  @PyMethodInfo(name = "PyObject_IsInstance", invoke = PyInvocation.BinaryToInt, method = false)
  boolean isinstance(Object o, Object t);

  @PyMethodInfo(name = "PyObject_IsSubclass", invoke = PyInvocation.BinaryToInt, method = false)
  boolean issubclass(Object o, Object t);

  @PyMethodInfo(name = "PyMethod_New", invoke = PyInvocation.Binary, method = false)
  PyMethod newMethod(Object func, Object self);

  @PyMethodInfo(name = "PyObject_Call", invoke = PyInvocation.Ternary, method = false)
  Object call(Object self, PyTuple args, PyDict kwargs);

  @PyMethodInfo(name = "PyIter_Next", invoke = PyInvocation.Unary, method = false, flags=PyMethodInfo.ACCEPT)
  Object next(Object self);
}
