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

import java.util.ArrayList;
import java.util.Collection;
import static org.testng.Assert.*;
import org.testng.annotations.AfterClass;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;
import python.lang.protocol.PyIterator;

/**
 *
 * @author nelson85
 */
public class PySetNGTest
{

  public PySetNGTest()
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
   * Test of iterator method, of class PySet.
   */
  @Test
  public void testIterator()
  {
    System.out.println("iterator");
    PySet instance = new PySet();
    PyIterator expResult = null;
    PyIterator result = instance.iterator();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of size method, of class PySet.
   */
  @Test
  public void testSize()
  {
    System.out.println("size");
    PySet instance = new PySet();
    int expResult = 0;
    int result = instance.size();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of isEmpty method, of class PySet.
   */
  @Test
  public void testIsEmpty()
  {
    System.out.println("isEmpty");
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.isEmpty();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of contains method, of class PySet.
   */
  @Test
  public void testContains()
  {
    System.out.println("contains");
    Object key = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.contains(key);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of add method, of class PySet.
   */
  @Test
  public void testAdd()
  {
    System.out.println("add");
    Object key = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.add(key);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of addItem method, of class PySet.
   */
  @Test
  public void testAddItem()
  {
    System.out.println("addItem");
    Object key = null;
    PySet instance = new PySet();
    instance.addItem(key);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of remove method, of class PySet.
   */
  @Test
  public void testRemove()
  {
    System.out.println("remove");
    Object key = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.remove(key);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of pop method, of class PySet.
   */
  @Test
  public void testPop()
  {
    System.out.println("pop");
    PySet instance = new PySet();
    Object expResult = null;
    Object result = instance.pop();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of clear method, of class PySet.
   */
  @Test
  public void testClear()
  {
    System.out.println("clear");
    PySet instance = new PySet();
    instance.clear();
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of toArray method, of class PySet.
   */
  @Test
  public void testToArray_0args()
  {
    System.out.println("toArray");
    PySet instance = new PySet();
    Object[] expResult = null;
    Object[] result = instance.toArray();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of toArray method, of class PySet.
   */
  @Test
  public void testToArray_GenericType()
  {
    System.out.println("toArray");
    PySet instance = new PySet();
    Object[] expResult = null;
    Object[] result = instance.toArray(new Object[0]);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of containsAll method, of class PySet.
   */
  @Test
  public void testContainsAll()
  {
    System.out.println("containsAll");
    Collection<?> c = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.containsAll(c);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of addAll method, of class PySet.
   */
  @Test
  public void testAddAll()
  {
    System.out.println("addAll");
    Collection<Object> c = new ArrayList<>();
    c.add("A");
    c.add("B");
    c.add("A");
    PySet instance = new PySet();
    boolean result = instance.addAll(c);
    assertEquals(result, true);
    assertEquals(instance.size(), 2);
    assertTrue(instance.contains("A"));
    assertTrue(instance.contains("B"));
  }

  /**
   * Test of retainAll method, of class PySet.
   */
  @Test
  public void testRetainAll()
  {
    System.out.println("retainAll");
    Collection<?> c = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.retainAll(c);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of removeAll method, of class PySet.
   */
  @Test
  public void testRemoveAll()
  {
    System.out.println("removeAll");
    Collection<?> c = null;
    PySet instance = new PySet();
    boolean expResult = false;
    boolean result = instance.removeAll(c);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

}
