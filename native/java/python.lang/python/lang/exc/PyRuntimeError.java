package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "RuntimeError", exact = true)
public class PyRuntimeError extends PyException
{
  protected PyRuntimeError()
  {
    super();
  }

  protected PyRuntimeError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyRuntimeError(ALLOCATOR, inst);
  }


}
