package python.lang;

import python.lang.protocol.PyNumber;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;

@PyTypeInfo(name = "float")
public class PyFloat extends Number implements PyNumber, PyObject
{

  final long _self;

  protected PyFloat()
  {
    this._self = 0;
  }

  protected PyFloat(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  public PyFloat(double v)
  {
    this(PyConstructor.CONSTRUCTOR, _ctor(v));
  }

  static Object _allocate(long inst)
  {
    return new PyFloat(PyConstructor.ALLOCATOR, inst);
  }

  public static PyFloat of(long d)
  {
    return (PyFloat) PyBuiltins.BUILTIN_STATIC.newFloat(d);
  }

  @Override
  public int intValue()
  {
    return NUMBER_STATIC.intValue(this);
  }

  @Override
  public long longValue()
  {
    return NUMBER_STATIC.longValue(this);
  }

  @Override
  public float floatValue()
  {
    return NUMBER_STATIC.floatValue(this);
  }

  @Override
  public double doubleValue()
  {
    return NUMBER_STATIC.doubleValue(this);
  }

  private native static long _ctor(double d);

}
