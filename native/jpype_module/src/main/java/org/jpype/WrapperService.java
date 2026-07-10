// --- file: org/jpype/bridge/WrapperService.java ---
package org.jpype;


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
     * Returns the set of Java interfaces that should be added to the 
     * generated wrapper for this Python type.
     * 
     * @param clsName a fully qualified class name with module prefix.
     * @return An array of interfaces, or null if only default wrapping is needed.
     */
    Class<?>[] getInterfaces(String clsName);

    /**
     * Optional: Provides specialized logic for the backend to handle
     * this specific type (e.g., custom memory mapping for buffers).
     */
    default void initialize(Backend backend) {
        // Default: no specialized backend initialization
    }

    /**
     * Classpath paths (resolved relative to this service's own class, e.g.
     * via {@code getClass().getResourceAsStream(path)}) to this provider's
     * {@code .pyspi} resources — one per Python class it registers eagerly,
     * plus one per mini-backend it needs. Read and replayed into the
     * {@link Installer} by {@link SpiLoader} at startup.
     *
     * Default empty: providers that only support the (not yet implemented)
     * lazy per-class lookup via {@link #getInterfaces} need not supply any.
     */
    default String[] getEagerResources() {
        return new String[0];
    }
}