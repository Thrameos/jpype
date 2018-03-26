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
#ifndef _JPFIELD_H_
#define _JPFIELD_H_

/**
 * Field object
 */
class JPField
{
private:
	JPField(const JPField&);

public :
	/**
	 * Create a new field based on class and java.lang.Field object
	 *
	 * clazz is the class with the field
	 * fld is the Field instance (java.lang.Field)
	 */
	JPField(JPObjectClass* clazz, jobject fld);
	
	/**
	 * destructor
	 */
	virtual ~JPField() NO_EXCEPT_FALSE;
	
public :
	bool isStatic() const;
	
	const string& getName() const;

	JPObjectClass* getClass() const
	{
		return m_Class;
	}
	
	PyObject* getStaticAttribute();
	void     setStaticAttribute(PyObject* val);
	
	PyObject* getAttribute(jobject inst);
	void     setAttribute(jobject inst, PyObject* val);

	bool isFinal() const
	{
		return m_IsFinal;
	}

private :
	string     m_Name;
	JPObjectClass*   m_Class;
	bool       m_IsStatic;
	bool       m_IsFinal;
	jobject    m_Field;
	jfieldID   m_FieldID;
	jclass     m_Type;
};

#endif // _JPFIELD_H_

