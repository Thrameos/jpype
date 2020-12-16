package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "OSError", exact = true)
public class PyOSError extends PyException
{
  protected PyOSError()
  {
    super();
  }

  protected PyOSError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyOSError(ALLOCATOR, inst);
  }


}
