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

  public PyString(String s)
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(s));
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

  private native static long _ctor(String s);
}
