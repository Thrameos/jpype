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
package python.lang;

import java.nio.file.Paths;
import org.jpype.python.Engine;
import org.jpype.python.EngineFactory;
import org.jpype.python.Scope;

/**
 *
 * @author nelson85
 */
public class PythonTest
{

  static Engine engine;
  static Scope scope;

  static Engine getEngine()
  {
    if (engine != null)
      return engine;

    System.out.println(Paths.get(".").toAbsolutePath());

    EngineFactory main = EngineFactory.getInstance();
    main.setProperty("python.exec", "python3");
    //main.setProperty("jpype.lib","_jpype.cpython-36m-x86_64-linux-gnu.so");
    //main.setProperty("python.lib","/usr/lib/x86_64-linux-gnu/libpython3.6m.so");
    System.out.println("Start");
    engine = main.create();
    System.out.println("Run");
    scope = engine.newScope();
    return engine;
  }

  Scope getScope()
  {
    if (scope == null)
      getEngine();
    return scope;
  }

}
