exc = [ArithmeticError, AssertionError, AttributeError, EOFError, Exception,
       FloatingPointError, IOError, ImportError, IndexError, KeyError, KeyboardInterrupt,
       LookupError, MemoryError, NameError, NotImplementedError, OSError, OverflowError,
       ReferenceError, RuntimeError, SyntaxError, SystemError, SystemExit,
       TypeError, ValueError, ZeroDivisionError, ]


def makeClass(exc, base):
    pName = exc.__name__
    eName = "Py" + exc.__name__
    bName = "Py" + base.__name__
    with open(eName + ".java", "w") as fd:
        print("\n".join([
            "package python.lang.exc;",
            "",
            "import org.jpype.python.annotation.PyTypeInfo;",
            "import org.jpype.python.internal.PyConstructor;",
            "import static org.jpype.python.internal.PyConstructor.ALLOCATOR;",
            "",
            f"@PyTypeInfo(name = \"{pName}\", exact = true)",
            f"public class {eName} extends {bName}",
            "{",
            f"  protected {eName}()",
            "  {",
            "    super();",
            "  }",
            "",
            f"  protected {eName}(PyConstructor key, long instance)",
            "  {",
            "    super(key, instance);",
            "  }",
            "",
            "  static Object _allocate(long inst)",
            "  {",
            f"    return new {eName}(ALLOCATOR, inst);",
            "  }",
            "",
            "",
            "}"]), file=fd)


for e in exc:
    makeClass(e, e.__mro__[1])
