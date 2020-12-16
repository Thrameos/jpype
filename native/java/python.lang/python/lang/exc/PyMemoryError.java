package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "MemoryError", exact = true)
public class PyMemoryError extends PyException
{
  protected PyMemoryError()
  {
    super();
  }

  protected PyMemoryError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyMemoryError(ALLOCATOR, inst);
  }


}
