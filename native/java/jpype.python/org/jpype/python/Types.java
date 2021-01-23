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
 * This is an internal class used to convert stubs into instances.
 * 
 * @author nelson85
 */
public class Types
{
  /**
   * Creates a static instance from a stub class.
   * 
   * @param <T>
   * @param c
   * @return 
   */
  public static <T> T newInstance(Class<T> c)
  {
    return PyTypeManager.getInstance().createStaticInstance(c);
  }
}
