// --- file: org/jpype/Interpreter.java ---
package org.jpype;

import python.lang.PyBuiltIn;

/**
 * Defines the core execution capabilities of a Python interpreter instance.
 */
public interface Interpreter extends AutoCloseable 
{
   PyBuiltIn getBuiltIn();
  
    /**
     * Closes the interpreter instance and disposes of its associated resources.
     */
    @Override
    void close();
}