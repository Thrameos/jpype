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

import org.jpype.python.Engine;
import python.lang.PyBuiltins;
import python.lang.PyDict;
import python.lang.PyString;
import static python.lang.PyBuiltins.*;

/**
 * Python execution engine.
 */
class EngineImpl implements Engine
{

  @Override
  public ScopeImpl newScope(String name)
  {
    PyDict globals = new PyDict();
    // Set up the minimum global dictionary required to function
    globals.put("__spec__", None);
    globals.put("__dict__", None);
    globals.put("__package__", None);
    globals.put("__name__", new PyString(name));
    globals.put("__builtins__", PyBuiltins.builtins());

    // We will add JPype module and types so the environment is usable
    ScopeImpl frame = new ScopeImpl(this, globals, globals); 
    frame.importModule("jpype");
    frame.importFrom("jpype.types", "*");
    return frame;
  }

}
