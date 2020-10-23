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
import java.util.Map;

/**
 * Representation of a JPackage in Java.
 *
 * This provides the dir and attributes for a JPackage and by extension jpype
 * imports. Almost all of the actual work happens in the PackageManager which
 * acts like the classloader to figure out what resource are available.
 *
 */
public class JPypePackage
{

  // Name of the package
  final String pkg;
  // A mapping from Python names into Paths into the module/jar file system.
  final Map<String, URI> contents;

  public JPypePackage(String pkg, Map<String, URI> contents)
  {
    this.pkg = pkg;
    this.contents = contents;
  }

  /**
   * Get an object from the package.
   *
   * This is used by the importer to create the attributes for `getattro`. The
   * type returned is polymorphic. We can potentially support any type of
   * resource (package, classes, property files, xml, data, etc). But for now we
   * are primarily interested in packages and classes. Packages are returned as
   * strings as loading the package info is not guaranteed to work. Classes are
   * returned as classes which are immediately converted into Python wrappers.
   * We can return other resource types so long as they have either a wrapper
   * type to place the instance into an Python object directly or a magic
   * wrapper which will load the resource into a Python object type.
   *
   * This should match the acceptable types in getContents so that everything in
   * the `dir` is also an attribute of JPackage.
   *
   * @param name is the name of the resource.
   * @return the object or null if no resource is found with a matching name.
   */
  public Object getObject(String name)
  {
    String entity = pkg + "." + name;
    try
    {
      return Class.forName(entity);
    } catch (ClassNotFoundException ex)
    {
      return entity;
    }
  }

  /**
   * Get a list of contents from a Java package.
   *
   * This will be used when creating the package `dir`
   *
   * @return
   */
  public String[] getContents()
  {
    return new String[0];
  }

  /**
   * Determine if a class is public.
   *
   * This checks if a class file contains a public class. When importing classes
   * we do not want to instantiate a class which is not public as it may result
   * in instantiation of static variables or unwanted class resources. The only
   * alternative is to read the class file and get the class modifier flags.
   * Unfortunately, the developers of Java were rather stingy on their byte
   * allocation and thus the field we want is not in the header but rather
   * buried after the constant pool. Further as they didn't give the actual size
   * of the tables in bytes, but rather in entries, that means we have to parse
   * the whole table just to get the access flags after it.
   *
   * @param p
   * @return
   */
  static boolean isPublic(Path p)
  {
    return true;
  }

}
