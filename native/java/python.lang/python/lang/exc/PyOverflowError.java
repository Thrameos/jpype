package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "OverflowError", exact = true)
public class PyOverflowError extends PyArithmeticError
{
  protected PyOverflowError()
  {
    super();
  }

  protected PyOverflowError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyOverflowError(ALLOCATOR, inst);
  }


}
