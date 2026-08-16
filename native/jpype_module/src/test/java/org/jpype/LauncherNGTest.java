// --- file: org/jpype/LauncherNGTest.java ---
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
package org.jpype;

import org.testng.annotations.Test;
import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNull;

/**
 * Exercises {@code Launcher}'s pure, side-effect-free logic:
 * {@link Launcher#parseVersion} and the fresh-instance getters/
 * {@code isPrepared()}. The rest of {@code Launcher} (detective-probe
 * subprocess execution, pip self-healing, on-disk cache read/write, native
 * library loading) drives real subprocesses/network/filesystem access as
 * part of process bootstrap and is exercised implicitly by every other test
 * class via {@code PyTestHarness}'s one-time interpreter startup - it is not
 * a good target for isolated unit tests without dedicated mocking
 * infrastructure (see plan/Coverage.md).
 */
public class LauncherNGTest
{

  @Test
  public void testParseVersionThreeParts()
  {
    assertEquals(Launcher.parseVersion("3.9.7"), new int[]
    {
      3, 9, 7
    });
  }

  @Test
  public void testParseVersionShortAndLong()
  {
    assertEquals(Launcher.parseVersion("3.12"), new int[]
    {
      3, 12, 0
    });
    // A 4th+ component is ignored (the array is fixed at 3 slots).
    assertEquals(Launcher.parseVersion("3.12.1.extra"), new int[]
    {
      3, 12, 1
    });
  }

  @Test
  public void testParseVersionNonNumericFallsBackToZeros()
  {
    // NumberFormatException is swallowed; whatever was parsed before the
    // bad component stays, later slots stay at their int[] default of 0.
    assertEquals(Launcher.parseVersion("abc"), new int[]
    {
      0, 0, 0
    });
  }

  @Test
  public void testFreshInstanceIsNotPrepared()
  {
    Launcher launcher = new Launcher();
    assertFalse(launcher.isPrepared());
    assertNull(launcher.getPythonExecutable());
    assertNull(launcher.getPythonLibrary());
    assertNull(launcher.getJpypeLibrary());
    assertNull(launcher.getJpypeVersion());
  }
}
