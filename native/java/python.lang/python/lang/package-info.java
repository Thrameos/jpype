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
package python.lang;

/**
 * Package holding basic types for Python.
 *
 * These are the concrete (or stubs for concrete) types that Python implements.
 *
 * The type system will also make its best guess as to the most appropriate type
 * to use for a wrapper. If if can't find an appropriate type it will use an
 * protocols as a mixin to represent the capabilities of the class. Even if the
 * wrapper is not exactly the correct type, all functionality is available
 * through the generic object interface though the attributes and call
 * mechanisms.
 *
 * Naming of classes is Py followed by the name as it appears in Python a few
 * exceptions.
 *
 * Builtin functions are held in the PyBuiltin class which has static methods
 * for each of the types.
 *
 */
