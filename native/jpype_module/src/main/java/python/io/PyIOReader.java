// --- file: python/io/PyIOReader.java ---
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

import java.io.Reader;
import python.lang.PyString;

/**
 * Adapts a {@link PyTextIOBase} to {@link java.io.Reader}, backing
 * {@link PyTextIOBase#asReader()}.
 *
 * Lets a {@code python.io} text object stand in wherever Java code expects a
 * real {@code Reader}. {@code read(char[], int, int)} forwards directly to a
 * single {@link PyTextIOBase#read(int)} call; the single-char {@code read()}
 * makes its own call per character, so prefer the bulk form for large
 * transfers.
 */
final class PyIOReader extends Reader
{

  private final PyTextIOBase stream;

  PyIOReader(PyTextIOBase stream)
  {
    this.stream = stream;
  }

  @Override
  public int read()
  {
    PyString chunk = stream.read(1);
    if (chunk.length() == 0)
      return -1;
    return chunk.charAt(0);
  }

  @Override
  public int read(char[] cbuf, int off, int len)
  {
    if (len == 0)
      return 0;
    PyString chunk = stream.read(len);
    int n = chunk.length();
    if (n == 0)
      return -1;
    chunk.toString().getChars(0, n, cbuf, off);
    return n;
  }

  @Override
  public void close()
  {
    stream.close();
  }

}
