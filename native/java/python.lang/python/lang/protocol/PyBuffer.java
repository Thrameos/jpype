package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import python.lang.PyObject;

@PyTypeInfo(name = "protocol.buffer", exact = true)
public interface PyBuffer extends PyObject
{
  // FIXME allow conversion to direct byte buffer
}
