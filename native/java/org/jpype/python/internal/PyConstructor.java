package org.jpype.python.internal;

import org.jpype.ref.JPypeReferenceQueue;

public class PyConstructor
{

  private static JPypeReferenceQueue referenceQueue = JPypeReferenceQueue.getInstance();
  public static PyConstructor ALLOCATOR = new PyConstructor(true);
  public static PyConstructor CONSTRUCTOR = new PyConstructor(false);

  static long cleanup = 0;
  private final boolean shouldReference;

  PyConstructor(boolean reference)
  {
    this.shouldReference = reference;
  }

  /**
   * Create a link between a Java object and Python object such that the Python
   * object may not be destroyed.
   *
   * This command is dangerous because it will increment the memory pointed to
   * by the long.
   *
   * @param javaObject
   * @param pyObject
   */
  public void link(Object javaObject, long pyObject)
  {
    if (pyObject == 0)
      return;
    if (shouldReference)
      incref(pyObject);
    if (cleanup == 0)
      cleanup = init();
    referenceQueue.registerRef(javaObject, pyObject, cleanup);
  }

  /**
   * Get the resources needed for this object
   */
  native static long init();

  /**
   * Increment the reference counter
   */
  native static void incref(long l);
}
