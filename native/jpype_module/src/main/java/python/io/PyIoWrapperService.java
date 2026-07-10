// --- file: python/io/PyIoWrapperService.java ---
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

import java.util.HashMap;
import java.util.Map;
import org.jpype.WrapperService;

/**
 * SPI provider mapping Python's {@code io}/{@code _io} classes onto the
 * {@code python.io} interfaces.
 *
 * This is the first real, non-hardcoded {@link WrapperService}: it is the
 * worked example for how a third-party package (numpy-shaped or otherwise)
 * would expose its own types, per {@code plan/SPI.md}/{@code plan/IO.md}.
 * The manifest below was drafted from the real {@code io} module using
 * {@code plan/tools/extract_module_shape.py} rather than hand-enumerated,
 * and deliberately covers two module names — {@code "io"} (the public
 * facade, where the abstract base classes report their {@code __module__})
 * and {@code "_io"} (the C accelerator module, where every concrete class
 * actually lives) — a split every provider that wraps a C-extension-backed
 * package will hit.
 *
 * Only {@code BytesIO}/{@code StringIO} and their abstract bases are
 * mapped in this first cut; the remaining classes from the extraction
 * script's output ({@code FileIO}, {@code BufferedReader/Writer/Random},
 * {@code TextIOWrapper}, {@code BufferedRWPair}) are deferred to the next
 * implementation step in {@code plan/IO.md}.
 *
 * <b>Not yet wired into {@code _jbridge.py}.</b> The lazy runtime hook
 * ({@code _jpype._cache.__missing__}, see {@code plan/SPI.md}) that would
 * consult this service on a cache miss does not exist yet; this class is
 * the drafted shape of the provider side only. Today, {@code io}'s classes
 * are registered eagerly and directly in {@code _jbridge.py} as a stand-in
 * — see the note there.
 */
public final class PyIoWrapperService implements WrapperService
{

  private static final Map<String, Class<?>[]> MANIFEST = new HashMap<>();

  static
  {
    // module="io" (public facade; abstract bases report __module__ here)
    MANIFEST.put("io.IOBase", new Class<?>[]
    {
      PyIOBase.class
    });
    MANIFEST.put("io.BufferedIOBase", new Class<?>[]
    {
      PyBufferedIOBase.class
    });
    MANIFEST.put("io.TextIOBase", new Class<?>[]
    {
      PyTextIOBase.class
    });

    // module="_io" (C accelerator; every concrete class lives here)
    MANIFEST.put("_io.BytesIO", new Class<?>[]
    {
      PyBytesIO.class
    });
    MANIFEST.put("_io.StringIO", new Class<?>[]
    {
      PyStringIO.class
    });
  }

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
  public Class<?>[] getInterfaces(String clsName)
  {
    return MANIFEST.get(clsName);
  }

}
