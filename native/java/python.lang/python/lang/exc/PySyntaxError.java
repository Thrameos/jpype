package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;

@PyTypeInfo(name = "SyntaxError", exact = true)
public class PySyntaxError extends PyException
{
  protected PySyntaxError()
  {
    super();
  }

  protected PySyntaxError(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  static Object _allocate(long inst)
  {
    return new PySyntaxError(ALLOCATOR, inst);
  }


}
