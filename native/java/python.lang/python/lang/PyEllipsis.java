package python.lang;

import org.jpype.python.internal.PyEllipsisPrivate;
import org.jpype.python.annotation.PyTypeInfo;

@PyTypeInfo(name = "ellipsis", internal = PyEllipsisPrivate.class)
public interface PyEllipsis extends PyObject
{
}
