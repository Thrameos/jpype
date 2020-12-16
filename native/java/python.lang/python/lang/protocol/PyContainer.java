package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import static python.lang.protocol.PySequence.SEQUENCE_STATIC;

@PyTypeInfo(name = "protocol.container", exact = true)
public interface PyContainer
{

  default boolean contains(Object o)
  {
    return SEQUENCE_STATIC.contains(this, o);
  }
}
