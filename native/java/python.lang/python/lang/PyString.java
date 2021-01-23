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

import org.jpype.python.Types;
import python.lang.protocol.PySequence;
import org.jpype.python.internal.PyStringStatic;
import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyBaseObject;
import org.jpype.python.internal.PyConstructor;

/**
 * Representation of a Python String.
 * 
 * FIXME should this be named PyUnicode to match the Python CAPI or PyString?
 * python.lang.PyString is more similar to java.lang.String.
 * 
 * @author nelson85
 */
@PyTypeInfo(name = "str")
public class PyString extends PyBaseObject implements CharSequence, PySequence<Object>
{

  final static PyStringStatic STRING_STATIC = Types.newInstance(PyStringStatic.class);

  protected PyString(PyConstructor key, long instance)
  {
    super(key, instance);
  }

  /**
   * Convert to a Python String representation.
   *
   * The actions taken depend on the type of the incoming argument. Existing
   * Python strings are simply referenced again. All other types are converted
   * to a new concrete Python string representation.
   *
   * @param s
   */
  public PyString(CharSequence s)
  {
    super(PyConstructor.CONSTRUCTOR, _fromType(s));
  }

  @Override
  public int length()
  {
    return PyBuiltins.len(this);
  }

  @Override
  public int size()
  {
    return PyBuiltins.len(this);
  }

  @Override
  public char charAt(int index)
  {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  @Override
  public CharSequence subSequence(int start, int end)
  {
    throw new UnsupportedOperationException("Not supported yet."); //To change body of generated methods, choose Tools | Templates.
  }

  static Object _allocate(long instance)
  {
    return new PyString(PyConstructor.ALLOCATOR, instance);
  }

  /**
   * Delegator based on the incoming type.
   *
   * @param s
   * @return
   */
  private static long _fromType(CharSequence s)
  {
    if (s == null)
      throw new NullPointerException("PyString instances may not null");

    // Give back the reference to the existing object.
    if (s instanceof PyString)
    {
      return PyBaseObject._getSelf((PyBaseObject) s);
    }

    if (s instanceof String)
    {
      return _ctor((String) s);
    }

    // Otherwise convert to String first
    return _ctor(s.toString());
  }
  
  @Override
  public String toString()
  {
    return _toString(this);
  }

  private native static String _toString(Object self);
  private native static long _ctor(String s);
}
