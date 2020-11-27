package org.jpype.python.internal;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.PyDict;

/**
 *
 * @author nelson85
 */
public interface PyFrameStatic
{

  @PyMethodInfo(name = "PyFrame_RunString", invoke = PyInvocation.Ternary, method = false)
  public Object runString(Object s, Object globals, Object locals);

  @PyMethodInfo(name = "PyFrame_Interactive", invoke = PyInvocation.Binary, method = false)
  public Object interactive(PyDict globals, PyDict locals);

}
