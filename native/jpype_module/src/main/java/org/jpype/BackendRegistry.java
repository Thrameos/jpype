// --- file: org/jpype/BackendRegistry.java ---
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

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Registry for SPI-provided "mini-backends".
 *
 * {@link Backend} is a single, fixed, shared interface — an SPI provider
 * (e.g. {@code python.io}) cannot add methods to it. Instead, a provider
 * declares its own small backend-shaped interface for whatever
 * module-level hooks it needs (construction, mainly), and {@code _jbridge.py}
 * builds that interface's proxy (bound to the provider's own Python-side
 * dispatch dict) and registers the instance here at init time. The
 * provider's own interface then exposes a static accessor
 * ({@code IO.instance()}) that looks itself up, rather than routing through
 * {@code Backend}.
 *
 * See {@code plan/SPI.md}.
 */
public final class BackendRegistry
{

  private static final Map<Class<?>, Object> REGISTRY = new ConcurrentHashMap<>();

  private BackendRegistry()
  {
  }

  public static <T> void register(Class<T> iface, T instance)
  {
    REGISTRY.put(iface, instance);
  }

  public static <T> T get(Class<T> iface)
  {
    Object instance = REGISTRY.get(iface);
    if (instance == null)
      throw new IllegalStateException("No backend registered for " + iface.getName());
    return iface.cast(instance);
  }

}
