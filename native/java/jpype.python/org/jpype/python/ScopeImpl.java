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

import static org.jpype.python.Statics.FRAME_STATIC;
import python.lang.PyDict;

/**
 * Execution Frame for holding local and global variables.
 *
 * A scripting context holds the global and local dictionaries.
 */
class ScopeImpl implements Scope
{
  private final Engine engine;
  private final PyDict globals;
  private final PyDict locals;

  ScopeImpl(Engine engine, PyDict globals, PyDict locals)
  {
    this.engine = engine;
    this.globals = (PyDict) globals;
    this.locals = (PyDict) locals;
  }
  
  public Engine getEngine()
  {
    return engine;
  }

  /**
   * Execute a statement in this context.
   *
   * @param s
   * @return
   */
  @Override
  public Object eval(String s)
  {
    return FRAME_STATIC.runString(s, globals, locals);
  }

  /**
   * Start interactive mode in this context.
   */
  @Override
  public void interactive()
  {
    FRAME_STATIC.interactive(globals, locals);
  }
  
  @Override
  public void set(String name, Object obj)
  {
    globals.put(obj, obj);
  }
  
  @Override
  public Object get(String name)
  {
    return globals.get(name);
  }

  /**
   * @return the globals
   */
  @Override
  public PyDict getGlobals()
  {
    return globals;
  }

  /**
   * @return the locals
   */
  @Override
  public PyDict getLocals()
  {
    return locals;
  }

  @Override
  public Object importModule(String module)
  {
    return FRAME_STATIC.runString("import " + module, globals, locals);
  }

  @Override
  public Object importModule(String module, String as)
  {
    return FRAME_STATIC.runString("import " + module + " as " + as, globals, locals);
  }

  @Override
  public Object importFrom(String module, String symbol)
  {
    return FRAME_STATIC.runString("from " + module + " import " + symbol, globals, locals);
  }

}
