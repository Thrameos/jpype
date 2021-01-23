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
import org.jpype.python.enums.PyInvocation;

/**
 * Annotation to direct PyTypeBuilder when creating a method wrapper.
 *
 * @author nelson85
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface PyMethodInfo
{
  
  public static final int ACCEPT = 1;
  public static final int BORROWED = 2;

  /**
   * Name of the Python method.
   */
  String name();

  /**
   * Indicates how to call this wrapper in C.
   *
   * @return
   */
  PyInvocation invoke();

  /**
   * Used for richcompare
   */
  int op() default -1;

  /**
   * Skip this argument.
   *
   * Used to create instance methods.
   *
   * @return
   */
  boolean method();

  /**
   * Accept null returns.
   *
   * Prevents a throw if the argument is null.
   *
   * @return
   */
  int flags() default 0;
}
