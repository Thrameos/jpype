package python.lang;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.exc.PyException;
import python.lang.exc.PySystemError;

@PyTypeInfo(name = "function")
public interface PyFunction extends PyObject
{

  /**
   * Return the code object associated with the function object op.
   *
   * @return
   */
  @PyMethodInfo(name = "PyFunction_GetCode", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getCode();

  /**
   * Return the globals dictionary associated with the function object op.
   *
   * @return
   */
  @PyMethodInfo(name = "PyFunction_GetGlobals", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getGlobals();

  /**
   * Return the __module__ attribute of the function object op.
   * <p>
   * This is normally a string containing the module name, but can be set to any
   * other object by Python code.
   *
   * @return
   */
  @PyMethodInfo(name = "PyFunction_GetModule", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getModule();

  /**
   * Return the argument default values of the function object op.
   * <p>
   * This can be a tuple of arguments or NULL.
   *
   * @return
   */
  @PyMethodInfo(name = "PyFunction_GetDefaults", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getDefaults();

  /**
   * Set the argument default values for the function object op.
   *
   * @param defaults must be Py_None or a tuple.
   * @throws PySystemError on failure.
   */
  @PyMethodInfo(name = "PyFunction_SetDefaults", invoke = PyInvocation.BinaryToInt, method = true)
  void setDefaults(Object defaults) throws PySystemError;

  /**
   * Return the closure associated with the function object op.
   * <p>
   * @return null or a tuple of cell objects.
   */
  @PyMethodInfo(name = "PyFunction_GetClosure", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getClosure();

  /**
   * Set the closure associated with the function object op.
   * <p>
   * @param closure must be Py_None or a tuple of cell objects.
   * @throws PySystemError on failure.
   */
  @PyMethodInfo(name = "PyFunction_SetClosure", invoke = PyInvocation.Binary, method = true)
  void setClosure(Object closure) throws PySystemError;

  /**
   * Get the annotations of the function object.
   * <p>
   * This can be a mutable dictionary or NULL.
   *
   * @return
   */
  @PyMethodInfo(name = "PyFunction_GetAnnotations", invoke = PyInvocation.Unary, method = true, flags = PyMethodInfo.BORROWED)
  Object getAnnotations();

  /**
   * Set the annotations for the function object op.
   *
   * @param annotations must be a dictionary or Py_None.
   * @throws PyException on failure.
   */
  @PyMethodInfo(name = "PyFunction_SetAnnotations", invoke = PyInvocation.Binary, method = true)
  void setAnnotations(Object annotations) throws PyException;

}
