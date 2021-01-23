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

import python.lang.exc.PyTypeError;

/**
 *
 * @author nelson85
 */
public class PyKeywords
{

  PyDict contents;

  private PyKeywords(PyDict dict)
  {
    this.contents = dict;
  }

  /**
   * Create a new keyword arguments.
   */
  public PyKeywords()
  {
    this(new PyDict());
  }

  /**
   * Use an existing dictionary as a keywords argument.
   *
   * This is the equivalent of {@code **kwargs}.
   *
   * @param dict
   * @return
   */
  public static PyKeywords use(PyDict dict)
  {
    return new PyKeywords(dict);
  }

  /**
   * Construct a set of keywords from a list of key/value pairs.
   *
   * Every other argument should be a string type. There should be an even
   * number of arguments.
   *
   * @param contents
   * @return
   */
  public static PyKeywords of(Object... contents)
  {
    PyKeywords out = new PyKeywords(new PyDict());
    for (int i = 0; i < contents.length; i += 2)
    {
      if (contents[i] instanceof CharSequence)
      {
        out.put((CharSequence) contents[i], contents[i + 1]);
      } else
      {
        throw new PyTypeError("Arguments must be char type");
      }
    }
    return out;
  }

  /**
   * Add a value to a keyword arguments.
   *
   * This can be chained to set up multiple values.
   *
   * @param key
   * @param value
   * @return
   */
  public PyKeywords put(CharSequence key, Object value)
  {
    this.contents.put(new PyString(key), value);
    return this;
  }

  public PyDict toDict()
  {
    return this.contents;
  }
}
