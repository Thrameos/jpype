package python.lang.protocol;

import org.jpype.python.annotation.PyTypeInfo;
import org.jpype.python.internal.PyNumberStatic;
import python.lang.PyObject;
import python.lang.PyString;
import python.lang.exc.PyBaseException;

@PyTypeInfo(name = "protocol.number", exact = true)
public interface PyNumber extends PyObject
{
  final static PyNumberStatic NUMBER_STATIC = PyNumberStatic.INSTANCE;

  /**
   * This is the equivalent of the Python expression o1 + o2.
   *
   * Return value: New reference.Returns the result of adding o1 and o2, or NULL
   * on failure.
   *
   *
   * @param o2
   * @return
   */
  default Object add(Object o2)
  {
    return NUMBER_STATIC.add(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 - o2'.
   *
   * @param o2
   * @return
   */
  default Object subtract(Object o2)
  {
    return NUMBER_STATIC.subtract(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 * o2'.
   *
   * @param o2
   * @return
   */
  default Object multiply(Object o2)
  {
    return NUMBER_STATIC.multiply(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 @ o2'.
   *
   * @param o2
   * @return
   */
  default Object matrixMultiply(Object o2)
  {
    return NUMBER_STATIC.matrixMultiply(this, o2);
  }

  /**
   * Equivalent to the “classic” division of integers.
   * @param o2
   * @return
   */
  default Object floorDivide(Object o2)
  {
    return NUMBER_STATIC.floorDivide(this, o2);
  }

  default Object trueDivide(Object o2)
  {
    return NUMBER_STATIC.trueDivide(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 % o2'.
   *
   * @param o2
   * @return
   */
  default Object remainder(Object o2)
  {
    return NUMBER_STATIC.remainder(this, o2);
  }

  /**
   * Equivalent of the Python expression 'divmod(o1,o2)'.
   *
   * @param o2
   * @return
   */
  default Object divmod(Object o2)
  {
    return NUMBER_STATIC.divmod(this, o2);
  }

  /**
   * Equivalent of the Python expression 'pow(o1, o2, o3)'.
   *
   * @param o2
   * @param o3
   * @return
   */
  default Object power(Object o2, Object o3)
  {
    return NUMBER_STATIC.power(this, o2, o3);
  }

  default Object power(Object o2)
  {
    return NUMBER_STATIC.power(this, o2);
  }

  /**
   * Equivalent of the Python expression '-o'.
   *
   * @return
   */
  default Object negative()
  {
    return NUMBER_STATIC.negative(this);
  }

  /**
   * Equivalent of the Python expression '+o'.
   *
   * @return
   */
  default Object positive()
  {
    return NUMBER_STATIC.positive(this);
  }

  /**
   * Equivalent of the Python expression 'abs(o)'.
   *
   * @return
   */
  default Object absolute()
  {
    return NUMBER_STATIC.absolute(this);
  }

  /**
   * Equivalent of the Python expression '~o'.
   *
   * @return
   */
  default Object invert()
  {
    return NUMBER_STATIC.invert(this);
  }

  /**
   * Equivalent of the Python expression 'o1 &lt;&lt; o2'.
   *
   * @param o2
   * @return
   */
  default Object leftShift(Object o2)
  {
    return NUMBER_STATIC.leftShift(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 >> o2'.
   *
   * @param o2
   * @return
   */
  default Object rightShift(Object o2)
  {
    return NUMBER_STATIC.rightShift(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 & o2'.
   *
   * @param o2
   * @return
   */
  default Object and(Object o2)
  {
    return NUMBER_STATIC.and(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 ^ o2'.
   *
   * @param o2
   * @return
   */
  default Object xor(Object o2)
  {
    return NUMBER_STATIC.xor(this, o2);
  }

  /**
   * Equivalent of the Python expression 'o1 | o2'.
   *
   * @param o2
   * @return
   */
  default Object or(Object o2)
  {
    return NUMBER_STATIC.or(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 += o2'.
   *
   * @param o2
   * @return
   */
  default Object addAssign(Object o2)
  {
    return NUMBER_STATIC.addAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 -= o2'.
   *
   * @param o2
   * @return
   */
  default Object subtractAssign(Object o2)
  {
    return NUMBER_STATIC.subtractAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 *= o2'.
   *
   * @param o2
   * @return
   */
  default Object multiplyAssign(Object o2)
  {
    return NUMBER_STATIC.multiplyAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 @= o2'.
   *
   * @param o2
   * @return
   */
  default Object matrixMultiplyAssign(Object o2)
  {
    return NUMBER_STATIC.matrixMultiplyAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 //= o2'.
   *
   * @param o2
   * @return
   */
  default Object floorDivideAssign(Object o2)
  {
    return NUMBER_STATIC.floorDivideAssign(this, o2);
  }

  default Object trueDivideAssign(Object o2)
  {
    return NUMBER_STATIC.trueDivideAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 %= o2'.
   *
   * @param o2
   * @return
   */
  default Object remainderAssign(Object o2)
  {
    return NUMBER_STATIC.remainderAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 **= o2'.
   *
   * @param o2
   * @return
   */
  default Object powerAssign(Object o2)
  {
    return NUMBER_STATIC.powerAssign(this, o2);
  }

  default Object powerAssign(Object o2, Object o3)
  {
    return NUMBER_STATIC.powerAssign(this, o2, o3);
  }

  /**
   * Equivalent of the Python statement 'o1 &lt;&lt;= o2'.
   *
   * @param o2
   * @return
   */
  default Object leftShiftAssign(Object o2)
  {
    return NUMBER_STATIC.leftShiftAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 >>= o2'.
   *
   * @param o2
   * @return
   */
  default Object rightShiftAssign(Object o2)
  {
    return NUMBER_STATIC.rightShiftAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 &= o2'.
   *
   * @param o2
   * @return
   */
  default Object andAssign(Object o2)
  {
    return NUMBER_STATIC.andAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 ^= o2'.
   *
   * @param o2
   * @return
   */
  default Object xorAssign(Object o2)
  {
    return NUMBER_STATIC.xorAssign(this, o2);
  }

  /**
   * Equivalent of the Python statement 'o1 |= o2'.
   *
   * @param o2
   * @return
   */
  default Object orAssign(Object o2)
  {
    return NUMBER_STATIC.orAssign(this, o2);
  }

  /**
   * Convert an integer n converted to base base as a string.
   *
   * The base argument must be one of 2, 8, 10, or 16.For base 2, 8, or 16, the
   * returned string is prefixed with a base marker of '0b', '0o', or '0x',
   * respectively.
   *
   * If n is not a Python int, it is converted with asIndex() first.
   *
   * @param base
   * @return
   */
  default PyString toBase(int base)
  {
    return NUMBER_STATIC.toBase(this, base);
  }

  /**
   * Convert to long or raise the exception.
   *
   * @param exc
   * @return
   */
  default long asSize(PyBaseException exc)
  {
    return NUMBER_STATIC.asSize(this, exc);
  }

}
