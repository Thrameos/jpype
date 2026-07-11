// --- file: python/io/PyIOWriter.java ---
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

import java.io.Writer;

/**
 * Adapts a {@link PyTextIOBase} to {@link java.io.Writer}, backing
 * {@link PyTextIOBase#asWriter()}.
 *
 * Each {@code write(char[], int, int)} builds a fresh Java {@code String}
 * from the given range and hands it to a single
 * {@link PyTextIOBase#write(CharSequence)} call; the single-char
 * {@code write(int)} makes its own call per character, so prefer the bulk
 * form for large transfers.
 */
final class PyIOWriter extends Writer
{

  private final PyTextIOBase stream;

  PyIOWriter(PyTextIOBase stream)
  {
    this.stream = stream;
  }

  @Override
  public void write(char[] cbuf, int off, int len)
  {
    stream.write(new String(cbuf, off, len));
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
