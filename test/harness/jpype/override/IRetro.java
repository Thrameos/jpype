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

// Customizer target dedicated to the multiple-registrations-for-the-same-
// -target tests (__jclass_init__ hook composition, retroactive sticky
// registration) - kept separate from the other override.* families so
// these tests don't interact with them regardless of execution order
// within the shared JVM session.
public interface IRetro
{
  int remove(Object o);
}
