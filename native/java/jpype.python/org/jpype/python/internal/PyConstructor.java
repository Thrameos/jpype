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
