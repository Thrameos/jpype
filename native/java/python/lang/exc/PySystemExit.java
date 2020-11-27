package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "SystemExit", exact = true)
public class PySystemExit extends PyBaseException
{
  protected PySystemExit()
  {
    super();
  }

  protected PySystemExit(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PySystemExit(ALLOCATOR, inst);
  }


}
