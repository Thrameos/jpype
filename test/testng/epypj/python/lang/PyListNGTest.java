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

import org.jpype.python.Engine;
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
public class PyListNGTest
{

  public PyListNGTest()
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
   * Test of get method, of class PyList.
   */
  @Test
  public void testGet()
  {
    System.out.println("get");
    PyList instance = new PyList();
    instance.add("A");
    instance.add("B");
    assertEquals(instance.get(0), "A");
    assertEquals(instance.get(1), "B");
  }

  /**
   * Test of setItem method, of class PyList.
   */
  @Test
  public void testSetItem()
  {
    System.out.println("setItem");
    PyList instance = new PyList();
    instance.add("A");
    instance.add("B");
    assertEquals(instance.get(0), "A");
    assertEquals(instance.get(1), "B");
    instance.setItem(0, "C");
    assertEquals(instance.get(0), "C");
  }

  /**
   * Test of insert method, of class PyList.
   */
  @Test
  public void testInsert()
  {
    System.out.println("insert");
    PyList instance = new PyList();
    instance.add("A");
    instance.add("B");
    assertEquals(instance.get(0), "A");
    assertEquals(instance.get(1), "B");
    instance.insert(0, "C");
    assertEquals(instance.get(0), "C");
    assertEquals(instance.get(1), "A");
  }

  /**
   * Test of append method, of class PyList.
   */
  @Test
  public void testAppend()
  {
    System.out.println("append");
    PyList instance = new PyList();
    instance.append("A");
    instance.append("B");
    assertEquals(instance.get(0), "A");
    assertEquals(instance.get(1), "B");
  }

  /**
   * Test of getSlice method, of class PyList.
   */
  @Test
  public void testGetSlice()
  {
    System.out.println("getSlice");
    PyList instance = new PyList();
    instance.append("A");
    instance.append("B");
    instance.getSlice(0, 1);
  }

  /**
   * Test of setSlice method, of class PyList.
   */
  @Test
  public void testSetSlice()
  {
    System.out.println("setSlice");
    int low = 0;
    int high = 0;
    Object item = null;
    PyList instance = new PyList();
    instance.setSlice(low, high, item);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of sort method, of class PyList.
   */
  @Test
  public void testSort()
  {
    System.out.println("sort");
    PyList instance = new PyList();
    instance.sort();
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of reverse method, of class PyList.
   */
  @Test
  public void testReverse()
  {
    System.out.println("reverse");
    PyList instance = new PyList();
    instance.reverse();
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of asTuple method, of class PyList.
   */
  @Test
  public void testAsTuple()
  {
    System.out.println("asTuple");
    PyList instance = new PyList();
    PyTuple expResult = null;
    PyTuple result = instance.asTuple();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

}
