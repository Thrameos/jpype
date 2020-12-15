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
