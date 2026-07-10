// --- file: python/io/PyIOOutputStream.java ---
/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 *
 *  See NOTICE file for details.
 */
package python.io;

import java.util.Arrays;
import python.lang.PyBytes;

/**
 * Adapts a {@link PyBufferedIOBase} to {@link java.io.OutputStream}, backing
 * {@link PyBufferedIOBase#asOutputStream()}.
 *
 * Each {@code write(byte[], int, int)} builds a fresh Python {@code bytes}
 * object from the given range and hands it to a single
 * {@link PyBufferedIOBase#write(python.lang.PyBuffer)} call; the
 * single-byte {@code write(int)} makes its own call per byte, so prefer the
 * bulk form for large transfers.
 */
final class PyIOOutputStream extends java.io.OutputStream
{

  private final PyBufferedIOBase stream;

  PyIOOutputStream(PyBufferedIOBase stream)
  {
    this.stream = stream;
  }

  @Override
  public void write(int b)
  {
    write(new byte[]
    {
      (byte) b
    }, 0, 1);
  }

  @Override
  public void write(byte[] b, int off, int len)
  {
    byte[] chunk = (off == 0 && len == b.length) ? b : Arrays.copyOfRange(b, off, off + len);
    PyBytes bytes = stream.builtin().bytes(chunk);
    stream.write(bytes);
  }

  @Override
  public void flush()
  {
    stream.flush();
  }

  @Override
  public void close()
  {
    stream.close();
  }

}
