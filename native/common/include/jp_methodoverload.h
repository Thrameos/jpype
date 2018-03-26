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
#ifndef _JPMETHODOVERLOAD_H_
#define _JPMETHODOVERLOAD_H_

class JPMethodOverload
{
private:
	JPMethodOverload(const JPMethodOverload& o);
public :
	JPMethodOverload(JPObjectClass* claz, jobject mth);
	
	virtual ~JPMethodOverload();
	
	EMatchType  matches(bool ignoreFirst, vector<PyObject*>& args) ;

	PyObject*   invokeInstance(vector<PyObject*>& arg);

	PyObject*   invokeStatic(vector<PyObject*>& arg);

	JPValue     invokeConstructor(JPObjectClass* cls, vector<PyObject*>& arg);

public :	
	string getSignature();

	bool isStatic() const
	{
		return m_IsStatic;
	}
	
	bool isFinal() const
	{
		return m_IsFinal;
	}

	bool isVarArgs() const
	{
		return m_IsVarArgs;
	}

	JPClass* getReturnType()
	{
    ensureTypeCache(); 
	  return  m_ReturnTypeCache;
	}

	unsigned char getArgumentCount() const
	{
		return (unsigned char)m_Arguments.size();
	}

	string getArgumentString();

  void packArgs(JPMallocCleaner<jvalue>& v, vector<PyObject*>& arg, size_t skip);
	bool isSameOverload(JPMethodOverload& o);
	string matchReport(vector<PyObject*>& args);
	bool isMoreSpecificThan(JPMethodOverload& other) const;
private:
	void ensureTypeCache() const;
private :
	JPObjectClass*                 m_Class;
	jobject                  m_Method;
	jmethodID                m_MethodID;
	jclass                   m_ReturnType;
	vector<jclass>           m_Arguments;
	bool                     m_IsStatic;
	bool                     m_IsFinal;
	bool                     m_IsVarArgs;
	bool                     m_IsConstructor;
	mutable vector<JPClass*>  m_ArgumentsTypeCache;
	mutable JPClass*          m_ReturnTypeCache;
};

#endif // _JPMETHODOVERLOAD_H_
