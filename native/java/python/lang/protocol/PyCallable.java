package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBuiltinStatic;
import python.lang.PyDict;
import python.lang.PyTuple;

@PyTypeInfo(name = "protocol.callable", exact = true)
public interface PyCallable
{
  default Object call(PyTuple args, PyDict kwargs)
  {
    return PyBuiltinStatic.INSTANCE.call(this, args, kwargs);
  }
}
