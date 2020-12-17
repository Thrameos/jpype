package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "EOFError", exact = true)
public class PyEOFError extends PyException
{
  protected PyEOFError()
  {
    super();
  }

  protected PyEOFError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyEOFError(ALLOCATOR, inst);
  }


}
