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
 * PyArguments is a special form of a Python tuple used with a PyCallable
 * object.
 *
 * PyArguments will expand into an argument list when passed to the call method.
 *
 * @author nelson85
 */
public class PyArguments
{

  private final PyTuple tuple;

  private PyArguments(PyTuple tuple)
  {
    this.tuple = tuple;
  }

  public static PyArguments use(PyTuple args)
  {
    return new PyArguments(args);
  }

  public PyTuple toTuple()
  {
    return this.tuple;
  }

}
