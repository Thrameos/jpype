package org.jpype.python.internal;

import java.util.Iterator;
import java.util.NoSuchElementException;

/**
 * Customizer for Iterator to have Java semantics.
 *
 * @author nelson85
 */
public abstract class PyIteratorPrivate implements Iterator
{

  public Object element_ = null;

  @Override
  public boolean hasNext()
  {
    if (element_ == null)
      element_ = PyBuiltinStatic.INSTANCE.next(this);
    return element_ != null;
  }

  @Override
  public Object next()
  {
    if (element_ == null)
      element_ = PyBuiltinStatic.INSTANCE.next(this);
    if (element_ == null)
    {
      throw new NoSuchElementException();
    }
    Object out = element_;
    element_ = null;
    return out;
  }

}
