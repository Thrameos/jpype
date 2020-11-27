package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "NameError", exact = true)
public class PyNameError extends PyException
{
  protected PyNameError()
  {
    super();
  }

  protected PyNameError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyNameError(ALLOCATOR, inst);
  }


}
