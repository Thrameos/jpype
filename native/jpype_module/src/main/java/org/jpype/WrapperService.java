// --- file: org/jpype/bridge/WrapperService.java ---
package org.jpype;

import java.util.Collections;

/**
 * Service provider interface for extending the Java view of Python types.
 * * Registered via Jigsaw: 
 * provides org.jpype.bridge.WrapperService with my.package.NumpyWrapperService;
 */
public interface WrapperService {

    /**
     * A list of fully qualified Python module names this
     * e.g., "numpy"
     * 
     * One Wrapper service can 
     */
    String[] getModuleNames();
    
    /**
     * Get the Python module this binding was targeting. 
     * @return a version string.
     */
    String getVersion();

    /**
     * Optional: Provides specialized logic for the backend to handle
     * this specific type (e.g., custom memory mapping for buffers).
     */
    default void initialize(Backend backend) {
        // Default: no specialized backend initialization
    }

    /**
     * Classpath paths (resolved relative to this service's own class, e.g.
     * via {@code getClass().getResourceAsStream(path)}) to every one of
     * this provider's {@code .pyspi} resources — one per Python class it
     * registers, plus one per mini-backend it needs. Read by
     * {@link SpiLoader} at startup and replayed into the {@link Installer}.
     *
     * Each resource declares its own {@code kind:} ({@code class} or
     * {@code backend}) and, for classes, whether it should resolve eagerly
     * (replayed immediately) or lazily (only imported/executed the first
     * time a matching Python type is actually seen crossing into Java, via
     * {@code lazy: true} in its header) — see {@code SpiResource} and
     * {@code plan/SPI.md}. A provider does not need separate methods for
     * eager vs. lazy; it just needs to list every resource here, typically
     * by scanning its own resource directory rather than hardcoding each
     * path (see {@code python.io.PyIoWrapperService} for a worked example).
     *
     * Default empty.
     */
    default Iterable<String> getResources() {
        return Collections.emptyList();
    }
}