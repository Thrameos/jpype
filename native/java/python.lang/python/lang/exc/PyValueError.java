package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "ValueError", exact = true)
public class PyValueError extends PyException
{
  protected PyValueError()
  {
    super();
  }

  protected PyValueError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyValueError(ALLOCATOR, inst);
  }


}
