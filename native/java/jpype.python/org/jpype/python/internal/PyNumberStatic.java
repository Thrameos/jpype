package org.jpype.python.internal;

import org.jpype.python.PyTypeManager;
import org.jpype.python.annotation.PyMethodInfo;
import org.jpype.python.enums.PyInvocation;
import python.lang.PyString;
import python.lang.exc.PyBaseException;

public interface PyNumberStatic
{
  public final static PyNumberStatic INSTANCE = PyTypeManager.getInstance()
          .createStaticInstance(PyNumberStatic.class);

  @PyMethodInfo(name = "PyNumber_Add", invoke = PyInvocation.Binary, method = false)
  Object add(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Subtract", invoke = PyInvocation.Binary, method = false)
  Object subtract(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Multiply", invoke = PyInvocation.Binary, method = false)
  Object multiply(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_MatrixMultiply", invoke = PyInvocation.Binary, method = false)
  Object matrixMultiply(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_FloorDivide", invoke = PyInvocation.Binary, method = false)
  Object floorDivide(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_TrueDivide", invoke = PyInvocation.Binary, method = false)
  Object trueDivide(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Remainder", invoke = PyInvocation.Binary, method = false)
  Object remainder(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Divmod", invoke = PyInvocation.Binary, method = false)
  Object divmod(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Power", invoke = PyInvocation.Ternary, method = false)
  Object power(Object self, Object o2, Object o3);

  @PyMethodInfo(name = "PyNumber_Power2", invoke = PyInvocation.Binary, method = false)
  Object power(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Negative", invoke = PyInvocation.Unary, method = false)
  Object negative(Object self);

  @PyMethodInfo(name = "PyNumber_Positive", invoke = PyInvocation.Unary, method = false)
  Object positive(Object self);

  @PyMethodInfo(name = "PyNumber_Absolute", invoke = PyInvocation.Unary, method = false)
  Object absolute(Object self);

  @PyMethodInfo(name = "PyNumber_Invert", invoke = PyInvocation.Unary, method = false)
  Object invert(Object self);

  @PyMethodInfo(name = "PyNumber_Lshift", invoke = PyInvocation.Binary, method = false)
  Object leftShift(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Rshift", invoke = PyInvocation.Binary, method = false)
  Object rightShift(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_And", invoke = PyInvocation.Binary, method = false)
  Object and(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Xor", invoke = PyInvocation.Binary, method = false)
  Object xor(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_Or", invoke = PyInvocation.Binary, method = false)
  Object or(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceAdd", invoke = PyInvocation.Binary, method = false)
  Object addAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceSubtract", invoke = PyInvocation.Binary, method = false)
  Object subtractAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceMultiply", invoke = PyInvocation.Binary, method = false)
  Object multiplyAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceMatrixMultiply", invoke = PyInvocation.Binary, method = false)
  Object matrixMultiplyAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceFloorDivide", invoke = PyInvocation.Binary, method = false)
  Object floorDivideAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceTrueDivide", invoke = PyInvocation.Binary, method = false)
  Object trueDivideAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceRemainder", invoke = PyInvocation.Binary, method = false)
  Object remainderAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlacePower2", invoke = PyInvocation.Binary, method = false)
  Object powerAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlacePower", invoke = PyInvocation.Ternary, method = false)
  Object powerAssign(Object self, Object o2, Object o3);

  @PyMethodInfo(name = "PyNumber_InPlaceLshift", invoke = PyInvocation.Binary, method = false)
  Object leftShiftAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceRshift", invoke = PyInvocation.Binary, method = false)
  Object rightShiftAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceAnd", invoke = PyInvocation.Binary, method = false)
  Object andAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceXor", invoke = PyInvocation.Binary, method = false)
  Object xorAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_InPlaceOr", invoke = PyInvocation.Binary, method = false)
  Object orAssign(Object self, Object o2);

  @PyMethodInfo(name = "PyNumber_ToBase", invoke = PyInvocation.BinaryInt, method = false)
  PyString toBase(Object self, int base);

  @PyMethodInfo(name = "PyNumber_AsSsize_t", invoke = PyInvocation.BinaryToLong, method = false)
  long asSize(Object self, PyBaseException exc);

  @PyMethodInfo(name = "PyNumber_IntValue", invoke = PyInvocation.AsInt, method = false)
  int intValue(Object self);

  @PyMethodInfo(name = "PyNumber_LongValue", invoke = PyInvocation.AsLong, method = false)
  long longValue(Object self);

  @PyMethodInfo(name = "PyNumber_FloatValue", invoke = PyInvocation.AsFloat, method = false)
  float floatValue(Object self);

  @PyMethodInfo(name = "PyNumber_DoubleValue", invoke = PyInvocation.AsDouble, method = false)
  double doubleValue(Object self);

}
