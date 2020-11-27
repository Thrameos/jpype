package python.lang;

import java.util.Collection;
import org.jpype.JPypeContext;
import python.lang.protocol.PySequence;
import org.jpype.python.PyTypeManager;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.CONSTRUCTOR;
import org.jpype.python.internal.PyTupleStatic;
import python.lang.exc.PyException;
import python.lang.exc.PyIndexError;

/**
 * Tuples are read only sequences of items.
 *
 * They may not be modified after being created.
 *
 * @author nelson85
 * @param <E>
 */
@PyTypeInfo(name = "tuple")
public class PyTuple<E> extends PyBaseObject implements PySequence<E>
{

  final static PyTupleStatic TUPLE_STATIC = PyTypeManager.getInstance()
          .createStaticInstance(PyTupleStatic.class);

  protected PyTuple(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  public PyTuple(Collection<? extends E> c)
  {
    super(PyConstructor.CONSTRUCTOR, _ctor(c.toArray()));
  }

  static <E> PyTuple<E> of(E... values)
  {
    return new PyTuple(PyConstructor.CONSTRUCTOR, _ctor(values));
  }

  /**
   * This is equivalent to the Python expression len(o).
   *
   * @return the number of objects in sequence o on success.
   * @throws PyException on failure.
   */
  @Override
  public int size()
  {
    return TUPLE_STATIC.size(this);
  }

  /**
   * Return the object at position pos in the tuple pointed to by p.
   *
   * @param i is the index of the item.
   * @return the item at the index.
   * @throws PyIndexError if the position is out of bounds.
   */
  @Override
  public E get(int i) throws PyIndexError
  {
    return (E) TUPLE_STATIC.get(this, i);
  }

  /**
   * This is the equivalent of the Python expression p[low:high].
   * <p>
   * Indexing from the end of the list is not supported.
   *
   * @param i1
   * @param i2
   * @return the slice of the tuple pointed to by p between low and high.
   * @throws PyException on failure.
   *
   */
  @Override
  public Object getSlice(int i1, int i2) throws PyException
  {
    return TUPLE_STATIC.getSlice(this, i1, i2);
  }

  @Override
  public void setItem(int i, E v)
  {
    throw new UnsupportedOperationException();
  }

  static Object _allocate(long instance)
  {
    return new PyTuple(PyConstructor.ALLOCATOR, instance);
  }

  private native static long _ctor(Object... o);

}
