package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "IndexError", exact = true)
public class PyIndexError extends PyLookupError
{
  protected PyIndexError()
  {
    super();
  }

  protected PyIndexError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyIndexError(ALLOCATOR, inst);
  }


}
