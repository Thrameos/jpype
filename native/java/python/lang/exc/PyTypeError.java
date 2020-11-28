package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "TypeError", exact = true)
public class PyTypeError extends PyException
{
  protected PyTypeError()
  {
    super();
  }

  protected PyTypeError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyTypeError(ALLOCATOR, inst);
  }

  public PyTypeError(String value)
  {
    // FIXME this needs to go to the PyException ctor
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }


}
