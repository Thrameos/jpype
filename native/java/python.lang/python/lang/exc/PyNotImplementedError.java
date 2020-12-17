package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "NotImplementedError", exact = true)
public class PyNotImplementedError extends PyRuntimeError
{
  protected PyNotImplementedError()
  {
    super();
  }

  protected PyNotImplementedError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyNotImplementedError(ALLOCATOR, inst);
  }


}
