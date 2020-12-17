package python.lang;

import org.jpype.python.annotation.PyTypeInfo;

@PyTypeInfo(name = "memoryview")
public interface PyMemoryView extends PyObject
{

  static PyMemoryView of(Object obj)
  {
    return PyBuiltins.BUILTIN_STATIC.newMemoryView(obj);
  }
}
