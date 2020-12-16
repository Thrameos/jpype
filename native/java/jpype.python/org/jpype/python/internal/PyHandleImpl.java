/** ***************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * See NOTICE file for details.
 **************************************************************************** */
package org.jpype.python.internal;

import python.lang.protocol.PyHandle;

/**
 *
 * @author nelson85
 */
public class PyHandleImpl implements PyHandle
{
  long _self;

  @SuppressWarnings("LeakingThisInConstructor")
  public PyHandleImpl(PyConstructor key, long instance)
  {
    this._self = instance;
    key.link(this, instance);
  }

  @Override
  public long _id()
  {
    return _self;
  }
}
