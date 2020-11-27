package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "AttributeError", exact = true)
public class PyAttributeError extends PyException
{
  protected PyAttributeError()
  {
    super();
  }

  protected PyAttributeError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyAttributeError(ALLOCATOR, inst);
  }


}
