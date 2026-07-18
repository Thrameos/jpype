// --- file: org/jpype/MainInterpreterDispatchNGTest.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package org.jpype;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import org.testng.annotations.Test;
import python.exceptions.PySystemExit;
import python.lang.PyObject;
import python.lang.PyTestHarness;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.fail;

/**
 * Exercises {@code MainInterpreter.dispatch} (plan/PythonCLI.md decision #2):
 * the {@code -c}/{@code -m}/bare-file/{@code -i} argv dispatch built on top
 * of {@link Runner}.
 */
public class MainInterpreterDispatchNGTest extends PyTestHarness
{

  @Test
  public void testDashCRunsCommand()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    interpreter.dispatch(new String[]
    {
      "-c", "import sys; sys.stdout.write('')"
    });
    // Interpreter must still be usable afterward.
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testDashMRunsModule()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    // "this" is the cheap stdlib Zen-of-Python easter-egg module used
    // elsewhere in RunnerNGTest - no external dependency, observable only
    // via its stdout side effect, safe to run unconditionally.
    interpreter.dispatch(new String[]
    {
      "-m", "this"
    });
    assertEquals(context.eval("1+1").toString(), "2");
  }

  @Test
  public void testBareFileRunsScript()
  throws IOException
  {
    Path script = Files.createTempFile("jpype-dispatch-test", ".py");
    try
    {
      Files.write(script, "_dispatch_result = 6 * 7\n".getBytes(StandardCharsets.UTF_8));
      MainInterpreter interpreter = MainInterpreter.getInstance();
      interpreter.dispatch(new String[]
      {
        script.toString()
      });
      assertEquals(context.eval("1+1").toString(), "2");
    } finally
    {
      Files.deleteIfExists(script);
    }
  }

  @Test
  public void testDashCMissingArgumentThrows()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-c"
      });
      fail("expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
    }
  }

  @Test
  public void testDashMMissingArgumentThrows()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-m"
      });
      fail("expected IllegalArgumentException");
    } catch (IllegalArgumentException expected)
    {
    }
  }

  @Test
  public void testDashCSystemExitPropagates()
  {
    MainInterpreter interpreter = MainInterpreter.getInstance();
    try
    {
      interpreter.dispatch(new String[]
      {
        "-c", "import sys; sys.exit(2)"
      });
      fail("expected PySystemExit");
    } catch (PySystemExit expected)
    {
    }
    assertEquals(context.eval("1+1").toString(), "2");
  }
}
