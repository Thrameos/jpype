#ifndef JPCALLBACK_H_
#define JPCALLBACK_H_

#include <jpype.h>
#include <Python.h>

// Exception safe callback handler.
// Use this whenever a java method is access a python object
// so that we have the python global lock.
class JPCallback
{
  PyGILState_STATE state;
  public:
    JPCallback()
		{
			state = PyGILState_Ensure();
		}
		~JPCallback()
		{
			PyGILState_Release(state);
		}
};

#endif
