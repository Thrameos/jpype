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
public interface EngineFactory
{

  /** 
   * Get the instance of the EngineFactory.
   * 
   * @return 
   */
  static EngineFactory getInstance()
  {
    return Statics.getEngineFactory();
  }
    
  /**
   * Set a property for the engine.
   *
   * Properties should be set before starting the engine. After the engine is
   * started most properties have no effect.
   *
   * @param key
   * @param value
   * @throws IllegalStateException if the engine is already started.
   */
  void setProperty(String key, Object value) throws IllegalStateException;

  /** 
   * Create a Python instance.
   * 
   * This can be only used once.
   * 
   * @return 
   * @throws IllegalStateException if the Python engine has already been started.
   */
  Engine create() throws IllegalStateException;
}
