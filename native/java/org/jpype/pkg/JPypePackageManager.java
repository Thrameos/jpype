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
package org.jpype.pkg;

import java.net.URI;
import java.nio.file.Path;
import java.util.Collections;
import java.util.Map;

/**
 * Manager for the contents of a package.
 *
 * This class uses a number of tricks to provide a way to determine what
 * packages are available in the class loader. It searches the jar path, the
 * boot path (Java 8), and the module path (Java 9+). It does not currently work
 * with alternative classloaders. This class was rumored to be unobtainium as
 * endless posts indicated that it wasn't possible to determine the contents of
 * a package in general nor to retrieve the package contents, but this appears
 * to be largely incorrect as the jar and jrt file system provide all the
 * required methods.
 *
 */
public class JPypePackageManager
{


  /**
   * Get the list of the contents of a package.
   *
   * @param packageName
   * @return the list of all resources found.
   */
  public static Map<String, URI> getContentMap(String packageName)
  {
    return Collections.EMPTY_MAP;
  }

  /**
   * Convert a URI into a path.
   *
   * This has special magic methods to deal with jar file systems.
   *
   * @param uri is the location of the resource.
   * @return the path to the uri resource.
   */
  static Path getPath(URI uri)
  {
    return null;
  }


}
