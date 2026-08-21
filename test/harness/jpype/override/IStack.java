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

// A customizer target dedicated to sticky-method stacking tests
// (3+ customizers, mismatched rename targets) - kept separate from I0's
// family so these tests don't interact with the inheritance/interface
// depth coverage on I0/I1, regardless of test execution order within the
// shared JVM session.
public interface IStack
{
  int remove(Object o);
}
