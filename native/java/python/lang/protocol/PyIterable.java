package python.lang.protocol;

import java.util.Iterator;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBuiltinStatic;

/**
 *
 * @param <E>
 */
@PyTypeInfo(name = "protocol.iterable", exact = true)
public interface PyIterable<E> extends Iterable<E>
{
  @Override
  default PyIterator<E> iterator()
  {
    return (PyIterator<E>) PyBuiltinStatic.INSTANCE.iter(this);
  }
}
