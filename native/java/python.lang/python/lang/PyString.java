package python.lang;

import python.lang.protocol.PySequence;
import org.jpype.python.internal.PyStringStatic;
import org.jpype.python.PyTypeManager;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;

@PyTypeInfo(name = "str")
public class PyString extends PyBaseObject implements CharSequence, PySequence<Object>
{

  final static PyStringStatic STRING_STATIC = PyTypeManager.getInstance()
          .createStaticInstance(PyStringStatic.class);

  protected PyString(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  /**
   * Convert to a Python String representation.
   *
   * The actions taken depend on the type of the incoming argument. Existing
   * Python strings are simply referenced again. All other types are converted
   * to a new concrete Python string representation.
   *
   * @param s
   */
  public PyString(CharSequence s)
  {
    super(PyConstructor.CONSTRUCTOR, _fromType(s));
  }

  @Override
  public int length()
  {
    return PyBuiltins.len(this);
  }

  @Override
  public int size()
  {
    return PyBuiltins.len(this);
  }

  @Override
  public char charAt(int index)
  {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  @Override
  public CharSequence subSequence(int start, int end)
  {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  static Object _allocate(long instance)
  {
    return new PyString(PyConstructor.ALLOCATOR, instance);
  }

  /**
   * Delegator based on the incoming type.
   *
   * @param s
   * @return
   */
  private static long _fromType(CharSequence s)
  {
    if (s == null)
      throw new NullPointerException("PyString instances may not null");

    // Give back the reference to the existing object.
    if (s instanceof PyString)
    {
      return PyBaseObject._getSelf((PyBaseObject) s);
    }

    if (s instanceof String)
    {
      return _ctor((String) s);
    }

    // Otherwise convert to String first
    return _ctor(s.toString());
  }
  
  public String toString()
  {
    return _toString(this);
  }

  private native static String _toString(Object self);
  private native static long _ctor(String s);
}
