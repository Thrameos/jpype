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
#ifndef _JPOBJECTCLASS_H_
#define _JPOBJECTCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPObjectClass : public JPClass
{
public :
	JPObjectClass(jclass c);
	virtual~ JPObjectClass();

public :
	
	virtual bool      isObjectType() const
	{ 
		return true; 
	}

public: 
	// Conversion methods
	virtual HostRef*   asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);

	// Array Methods
	PyObject* getArrayRangeToSequence(jarray, int start, int length);
  void setArrayRange(jarray, int start, int len, PyObject*);
	
public:
	// Class specific implementation
	/** 
	 * Called to fully load base classes and members 
	 */
	virtual void postLoad();
	
	HostRef*                getStaticAttribute(const string& attr_name);
	void                    setStaticAttribute(const string& attr_name, HostRef* val);
	
	JPObject*               newInstance(vector<HostRef*>& args);

	JPField*                getInstanceField(const string& name);
	JPField*                getStaticField(const string& name);
	JPMethod*		getMethod(const string& name);

	vector<JPMethod*>	getMethods() const;

	map<string, JPField*>& getStaticFields()
	{
		return m_StaticFields;
	}
	
	map<string, JPField*>& getInstanceFields()
	{
		return m_InstanceFields;
	}

	bool isArray();
	bool isFinal();
	bool isAbstract();
	bool isInterface()
	{
		return m_IsInterface;
	}

	long getClassModifiers();

	JPObjectClass* getSuperClass();
	const vector<JPObjectClass*>& getInterfaces() const;

	bool isSubclass(JPObjectClass*);
	
	string describe();

private :
	void loadSuperClass();	
	void loadSuperInterfaces();	
	void loadFields();	
	void loadMethods();	
	void loadConstructors();	

private :
	jclass                  m_Class;
	JPTypeName              m_Name;

	// Caches for class fields, methods, ctors, and parents
	bool                    m_IsInterface;
	JPObjectClass*		      m_SuperClass;
	vector<JPObjectClass*>	m_SuperInterfaces;
	map<string, JPField*>   m_StaticFields;
	map<string, JPField*>   m_InstanceFields;
	map<string, JPMethod*>	m_Methods;
	JPMethod*		m_Constructors;
};


#endif // _JPOBJECTCLASS_H_
