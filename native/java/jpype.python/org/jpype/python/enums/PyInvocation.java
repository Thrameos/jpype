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
package org.jpype.python.enums;

import java.lang.reflect.Method;
import org.jpype.python.internal.PyInvoker;

/**
 * Enums for dispatching to Python.
 *
 * This is an FFI interface for accessing Python methods from Java.
 *
 */
public enum PyInvocation
{
  /**
   * Object func(Object)
   */
  NoArgs(getMethod("invokeNone")),
  FromInt(getMethod("invokeFromInt")),
  FromLong(getMethod("invokeFromLong")),
  FromDouble(getMethod("invokeFromDouble")),
  FromDouble2(getMethod("invokeFromDouble2")),
  FromJObject(getMethod("invokeFromJObject")),
  /**
   * Object func(Object)
   */
  Unary(getMethod("invokeUnary")),
  /**
   * int func(Object)
   */
  AsBoolean(getMethod("invokeAsBoolean")),
  /**
   * int func(Object)
   */
  AsInt(getMethod("invokeAsInt")),
  /**
   * long func(Object)
   */
  AsLong(getMethod("invokeAsLong")),
  /**
   * float func(Object)
   */
  AsFloat(getMethod("invokeAsFloat")),
  /**
   * double func(Object)
   */
  AsDouble(getMethod("invokeAsDouble")),
  /**
   * Object func(Object, Object)
   */
  Binary(getMethod("invokeBinary")),
  /**
   * Object func(Object, int)
   */
  BinaryInt(getMethod("invokeBinaryInt")),
  /**
   * int func(Object, Object)
   */
  BinaryToInt(getMethod("invokeBinaryToInt")),
  /**
   * long func(Object, Object)
   */
  BinaryToLong(getMethod("invokeBinaryToLong")),
  /**
   * Object func(Object, Object, Object)
   */
  Ternary(getMethod("invokeTernary")),
  /**
   * Object func(Object, int, int)
   */
  GetSlice(getMethod("invokeGetSlice")),
  /**
   * Object func(Object, int, int, Object)
   */
  SetSlice(getMethod("invokeSetSlice")),
  /**
   * int func(Object, int, int)
   */
  DelSlice(getMethod("invokeDelSlice")),
  /**
   * int func(Object, str)
   */
  DelStr(getMethod("invokeDelStr")),
  /**
   * Object func(Object, str)
   */
  GetStr(getMethod("invokeGetStr")),
  /**
   * int func(Object, Object, Object)
   */
  SetObj(getMethod("invokeSetObj")),
  /**
   * int func(Object, String, Object)
   */
  SetStr(getMethod("invokeSetStr")),
  /**
   * int func(Object, int, Object)
   */
  SetInt(getMethod("invokeSetInt")),
  /**
   * int func(Object, int, Object)
   */
  SetIntToObj(getMethod("invokeSetIntToObj")),
  /**
   * int func(Object, int)
   */
  IntOperator1(getMethod("invokeIntOperator1")),
  /**
   * int func(Object, Object, int)
   */
  IntOperator2(getMethod("invokeIntOperator2")),
  /**
   * Object func(Object[])
   */
  Array(getMethod("invokeArray"));

  PyInvocation(Method method)
  {
    this.method = method;
  }

  public Method getMethod()
  {
    return method;
  }

  private static Method getMethod(String name)
  {
    for (Method m : PyInvoker.class.getMethods())
    {
      if (m.getName().equals(name))
        return m;
    }
    throw new RuntimeException("Method " + name + " not found.");
  }

  Method method;
}
