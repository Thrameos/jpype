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
#ifndef _JPCLASS_H_
#define _JPCLASS_H_

/**
 * Class to wrap Java Class and provide low-level behavior
 */
class JPClass : public JPType
{
public :
	JPClass(jclass c);
	virtual~ JPClass();

public :
	virtual jclass getNativeClass() const
	{
		return m_Class;
	}

	virtual const JPTypeName&  getName() const
	{
		return m_Name;
	}
	
	virtual bool      isObjectType() const
	{ 
		return true; 
	}

public : // JPType implementation
	virtual HostRef*   asHostObject(jvalue val);
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
	
	// JPType implementation
	virtual HostRef* getStaticValue(jclass c, jfieldID fid);
	virtual void      setStaticValue(jclass c, jfieldID fid, HostRef* val);
	virtual HostRef* getInstanceValue(jobject c, jfieldID fid);
	virtual void      setInstanceValue(jobject c, jfieldID fid, HostRef* val);
	virtual HostRef*   asHostObjectFromObject(jvalue val);

	virtual jobject convertToJavaObject(HostRef* obj);

	virtual HostRef* invokeStatic(jclass, jmethodID, jvalue*);
	virtual HostRef* invoke(jobject, jclass clazz, jmethodID, jvalue*);

	virtual jarray    newArrayInstance(int size);
	virtual vector<HostRef*> getArrayRange(jarray, int start, int length);
	virtual void      setArrayRange(jarray, int start, int length, vector<HostRef*>& vals);
	virtual HostRef* getArrayItem(jarray, int ndx);
	virtual void      setArrayItem(jarray, int ndx, HostRef* val);
	virtual PyObject* getArrayRangeToSequence(jarray, int start, int length);
	virtual void setArrayRange(jarray, int start, int len, PyObject*);

	virtual HostRef*   convertToDirectBuffer(HostRef* src);
	virtual bool isSubTypeOf(const JPType& other) const;

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

	JPClass* getSuperClass();
	const vector<JPClass*>& getInterfaces() const;

	bool isSubclass(JPClass*);
	
	string describe();


private :
	void loadSuperClass();	
	void loadSuperInterfaces();	
	void loadFields();	
	void loadMethods();	
	void loadConstructors();	

	jobject buildObjectWrapper(HostRef* obj);

private :
	jclass                  m_Class;
	JPTypeName              m_Name;

	// Caches for class fields, methods, ctors, and parents
	bool                    m_IsInterface;
	JPClass*		m_SuperClass;
	vector<JPClass*>	m_SuperInterfaces;
	map<string, JPField*>   m_StaticFields;
	map<string, JPField*>   m_InstanceFields;
	map<string, JPMethod*>	m_Methods;
	JPMethod*		m_Constructors;
};


#endif // _JPCLASS_H_
