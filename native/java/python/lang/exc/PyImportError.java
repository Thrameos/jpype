package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "ImportError", exact = true)
public class PyImportError extends PyException
{
  protected PyImportError()
  {
    super();
  }

  protected PyImportError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyImportError(ALLOCATOR, inst);
  }


}
