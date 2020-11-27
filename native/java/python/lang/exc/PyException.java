package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "Exception", exact = true)
public class PyException extends PyBaseException
{
  protected PyException()
  {
    super();
  }

  protected PyException(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyException(ALLOCATOR, inst);
  }


}
