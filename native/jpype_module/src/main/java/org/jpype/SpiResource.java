// --- file: org/jpype/SpiResource.java ---
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

import java.util.HashMap;
import java.util.Map;

/**
 * Parses one {@code .pyspi} resource: a small {@code key: value} header, a
 * line containing only {@code ---}, then a blob of Python source. One
 * resource per Python class (or per mini-backend). See {@code plan/SPI.md}.
 *
 * Example (class registration, eager - replayed immediately at startup):
 * <pre>
 * kind: class
 * module: _io
 * class: BytesIO
 * interface: python.io.PyBytesIO
 * ---
 * METHODS = {
 *     "getvalue": lambda x: x.getvalue(),
 * }
 * </pre>
 *
 * Example (class registration, lazy - only imported/registered the first
 * time an instance of it is actually seen crossing into Java):
 * <pre>
 * kind: class
 * module: _io
 * class: StringIO
 * interface: python.io.PyStringIO
 * lazy: true
 * ---
 * METHODS = {
 *     "getvalue": lambda x: x.getvalue(),
 * }
 * </pre>
 *
 * Example (mini-backend registration - always eager, see {@link SpiLoader}):
 * <pre>
 * kind: backend
 * interface: python.io.IO
 * ---
 * METHODS = {
 *     "bytesIO": lambda: __import__("io").BytesIO(),
 * }
 * </pre>
 */
final class SpiResource
{

  final String kind;       // "class" or "backend"
  final String module;     // null when kind=backend
  final String className;  // null when kind=backend
  final String javaInterface;
  final boolean lazy;      // always false when kind=backend
  final String body;

  private SpiResource(String kind, String module, String className, String javaInterface, boolean lazy, String body)
  {
    this.kind = kind;
    this.module = module;
    this.className = className;
    this.javaInterface = javaInterface;
    this.lazy = lazy;
    this.body = body;
  }

  static SpiResource parse(String text)
  {
    int sep = text.indexOf("\n---\n");
    if (sep < 0)
      throw new IllegalArgumentException("Missing '---' header/body separator in .pyspi resource");
    String header = text.substring(0, sep);
    String body = text.substring(sep + 5);

    Map<String, String> fields = new HashMap<>();
    for (String line : header.split("\n"))
    {
      line = line.trim();
      if (line.isEmpty() || line.startsWith("#"))
        continue;
      int colon = line.indexOf(':');
      if (colon < 0)
        throw new IllegalArgumentException("Malformed .pyspi header line: " + line);
      fields.put(line.substring(0, colon).trim(), line.substring(colon + 1).trim());
    }

    String kind = fields.getOrDefault("kind", "class");
    String javaInterface = fields.get("interface");
    if (javaInterface == null)
      throw new IllegalArgumentException("Missing 'interface:' in .pyspi header");
    boolean lazy = "true".equals(fields.getOrDefault("lazy", "false"));

    if ("backend".equals(kind))
    {
      if (lazy)
        throw new IllegalArgumentException("mini-backends must be eager (interface=" + javaInterface + ")");
      return new SpiResource(kind, null, null, javaInterface, false, body);
    }

    String module = fields.get("module");
    String className = fields.get("class");
    if (module == null || className == null)
      throw new IllegalArgumentException(
              "Missing 'module:'/'class:' in .pyspi header for kind=class (interface="
              + javaInterface + ")");
    return new SpiResource(kind, module, className, javaInterface, lazy, body);
  }

}
