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
public class PyTupleNGTest
{

  public PyTupleNGTest()
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
   * Test of of method, of class PyTuple.
   */
  @Test
  public void testOf()
  {
    System.out.println("of");
    PyTuple result = PyTuple.of("A", "B", "C");
    assertEquals(result.size(), 3);
    assertEquals(result.get(0), "A");
    assertEquals(result.get(1), "B");
    assertEquals(result.get(2), "C");
  }

  /**
   * Test of ofRange method, of class PyTuple.
   */
  @Test
  public void testOfRange()
  {
    System.out.println("ofRange");
    PyTuple result = PyTuple.ofRange(new Object[]
    {
      "A", "B", "C"
    }, 0, 2);
    assertEquals(result.size(), 2);
    assertEquals(result.get(0), "A");
    assertEquals(result.get(1), "B");
  }

  /**
   * Test of size method, of class PyTuple.
   */
  @Test
  public void testSize()
  {
    System.out.println("size");
    PyTuple instance = PyTuple.of("A");
    assertEquals(instance.size(), 1);
    instance = PyTuple.of("A", "B");
    assertEquals(instance.size(), 2);
    instance = PyTuple.of("A", "B", "C");
    assertEquals(instance.size(), 3);
  }

  /**
   * Test of get method, of class PyTuple.
   */
  @Test
  public void testGet()
  {
    System.out.println("get");
    int i = 0;
    PyTuple instance = PyTuple.of("A", "B", "C");
    assertEquals(instance.get(0), "A");
    assertEquals(instance.get(1), "B");
    assertEquals(instance.get(2), "C");

    Object expResult = null;
    Object result = instance.get(i);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of getSlice method, of class PyTuple.
   */
  @Test
  public void testGetSlice()
  {
    System.out.println("getSlice");
    PyTuple instance = PyTuple.of("A", "B", "C");
    Object result = instance.getSlice(1, 3);

    // FIXME
  }

  /**
   * Test of setItem method, of class PyTuple.
   */
  @Test
  public void testSetItem()
  {
    System.out.println("setItem");
    PyTuple instance = PyTuple.of("A");
    try
    {
      instance.setItem(0, "A");
      fail("Must throw");
    } catch (UnsupportedOperationException a)
    {
    }
  }

}
