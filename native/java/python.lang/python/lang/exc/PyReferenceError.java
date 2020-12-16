package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "ReferenceError", exact = true)
public class PyReferenceError extends PyException
{
  protected PyReferenceError()
  {
    super();
  }

  protected PyReferenceError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyReferenceError(ALLOCATOR, inst);
  }


}
