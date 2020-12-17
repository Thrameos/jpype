package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "KeyboardInterrupt", exact = true)
public class PyKeyboardInterrupt extends PyBaseException
{
  protected PyKeyboardInterrupt()
  {
    super();
  }

  protected PyKeyboardInterrupt(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyKeyboardInterrupt(ALLOCATOR, inst);
  }


}
