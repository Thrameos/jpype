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
