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
#ifndef _JPPROXY_H_
#define _JPPROXY_H_

class JPProxy
{
public:
	/** Create a proxy.
	 * intf is a list of all the java interfaces that this proxy implements.
	 */
	JPProxy(PyObject* inst, vector<JPObjectClass*>& intf);

	virtual ~JPProxy();

	/** Create all of the hooks needed for proxy. 
	 * This is called when the jvm is connected.
	 */
	static void init();
	jobject getProxy();

	bool implements(JPObjectClass* interface);

private :
	PyObject*               m_Instance;
	vector<JPObjectClass*> m_Interfaces;
};

#endif // JPPROXY_H
