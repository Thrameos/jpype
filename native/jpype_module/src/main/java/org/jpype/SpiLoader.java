// --- file: org/jpype/SpiLoader.java ---
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
package org.jpype;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ServiceLoader;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Drives eager SPI registration at interpreter startup.
 *
 * Discovers {@link WrapperService} providers via {@code ServiceLoader},
 * reads each provider's declared {@code .pyspi} resources
 * ({@link WrapperService#getEagerResources()}), and replays them into the
 * {@link Installer} (implemented by {@code _jbridge.py}). Called from
 * {@link MainInterpreter#setInstaller}. See {@code plan/SPI.md}.
 */
public final class SpiLoader
{

  private static final Logger LOGGER = Logger.getLogger(SpiLoader.class.getName());

  private SpiLoader()
  {
  }

  public static void load(Installer installer)
  {
    for (WrapperService service : ServiceLoader.load(WrapperService.class))
    {
      for (String resourcePath : service.getEagerResources())
      {
        SpiResource resource = SpiResource.parse(readResource(service.getClass(), resourcePath));
        if ("backend".equals(resource.kind))
          installer.registerBackend(resource.javaInterface, resource.body);
        else
          installer.registerClass(resource.module, resource.className, resource.javaInterface, resource.body);
        LOGGER.log(Level.FINE, "SPI registered {0} from {1}",
                new Object[]
                {
                  resource.javaInterface, resourcePath
                });
      }
    }
  }

  private static String readResource(Class<?> anchor, String path)
  {
    try (InputStream is = anchor.getResourceAsStream(path))
    {
      if (is == null)
        throw new IllegalStateException(
                "SPI resource not found: " + path + " (relative to " + anchor.getName() + ")");
      return new String(is.readAllBytes(), StandardCharsets.UTF_8);
    } catch (IOException ex)
    {
      throw new IllegalStateException("Failed to read SPI resource: " + path, ex);
    }
  }

}
