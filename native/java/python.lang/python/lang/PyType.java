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

import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.enums.PyInvocation;

@PyTypeInfo(name = "type")
public interface PyType extends PyObject
{

  @PyMethodInfo(name = "PyType_Name", invoke = PyInvocation.Unary, method = true)
  public String getName();

  // FIXME there should be a bunch of things here.
  // FIXME we should be able to implement a Python type with recognized
  // interfaces from within Java if needed.  We can already implement a class
  // generically as Java classes appear as Python.
}
