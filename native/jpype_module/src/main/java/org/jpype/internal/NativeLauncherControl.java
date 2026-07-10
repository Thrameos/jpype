// --- file: org/jpype/internal/NativeLauncherControl.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package org.jpype.internal;

/**
 * Internal behaviors used by the binding.
 *
 * Loaded by the _jpype module.
 *
 */
public class NativeLauncherControl
{

  public native static NativeContext startMain(String[] modulePaths, String[] args,
          String name, String prefix, String home, String exec_prefix, String executable,
          boolean isolated, boolean fault_handler, boolean quiet, boolean verbose,
          boolean site_import, boolean user_site, boolean write_bytecode, Object interpreter);

  public native static void interactive(long context);

  public native static void finishMain(long context);

  /**
   * Check whether the calling thread currently holds the Python GIL.
   *
   * This is intended to be asserted from Java code at points where the
   * calling thread is guaranteed to have already returned control to Java
   * (e.g. after any bridge call, or in test teardown) so that a leaked or
   * double-released GIL hold on some invisible, ungrepped code path can be
   * caught close to its source rather than surfacing later as a hang.
   *
   * Safe to call whether or not the calling thread currently holds the GIL.
   */
  public native static boolean isGilHeld();

}
