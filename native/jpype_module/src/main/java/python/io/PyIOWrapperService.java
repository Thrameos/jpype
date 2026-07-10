// --- file: python/io/PyIOWrapperService.java ---
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

import org.jpype.SpiLoader;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code io}/{@code _io} classes onto the
 * {@code python.io} interfaces.
 *
 * This is the first real, non-hardcoded {@link WrapperService}: it is the
 * worked example for how a third-party package (numpy-shaped or otherwise)
 * would expose its own types, per {@code plan/SPI.md}/{@code plan/IO.md}.
 * It deliberately covers two module names — {@code "io"} (the public
 * facade, where the abstract base classes report their {@code __module__})
 * and {@code "_io"} (the C accelerator module, where every concrete class
 * actually lives) — a split every provider that wraps a C-extension-backed
 * package will hit.
 *
 * Only {@code BytesIO}/{@code StringIO} and their abstract bases are
 * mapped in this first cut; the remaining classes ({@code FileIO},
 * {@code BufferedReader/Writer/Random}, {@code TextIOWrapper},
 * {@code BufferedRWPair}) are deferred to the next implementation step in
 * {@code plan/IO.md}.
 *
 * {@link #getResources()} just scans this provider's own resource
 * directory ({@link SpiLoader#listPyspiResources}) — adding a class means
 * dropping a new {@code .pyspi} file under {@code python/io/spi/}, nothing
 * else here needs to change. Each resource declares its own eager/lazy
 * behavior in its header: {@code IOBase}/{@code BufferedIOBase}/
 * {@code TextIOBase}/{@code BytesIO} and the {@code IO} mini-backend are
 * eager (replayed immediately at startup); {@code StringIO} has
 * {@code lazy: true} — only imported/registered the first time a
 * {@code StringIO} instance actually crosses into Java, via the
 * {@code _jpype._cache} probe-miss hook (see {@code plan/SPI.md}).
 */
public final class PyIOWrapperService implements WrapperService
{

  @Override
  public String[] getModuleNames()
  {
    return new String[]
    {
      "io", "_io"
    };
  }

  @Override
  public String getVersion()
  {
    return "1";
  }

  @Override
  public Iterable<String> getResources()
  {
    return SpiLoader.listPyspiResources(PyIOWrapperService.class, "/python/io/spi");
  }

}
