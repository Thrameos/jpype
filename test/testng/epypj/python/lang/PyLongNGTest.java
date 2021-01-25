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
public class PyLongNGTest
{

  public PyLongNGTest()
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
   * Test of intValue method, of class PyLong.
   */
  @Test
  public void testIntValue()
  {
    System.out.println("intValue");
    PyLong instance = null;
    int expResult = 0;
    int result = instance.intValue();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of longValue method, of class PyLong.
   */
  @Test
  public void testLongValue()
  {
    System.out.println("longValue");
    PyLong instance = null;
    long expResult = 0L;
    long result = instance.longValue();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of floatValue method, of class PyLong.
   */
  @Test
  public void testFloatValue()
  {
    System.out.println("floatValue");
    PyLong instance = null;
    float expResult = 0.0F;
    float result = instance.floatValue();
    assertEquals(result, expResult, 0.0);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of doubleValue method, of class PyLong.
   */
  @Test
  public void testDoubleValue()
  {
    System.out.println("doubleValue");
    PyLong instance = null;
    double expResult = 0.0;
    double result = instance.doubleValue();
    assertEquals(result, expResult, 0.0);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of toString method, of class PyLong.
   */
  @Test
  public void testToString()
  {
    System.out.println("toString");
    PyLong instance = null;
    String expResult = "";
    String result = instance.toString();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of hashCode method, of class PyLong.
   */
  @Test
  public void testHashCode()
  {
    System.out.println("hashCode");
    PyLong instance = null;
    int expResult = 0;
    int result = instance.hashCode();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of equals method, of class PyLong.
   */
  @Test
  public void testEquals()
  {
    System.out.println("equals");
    Object obj = null;
    PyLong instance = null;
    boolean expResult = false;
    boolean result = instance.equals(obj);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

}
