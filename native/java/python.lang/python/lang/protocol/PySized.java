package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBuiltinStatic;

@PyTypeInfo(name = "protocol.sized", exact = true)
public interface PySized
{

  default int size()
  {
    return PyBuiltinStatic.INSTANCE.len(this);
  }
}
