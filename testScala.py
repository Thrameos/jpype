import jpype
from pathlib import Path
jpype.startJVM(classpath=["lib/*"])
print(jpype.JClass("scala.collection.Iterator$Leading$1"))
