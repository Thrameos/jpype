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
package org.jpype.python.internal;

/**
 * This is the home for native methods that call into the module.
 * 
 * @author nelson85
 */
public class Native
{
  
  public static native void start();
  
  /** 
   * (internal) Find a C language symbol in the existing library.
   * 
   * @param str
   * @return 
   */
  public static native long getSymbol(String str);


  /** 
   * (internal) Adds a shared library to the search path for symbols.
   * 
   * This is mainly an internal call, though some extension modules may require
   * it.
   * 
   * FIXME This should take a Path object.
   * 
   * @param str 
   */
  public static native void addLibrary(String str);

}
