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
package python.lang;

import static org.testng.Assert.*;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

/**
 *
 * @author nelson85
 */
public class PyStringNGTest
{

  public PyStringNGTest()
  {
  }

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    PythonTest.getEngine();

  }

  @AfterClass
  public static void tearDownClass() throws Exception
  {
  }

  @BeforeMethod
  public void setUpMethod() throws Exception
  {
  }

  @AfterMethod
  public void tearDownMethod() throws Exception
  {
  }

  /**
   * Test of length method, of class PyString.
   */
  @Test
  public void testLength()
  {
    System.out.println("length");
    PyString instance = null;
    int expResult = 0;
    int result = instance.length();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of size method, of class PyString.
   */
  @Test
  public void testSize()
  {
    System.out.println("size");
    PyString instance = new PyString("ABC");
    assertEquals(instance.size(), 3);
  }

  /**
   * Test of charAt method, of class PyString.
   */
  @Test
  public void testCharAt()
  {
    System.out.println("charAt");
    PyString instance = new PyString("ABC");
    assertEquals(instance.charAt(0), "A");
  }

  /**
   * Test of subSequence method, of class PyString.
   */
  @Test
  public void testSubSequence()
  {
    System.out.println("subSequence");
    int start = 0;
    int end = 0;
    PyString instance = new PyString("ABC");
    CharSequence result = instance.subSequence(1, 3);
    assertEquals(result, "BC");
  }

  /**
   * Test of toString method, of class PyString.
   */
  @Test
  public void testToString()
  {
    System.out.println("toString");
    PyString instance = new PyString("ABC");
    String result = instance.toString();
    assertEquals(result, "ABC");
  }

}
