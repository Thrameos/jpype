package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "KeyError", exact = true)
public class PyKeyError extends PyLookupError
{
  protected PyKeyError()
  {
    super();
  }

  protected PyKeyError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyKeyError(ALLOCATOR, inst);
  }


}
