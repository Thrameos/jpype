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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

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

}
