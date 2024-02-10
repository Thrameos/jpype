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

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
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
public class PyBuiltinsNGTest
{

  public PyBuiltinsNGTest()
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
   * Test of hasattr method, of class PyBuiltins.
   */
  @Test
  public void testHasattr()
  {
    System.out.println("hasattr");
    Object o = null;
    CharSequence attr = null;
    boolean expResult = false;
    boolean result = PyBuiltins.hasattr(o, attr);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of delattr method, of class PyBuiltins.
   */
  @Test
  public void testDelattr()
  {
    System.out.println("delattr");
    Object o = null;
    CharSequence attr = null;
    PyBuiltins.delattr(o, attr);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of getattr method, of class PyBuiltins.
   */
  @Test
  public void testGetattr()
  {
    System.out.println("getattr");
    Object o = null;
    CharSequence attr = null;
    Object expResult = null;
    Object result = PyBuiltins.getattr(o, attr);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of setattr method, of class PyBuiltins.
   */
  @Test
  public void testSetattr()
  {
    System.out.println("setattr");
    Object o = null;
    CharSequence attr = null;
    Object value = null;
    PyBuiltins.setattr(o, attr, value);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of not method, of class PyBuiltins.
   */
  @Test
  public void testNot()
  {
    System.out.println("not");
    PyObject o = null;
    boolean expResult = false;
    boolean result = PyBuiltins.not(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of isTrue method, of class PyBuiltins.
   */
  @Test
  public void testIsTrue()
  {
    System.out.println("isTrue");
    PyObject o = null;
    boolean expResult = false;
    boolean result = PyBuiltins.isTrue(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of eq method, of class PyBuiltins.
   */
  @Test
  public void testEq()
  {
    System.out.println("eq");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.eq(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of ne method, of class PyBuiltins.
   */
  @Test
  public void testNe()
  {
    System.out.println("ne");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.ne(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of gt method, of class PyBuiltins.
   */
  @Test
  public void testGt()
  {
    System.out.println("gt");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.gt(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of lt method, of class PyBuiltins.
   */
  @Test
  public void testLt()
  {
    System.out.println("lt");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.lt(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of ge method, of class PyBuiltins.
   */
  @Test
  public void testGe()
  {
    System.out.println("ge");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.ge(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of le method, of class PyBuiltins.
   */
  @Test
  public void testLe()
  {
    System.out.println("le");
    Object a = null;
    Object b = null;
    boolean expResult = false;
    boolean result = PyBuiltins.le(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of divmod method, of class PyBuiltins.
   */
  @Test
  public void testDivmod()
  {
    System.out.println("divmod");
    Object a = null;
    Object b = null;
    Object expResult = null;
    Object result = PyBuiltins.divmod(a, b);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of callable method, of class PyBuiltins.
   */
  @Test
  public void testCallable()
  {
    System.out.println("callable");
    Object o = null;
    boolean expResult = false;
    boolean result = PyBuiltins.callable(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of dir method, of class PyBuiltins.
   */
  @Test
  public void testDir()
  {
    System.out.println("dir");
    PyObject o = null;
    PyObject expResult = null;
    PyObject result = PyBuiltins.dir(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of hash method, of class PyBuiltins.
   */
  @Test
  public void testHash()
  {
    System.out.println("hash");
    Object o = null;
    long expResult = 0L;
    long result = PyBuiltins.hash(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of help method, of class PyBuiltins.
   */
  @Test
  public void testHelp()
  {
    System.out.println("help");
    Object o = null;
    Object expResult = null;
    Object result = PyBuiltins.help(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of isinstance method, of class PyBuiltins.
   */
  @Test
  public void testIsinstance()
  {
    System.out.println("isinstance");
    Object o = null;
    Object t = null;
    boolean expResult = false;
    boolean result = PyBuiltins.isinstance(o, t);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of issubclass method, of class PyBuiltins.
   */
  @Test
  public void testIssubclass()
  {
    System.out.println("issubclass");
    Object o = null;
    Object t = null;
    boolean expResult = false;
    boolean result = PyBuiltins.issubclass(o, t);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of ascii method, of class PyBuiltins.
   */
  @Test
  public void testAscii()
  {
    System.out.println("ascii");
    Object o = null;
    Object expResult = null;
    Object result = PyBuiltins.ascii(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of repr method, of class PyBuiltins.
   */
  @Test
  public void testRepr()
  {
    System.out.println("repr");
    Object o = null;
    PyString expResult = null;
    PyString result = PyBuiltins.repr(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of str method, of class PyBuiltins.
   */
  @Test
  public void testStr()
  {
    System.out.println("str");
    Object o = null;
    PyString expResult = null;
    PyString result = PyBuiltins.str(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of type method, of class PyBuiltins.
   */
  @Test
  public void testType()
  {
    System.out.println("type");
    PyObject o = null;
    PyType expResult = null;
    PyType result = PyBuiltins.type(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of id method, of class PyBuiltins.
   */
  @Test
  public void testId()
  {
    System.out.println("id");
    Object o = null;
    PyLong expResult = null;
    PyLong result = PyBuiltins.id(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of builtins method, of class PyBuiltins.
   */
  @Test
  public void testBuiltins()
  {
    System.out.println("builtins");
    PyDict expResult = null;
    PyDict result = PyBuiltins.builtins();
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of len method, of class PyBuiltins.
   */
  @Test
  public void testLen()
  {
    System.out.println("len");
    PyObject o = null;
    int expResult = 0;
    int result = PyBuiltins.len(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of lengthHint method, of class PyBuiltins.
   */
  @Test
  public void testLengthHint()
  {
    System.out.println("lengthHint");
    PyObject o = null;
    int defaultValue = 0;
    int expResult = 0;
    int result = PyBuiltins.lengthHint(o, defaultValue);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of getItem method, of class PyBuiltins.
   */
  @Test
  public void testGetItem()
  {
    System.out.println("getItem");
    PyObject o = null;
    Object key = null;
    Object expResult = null;
    Object result = PyBuiltins.getItem(o, key);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of setItem method, of class PyBuiltins.
   */
  @Test
  public void testSetItem()
  {
    System.out.println("setItem");
    PyObject o = null;
    Object key = null;
    Object v = null;
    PyBuiltins.setItem(o, key, v);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of delItem method, of class PyBuiltins.
   */
  @Test
  public void testDelItem()
  {
    System.out.println("delItem");
    PyObject o = null;
    Object key = null;
    PyBuiltins.delItem(o, key);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of iter method, of class PyBuiltins.
   */
  @Test
  public void testIter()
  {
    System.out.println("iter");
    PyObject o = null;
    Iterator expResult = null;
    Iterator result = PyBuiltins.iter(o);
    assertEquals(result, expResult);
    // TODO review the generated test code and remove the default call to fail.
    fail("The test case is a prototype.");
  }

  /**
   * Test of bool method, of class PyBuiltins.
   */
  @Test
  public void testBool()
  {
    System.out.println("bool");
    boolean result = PyBuiltins.bool(new PyLong(0));
    assertEquals(result, false);
    result = PyBuiltins.bool(new PyLong(1));
    assertEquals(result, true);
  }

  /**
   * Test of bytes method, of class PyBuiltins.
   */
  @Test
  public void testBytes_ByteBuffer()
  {
    System.out.println("bytes");
    Object result = PyBuiltins.bytes(ByteBuffer.wrap(new byte[]
    {
      1, 2, 3
    }));
  }

  /**
   * Test of bytes method, of class PyBuiltins.
   */
  @Test
  public void testBytes_byteArr()
  {
    System.out.println("bytes");
    Object result = PyBuiltins.bytes(new byte[]
    {
      1, 2, 3
    });
  }

  /**
   * Test of bytearray method, of class PyBuiltins.
   */
  @Test
  public void testBytearray()
  {
    System.out.println("bytearray");
    Object result = PyBuiltins.bytearray(ByteBuffer.wrap(new byte[]
    {
      1, 2, 3
    }));
  }

  /**
   * Test of complex method, of class PyBuiltins.
   */
  @Test
  public void testComplex()
  {
    System.out.println("complex");
    Object result = PyBuiltins.complex(1, 2);
    assertEquals(result, null);
  }

  /**
   * Test of dict method, of class PyBuiltins.
   */
  @Test
  public void testDict()
  {
    System.out.println("dict");
    PyDict result = PyBuiltins.dict();
    assertEquals(result.size(), 0);
  }

  /**
   * Test of frozenset method, of class PyBuiltins.
   */
  @Test
  public void testFrozenset()
  {
    System.out.println("frozenset");
    Iterable s = Arrays.asList("A", "B", "C");
    PySet result = PyBuiltins.frozenset(s);
    // FIXME
  }

  /**
   * Test of list method, of class PyBuiltins.
   */
  @Test
  public void testList()
  {
    System.out.println("list");
    Collection e = Arrays.asList("A", "B", "C");
    PyList result = PyBuiltins.list(e);
    assertEquals(result.get(0), "A");
    assertEquals(result.get(1), "B");
    assertEquals(result.get(2), "C");
  }

  /**
   * Test of memoryview method, of class PyBuiltins.
   */
  @Test
  public void testMemoryview()
  {
    System.out.println("memoryview");
    Object o = new PyString("ABC");
    PyMemoryView result = PyBuiltins.memoryview(o);
    // FIXME
  }

  /**
   * Test of set method, of class PyBuiltins.
   */
  @Test
  public void testSet_Iterable()
  {
    System.out.println("set");
    Iterable s = Arrays.asList("A", "B", "A");
    PySet result = PyBuiltins.set(s);
    assertEquals(result.size(), 2);
  }

  /**
   * Test of set method, of class PyBuiltins.
   */
  @Test
  public void testSet_GenericType()
  {
    System.out.println("set");
    PySet result = PyBuiltins.set("A", "B", "A");
    assertEquals(result.size(), 2);
  }

  /**
   * Test of slice method, of class PyBuiltins.
   */
  @Test
  public void testSlice_Object_Object()
  {
    PySlice result = PyBuiltins.slice(0, 10);
    // FIXME
  }

  /**
   * Test of slice method, of class PyBuiltins.
   */
  @Test
  public void testSlice_3args()
  {
    PySlice result = PyBuiltins.slice(0, 10, 2);
    // FIXME
  }

  /**
   * Test of tuple method, of class PyBuiltins.
   */
  @Test
  public void testTuple()
  {
    System.out.println("tuple");
    PyTuple result = PyBuiltins.tuple("A", "B", "C");
    assertEquals(result, PyTuple.of("A", "B", "C"));
  }

  /**
   * Test of object method, of class PyBuiltins.
   */
  @Test
  public void testObject()
  {
    System.out.println("object");
    Object result = PyBuiltins.object();
    assertNotEquals(result, null);
  }

}
