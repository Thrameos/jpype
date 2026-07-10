// --- file: python/io/PyStringIONGTest.java ---
package python.io;

import static org.testng.Assert.*;
import org.testng.annotations.Test;
import python.lang.PyString;
import python.lang.PyTestHarness;

public class PyStringIONGTest extends PyTestHarness
{

  @Test
  public void testCreateEmpty()
  {
    PyStringIO instance = IO.instance().stringIO();

    assertNotNull(instance);
    assertFalse(instance.closed());
  }

  @Test
  public void testWriteAndGetvalue()
  {
    PyStringIO instance = IO.instance().stringIO();

    int written = instance.write("hello");

    assertEquals(written, 5);
    PyString value = instance.getvalue();
    assertEquals(value.toString(), "hello");
  }

  @Test
  public void testCreateWithInitialString()
  {
    PyStringIO instance = IO.instance().stringIO("abc");

    PyString value = instance.getvalue();
    assertEquals(value.toString(), "abc");
  }

  @Test
  public void testReadAll()
  {
    PyStringIO instance = IO.instance().stringIO("abcdef");

    PyString read = instance.read();

    assertEquals(read.toString(), "abcdef");
  }

  @Test
  public void testReadWithSize()
  {
    PyStringIO instance = IO.instance().stringIO("abcdef");

    PyString read = instance.read(3);

    assertEquals(read.toString(), "abc");
  }

  @Test
  public void testReadline()
  {
    PyStringIO instance = IO.instance().stringIO("first\nsecond\n");

    PyString line = instance.readline();

    assertEquals(line.toString(), "first\n");
  }

  @Test
  public void testSeekAndTell()
  {
    PyStringIO instance = IO.instance().stringIO("abcdef");

    instance.seek(2);

    assertEquals(instance.tell(), 2L);
    assertEquals(instance.read().toString(), "cdef");
  }

  @Test
  public void testClose()
  {
    PyStringIO instance = IO.instance().stringIO();

    instance.close();

    assertTrue(instance.closed());
  }
}
