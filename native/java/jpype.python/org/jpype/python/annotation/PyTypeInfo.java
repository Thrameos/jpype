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
package org.jpype.python.annotation;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Annotation to define how to apply wrappers to Python objects.
 *
 * @author nelson85
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface PyTypeInfo
{

  /**
   * The name of the Python class.
   *
   * @return
   */
  String name();

  /**
   * Indicates the wrapper should only be applied to exact type matches.
   *
   * @return
   */
  boolean exact() default false;

  /**
   * Indicates that private methods will be implement
   *
   * @return
   */
  Class internal() default PyTypeInfo.class;
}
