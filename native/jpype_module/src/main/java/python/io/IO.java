// --- file: python/io/IO.java ---
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

import org.jpype.MainInterpreter;
import python.lang.PyBuffer;

/**
 * Module-level hooks for {@code python.io} — the mini-backend for this
 * package, playing the same role {@link org.jpype.Backend} plays for
 * {@code python.lang}, scoped to this SPI provider.
 *
 * {@code _jbridge.py} builds this interface's proxy (bound to its own
 * Python-side dispatch dict, independent from {@code Backend}'s) and
 * registers it on the current interpreter's {@code NativeContext} at init
 * time (scoped per-interpreter, not a JVM-wide static — see
 * {@code NativeContext#registerBackend}) — this is the user-facing entry
 * point for constructing {@code python.io} objects; there is deliberately no
 * {@code python.lang.PyBuiltIn} equivalent, since core {@code python.lang}
 * should not need to know about any given SPI provider.
 *
 * {@link #instance()} is a convenience for when no live {@code PyObject} is
 * already in hand (it still bottoms out at the single {@code MainInterpreter}
 * singleton, same as {@link org.jpype.MainInterpreter#getBackend()} /
 * {@code getInstaller()} elsewhere). Given any existing {@code PyObject},
 * prefer {@code obj.builtin().getBackend(IO.class)} instead — that reaches
 * the same per-interpreter registry through the same no-statics path
 * {@link python.lang.PyObject#builtin()} already uses (each proxy carries
 * its own interpreter-bound {@code PyBuiltIn}), so it works without ever
 * touching {@code MainInterpreter}.
 */
public interface IO
{

  static IO instance()
  {
    return MainInterpreter.getInstance().getBuiltIn().getBackend(IO.class);
  }

  /**
   * Creates a new, empty {@code io.BytesIO}.
   *
   * @return a new {@link PyBytesIO} instance.
   */
  PyBytesIO bytesIO();

  /**
   * Creates a new {@code io.BytesIO} pre-populated with the contents of
   * {@code initial}.
   *
   * @param initial the initial contents of the stream.
   * @return a new {@link PyBytesIO} instance.
   */
  PyBytesIO bytesIO(PyBuffer initial);

  /**
   * Creates a new, empty {@code io.StringIO}.
   *
   * @return a new {@link PyStringIO} instance.
   */
  PyStringIO stringIO();

  /**
   * Creates a new {@code io.StringIO} pre-populated with {@code initial}.
   *
   * @param initial the initial contents of the stream.
   * @return a new {@link PyStringIO} instance.
   */
  PyStringIO stringIO(CharSequence initial);

}
