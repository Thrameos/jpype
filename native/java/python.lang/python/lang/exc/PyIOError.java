package python.lang.exc;

import org.jpype.python.annotation.PyTypeInfo;

@PyTypeInfo(name = "IOError", exact = true)
public class PyIOError extends PyBaseException
{
  protected PyIOError()
  {
  }

}
