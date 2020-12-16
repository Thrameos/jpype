package python.lang;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Paths;
import static python.lang.PyBuiltins.*;

/**
 * Python execution engine.
 *
 * This should act like a factory in which we can set all the variables for the
 * engine and then start execution. Once started it can produce new execution
 * frames for executing code. All modules are shared between frames.
 *
 */
public class PyEngine
{

  static PyEngine INSTANCE = null;
  static String pythonExec = "python";
  static String jpypeLib = null;
  static String pythonLibrary = null;

  public static PyEngine getInstance()
  {
    if (INSTANCE == null)
      INSTANCE = new PyEngine();
    return INSTANCE;
  }

  /**
   * Start the Python engine.
   */
  public void start()
  {
    String library = getLibrary();
    if (library == null)
    {
      throw new RuntimeException("Unable to find _jpype module");
    }
    System.load(pythonLibrary);
    System.load(library);
    this.addLibrary(pythonLibrary);
    this.addLibrary(library);
    System.out.println("=============");
    //start_();
    System.out.println("1");
    System.out.println("2");
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItem"));
    System.out.println(this.getSymbol("PyDict_GetItemB"));
    System.out.println("=============");
  }

  public PyExecutionFrame newFrame()
  {
    PyDict globals = new PyDict();
    globals.merge(PyBuiltins.BUILTIN_STATIC.builtins(), true);
    globals.put("__spec__", None);
    globals.put("__dict__", None);
    globals.put("__package__", None);
    globals.put("__name__", new PyString("__main__"));
    PyExecutionFrame frame = new PyExecutionFrame(globals, globals);
    frame.importModule("jpype");
    frame.importFrom("jpype.types", "*");
    return frame;
  }

//<editor-fold desc="utility" defaultstate="collapsed">
  /**
   * Get the shared library parameters.
   *
   * @return
   */
  public static String getLibrary()
  {
    if (jpypeLib != null)
      return jpypeLib;
    try
    {
      String python = System.getProperty("python", pythonExec);
      String[] cmd =
      {
        python, "-c",
        "import importlib\n"
        + "import sysconfig\n"
        + "print(importlib.util.find_spec('_jpype').origin)\n"
      };
      ProcessBuilder pb = new ProcessBuilder(cmd);
      pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
      Process process = pb.start();
      BufferedReader out = new BufferedReader(new InputStreamReader(process.getInputStream()));
      process.waitFor();
      return out.readLine();
    } catch (IOException | InterruptedException ex)
    {
      ex.printStackTrace();
    }
    return null;
  }

//</editor-fold>
  public static void main(String[] args)
  {
    try
    {
      PyEngine main = getInstance();
      System.out.println("Start");
      main.start();
      System.out.println("Run");
      PyExecutionFrame frame = main.newFrame();
      frame.interactive();
      frame.run("print('hello world')");
    } catch (Exception ex)
    {
      ex.printStackTrace();
    }
  }

  public void setProperty(String key, Object value)
  {
    if (key.equals("python.exec"))
    {
      pythonExec = (String) value;
      return;
    }
    if (key.equals("python.lib"))
    {
      pythonLibrary = Paths.get((String) value).toAbsolutePath().toString();
      return;
    }
    if (key.equals("jpype.lib"))
    {
      jpypeLib = Paths.get((String) value).toAbsolutePath().toString();
      return;
    }
    throw new UnsupportedOperationException("Unknown property " + key);
  }

  native void start_();
  
  public native long getSymbol(String str);

  public native void addLibrary(String str);
}
