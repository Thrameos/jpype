package python.lang;

import org.jpype.python.annotation.PyTypeInfo;

/**
 * Python Object protocol.
 *
 * If it isn't here then it needs to be cast to the appropriate type to access.
 * Use 'obj instanceOf type` for type checks.
 */
@PyTypeInfo(name = "object")
public interface PyObject
{

  /**
   * Equivalent to the Python expression 'hasattr(o, attr_name)'.
   *
   * This function always succeeds.
   *
   * @param name
   * @return
   */
  default boolean hasAttr(String name)
  {
    return PyBuiltins.hasattr(this, name);
  }

  /**
   * Equivalent of the Python expression 'o.attr_name'.
   *
   * @param name
   * @return
   */
  default Object getAttr(CharSequence name)
  {
    return PyBuiltins.getattr(this, name);
  }

  /**
   * Equivalent of the Python statement 'o.attr_name = v'.
   *
   * @param name
   * @param value
   */
  default void setAttr(CharSequence name, Object value)
  {
    PyBuiltins.setattr(this, name, value);
  }

  /**
   * Equivalent of the Python statement 'del o'.
   *
   * @param name
   */
  default void delAttr(CharSequence name)
  {
    PyBuiltins.delattr(this, name);
  }

}
