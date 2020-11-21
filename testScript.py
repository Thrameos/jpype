from java.util.function import Predicate
from javax.script import ScriptContext
from javax.script import ScriptEngineManager
import jpype
import jpype.imports
from jpype.types import *

jpype.startJVM(classpath="lib/*")


manager = ScriptEngineManager()
engine = manager.getEngineByName("JavaScript")
print(manager.getEngineFactories())
bindings = engine.getBindings(ScriptContext.ENGINE_SCOPE)
bindings.put("polyglot.js.allowHostAccess", True)
bindings.put("polyglot.js.allowHostClassLookup", Predicate @ (lambda s: True))
bindings.put("javaObj", JObject)
print(engine.eval("(javaObj instanceof Java.type('java.lang.Object'));"))
