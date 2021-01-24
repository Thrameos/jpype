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

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.jpype.python.Engine;
import org.jpype.python.Scope;
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
public class PyDictNGTest
{

  private static Scope scope;

  public PyDictNGTest()
  {
  }

  @BeforeClass
  public static void setUpClass() throws Exception
  {
    Engine engine = PythonTest.getEngine();
    scope = engine.newScope();
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
   * Test of asReadOnly method, of class PyDict.
   */
  @Test
  public void testAsReadOnly()
  {
    System.out.println("asReadOnly");
    PyDict instance = new PyDict();
    // This should succeed
    instance.put("A", "B");

    // This should fail
    PyDict result = instance.asReadOnly();
    result.put("A", "B");
  }

  /**
   * Test of clear method, of class PyDict.
   */
  @Test
  public void testClear()
  {
    System.out.println("clear");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    assertEquals(instance.size(), 2);
    instance.clear();
    assertEquals(instance.size(), 0);
  }

  /**
   * Test of containsKey method, of class PyDict.
   */
  @Test
  public void testContainsKey()
  {
    System.out.println("containsKey");
    Object key = null;
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    assertEquals(instance.containsKey("C"), false);
    assertEquals(instance.containsKey("A"), true);
  }

  /**
   * Test of containsValue method, of class PyDict.
   */
  @Test
  public void testContainsValue()
  {
    System.out.println("containsValue");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    assertEquals(instance.containsValue(3), false);
    assertEquals(instance.containsKey(1), true);
  }

  /**
   * Test of clone method, of class PyDict.
   */
  @Test
  public void testClone() throws Exception
  {
    System.out.println("clone");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    PyDict result = instance.clone();
    assertFalse(result == instance);
    assertTrue(result.equals(instance));
  }

  /**
   * Test of put method, of class PyDict.
   */
  @Test
  public void testPut()
  {
    System.out.println("put");
    PyDict instance = new PyDict();
    assertEquals(instance.put("A", 1), null);
    assertEquals(instance.put("A", 2), 1);
  }

  /**
   * Test of setItem method, of class PyDict.
   */
  @Test
  public void testSetItem()
  {
    System.out.println("setItem");
    PyDict instance = new PyDict();
    instance.setItem("A", 1);
    instance.setItem(2, 2);
  }

  /**
   * Test of delItem method, of class PyDict.
   */
  @Test
  public void testDelItem()
  {
    System.out.println("delItem");
    Object key = null;
    PyDict instance = new PyDict();
    instance.setItem("A", 1);
    assertTrue(instance.containsKey("A"));
    instance.delItem("A");
    assertFalse(instance.containsKey("A"));
  }

  /**
   * Test of remove method, of class PyDict.
   */
  @Test
  public void testRemove()
  {
    System.out.println("remove");
    Object key = null;
    PyDict instance = new PyDict();
    instance.setItem("A", 1);
    assertTrue(instance.containsKey("A"));
    assertEquals(instance.remove("A"), 1);
    assertFalse(instance.containsKey("A"));
  }

  /**
   * Test of get method, of class PyDict.
   */
  @Test
  public void testGet()
  {
    System.out.println("get");
    PyDict instance = new PyDict();
    instance.setItem("A", 1);
    assertEquals(instance.get("A"), 1);
    assertEquals(instance.get("B"), null);
  }

  /**
   * Test of getItemWithError method, of class PyDict.
   */
  @Test
  public void testGetItemWithError()
  {
    System.out.println("getItemWithError");
    Object key = null;
    PyDict instance = new PyDict();
    instance.setItem("A", 1);
    assertEquals(instance.getItemWithError("A"), 1);
    assertEquals(instance.getItemWithError("B"), null);
  }

  /**
   * Test of setDefault method, of class PyDict.
   */
  @Test
  public void testSetDefault()
  {
    System.out.println("setDefault");
    Object key = null;
    Object defaultobj = null;
    PyDict instance = new PyDict();
    Object expResult = null;
    Object result = instance.setDefault(key, defaultobj);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of items method, of class PyDict.
   */
  @Test
  public void testItems()
  {
    System.out.println("items");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 1);
    PyList result = instance.items();
    assertEquals(result.size(), 2);
  }

  /**
   * Test of entrySet method, of class PyDict.
   */
  @Test
  public void testEntrySet()
  {
    System.out.println("entrySet");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    Set<Map.Entry> result = instance.entrySet();
    Map.Entry first = result.iterator().next();
    assertEquals(first.getKey(), "A");
    assertEquals(first.getValue(), 1);
  }

  /**
   * Test of keys method, of class PyDict.
   */
  @Test
  public void testKeys()
  {
    System.out.println("keys");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    PyList result = instance.keys();
    assertEquals(result.size(), 2);
    String[] expected =
    {
      "A", "B"
    };
    for (int i = 0; i < 2; i++)
      assertEquals(result.get(i), expected[i]);
  }

  /**
   * Test of keySet method, of class PyDict.
   */
  @Test
  public void testKeySet()
  {
    System.out.println("keySet");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    Set result = instance.keySet();
    String[] expected =
    {
      "A", "B"
    };
    for (int i = 0; i < 2; i++)
      assertTrue(result.contains(expected[i]));
  }

  /**
   * Test of values method, of class PyDict.
   */
  @Test
  public void testValues()
  {
    System.out.println("values");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    PyList result = instance.values();
    Object[] expected =
    {
      1, 2
    };
    for (int i = 0; i < 2; i++)
      assertEquals(result.get(i), expected[i]);
  }

  /**
   * Test of size method, of class PyDict.
   */
  @Test
  public void testSize()
  {
    System.out.println("size");
    PyDict instance = new PyDict();
    instance.put("A", 1);
    instance.put("B", 2);
    assertEquals(instance.size(), 2);
  }

  /**
   * Test of merge method, of class PyDict.
   */
  @Test
  public void testMerge()
  {
    System.out.println("merge");
    boolean override = false;
    PyDict instance = new PyDict();
        PyDict dict = new PyDict();
    dict.put("A", 1);
    dict.put("B", 2);
    instance.merge(dict, override);
    PyList result = instance.keys();
    String[] expected =
    {
      "A", "B"
    };
    for (int i = 0; i < 2; i++)
      assertTrue(result.contains(expected[i]));
  }

  /**
   * Test of putAll method, of class PyDict.
   */
  @Test
  public void testPutAll()
  {
    System.out.println("putAll");
    Map dict = new HashMap<>();
    PyDict instance = new PyDict();
    dict.put("A", 1);
    dict.put("B", 2);
    instance.putAll(dict);
    PyList result = instance.keys();
    String[] expected =
    {
      "A", "B"
    };
    for (int i = 0; i < 2; i++)
      assertTrue(result.contains(expected[i]));
  }

  /**
   * Test of update method, of class PyDict.
   */
  @Test
  public void testUpdate()
  {
    System.out.println("update");
//    Object b = null;
//    PyDict instance = new PyDict();
//    instance.update(b);
  }

  /**
   * Test of mergeFromSeq2 method, of class PyDict.
   */
  @Test
  public void testMergeFromSeq2()
  {
    System.out.println("mergeFromSeq2");
//    Object seq2 = null;
//    boolean override = false;
//    PyDict instance = new PyDict();
//    instance.mergeFromSeq2(seq2, override);
//    // TODO review the generated test code and remove the default call to fail.
//    fail("The test case is a prototype.");
  }

  /**
   * Test of isEmpty method, of class PyDict.
   */
  @Test
  public void testIsEmpty()
  {
    System.out.println("isEmpty");
    PyDict instance = new PyDict();
    assertTrue(instance.isEmpty());
    instance.put("A", 1);
    assertFalse(instance.isEmpty());
  }

}
