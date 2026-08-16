import jep.Interpreter;
import jep.SharedInterpreter;
import jep.DirectNDArray;

import java.nio.ByteBuffer;
import java.nio.DoubleBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * Java-drives-Python array-transfer bake-off, JEP side. JEP has no bulk
 * multidim/copyInto-style fast path at all (its only non-naive tool is
 * DirectNDArray, a flat direct-buffer <-> numpy.ndarray zero-copy view) -
 * each model below uses JEP's actual best available route, naive where
 * there isn't one, labeled accordingly.
 */
public class JepArrayBench {
    public static void main(String[] args) throws Exception {
        BenchCommon bc = new BenchCommon();
        try (Interpreter interp = new SharedInterpreter()) {
            interp.exec("import sys; sys.path.insert(0, '.')");
            interp.exec("import bench_array");

            // ---- Model 1 comparator: naive element marshaling (no bulk path exists) ----
            for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
                final int size = BenchCommon.SIZES[idx];
                final int nCalls = BenchCommon.N_CALLS[idx];
                List<Object> src = new ArrayList<>(size);
                for (int i = 0; i < size; i++) src.add((double) i);
                bc.timeit("copyInto_naive_" + size, nCalls, () -> {
                    for (int i = 0; i < nCalls; i++) {
                        interp.invoke("bench_array.naive_list_sum", src);
                    }
                });
            }

            // ---- Model 2: direct-buffer-shared via DirectNDArray (JEP's real fast path) ----
            for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
                final int size = BenchCommon.SIZES[idx];
                final int nCalls = BenchCommon.N_CALLS[idx];
                ByteBuffer bb = ByteBuffer.allocateDirect(size * 8);
                DoubleBuffer buf = bb.asDoubleBuffer();
                for (int i = 0; i < size; i++) buf.put(i, (double) i);
                DirectNDArray<DoubleBuffer> nd = new DirectNDArray<>(buf);
                bc.timeit("directBuffer_" + size, nCalls, () -> {
                    for (int i = 0; i < nCalls; i++) {
                        interp.invoke("bench_array.sum_direct_buffer", nd);
                    }
                });
            }

            // ---- Model 3 comparator: python-side slicing only (no Java-side array type to slice) ----
            for (int idx = 0; idx < BenchCommon.SIZES.length; idx++) {
                final int size = BenchCommon.SIZES[idx];
                final int nCalls = BenchCommon.N_CALLS[idx];
                final int step = BenchCommon.SLICE_STEPS[idx];
                interp.invoke("bench_array.make_slice_source", size);
                bc.timeit("slice_python_" + size, nCalls, () -> {
                    for (int i = 0; i < nCalls; i++) {
                        interp.invoke("bench_array.sum_slice", step);
                    }
                });
            }

            // ---- Model 4a comparator: naive per-row loop (no multidim accelerator) ----
            int[][] matShapes = {{10, 10}, {300, 300}, {1000, 1000}};
            for (int[] shape : matShapes) {
                final int rows = shape[0], cols = shape[1];
                List<Object> matRows = new ArrayList<>(rows);
                for (int r = 0; r < rows; r++) {
                    List<Object> row = new ArrayList<>(cols);
                    for (int c = 0; c < cols; c++) row.add((double) (r * cols + c));
                    matRows.add(row);
                }
                final int nCalls = rows * cols > 200_000 ? 10 : 500;
                bc.timeit("multidim_naive_" + rows + "x" + cols, nCalls, () -> {
                    for (int i = 0; i < nCalls; i++) {
                        interp.invoke("bench_array.sum_2d_looped", matRows);
                    }
                });
            }

            // ---- Model 4b: JEP's real fast path for multidim, IF the data is
            // already a flat direct buffer with known dimensions ----
            for (int[] shape : matShapes) {
                final int rows = shape[0], cols = shape[1];
                int n = rows * cols;
                ByteBuffer bb = ByteBuffer.allocateDirect(n * 8);
                DoubleBuffer buf = bb.asDoubleBuffer();
                for (int i = 0; i < n; i++) buf.put(i, (double) i);
                DirectNDArray<DoubleBuffer> nd = new DirectNDArray<>(buf, rows, cols);
                final int nCalls = n > 200_000 ? 10 : 500;
                bc.timeit("multidim_directbuffer_" + rows + "x" + cols, nCalls, () -> {
                    for (int i = 0; i < nCalls; i++) {
                        interp.invoke("bench_array.sum_direct_buffer", nd);
                    }
                });
            }
        }
        bc.printJson();
    }
}
