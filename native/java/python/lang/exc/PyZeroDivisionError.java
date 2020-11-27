package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "ZeroDivisionError", exact = true)
public class PyZeroDivisionError extends PyArithmeticError
{
  protected PyZeroDivisionError()
  {
    super();
  }

  protected PyZeroDivisionError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyZeroDivisionError(ALLOCATOR, inst);
  }


}
