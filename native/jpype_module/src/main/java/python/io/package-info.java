// --- file: python/io/package-info.java ---
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

/**
 * Java front-end for Python's {@code io} module, following the same
 * conventions as {@link python.lang}.
 *
 * This package is also the first real consumer of the SPI
 * ({@code org.jpype.WrapperService}/{@code org.jpype.Installer}): nothing
 * here is hand-wired into {@code _jbridge.py} the way {@code python.lang}'s
 * builtin types are. {@link PyIoWrapperService#getEagerResources()} lists
 * this package's {@code .pyspi} resources (under
 * {@code src/main/resources/python/io/spi/}), each naming a Python class
 * (module + name) or mini-backend, its Java interface, and a blob of Python
 * source; {@code org.jpype.SpiLoader} reads them at startup and replays them
 * into {@code Installer}, implemented by {@code _jbridge.py}. This is the
 * same mechanism a third-party package would use to expose its own types.
 * See {@code plan/SPI.md} and {@code plan/IO.md} in the repository root.
 *
 * Two registration paths exist and serve different purposes: the eager one
 * above (live, used by every class in this package today) and a lazy
 * per-class lookup ({@code org.jpype.WrapperService#getInterfaces}) for a
 * {@code _jpype._cache.__missing__} runtime hook that does not exist yet —
 * see {@code plan/SPI.md}'s "Lazy granularity: by module, not by class"
 * section. {@code python.io} only exercises the eager path so far, since
 * {@code io}/{@code _io} are stdlib and normally already imported.
 */
