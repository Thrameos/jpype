import java.util.LinkedHashMap;
import java.util.Map;

/** Shared constants + timing/JSON helpers for the array-transfer bake-off. */
public class BenchCommon {
    public static final int[] SIZES = {1_000, 100_000, 1_000_000};
    public static final int[] N_CALLS = {2000, 100, 10};
    public static final int[] SLICE_STEPS = {2, 2, 2};

    public interface Timed {
        void run() throws Exception;
    }

    private final Map<String, double[]> results = new LinkedHashMap<>();

    public void timeit(String label, int n, Timed fn) throws Exception {
        long t0 = System.nanoTime();
        fn.run();
        long t1 = System.nanoTime();
        double dt = (t1 - t0) / 1e9;
        results.put(label, new double[]{dt, n, dt / n * 1e6});
        System.err.printf("%s: %.4fs for %d -> %.3f us/call%n", label, dt, n, dt / n * 1e6);
    }

    public void printJson() {
        StringBuilder sb = new StringBuilder("{");
        boolean first = true;
        for (Map.Entry<String, double[]> e : results.entrySet()) {
            if (!first) sb.append(",");
            first = false;
            double[] v = e.getValue();
            sb.append("\"").append(e.getKey()).append("\":{");
            sb.append("\"total_s\":").append(v[0]).append(",");
            sb.append("\"n\":").append((int) v[1]).append(",");
            sb.append("\"per_call_us\":").append(v[2]);
            sb.append("}");
        }
        sb.append("}");
        System.out.println(sb);
    }
}
