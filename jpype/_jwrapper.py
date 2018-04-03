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
from ._jpackage import JPackage

if sys.version > '3':
    unicode = str
    long = int

def _initialize():
    _jpype.setResource('WrapperClass', _JWrapper)
    _jpype.setResource('StringWrapperClass', JString)
    JBoolean.__javaclass__=_jpype.PyJPClass('boolean', True)
    JByte.__javaclass__=_jpype.PyJPClass('byte', True)
    JChar.__javaclass__=_jpype.PyJPClass('char', True)
    JShort.__javaclass__=_jpype.PyJPClass('short', True)
    JInt.__javaclass__=_jpype.PyJPClass('int', True)
    JLong.__javaclass__=_jpype.PyJPClass('long', True)
    JFloat.__javaclass__=_jpype.PyJPClass('float', True)
    JDouble.__javaclass__=_jpype.PyJPClass('double', True)
    JString.__javaclass__=_jpype.PyJPClass('java.lang.String')

class _JWrapper(object):
    def __init__(self, v):
        if v is not None:
            self._pyv = v
            self._value = _jpype.PyJPValue(self.__javaclass__, v)
        else:
            self._value = None


class JByte(_JWrapper):
    pass

class JShort(_JWrapper):
    pass

class JInt(_JWrapper):
    pass

class JLong(_JWrapper):
    pass

class JFloat(_JWrapper):
    pass

class JDouble(_JWrapper):
    pass

class JChar(_JWrapper):
    pass

class JBoolean(_JWrapper):
    pass

# FIXME this should likely just go to the java.lang.String class 
class JString(_JWrapper):
    pass

def _getDefaultTypeName(obj):
    # Get the attribute if it exists
    try:
        return obj.__javaclass__
    except AttributeError:
        pass

    if obj is True or obj is False:
        return _jclass.JClass('java.lang.Boolean')

    if isinstance(obj, str) or isinstance(obj, unicode):
        return _jclass.JClass("java.lang.String")

    if isinstance(obj, int):
        return _jclass.JClass("java.lang.Integer")

    if isinstance(obj, long):
        return _jclass.JClass("java.lang.Long")

    if isinstance(obj, float):
        return _jclass.JClass("java.lang.Double")

#    if isinstance(obj, _jclass._JavaClass):
#        return obj.__javaclassname__
#
#    if isinstance(obj, JPackage("java").lang.Class):
#        return obj.__class__.__javaclass__.getName()
#
#    if isinstance(obj, _JWrapper):
#        return obj.typeName

    raise TypeError(
        "Unable to determine the default type of {0}".format(obj.__class__))

class JObject(_JWrapper):
    #typeName="java.lang.Object"
    def __init__(self, v, tp=None):
        if tp is None:
            tp = _jpype.PyJPClass(_getDefaultTypeName(v))
        if isinstance(tp, _jclass._JavaClass):
            tp = tp.__javaclass__

        self.__javaclass__ = tp
        self._value = _jpype.PyJPValue(self.__javaclass__, v)
