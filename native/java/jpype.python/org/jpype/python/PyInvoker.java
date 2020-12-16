package org.jpype.python;

/**
 * Class to invoke methods in Python.
 *
 * Invocation rules.
 * <ul>
 * <li>Methods must not steal a reference from the arguments.
 * <li>Methods must return a new reference (not borrowed).
 * <li>Methods that return null but do not set an error will get null.
 * <li>Methods that set an error will throw.
 * <li>Methods must match the Java signature if there is a conflict or be
 * renamed with a trailing underscore.
 * </ul>
 *
 * If a Python method is unable to implement this, it will be wrapped and a
 * suffix will be added so that the default behavior and the required behavior
 * can distinguished.
 *
 */
public class PyInvoker
{

  public static native Object invokeNone(long entry, int flags);

  public static native Object invokeFromInt(long entry, int flags, int i);

  public static native Object invokeFromLong(long entry, int flags, long i);

  public static native Object invokeFromDouble(long entry, int flags, double i);

  public static native Object invokeFromJObject(long entry, int flags, Object i);

  public static native Object invokeUnary(long entry, int flags, Object a0);

  public static native boolean invokeAsBoolean(long entry, int flags, Object a0);

  public static native int invokeAsInt(long entry, int flags, Object a0);

  public static native long invokeAsLong(long entry, int flags, Object a0);

  public static native float invokeAsFloat(long entry, int flags, Object a0);

  public static native double invokeAsDouble(long entry, int flags, Object a0);

  public static native Object invokeBinary(long entry, int flags, Object a0, Object a1);

  public static native Object invokeBinaryInt(long entry, int flags, Object a0, int a1);

  public static native int invokeBinaryToInt(long entry, int flags, Object a0, Object a1);

  public static native long invokeBinaryToLong(long entry, int flags, Object a0, Object a1);

  public static native Object invokeTernary(long entry, int flags, Object a0, Object a1, Object a2);

  public static native int invokeDelSlice(long entry, int flags, Object a0, int a1, int a2);

  public static native Object invokeGetSlice(long entry, int flags, Object a0, int a1, int a2);

  public static native int invokeSetSlice(long entry, int flags, Object a0, int a1, int a2, Object a3);

  public static native int invokeDelStr(long entry, int flags, Object a0, String a1);

  public static native Object invokeGetStr(long entry, int flags, Object a0, String a1);

  public static native int invokeSetObj(long entry, int flags, Object a0, Object a1, Object a2);

  public static native int invokeSetStr(long entry, int flags, Object a0, String a1, Object a2);

  public static native int invokeSetInt(long entry, int flags, Object a0, int a1, Object a2);

  public static native Object invokeSetIntToObj(long entry, int flags, Object a0, int a1, Object a2);

  public static native int invokeIntOperator1(long entry, int flags, Object a0, int op);

  public static native int invokeIntOperator2(long entry, int flags, Object a0, Object a1, int op);

  public static native Object invokeArray(long entry, int flags, Object[] a1);

}
