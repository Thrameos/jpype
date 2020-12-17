package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "LookupError", exact = true)
public class PyLookupError extends PyException
{
  protected PyLookupError()
  {
    super();
  }

  protected PyLookupError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyLookupError(ALLOCATOR, inst);
  }


}
