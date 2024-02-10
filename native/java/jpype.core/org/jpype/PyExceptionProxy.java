package org.jpype;

/**
 *
 * @author nelson85
 */
public class PyExceptionProxy extends RuntimeException
{

  long cls;
  long value;

  public PyExceptionProxy(long l0, long l1)
  {
    cls = l0;
    value = l1;
  }

  @Override
  public String getMessage()
  {
    return _getMessage(this.cls, this.value);
  }
  
  private native static String _getMessage(long cls, long value);

}
