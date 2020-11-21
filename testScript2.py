import jpype
import jpype.imports
from jpype.types import *

jpype.startJVM(classpath="lib/rhino-1.7.13.jar")


def run_script(script, bindings_in={}, bindings_out={},
               class_loader=None):
    '''Run a piece of JavaScript code.

    :param script: script to run
    :type script: string

    :param bindings_in: global variable names and values to assign to them.
    :type bindings_in: dict

    :param bindings_out: a dictionary for returning variables. The
                         keys should be global variable names. After
                         the script has run, the values of these
                         variables will be assigned to the appropriate
                         value slots in the dictionary. For instance,
                         ``bindings_out = dict(foo=None)`` to get the
                         value for the "foo" variable on output.

    :param class_loader: class loader for scripting context

    :returns: the object that is the result of the evaluation.
    '''
    from org.mozilla.javascript import ContextFactory, ImporterTopLevel, WrappedException
    context = ContextFactory.getGlobal().enterContext()
    sourceName = JString("<java-python-bridge>")
    try:
        if class_loader is not None:
            context.setApplicationClassLoader(class_loader)

        scope = ImporterTopLevel(context)
        for k, v in bindings_in.items():
            scope.put(k, scope, v)
        result = context.evaluateString(scope, script, sourceName, 0, None)
        for k in list(bindings_out):
            bindings_out[k] = scope.get(k, scope)

    except WrappedException as e:
        raise e.getWrappedException()
    finally:
        context.exit()
    return result


bindings_out = {'a': None, 'b': None}
run_script("var a=2*b", bindings_in={'b': 2}, bindings_out=bindings_out)
print(bindings_out['a'])
