// --- file: python/io/PyTextIOBase.java ---
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
import java.io.Writer;
import python.lang.PyString;

/**
 * Java front-end interface for Python's {@code io.TextIOBase} — text
 * streams (Python {@code str} in and out), e.g. {@link PyStringIO}.
 */
public interface PyTextIOBase extends PyIOBase
{

  /**
   * Reads and returns up to {@code size} characters. If {@code size} is
   * negative or omitted, reads until EOF, matching
   * {@code io.TextIOBase.read}.
   *
   * @param size the maximum number of characters to read, or a negative
   * value to read until EOF.
   * @return the text read; empty (not {@code null}) at EOF.
   */
  PyString read(int size);

  /**
   * Equivalent to {@code read(-1)}: reads until EOF.
   *
   * @return the text read; empty (not {@code null}) at EOF.
   */
  PyString read();

  /**
   * Reads a single line, keeping the trailing newline if present; empty
   * string at EOF.
   *
   * @return the line read.
   */
  PyString readline();

  /**
   * Writes {@code text} to the stream.
   *
   * @param text the text to write.
   * @return the number of characters written.
   */
  int write(CharSequence text);

  /**
   * Wraps this stream as a standard {@link java.io.Reader}, so it can be
   * passed to Java APIs that expect one. Each single-char {@code read()}
   * call makes its own call into {@link #read(int)}; for large transfers,
   * prefer {@code read(char[], int, int)} on the returned reader, which
   * forwards directly to a single {@link #read(int)} call sized to the
   * request.
   *
   * @return a {@code Reader} view of this stream.
   */
  default Reader asReader()
  {
    return new PyIOReader(this);
  }

  /**
   * Wraps this stream as a standard {@link java.io.Writer}, so it can be
   * passed to Java APIs that expect one. Each single-char {@code write()}
   * call makes its own call into {@link #write(CharSequence)}; for large
   * transfers, prefer {@code write(char[], int, int)} on the returned
   * writer, which forwards directly to a single {@link #write(CharSequence)}
   * call sized to the request.
   *
   * @return a {@code Writer} view of this stream.
   */
  default Writer asWriter()
  {
    return new PyIOWriter(this);
  }

}
