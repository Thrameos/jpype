package python.lang;

import org.jpype.python.internal.PyNonePrivate;
import org.jpype.python.annotation.PyTypeInfo;

/**
 * Python None type.
 *
 * This object is a singleton.
 */
@PyTypeInfo(name = "NoneType", internal = PyNonePrivate.class)
public interface PyNone extends PyObject
{
}
