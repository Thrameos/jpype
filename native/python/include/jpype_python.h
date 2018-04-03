/*****************************************************************************
   Copyright 2004 Steve M�nard

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
*****************************************************************************/   
#ifndef _JPYPE_PYTHON_H_
#define _JPYPE_PYTHON_H_



// This file defines the _jpype module's interface and initializes it
#include <Python.h>
#include <jpype.h>

// TODO figure a better way to do this .. Python dependencies should not be in common code

#include <pyport.h>
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

// =================================================================
#define ASSERT_JAVA_INITIALIZED JPPyni::assertInitialized();

#define PY_CHECK(op) op; { if (PyErr_Occurred()) throw PythonException();  };
#define PY_STANDARD_CATCH catch(...) { JPPyni::handleException(); }

// =================================================================

#include "pyjp_module.h"
#include "pyjp_monitor.h"
#include "pyjp_method.h"
#include "pyjp_array.h"
#include "pyjp_arrayclass.h"
#include "pyjp_class.h"
#include "pyjp_field.h"
#include "pyjp_proxy.h"
#include "pyjp_value.h"

#include "jpype_memory_view.h"

// Utility method
//PyObject* detachRef(HostRef* ref);

#endif // _JPYPE_PYTHON_H_
