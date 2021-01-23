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
package org.jpype.python.internal;

import java.net.URI;
import java.util.HashMap;
import java.util.Map;
import org.jpype.JPypeContext;
import org.jpype.pkg.JPypePackageManager;
import org.jpype.python.annotation.PyTypeInfo;

/**
 * Index used to define the interfaces for a Python Type.
 * <p>
 * Implementations of this class must have the package python.__modules__.
 *
 * @author nelson85
 */
public interface PyModuleIndex
{

  Class getWrapper(String name);

  /**
   * Function for constructing an index of wrappers by scanning a Python package
   * for annotations.
   *
   * @param pkg
   * @return
   */
  public static Map<String, Class> scanPackage(String pkg)
  {
    HashMap<String, Class> out = new HashMap<>();
    ClassLoader cl = JPypeContext.getInstance().getClassLoader();
    Map<String, URI> content = JPypePackageManager.getContentMap(pkg);
    for (Map.Entry<String, URI> entry : content.entrySet())
    {
      Class c;
      try
      {
        c = Class.forName(pkg + "." + entry.getKey(), false, cl);
        PyTypeInfo ti = (PyTypeInfo) c.getAnnotation(PyTypeInfo.class);
        if (ti == null)
          continue;
        out.put(ti.name(), c);
      } catch (ClassNotFoundException ex)
      {
      }
    }
    return out;
  }
}
