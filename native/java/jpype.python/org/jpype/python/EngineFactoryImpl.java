/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package org.jpype.python;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Paths;
import org.jpype.python.internal.Native;
import org.jpype.python.internal.PyFrameStatic;

/**
 *
 * @author nelson85
 */
class EngineFactoryImpl implements EngineFactory
{

  String pythonExec = "python";
  String jpypeLibrary = null;
  String pythonLibrary = null;

  boolean started = false;

  @Override
  public void setProperty(String key, Object value)
  {
    if (started)
      throw new IllegalStateException();

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
      jpypeLibrary = Paths.get((String) value).toAbsolutePath().toString();
      return;
    }
    throw new UnsupportedOperationException("Unknown property " + key);
  }

  @Override
  public Engine create()
  {
    // We can only create one engine as all have shared instances
    if (started)
      throw new IllegalStateException();

    // Get the _jpype extension library
    resolveLibraries();
    if (jpypeLibrary == null || pythonLibrary == null)
    {
      throw new RuntimeException("Unable to find _jpype module");
    }

    System.out.println("Load " + pythonLibrary);
    System.out.println("Load " + jpypeLibrary);

    // Load libraries in Java so they are available for native calls.
    if (Paths.get(pythonLibrary).isAbsolute())
      System.load(pythonLibrary);
    else
      System.loadLibrary(pythonLibrary);
    System.load(jpypeLibrary);

    // Add to FFI name lookup table
    Native.addLibrary(pythonLibrary);
    Native.addLibrary(jpypeLibrary);

    // Start the Python
    Native.start();
    started = true;

    // Connect up the natives
    Statics.FRAME_STATIC = PyTypeManager.getInstance().createStaticInstance(PyFrameStatic.class);
    return new EngineImpl();
  }

//<editor-fold desc="utility" defaultstate="collapsed">
  /**
   * Get the shared library parameters.
   *
   * @return
   */
  public void resolveLibraries()
  {
    // System properties dub compiled in paths
    this.jpypeLibrary = System.getProperty("jpype.lib", jpypeLibrary);
    this.pythonLibrary = System.getProperty("python.lib", pythonLibrary);

    // No need to do a probe
    if (this.jpypeLibrary != null && this.pythonLibrary != null)
      return;

    try
    {
      System.out.println("Probe");
      String python = pythonExec;
      String[] cmd =
      {
        python, "-c",
        "import importlib\n"
        + "import sysconfig\n"
        + "import os\n"
        + "gcv = sysconfig.get_config_var\n"
        + "print(importlib.util.find_spec('_jpype').origin)\n"
        + "print(os.path.join(gcv('LIBDIR')+gcv('multiarchsubdir'),gcv('LDLIBRARY')))"
      };
      ProcessBuilder pb = new ProcessBuilder(cmd);
      pb.redirectOutput(ProcessBuilder.Redirect.PIPE);
      Process process = pb.start();
      BufferedReader out = new BufferedReader(new InputStreamReader(process.getInputStream()));
      process.waitFor();
      String a = out.readLine();
      String b = out.readLine();
      if (jpypeLibrary == null)
        jpypeLibrary = a;
      if (pythonLibrary == null)
        pythonLibrary = b;

    } catch (IOException | InterruptedException ex)
    {
      ex.printStackTrace();
    }
  }

//</editor-fold>
}
