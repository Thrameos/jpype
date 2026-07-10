// --- file: python/io/PyIOInputStream.java ---
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

import java.io.InputStream;
import python.lang.PyBytes;

/**
 * Adapts a {@link PyBufferedIOBase} to {@link java.io.InputStream}, backing
 * {@link PyBufferedIOBase#asInputStream()}.
 *
 * Bytes are round-tripped one Python call at a time via
 * {@code PyBytes.get(int)}; this is not tuned for throughput — it exists so
 * a {@code python.io} object can stand in wherever Java code expects a real
 * {@code InputStream} (see {@code plan/IO.md}).
 */
final class PyIOInputStream extends InputStream
{

  private final PyBufferedIOBase stream;

  PyIOInputStream(PyBufferedIOBase stream)
  {
    this.stream = stream;
  }

  @Override
  public int read()
  {
    PyBytes chunk = stream.read(1);
    if (chunk.size() == 0)
      return -1;
    return (int) chunk.builtin().asLong(chunk.get(0)) & 0xFF;
  }

  @Override
  public int read(byte[] b, int off, int len)
  {
    if (len == 0)
      return 0;
    PyBytes chunk = stream.read(len);
    int n = chunk.size();
    if (n == 0)
      return -1;
    for (int i = 0; i < n; i++)
      b[off + i] = (byte) (chunk.builtin().asLong(chunk.get(i)) & 0xFF);
    return n;
  }

  @Override
  public void close()
  {
    stream.close();
  }

}
