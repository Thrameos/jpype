package python.lang;

import org.jpype.python.internal.PyBoolPrivate;
import org.jpype.python.annotation.PyTypeInfo;

/**
 * Python Boolean type.
 *
 * This is a dummy wrapper as we will always translate the Python instance True
 * and False to Java TRUE and Java FALSE.
 */
@PyTypeInfo(name = "bool", internal = PyBoolPrivate.class)
public interface PyBool extends PyObject
{
}
