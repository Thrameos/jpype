package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;

@PyTypeInfo(name = "protocol.collection", exact = true)
public interface PyCollection<E> extends PySized, PyIterable<E>, PyContainer
{

}
