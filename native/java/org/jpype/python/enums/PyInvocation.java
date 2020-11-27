package org.jpype.python.enums;

import java.lang.reflect.Method;
import org.jpype.python.PyInvoker;

/**
 * Enums for dispatching to Python.
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
