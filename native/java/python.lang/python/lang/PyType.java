package python.lang;

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;

@PyTypeInfo(name = "type")
public interface PyType extends PyObject
{

  @PyMethodInfo(name = "PyType_Name", invoke = PyInvocation.Unary, method = true)
  public String getName();

  // FIXME there should be a bunch of things here.
  // FIXME we should be able to implement a Python type with recognized
  // interfaces from within Java if needed.  We can already implement a class
  // generically as Java classes appear as Python.
}
