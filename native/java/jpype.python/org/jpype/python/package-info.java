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
package org.jpype.python;

/**
 * Package for JPype Python script engine.
 *
 * This is the "native" interface which presents the Python script engine. It
 * does not currently conform to Graalvm Polyglot nor a Java ScriptEngine.
 *
 * Graalvm polyglot requires the language to be specified each time a script it
 * interpreted, rather than allowing a language context which can then be acted
 * on so it doesn't really apply.
 *
 * The Javax ScriptEngine assumes one script engine with a set of variable
 * bindings and input/output called a Context, the variable binding itself as
 * Bindings, and a ScriptEngineFactory that produces the ScriptEngine. This
 * again is not a great match. Although Python can have multiple interpreters,
 * most modules to not support a second interpreter so the the concept to more
 * than one interpreter is not great. Python has two types of Bindings (globals
 * and locals). We could drop locals to make it more similar. It is also unclear
 * how we can tie stdin and stdout for one scope as Python has a global concept
 * of sys.stdin and sys.stdout.
 *
 * As locals and globals are the same, we could just drop locals from
 * consideration. But Javax also has the concept of ENGINE_SCOPE which contains
 * all those things that are shared amongst all bindings. 
 *
 */
