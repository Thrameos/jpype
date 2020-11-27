package python.lang;

import org.jpype.python.annotation.PyTypeInfo;

@PyTypeInfo(name = "slice")
public interface PySlice extends PyObject
{

  static PySlice of(Object start, Object end)
  {
//    return PyBuiltins.BUILTIN_STATIC.newSlice(start, end, None);
    return null;
  }

  static PySlice of(Object start, Object end, Object step)
  {
    return PyBuiltins.BUILTIN_STATIC.newSlice(start, end, step);
  }
}
