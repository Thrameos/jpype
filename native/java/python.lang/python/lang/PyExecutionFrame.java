package python.lang;

import org.jpype.python.internal.PyFrameStatic;
import org.jpype.python.PyTypeManager;

/**
 * Execution Frame for holding local and global variables.
 *
 * A scripting context holds the global and local dictionaries.
 */
public class PyExecutionFrame
{

  static PyFrameStatic FRAME_STATIC;

  private final PyDict globals;
  private final PyDict locals;

  PyExecutionFrame(PyDict globals, PyDict locals)
  {
    this.globals = (PyDict) globals;
    this.locals = (PyDict) locals;
    if (FRAME_STATIC == null)
    {
      FRAME_STATIC = PyTypeManager.getInstance().createStaticInstance(PyFrameStatic.class);
    }
  }

  /**
   * Execute a statement in this context.
   *
   * @param s
   * @return
   */
  public Object run(String s)
  {
    return FRAME_STATIC.runString(s, globals, locals);
  }

  /**
   * Start interactive mode in this context.
   */
  public void interactive()
  {
    FRAME_STATIC.interactive(globals, locals);
  }

  /**
   * @return the globals
   */
  public PyDict getGlobals()
  {
    return globals;
  }

  /**
   * @return the locals
   */
  public PyDict getLocals()
  {
    return locals;
  }

  public Object importModule(String module)
  {
    return FRAME_STATIC.runString("import " + module, globals, locals);
  }

  public Object importModule(String module, String as)
  {
    return FRAME_STATIC.runString("import " + module + " as " + as, globals, locals);
  }

  public Object importFrom(String module, String symbol)
  {
    return FRAME_STATIC.runString("from " + module + " import " + symbol, globals, locals);
  }

}
