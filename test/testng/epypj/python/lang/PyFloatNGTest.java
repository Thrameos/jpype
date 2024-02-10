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
public class PyFloatNGTest
{

  public PyFloatNGTest()
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
   * Test of of method, of class PyFloat.
   */
  @Test
  public void testOf()
  {
    System.out.println("of");
    long d = 0L;
    PyFloat expResult = null;
    PyFloat result = PyFloat.of(d);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of intValue method, of class PyFloat.
   */
  @Test
  public void testIntValue()
  {
    System.out.println("intValue");
    PyFloat instance = new PyFloat();
    int expResult = 0;
    int result = instance.intValue();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of longValue method, of class PyFloat.
   */
  @Test
  public void testLongValue()
  {
    System.out.println("longValue");
    PyFloat instance = new PyFloat();
    long expResult = 0L;
    long result = instance.longValue();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of floatValue method, of class PyFloat.
   */
  @Test
  public void testFloatValue()
  {
    System.out.println("floatValue");
    PyFloat instance = new PyFloat();
    float expResult = 0.0F;
    float result = instance.floatValue();
    assertEquals(result, expResult, 0.0);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of doubleValue method, of class PyFloat.
   */
  @Test
  public void testDoubleValue()
  {
    System.out.println("doubleValue");
    PyFloat instance = new PyFloat();
    double expResult = 0.0;
    double result = instance.doubleValue();
    assertEquals(result, expResult, 0.0);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

}
