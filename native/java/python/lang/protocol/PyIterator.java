package python.lang.protocol;

import org.jpype.python.internal.PyIteratorPrivate;
import java.util.Iterator;
import org.jpype.python.annotation.PyTypeInfo;
import python.lang.PyObject;

/**
 *
 * @author nelson85
 * @param <E>
 */
@PyTypeInfo(name = "protocol.iterator", exact = true, internal = PyIteratorPrivate.class)
public interface PyIterator<E> extends PyObject, Iterator<E>
{

  @Override
  boolean hasNext();

  @Override
  E next();
}
