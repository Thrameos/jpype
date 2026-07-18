import org.jpype.MainInterpreter;
import org.jpype.Script;
import python.lang.PyCallable;
import python.lang.PyDict;

import java.nio.ByteBuffer;
import java.nio.DoubleBuffer;

/**
 * Java-drives-Python array-transfer bake-off, jpype side. Java is only the
 * trigger/stopwatch here - every model's actual work happens in
 * bench_array.py, same as the jpype half of any other Java-drives-Python
 * benchmark in this repo.
 */
public class JpypeArrayBench {
    public static void main(String[] args) throws Throwable {
        BenchCommon bc = new BenchCommon();
        MainInterpreter.getInstance().start(new String[0]);
        Script context = new Script(MainInterpreter.getInstance());
        context.importModule("bench_array");
        PyDict noKwargs = context.dict();

        PyCallable setupCopyInto = (PyCallable) context.eval("bench_array.setup_copy_into");
        PyCallable copyInto = (PyCallable) context.eval("bench_array.copy_into");
        PyCallable setupDirectBuffer = (PyCallable) context.eval("bench_array.setup_direct_buffer");
        PyCallable sumDirectBufferShared = (PyCallable) context.eval("bench_array.sum_direct_buffer_shared");
        PyCallable makeSliceSource = (PyCallable) context.eval("bench_array.make_slice_source");
        PyCallable sumSlice = (PyCallable) context.eval("bench_array.sum_slice");
        PyCallable setupJavaArraySlice = (PyCallable) context.eval("bench_array.setup_java_array_slice");
        PyCallable sumJavaArraySlice = (PyCallable) context.eval("bench_array.sum_java_array_slice");
        PyCallable sum2dBulk = (PyCallable) context.eval("bench_array.sum_2d_bulk");

        // ---- Model 1: copyInto (jpype's bulk-copy fast path) ----
        for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
            final int size = BenchCommon.SIZES[idx];
            final int nCalls = BenchCommon.N_CALLS[idx];
            context.call(setupCopyInto, context.tuple(size), noKwargs);
            bc.timeit("copyInto_" + size, nCalls, () -> {
                for (int i = 0; i < nCalls; i++) {
                    context.call(copyInto, context.tuple(), noKwargs);
                }
            });
        }

        // ---- Model 2: direct-buffer-shared ----
        for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
            final int size = BenchCommon.SIZES[idx];
            final int nCalls = BenchCommon.N_CALLS[idx];
            ByteBuffer bb = ByteBuffer.allocateDirect(size * 8);
            DoubleBuffer buf = bb.asDoubleBuffer();
            for (int i = 0; i < size; i++) buf.put(i, (double) i);
            context.call(setupDirectBuffer, context.tuple(buf), noKwargs);
            bc.timeit("directBuffer_" + size, nCalls, () -> {
                for (int i = 0; i < nCalls; i++) {
                    context.call(sumDirectBufferShared, context.tuple(), noKwargs);
                }
            });
        }

        // ---- Model 3: slicing (zero-copy view) ----
        for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
            final int size = BenchCommon.SIZES[idx];
            final int nCalls = BenchCommon.N_CALLS[idx];
            final int step = BenchCommon.SLICE_STEPS[idx];
            context.call(makeSliceSource, context.tuple(size), noKwargs);
            bc.timeit("slice_python_" + size, nCalls, () -> {
                for (int i = 0; i < nCalls; i++) {
                    context.call(sumSlice, context.tuple(step), noKwargs);
                }
            });

            context.call(setupJavaArraySlice, context.tuple(size), noKwargs);
            bc.timeit("slice_javaArray_" + size, nCalls, () -> {
                for (int i = 0; i < nCalls; i++) {
                    context.call(sumJavaArraySlice, context.tuple(step), noKwargs);
                }
            });
        }

        // ---- Model 4: multidimensional bulk transfer (real double[][]) ----
        int[][] matShapes = {{10, 10}, {300, 300}, {1000, 1000}};
        for (int[] shape : matShapes) {
            final int rows = shape[0], cols = shape[1];
            double[][] mat = new double[rows][cols];
            for (int r = 0; r < rows; r++)
                for (int c = 0; c < cols; c++)
                    mat[r][c] = r * cols + c;
            final int nCalls = rows * cols > 200_000 ? 10 : 500;
            bc.timeit("multidim_" + rows + "x" + cols, nCalls, () -> {
                for (int i = 0; i < nCalls; i++) {
                    context.call(sum2dBulk, context.tuple((Object) mat), noKwargs);
                }
            });
        }

        bc.printJson();
        MainInterpreter.getInstance().close();
    }
}
