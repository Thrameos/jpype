package python.lang;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;

@PyTypeInfo(name = "method")
public interface PyMethod extends PyObject
{

  /**
   * Return a new method object, with func being any callable object and self
   * the instance the method should be bound .
   *
   * @param func is the function that will be called when the method is called.
   * @param self must not be null.
   * @return
   */
  static PyMethod of(Object func, Object self)
  {
    return PyBuiltins.BUILTIN_STATIC.newMethod(func, self);
  }

  /**
   * Get the function object associated with the method meth.
   *
   * @return
   */
  @PyMethodInfo(name = "PyMethod_Function", invoke = PyInvocation.Unary, method = true,  flags = PyMethodInfo.BORROWED)
  Object getFunction();

  /**
   * Get the instance associated with the method meth.
   *
   * @return
   */
  @PyMethodInfo(name = "PyMethod_Self", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getSelf();

}
