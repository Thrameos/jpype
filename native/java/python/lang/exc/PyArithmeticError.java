package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "ArithmeticError", exact = true)
public class PyArithmeticError extends PyException
{
  protected PyArithmeticError()
  {
    super();
  }

  protected PyArithmeticError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyArithmeticError(ALLOCATOR, inst);
  }


}
