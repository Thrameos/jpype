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
public class PyKeywordsNGTest
{

  public PyKeywordsNGTest()
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
   * Test of use method, of class PyKeywords.
   */
  @Test
  public void testUse()
  {
    System.out.println("use");
    PyDict dict = null;
    PyKeywords expResult = null;
    PyKeywords result = PyKeywords.use(dict);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of of method, of class PyKeywords.
   */
  @Test
  public void testOf()
  {
    System.out.println("of");
    Object[] contents = null;
    PyKeywords expResult = null;
    PyKeywords result = PyKeywords.of(contents);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of put method, of class PyKeywords.
   */
  @Test
  public void testPut()
  {
    System.out.println("put");
    CharSequence key = null;
    Object value = null;
    PyKeywords instance = new PyKeywords();
    PyKeywords expResult = null;
    PyKeywords result = instance.put(key, value);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of toDict method, of class PyKeywords.
   */
  @Test
  public void testToDict()
  {
    System.out.println("toDict");
    PyKeywords instance = new PyKeywords();
    PyDict expResult = null;
    PyDict result = instance.toDict();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

}
