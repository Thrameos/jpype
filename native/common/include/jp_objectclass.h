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

public: 
	// Conversion methods
	virtual PyObject*   asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(PyObject* obj);
	virtual jvalue     convertToJava(PyObject* obj);

public:
	// Class specific implementation
	virtual bool isObjectType() const;
	virtual bool isInterface() const;

	JPObjectClass* getSuperClass() const;
	const vector<JPObjectClass*>& getInterfaces() const;

public:
	/** 
	 * Called to fully load base classes and members 
	 */
	virtual void postLoad();

  /** Get the value of a static field.
	 * throws AttributeError if not found.
	 */	
	PyObject* getStaticAttribute(const string& attr_name);

  /** Set the value of a static field.
	 * throws AttributeError if not found.
	 */	
	void setStaticAttribute(const string& attr_name, PyObject* val);

  /** Create a new instance Java object.
	 */	
	JPValue newInstance(vector<PyObject*>& args);

	/** Get a member field.
	 * Returns the field or NULL if not found.
	 */
	JPField* getInstanceField(const string& name);

	/** Get a static field.
	 * Returns the field or NULL if not found.
	 */
	JPField* getStaticField(const string& name);

  /** Get a method.
	 * Returns the method or NULL if not found.
	 */
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

	string describe();

protected:
	virtual void loadSuperClass();	
	virtual void loadSuperInterfaces();	
	virtual void loadFields();	
	virtual void loadMethods();	
	virtual void loadConstructors();	

private:
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
