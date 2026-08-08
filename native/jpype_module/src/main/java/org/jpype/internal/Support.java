/*****************************************************************************
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
 *****************************************************************************/
package org.jpype.internal;

import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.DoubleBuffer;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.LongBuffer;
import java.nio.ShortBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.IntStream;

/**
 * Static helpers for multi-dimensional primitive array transfer, called
 * exclusively from C++ (see JPJavaFrame::collectRectangular/assemble).
 * Split out of JPypeContext so this can be looked up and invoked as plain
 * static methods (GetStaticMethodID) instead of round-tripping through the
 * context instance.
 */
class Support
{

  private Support()
  {
  }

  /**
   * Helper function for collect rectangular.
   */
  private static boolean collect(List<Object> l, Object o, int q, int[] shape, int d)
  {
    if (Array.getLength(o) != shape[q])
      return false;
    if (q + 1 == d)
    {
      l.add(o);
      return true;
    }
    for (int i = 0; i < shape[q]; ++i)
    {
      if (!collect(l, Array.get(o, i), q + 1, shape, d))
        return false;
    }
    return true;
  }

  /**
   * Collect up a rectangular primitive array for a Python memory view.
   *
   * If it is a rectangular primitive array then the result will be an object
   * array containing. - the primitive type - an int array with the shape of the
   * array - each of the primitive arrays that will need be visited in order.
   *
   * This is the safest way to provide a view as we are verifying and collected
   * thus even if something mutates the shape of the array after we have
   * visited, we have a locked copy.
   *
   * @param o is the object to be tested.
   * @return null if the object is not a rectangular primitive array.
   */
  public static Object[] collectRectangular(Object o)
  {
    if (o == null || !o.getClass().isArray())
      return null;

    // We only support flattening up to 4 dimensions for fast transfer
    int[] shape = new int[4];
    int d = 0;

    Object o1 = o;
    Class<?> c1 = o1.getClass();
    while (c1.isArray())
    {
      // If we hit a 5th nested dimension, immediately reject it before doing work
      if (d == 4)
        return null;

      int l = Array.getLength(o1);
      if (l == 0)
        return null;

      shape[d++] = l;
      o1 = Array.get(o1, 0);
      if (o1 == null)
        return null;

      c1 = c1.getComponentType();
    }

    if (!c1.isPrimitive())
      return null;

    ArrayList<Object> out = new ArrayList<>();
    out.add(c1);

    shape = Arrays.copyOfRange(shape, 0, d);
    out.add(shape);

    int total = 1;
    for (int i = 0; i < d - 1; i++)
      total *= shape[i];
    out.ensureCapacity(total + 2);

    if (!collect(out, o, 0, shape, d))
      return null;

    return out.toArray();
  }

  public static Object unpack(int size, Object parts)
  {
    Object e0 = Array.get(parts, 0);
    Class<?> c = e0.getClass();
    int segments = Array.getLength(parts) / size;
    Object a2;
    Object a1 = Array.newInstance(Array.newInstance(c, size).getClass(), segments);
    int k = 0;
    for (int i = 0; i < segments; i++)
    {
      a2 = Array.newInstance(c, size);

      for (int j = 0; j < size; j++, k++)
      {
        Object o = Array.get(parts, k);
        Array.set(a2, j, o);
      }

      Array.set(a1, i, a2);
    }
    return a1;
  }

  public static Object assemble(int[] dims, Object parts)
  {
    int n = dims.length;
    if (n == 1)
      return Array.get(parts, 0);
    if (n == 2)
      return Array.get(unpack(dims[0], parts), 0);
    for (int i = 0; i < n - 2; ++i)
    {
      parts = unpack(dims[n - i - 2], parts);
    }
    return parts;
  }

  // ---- Phase 3.6 (plan/ArrayTransferPhase3.md): buffer-handoff push/pull.
  //
  // Both methods below replace an O(leaf-array-count) JNI-call sequence
  // (one GetPrimitiveArrayCritical-pinned pack loop per leaf on push, one
  // reflective Array.get + one Get<Type>ArrayRegion per leaf on pull) with
  // a *single* JNI entry into one of these methods, after which
  // everything is plain Java: bulk java.nio typed-buffer reads/writes
  // (IntBuffer.get/put etc.) against a caller-supplied direct buffer, no
  // further JNI calls at all. Validated by the 3.3b/3.4 experiments
  // (DeepBench.java's fillBuffer*/collectBuffer* prototypes, now
  // superseded by these) -- serial wins unconditionally over the old
  // per-leaf approach; parallel (IntStream.parallel()) only pays off
  // above roughly 1e6-1e7 total elements. That decision is made here, in
  // Java (leafRange() below), not by the C++ caller -- C++ has no better
  // way to reason about IntStream/ForkJoinPool dispatch cost than Java
  // does, and total element count is already known on this side once the
  // shape is in hand, so there's no reason to compute it twice or thread
  // a boolean across the JNI boundary.
  //
  // `typeCode` is the JNI primitive type signature character
  // (Z/B/C/S/I/J/F/D, see JPPrimitiveType::getTypeCode()). The caller
  // must have already set the buffer's contents in host byte order, or
  // -- as here -- this method sets `.order(nativeOrder())` on its own
  // view before reading/writing, since a fresh direct buffer (JNI
  // NewDirectByteBuffer) always defaults to big-endian regardless of
  // platform (a real bug caught during the 3.4 experiment's sanity
  // check, not just defensive boilerplate).
  //
  // There is no BooleanBuffer in java.nio -- boolean is the one type
  // handled as a manual per-element byte loop instead of a bulk typed
  // get/put, still with no JNI/reflection per element or per leaf.

  // Deliberately conservative -- the 3.3b/3.4 experiments found the real
  // crossover ranges from ~1e6 (push) to ~1e7 (pull) elements depending on
  // per-leaf task size, not just total count, so a single shared
  // threshold errs toward "not worth it yet" rather than risking the far
  // worse case (measured up to ~35x slower) of dispatching parallel work
  // that doesn't pay for itself.
  private static final long PARALLEL_THRESHOLD_ELEMENTS = 1_000_000L;

  private static IntStream leafRange(int leaves, int leafLength)
  {
    IntStream range = IntStream.range(0, leaves);
    long total = (long) leaves * leafLength;
    return total >= PARALLEL_THRESHOLD_ELEMENTS ? range.parallel() : range;
  }

  // Mirrors JPRawTransferMode in native/common/include/jpype.h -- kept in
  // sync by hand, there being no shared header between the two languages.
  // RAW_NONE (0) never reaches here: the C++ caller only takes this path
  // (rather than the general element-by-element converter path) when it
  // resolved something other than NONE.
  private static final int RAW_NATIVE = 1;
  private static final int RAW_SWAPPED = 2;
  private static final int RAW_HALF_NATIVE = 3;
  private static final int RAW_HALF_SWAPPED = 4;

  private static ByteOrder swapped(ByteOrder order)
  {
    return order == ByteOrder.BIG_ENDIAN ? ByteOrder.LITTLE_ENDIAN : ByteOrder.BIG_ENDIAN;
  }

  /**
   * Decode a single IEEE 754 binary16 (numpy float16 / Python 'e' format)
   * value into its exact float32 equivalent. Standard bit-twiddling
   * decode (subnormal/normal/inf-or-nan branches) -- there is no half
   * type or ByteBuffer support in java.nio to lean on instead.
   */
  private static float halfToFloat(short bits)
  {
    int h = bits & 0xFFFF;
    int sign = (h & 0x8000) << 16;
    int exp = (h & 0x7C00) >> 10;
    int frac = h & 0x03FF;
    if (exp == 0)
    {
      if (frac == 0)
        return Float.intBitsToFloat(sign);
      // Subnormal half -> normalize into a normal float32.
      int e = -1;
      do
      {
        e++;
        frac <<= 1;
      } while ((frac & 0x0400) == 0);
      frac &= 0x03FF;
      int exp32 = 127 - 15 - e;
      return Float.intBitsToFloat(sign | (exp32 << 23) | (frac << 13));
    }
    if (exp == 0x1F)
      return Float.intBitsToFloat(sign | 0x7F800000 | (frac << 13));
    return Float.intBitsToFloat(sign | ((exp - 15 + 127) << 23) | (frac << 13));
  }

  /**
   * Build a rectangular multi-dimensional primitive array of the given
   * shape from a flat, C-contiguous direct buffer -- the push-side half
   * of the buffer-handoff redesign. `src`'s declared byte order is not
   * meaningful yet (a fresh NewDirectByteBuffer always defaults to
   * big-endian regardless of platform); `mode` says how to interpret it.
   *
   * @param typeCode primitive type signature character (the *target*
   * type -- for RAW_HALF_* this differs from the source's own type,
   * which is always 16-bit float).
   * @param mode one of RAW_NATIVE/RAW_SWAPPED/RAW_HALF_NATIVE/RAW_HALF_SWAPPED.
   * @param src a direct buffer over the source's total element count.
   * @param shape the array's shape, outermost dimension first.
   * @return the assembled array (e.g. int[][] for a 2D shape).
   */
  public static Object fillFromBuffer(char typeCode, int mode, ByteBuffer src, int[] shape)
  {
    if (mode == RAW_HALF_NATIVE || mode == RAW_HALF_SWAPPED)
    {
      src.order(mode == RAW_HALF_NATIVE ? ByteOrder.nativeOrder() : swapped(ByteOrder.nativeOrder()));
      return fillFromHalfBuffer(typeCode, src, shape);
    }
    src.order(mode == RAW_SWAPPED ? swapped(ByteOrder.nativeOrder()) : ByteOrder.nativeOrder());
    int dims = shape.length;
    int last = shape[dims - 1];
    int leaves = 1;
    for (int i = 0; i < dims - 1; i++)
      leaves *= shape[i];

    IntStream range = leafRange(leaves, last);

    Object flat;
    switch (typeCode)
    {
      case 'I':
      {
        IntBuffer buf = src.asIntBuffer();
        int[][] out = new int[leaves][];
        range.forEach(i ->
        {
          int[] row = new int[last];
          IntBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'D':
      {
        DoubleBuffer buf = src.asDoubleBuffer();
        double[][] out = new double[leaves][];
        range.forEach(i ->
        {
          double[] row = new double[last];
          DoubleBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'J':
      {
        LongBuffer buf = src.asLongBuffer();
        long[][] out = new long[leaves][];
        range.forEach(i ->
        {
          long[] row = new long[last];
          LongBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'F':
      {
        FloatBuffer buf = src.asFloatBuffer();
        float[][] out = new float[leaves][];
        range.forEach(i ->
        {
          float[] row = new float[last];
          FloatBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'S':
      {
        ShortBuffer buf = src.asShortBuffer();
        short[][] out = new short[leaves][];
        range.forEach(i ->
        {
          short[] row = new short[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'C':
      {
        CharBuffer buf = src.asCharBuffer();
        char[][] out = new char[leaves][];
        range.forEach(i ->
        {
          char[] row = new char[last];
          CharBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'B':
      {
        byte[][] out = new byte[leaves][];
        range.forEach(i ->
        {
          byte[] row = new byte[last];
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          dup.get(row, 0, last);
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'Z':
      {
        boolean[][] out = new boolean[leaves][];
        range.forEach(i ->
        {
          boolean[] row = new boolean[last];
          ByteBuffer dup = src.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = dup.get() != 0;
          out[i] = row;
        });
        flat = out;
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }

    // assemble()/unpack() work by reflection (Array.get/Array.set), so a
    // concretely-typed leaf array (int[][], double[][], ...) works
    // exactly like the Object[] they were originally written for -- no
    // changes needed there. Nesting itself is O(leaf count), already
    // cheap, not the part this redesign targets.
    return assemble(shape, flat);
  }

  /**
   * fillFromBuffer's RAW_HALF_NATIVE/RAW_HALF_SWAPPED case: `src` holds
   * IEEE 754 binary16 (numpy float16) values rather than typeCode's own
   * native encoding, so every element needs an actual decode (there is no
   * bulk java.nio path for a type java.nio doesn't know about) -- still
   * one JNI entry and no per-row critical sections, just a per-element
   * decode+cast done in bulk Java instead of one converter() call per
   * element back on the C++ side.
   */
  private static Object fillFromHalfBuffer(char typeCode, ByteBuffer src, int[] shape)
  {
    int dims = shape.length;
    int last = shape[dims - 1];
    int leaves = 1;
    for (int i = 0; i < dims - 1; i++)
      leaves *= shape[i];

    ShortBuffer buf = src.asShortBuffer();
    IntStream range = leafRange(leaves, last);

    Object flat;
    switch (typeCode)
    {
      case 'D':
      {
        double[][] out = new double[leaves][];
        range.forEach(i ->
        {
          double[] row = new double[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'F':
      {
        float[][] out = new float[leaves][];
        range.forEach(i ->
        {
          float[] row = new float[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'J':
      {
        long[][] out = new long[leaves][];
        range.forEach(i ->
        {
          long[] row = new long[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (long) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'I':
      {
        int[][] out = new int[leaves][];
        range.forEach(i ->
        {
          int[] row = new int[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (int) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'S':
      {
        short[][] out = new short[leaves][];
        range.forEach(i ->
        {
          short[] row = new short[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (short) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'C':
      {
        char[][] out = new char[leaves][];
        range.forEach(i ->
        {
          char[] row = new char[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (char) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'B':
      {
        byte[][] out = new byte[leaves][];
        range.forEach(i ->
        {
          byte[] row = new byte[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = (byte) halfToFloat(dup.get());
          out[i] = row;
        });
        flat = out;
        break;
      }
      case 'Z':
      {
        boolean[][] out = new boolean[leaves][];
        range.forEach(i ->
        {
          boolean[] row = new boolean[last];
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            row[j] = halfToFloat(dup.get()) != 0;
          out[i] = row;
        });
        flat = out;
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
    return assemble(shape, flat);
  }

  /**
   * Bulk-write a rectangular multi-dimensional primitive array's contents
   * into a caller-supplied direct buffer -- the pull-side half. `dest`'s
   * capacity must already match the total element count implied by
   * `collected`'s shape entry.
   *
   * @param typeCode primitive type signature character.
   * @param collected the result of {@link #collectRectangular}: [0] =
   * leaf component Class (unused here, typeCode is passed separately by
   * the C++ caller instead), [1] = int[] shape, [2..] = leaf arrays in
   * row-major order.
   * @param dest a direct, writable buffer of the right total byte
   * capacity.
   */
  public static void collectToBuffer(char typeCode, Object[] collected, ByteBuffer dest)
  {
    dest.order(ByteOrder.nativeOrder());
    int[] shape = (int[]) collected[1];
    int last = shape[shape.length - 1];
    int leaves = collected.length - 2;

    IntStream range = leafRange(leaves, last);

    switch (typeCode)
    {
      case 'I':
      {
        IntBuffer buf = dest.asIntBuffer();
        range.forEach(i ->
        {
          IntBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((int[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'D':
      {
        DoubleBuffer buf = dest.asDoubleBuffer();
        range.forEach(i ->
        {
          DoubleBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((double[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'J':
      {
        LongBuffer buf = dest.asLongBuffer();
        range.forEach(i ->
        {
          LongBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((long[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'F':
      {
        FloatBuffer buf = dest.asFloatBuffer();
        range.forEach(i ->
        {
          FloatBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((float[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'S':
      {
        ShortBuffer buf = dest.asShortBuffer();
        range.forEach(i ->
        {
          ShortBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((short[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'C':
      {
        CharBuffer buf = dest.asCharBuffer();
        range.forEach(i ->
        {
          CharBuffer dup = buf.duplicate();
          dup.position(i * last);
          dup.put((char[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'B':
      {
        range.forEach(i ->
        {
          ByteBuffer dup = dest.duplicate();
          dup.position(i * last);
          dup.put((byte[]) collected[i + 2], 0, last);
        });
        break;
      }
      case 'Z':
      {
        range.forEach(i ->
        {
          boolean[] row = (boolean[]) collected[i + 2];
          ByteBuffer dup = dest.duplicate();
          dup.position(i * last);
          for (int j = 0; j < last; j++)
            dup.put(row[j] ? (byte) 1 : (byte) 0);
        });
        break;
      }
      default:
        throw new IllegalArgumentException("Unknown primitive type code: " + typeCode);
    }
  }

}
