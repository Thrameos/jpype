package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBuiltinStatic;
import python.lang.PyArguments;
import python.lang.PyDict;
import python.lang.PyKeywords;
import python.lang.PyTuple;

@PyTypeInfo(name = "protocol.callable", exact = true)
public interface PyCallable
{

  /**
   * Call a Python object as a callable.
   *
   * Normally the argument list is converted into a PyTuple an passed to the
   * Python call.
   *
   * This method has interactions with PyArguments and PyKeywords types. If the
   * last argument is type PyKeywords, then it will be used to expand to list of
   * keywords. If the keywords are proceeded by a PyArguments type or the last
   * argument is PyArguments, then that object will expand into a list of
   * arguments. This can be used to express the {@code *args} and
   * {@code **kwargs} formulations to a Python call.
   *
   * If the only argument is to be None use {@code call(PyBuiltins.None) rather
   * than {@code call(null)} as the latter is likely to be misinterpreted as an
   * empty argument list without a cast.
   *
   * A PyDict can be converted into a PyKeywords and a PyTyple can be converted
   * into a PyArguments.
   *
   * @param args
   * @return an object or null if the return is None.
   */
  default Object call(Object... args)
  {
    PyTuple tuple;
    PyDict kwargs = null;

    // Passing a null to the arguments
    if (args == null || args.length == 0)
    {
      return PyBuiltinStatic.INSTANCE.call(this, null, null);
    }
    
    int n = args.length;

    // Handle PyKeywords
    if (n > 0 && args[n - 1] instanceof PyKeywords)
    {
      kwargs = ((PyKeywords) args[n - 1]).toDict();
      n--; // Strip the last argument from the arguments
    }

    // Handle PyArguments
    if (n > 0 && args[n - 1] instanceof PyArguments)
    {
      // Extract the tuple backing it.
      tuple = ((PyArguments) args[n - 1]).toTuple();
      
      // If we have both regular and PyArguments then we have to merge them
      if (n > 1)
        tuple = PyProtocolInternal.merge(args, 0, n - 1, tuple);
    } else
    {
      // Nothing special just pack the arguments in a tuple.
      tuple = PyTuple.ofRange(args, 0, n);
    }

    // Call the native method
    return PyBuiltinStatic.INSTANCE.call(this, tuple, kwargs);
  }

}