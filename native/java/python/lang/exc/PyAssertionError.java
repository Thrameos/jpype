package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "AssertionError", exact = true)
public class PyAssertionError extends PyException
{
  protected PyAssertionError()
  {
    super();
  }

  protected PyAssertionError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyAssertionError(ALLOCATOR, inst);
  }


}
