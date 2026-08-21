/* ****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */
package jpype.override;

// Redeclares remove() at this depth despite also implementing I1 - the
// "ArrayList" role: confirms a genuine re-override still gets its own
// fresh rename even below a farther-down interface.
public class ISubOverride extends IBase implements I1
{
  public int remove(Object o)
  {
    return 2;
  }
}
