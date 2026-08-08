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
package jpype.benchmark;

import java.util.List;
import java.nio.IntBuffer;
import java.util.stream.IntStream;

// Cross-library benchmark harness for deeper conversion-chain paths than
// project/benchmark/bench_*.py's simple Math.max/Integer/String cases:
// overload resolution across many candidates, and array/List argument
// conversion (which is inherently content-dependent -- see
// JPConversionSequence in jp_classhints.cpp -- so never cacheable, unlike
// the hint-list scan this session's work targeted).
public class DeepBench
{

  public static class T0
  {
  }

  public static class T1
  {
  }

  public static class T2
  {
  }

  public static class T3
  {
  }

  public static class T4
  {
  }

  public static class T5
  {
  }

  public static class T6
  {
  }

  public static class T7
  {
  }

  public static class T8
  {
  }

  public static class T9
  {
  }

  public static class T10
  {
  }

  public static class T11
  {
  }

  public static class T12
  {
  }

  public static class T13
  {
  }

  public static class T14
  {
  }

  public static class T15
  {
  }

  public static int call(T0 a)
  {
    return 0;
  }

  public static int call(T1 a)
  {
    return 1;
  }

  public static int call(T2 a)
  {
    return 2;
  }

  public static int call(T3 a)
  {
    return 3;
  }

  public static int call(T4 a)
  {
    return 4;
  }

  public static int call(T5 a)
  {
    return 5;
  }

  public static int call(T6 a)
  {
    return 6;
  }

  public static int call(T7 a)
  {
    return 7;
  }

  public static int call(T8 a)
  {
    return 8;
  }

  public static int call(T9 a)
  {
    return 9;
  }

  public static int call(T10 a)
  {
    return 10;
  }

  public static int call(T11 a)
  {
    return 11;
  }

  public static int call(T12 a)
  {
    return 12;
  }

  public static int call(T13 a)
  {
    return 13;
  }

  public static int call(T14 a)
  {
    return 14;
  }

  // T15 -- the last of 16 overloads, worst case for a linear scan starting
  // from the first candidate.
  public static int call(T15 a)
  {
    return 15;
  }

  public static long sumIntArray(int[] a)
  {
    long s = 0;
    for (int x : a)
      s += x;
    return s;
  }

  // Identity passthrough for a 1D int array -- used by jep/array_multidim.py
  // to manually assemble a genuine multi-dimensional Java array one row at a
  // time (jep's numpy fast path only ever targets a flat int[]; there's no
  // way to bulk-load a multi-dim array in one call, so each leaf row is
  // bulk-converted via this method instead, and the nesting above that is
  // pure Python-side array construction -- see that file for details).
  public static int[] identityIntArray(int[] a)
  {
    return a;
  }

  public static long sumIntList(List<Integer> a)
  {
    long s = 0;
    for (int x : a)
      s += x;
    return s;
  }

  // 2D variant of sumIntArray -- component type is itself int[], so
  // conversion recurses through the array-conversion machinery once per
  // outer element in addition to the per-element work each inner array
  // already does.
  public static long sum2DIntArray(int[][] a)
  {
    long s = 0;
    for (int[] row : a)
      for (int x : row)
        s += x;
    return s;
  }

  // 3D/4D/5D variants of sum2DIntArray -- see project/benchmark/bench_arrays_*.py,
  // which sweeps nesting depth (holding total element count fixed) to isolate
  // per-dimension conversion overhead from raw element count.
  public static long sum3DIntArray(int[][][] a)
  {
    long s = 0;
    for (int[][] plane : a)
      for (int[] row : plane)
        for (int x : row)
          s += x;
    return s;
  }

  public static long sum4DIntArray(int[][][][] a)
  {
    long s = 0;
    for (int[][][] cube : a)
      for (int[][] plane : cube)
        for (int[] row : plane)
          for (int x : row)
            s += x;
    return s;
  }

  public static long sum5DIntArray(int[][][][][] a)
  {
    long s = 0;
    for (int[][][][] hcube : a)
      for (int[][][] cube : hcube)
        for (int[][] plane : cube)
          for (int[] row : plane)
            for (int x : row)
              s += x;
    return s;
  }

  // "make*IntArray" -- the Java-side counterpart of sum*IntArray, for
  // benchmarking the opposite direction (a Java array's contents flowing
  // back into Python as the method return value) at the same sizes/depths.
  // Filled (not left zeroed) so a bulk buffer-protocol readback path can't
  // be short-circuited by an all-zeroes special case on either side.
  public static int[] makeIntArray(int n)
  {
    int[] a = new int[n];
    for (int i = 0; i < n; i++)
      a[i] = i;
    return a;
  }

  public static int[][] make2DIntArray(int n)
  {
    int[][] a = new int[n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        a[i][j] = i * n + j;
    return a;
  }

  public static int[][][] make3DIntArray(int n)
  {
    int[][][] a = new int[n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          a[i][j][k] = (i * n + j) * n + k;
    return a;
  }

  public static int[][][][] make4DIntArray(int n)
  {
    int[][][][] a = new int[n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            a[i][j][k][l] = ((i * n + j) * n + k) * n + l;
    return a;
  }

  public static int[][][][][] make5DIntArray(int n)
  {
    int[][][][][] a = new int[n][n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
            for (int m = 0; m < n; m++)
              a[i][j][k][l][m] = (((i * n + j) * n + k) * n + l) * n + m;
    return a;
  }

  // ---- phase 3.3b/3.4 experiment (plan/ArrayTransferPhase3.md): does a
  // direct-buffer source + pure-Java reshape (no GetPrimitiveArrayCritical
  // at all -- IntBuffer reads and fresh-array writes are both plain,
  // GC-safe Java operations, so the leaf-array fill can be safely
  // parallelized with an ordinary IntStream) beat the current single JNI
  // critical section per leaf array that JPConversionMultiArrayBuffer
  // uses today? `buf` is expected to be a direct IntBuffer -- on the
  // Python side, jpype.nio.convertToDirectBuffer(numpy_array) zero-copy
  // wraps the source memory (JNI NewDirectByteBuffer over the existing
  // buffer-protocol pointer, no host-side copy), so this method never
  // touches JNI at all after being entered -- everything below is pure
  // Java. Each leaf's `duplicate()` gives it an independent
  // position/limit so concurrent reads from different leaves need no
  // synchronization.

  public static int[][] fillBuffer2D(IntBuffer buf, int n)
  {
    int[][] out = new int[n][n];
    for (int i = 0; i < n; i++)
    {
      IntBuffer dup = buf.duplicate();
      dup.position(i * n);
      dup.get(out[i], 0, n);
    }
    return out;
  }

  public static int[][] fillBuffer2DParallel(IntBuffer buf, int n)
  {
    int[][] out = new int[n][n];
    IntStream.range(0, n).parallel().forEach(i ->
    {
      IntBuffer dup = buf.duplicate();
      dup.position(i * n);
      dup.get(out[i], 0, n);
    });
    return out;
  }

  public static int[][][] fillBuffer3D(IntBuffer buf, int n)
  {
    int[][][] out = new int[n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
      {
        IntBuffer dup = buf.duplicate();
        dup.position((i * n + j) * n);
        dup.get(out[i][j], 0, n);
      }
    return out;
  }

  public static int[][][] fillBuffer3DParallel(IntBuffer buf, int n)
  {
    int[][][] out = new int[n][n][n];
    IntStream.range(0, n * n).parallel().forEach(idx ->
    {
      int i = idx / n, j = idx % n;
      IntBuffer dup = buf.duplicate();
      dup.position(idx * n);
      dup.get(out[i][j], 0, n);
    });
    return out;
  }

  public static int[][][][] fillBuffer4D(IntBuffer buf, int n)
  {
    int[][][][] out = new int[n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
        {
          IntBuffer dup = buf.duplicate();
          dup.position(((i * n + j) * n + k) * n);
          dup.get(out[i][j][k], 0, n);
        }
    return out;
  }

  public static int[][][][] fillBuffer4DParallel(IntBuffer buf, int n)
  {
    int[][][][] out = new int[n][n][n][n];
    IntStream.range(0, n * n * n).parallel().forEach(idx ->
    {
      int i = idx / (n * n), j = (idx / n) % n, k = idx % n;
      IntBuffer dup = buf.duplicate();
      dup.position(idx * n);
      dup.get(out[i][j][k], 0, n);
    });
    return out;
  }

  public static int[][][][][] fillBuffer5D(IntBuffer buf, int n)
  {
    int[][][][][] out = new int[n][n][n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        for (int k = 0; k < n; k++)
          for (int l = 0; l < n; l++)
          {
            IntBuffer dup = buf.duplicate();
            dup.position((((i * n + j) * n + k) * n + l) * n);
            dup.get(out[i][j][k][l], 0, n);
          }
    return out;
  }

  public static int[][][][][] fillBuffer5DParallel(IntBuffer buf, int n)
  {
    int[][][][][] out = new int[n][n][n][n][n];
    IntStream.range(0, n * n * n * n).parallel().forEach(idx ->
    {
      int i = idx / (n * n * n), j = (idx / (n * n)) % n,
          k = (idx / n) % n, l = idx % n;
      IntBuffer dup = buf.duplicate();
      dup.position(idx * n);
      dup.get(out[i][j][k][l], 0, n);
    });
    return out;
  }

  // Chunked variant of fillBuffer3DParallel: one parallel task per leaf
  // array dispatches Runtime.availableProcessors()-many-times more tasks
  // than there are cores, each doing almost no work (a few hundred bytes)
  // -- dispatch overhead per task dominates regardless of total leaf
  // count. This instead splits the leaves into exactly
  // availableProcessors() contiguous chunks, each handled by one task
  // doing a plain serial loop internally -- amortizes dispatch cost over
  // a chunk's worth of leaves instead of paying it per leaf.
  public static int[][][] fillBuffer3DChunked(IntBuffer buf, int n)
  {
    int[][][] out = new int[n][n][n];
    int leaves = n * n;
    int chunks = Math.min(leaves, Runtime.getRuntime().availableProcessors());
    IntStream.range(0, chunks).parallel().forEach(c ->
    {
      int lo = (int) ((long) leaves * c / chunks);
      int hi = (int) ((long) leaves * (c + 1) / chunks);
      IntBuffer dup = buf.duplicate();
      for (int idx = lo; idx < hi; idx++)
      {
        dup.position(idx * n);
        dup.get(out[idx / n][idx % n], 0, n);
      }
    });
    return out;
  }

  // Same idea, but for a list-sourced push: `flat` is an ordinary Java
  // heap int[] (built cheaply by the existing tight 1D list conversion --
  // JPIntType::setArrayRange, one JNI pin for the whole flat array, no
  // per-row JNI calls). Reading a Java array from Java code needs no
  // critical section either -- System.arraycopy is the heap-array
  // equivalent of the IntBuffer.get bulk read above.

  public static int[][] fillFlat2D(int[] flat, int n)
  {
    int[][] out = new int[n][n];
    for (int i = 0; i < n; i++)
      System.arraycopy(flat, i * n, out[i], 0, n);
    return out;
  }

  public static int[][] fillFlat2DParallel(int[] flat, int n)
  {
    int[][] out = new int[n][n];
    IntStream.range(0, n).parallel().forEach(i ->
        System.arraycopy(flat, i * n, out[i], 0, n));
    return out;
  }

  public static int[][][] fillFlat3D(int[] flat, int n)
  {
    int[][][] out = new int[n][n][n];
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        System.arraycopy(flat, (i * n + j) * n, out[i][j], 0, n);
    return out;
  }

  public static int[][][] fillFlat3DParallel(int[] flat, int n)
  {
    int[][][] out = new int[n][n][n];
    IntStream.range(0, n * n).parallel().forEach(idx ->
        System.arraycopy(flat, idx * n, out[idx / n][idx % n], 0, n));
    return out;
  }

  // "object" category: argument matching + return-value wrapping for a
  // plain Object, as opposed to a primitive/boxed/array/string value.
  public static Object identity(Object o)
  {
    return o;
  }

  // "proxy" category: Java calling back into Python through an interface
  // a Python object implements. Each library has its own mechanism for
  // exposing a Python object as this interface (see
  // project/benchmark/README.md); invokeCallback itself is the same call
  // for all of them once that binding exists.
  public interface Callback
  {
    int run(int x);
  }

  public static int invokeCallback(Callback cb, int x)
  {
    return cb.run(x);
  }

  // Loop-on-the-Java-side variant: needed for libraries (jpy) whose
  // Python-side binding can't reliably call methods on a proxy object
  // directly -- see project/benchmark/README.md. Divide the result's
  // wall-clock time by iterations for a per-call figure.
  public static long invokeCallbackLoop(Callback cb, int iterations)
  {
    long sum = 0;
    for (int i = 0; i < iterations; i++)
      sum += cb.run(i);
    return sum;
  }

  // Regression coverage for jp_proxy.cpp's getArgs(): a proxy callback
  // argument's runtime class isn't always the declared one -- covers both
  // a genuinely null argument (GetObjectClass/IsSameObject must not be
  // called on it) and a covariant one (declared Object, actual T15).
  public interface ObjectCallback
  {
    Object handle(Object o);
  }

  public static Object invokeObjectCallbackWithNull(ObjectCallback cb)
  {
    return cb.handle(null);
  }

  public static Object invokeObjectCallbackWithSubtype(ObjectCallback cb)
  {
    return cb.handle(new T15());
  }

  public static Object invokeObjectCallback(ObjectCallback cb, Object o)
  {
    return cb.handle(o);
  }
}
