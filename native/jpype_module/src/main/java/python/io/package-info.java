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
 * ({@code org.jpype.WrapperService}): the mapping from a concrete Python
 * {@code io}/{@code _io} class (e.g. {@code _io.BytesIO}) to the interfaces
 * declared here is not hand-listed the way {@code python.lang}'s builtin
 * types are; it is discovered through {@link org.jpype.WrapperService}, the
 * same mechanism a third-party package would use to expose its own types.
 * See {@code plan/SPI.md} and {@code plan/IO.md} in the repository root.
 *
 * As of this first cut, the SPI's lazy runtime hook
 * ({@code _jpype._cache.__missing__}) does not exist yet, so
 * {@code python.io.PyIoWrapperService}'s manifest is applied eagerly at
 * {@code _jbridge.initialize()} time as a stand-in — {@code io}/{@code _io}
 * are stdlib and normally already imported, so this is honest and
 * functional today, but should be switched to the lazy hook once it lands
 * rather than treated as the permanent mechanism.
 */
