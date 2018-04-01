#*****************************************************************************
#   Copyright 2004-2008 Steve Menard
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#*****************************************************************************
import sys

import _jpype

from . import _jclass
from . import _jarray
from . import _jwrapper
from . import _jproxy
from . import _jexception
from . import _jcollection
from . import _jobject
from . import _jio
from . import _properties
from . import nio
from . import reflect

# Reference daemon is now an automatic function of the jvm.  
# The user should not need to interfer with it.
#from . import _refdaemon

#_usePythonThreadForDaemon = False
#
#def setUsePythonThreadForDeamon(v):
#    global _usePythonThreadForDaemon
#    _usePythonThreadForDaemon = v

_initializers=[]

def registerJVMInitializer(func):
    """Register a function to be called after jvm is started"""
    _initializers.append(func)

def _initialize():
    _properties._initialize()
    _jclass._initialize()
    _jarray._initialize()
    _jwrapper._initialize()
    _jproxy._initialize()
    _jexception._initialize()
    _jcollection._initialize()
    _jobject._initialize()
    nio._initialize()
    reflect._initialize()
    for func in _initializers:
        func()

def isJVMStarted() :
    return _jpype.isStarted()

def startJVM(jvm, *args):
    """
    Starts a Java Virtual Machine

    :param jvm:  Path to the jvm library file (libjvm.so, jvm.dll, ...)
    :param args: Arguments to give to the JVM
    """
    _jpype.startup(jvm, tuple(args), True)
    _initialize()

    # start the reference daemon thread
#    if _usePythonThreadForDaemon:
#        _refdaemon.startPython()
#    else:
#        _refdaemon.startJava()

def attachToJVM(jvm):
    _jpype.attach(jvm)
    _initialize()

def shutdownJVM():
#    _refdaemon.stop()
    _jpype.shutdown()

def isThreadAttachedToJVM():
    return _jpype.isThreadAttachedToJVM()

def attachThreadToJVM():
    _jpype.attachThreadToJVM()

def detachThreadFromJVM():
    _jpype.detachThreadFromJVM()


def get_default_jvm_path():
    """
    Retrieves the path to the default or first found JVM library

    :return: The path to the JVM shared library file
    :raise ValueError: No JVM library found
    """
    if sys.platform in ("win32", "cygwin"):
        # Windows or Cygwin
        from ._windows import WindowsJVMFinder
        finder = WindowsJVMFinder()
    elif sys.platform == "darwin":
        # Mac OS X
        from ._darwin import DarwinJVMFinder
        finder = DarwinJVMFinder()
    else:
        # Use the Linux way for other systems
        from ._linux import LinuxJVMFinder
        finder = LinuxJVMFinder()

    return finder.get_jvm_path()

# Naming compatibility
getDefaultJVMPath = get_default_jvm_path


class ConversionConfigClass(object):
    def __init__(self):
        self._convertString = 1

    def _getConvertString(self):
        return self._convertString

    def _setConvertString(self, value):
        if value:
            self._convertString = 1
        else:
            self._convertString = 0

        _jpype.setConvertStringObjects(self._convertString)

    string = property(_getConvertString, _setConvertString, None)

ConversionConfig = ConversionConfigClass()
