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

/**
 *
 * @author nelson85
 */
public interface Engine
{

  /**
   * Create a context for executing Python commands.
   *
   * Each context is an independent scope with its own symbols. Modules from
   * different contexts are shared as there is only one Python interpreter.
   *
   * @param name
   * @return a new interpreter context.
   */
  Scope newScope(String name);

  default Scope newScope()
  {
    return Engine.this.newScope("__main__");
  }

  // FIXME we need a way to create code objects that can then be executed in a scope.
  
}
