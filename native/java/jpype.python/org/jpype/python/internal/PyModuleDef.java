/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype.python.internal;

import java.util.HashMap;
import python.lang.PyModule;

/**
 *
 * @author nelson85
 */
public class PyModuleDef
{

  static HashMap<Long, PyModuleDef> definitions = new HashMap<>();
  long moduleDef;

  final static PyBuiltinStatic BUILTIN_STATIC = PyBuiltinStatic.INSTANCE;

  private PyModuleDef(long moduleDef)
  {
    this.moduleDef = moduleDef;

    // Unpack the methods list
    definitions.put(moduleDef, this);
  }

  /**
   * Get the definition for a module.
   *
   * @param module
   * @return the module definition or null if the module is not internally
   * defined.
   */
  public PyModuleDef getDefinition(PyModule module)
  {
    long ptr = _find(module);
    if (ptr == 0)
      return null;
    if (definitions.containsKey(ptr))
      return definitions.get(ptr);
    return new PyModuleDef(ptr);
  }
  
  native static long _find(Object o);
  native static String _getName(long ptr);
  native static Object[][] _getMethods(long ptr);
}
