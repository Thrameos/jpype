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

import java.util.HashMap;
import org.jpype.JPypeContext;

/**
 * Class load for dynamic classes.
 *
 * @author nelson85
 */
class PyTypeLoader extends ClassLoader
{

  final HashMap<Integer, Class> cache = new HashMap<>();
  final PyTypeBuilder builder;

  PyTypeLoader(PyTypeBuilder builder)
  {
    super(JPypeContext.getInstance().getClassLoader());
    this.builder = builder;
  }

  /**
   * Find a class with the specified interfaces.
   *
   * @param name
   * @param concrete
   * @param interfaces
   * @return
   */
  public Class findClass(String name, Class concrete, Class[] interfaces)
  {
    int hash = 0;
    for (int i = 0; i < interfaces.length; i++)
    {
      hash = hash * 0xa2060073 + interfaces[i].getName().hashCode();
    }
    if (concrete != null)
      hash = hash * 0xa2060073 + concrete.getName().hashCode();
    Class cls = cache.get(hash);
    if (cls == null)
    {
      String className = String.format("%s$%08x", name, hash);
      byte[] byteCode = builder.newClass(className, concrete, interfaces);
      cls = defineClass(className, byteCode, 0, byteCode.length);
      cache.put(hash, cls);
    }
    return cls;
  }

}
