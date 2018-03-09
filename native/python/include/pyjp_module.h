/*****************************************************************************
   Copyright 2004 Steve Ménard

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
#ifndef _PYJMODULE_H_
#define _PYJMODULE_H_

namespace PyJPModule
{
	void errorOccurred();

	PyObject* startup(PyObject* obj, PyObject* args);
	PyObject* attach(PyObject* obj, PyObject* args);
	PyObject* dumpJVMStats(PyObject* obj);
	PyObject* shutdown(PyObject* obj);
	PyObject* synchronized(PyObject* obj, PyObject* args);
	PyObject* isStarted(PyObject* obj);
	PyObject* attachThread(PyObject* obj);
	PyObject* detachThread(PyObject* obj);
	PyObject* isThreadAttached(PyObject* obj);
	PyObject* getJException(PyObject* obj, PyObject* args);
	PyObject* raiseJava(PyObject* obj, PyObject* args);
	PyObject* attachThreadAsDaemon(PyObject* obj);
	PyObject* startReferenceQueue(PyObject* obj, PyObject* args);
	PyObject* stopReferenceQueue(PyObject* obj);
	PyObject* setConvertStringObjects(PyObject* obj, PyObject* args);
	PyObject* setResource(PyObject* obj, PyObject* args);
	PyObject* convertToDirectBuffer(PyObject* self, PyObject* arg);
}

#endif // _PYMODULE_H_
