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

/**
 * These classes act as customizers for Python classes to make it easier to add
 * methods to the wrapper implementation.
 * <p>
 * All fields and public methods are copied to the resulting wrapper. The fields
 * all must be initialized to null. Methods which are abstract and have
 * PyMethodInfo are implement by the PyTypeBuilder. All others are copied to the
 * corresponding wrapper class.
 * <p>
 * They should be concrete classes so that they can have all types of methods
 * (static, public, private). These classes will not appear in the public type
 * tree as they are merely used as templates. Any public method which is exposed
 * should be declared in the interface. They cannot override methods defined in
 * the interfaces using PyMethodInfo.
 * <p>
 * The only difficulty is how to implement static methods in the interfaces to
 * be delegated to here. These templates can't be instantiated as the Python
 * methods don't exist yet and interfaces refer to the private class instance
 * directly. Once we are on Java 9 or later, this will get a bit easier as Java
 * 9 permits more method types in interface classes. For now we will have to
 * implement those methods we need directly using the invoker.
 *
 */
