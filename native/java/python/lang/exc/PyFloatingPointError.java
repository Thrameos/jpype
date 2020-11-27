package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "FloatingPointError", exact = true)
public class PyFloatingPointError extends PyArithmeticError
{
  protected PyFloatingPointError()
  {
    super();
  }

  protected PyFloatingPointError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyFloatingPointError(ALLOCATOR, inst);
  }


}
