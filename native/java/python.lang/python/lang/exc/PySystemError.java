package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "SystemError", exact = true)
public class PySystemError extends PyException
{
  protected PySystemError()
  {
    super();
  }

  protected PySystemError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PySystemError(ALLOCATOR, inst);
  }


}
