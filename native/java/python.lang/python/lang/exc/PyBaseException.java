package python.lang.exc;

import org.jpype.manager.TypeInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyConstructor;
import static org.jpype.python.internal.PyConstructor.ALLOCATOR;
import python.lang.PyBuiltins;
import python.lang.PyObject;

/**
 * A container for holding Python exceptions.
 *
 * Java does not have an interface for throwable, so we have to throw a wrapper
 * class that holds the actual Python Exception.
 */
@PyTypeInfo(name = "BaseException", exact = true)
public class PyBaseException extends RuntimeException implements PyObject
{
  long _self;

  public PyBaseException()
  {
    super();
  }

  PyBaseException(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  static Object _allocate(long inst)
  {
    return new PyBaseException(ALLOCATOR, inst);
  }

  @Override
  public String getMessage()
  {
    System.out.println("Get message " + PyBuiltins.str(this));
    return "IMPLEMENT ME"; //PyBuiltins.str(this).toString();
  }

}
