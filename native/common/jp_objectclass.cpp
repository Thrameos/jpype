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
#include <jpype.h>

JPObjectClass::JPObjectClass(jclass c) :
	JPClass(c),
	m_SuperClass(NULL),
	m_Constructors(NULL)
{
}

JPObjectClass::~JPObjectClass()
{
	delete m_Constructors;

	// interfaces of this cannot be simply deleted here, since they may be also
	// super types of other classes, which would be invalidated by doing so.

	for (map<string, JPMethod*>::iterator mthit = m_Methods.begin(); mthit != m_Methods.end(); mthit ++)
	{
		delete mthit->second;
	}

	for (map<string, JPField*>::iterator fldit = m_InstanceFields.begin(); fldit != m_InstanceFields.end(); fldit++)
	{
		delete fldit->second;
	}

	for (map<string, JPField*>::iterator fldit2 = m_StaticFields.begin(); fldit2 != m_StaticFields.end(); fldit2++)
	{
		delete fldit2->second;
	}
}

bool JPObjectClass::isInterface() const
{
	return m_IsInterface;
}
	
void JPObjectClass::postLoad()
{
	TRACE_IN("JPObjectClass::postLoad");

	// Is this an interface?
	m_IsInterface = JPJni::isInterface(m_Class);

	loadSuperClass();
	loadSuperInterfaces();
	loadFields();
	loadMethods();
	loadConstructors();
	TRACE_OUT;
}

void JPObjectClass::loadSuperClass()
{
	TRACE_IN("JPObjectClass::loadSuperClass");
	JPLocalFrame frame;
	// base class .. if any
	if (!m_IsInterface)
	{
		jclass baseClass = JPEnv::getJava()->GetSuperclass(m_Class);
		m_SuperClass = (JPObjectClass*)JPTypeManager::findClass(baseClass);
	}
	TRACE_OUT;
}

void JPObjectClass::loadSuperInterfaces()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPObjectClass::loadSuperInterfaces");
	// Super interfaces
	vector<jclass> intf = JPJni::getInterfaces(frame, m_Class);
	for (vector<jclass>::iterator it = intf.begin(); it != intf.end(); it++)
	{
		JPObjectClass* interface = (JPObjectClass*)JPTypeManager::findClass(*it);
		m_SuperInterfaces.push_back(interface);
	}
	TRACE_OUT;
}

void JPObjectClass::loadFields()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPObjectClass::loadFields");
	// fields
	vector<jobject> fields = JPJni::getDeclaredFields(frame, m_Class);

	for (vector<jobject>::iterator it = fields.begin(); it != fields.end(); it++)
	{
		JPField* field = new JPField(this, *it);
		if (field->isStatic())
		{
			m_StaticFields[field->getName()] = field;
		}
		else
		{
			m_InstanceFields[field->getName()] = field;
		}
	}
	TRACE_OUT;
}

void JPObjectClass::loadMethods()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPObjectClass::loadMethods");

	// methods
	vector<jobject> methods = JPJni::getMethods(frame, m_Class);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		const string& name = JPJni::getMemberName(*it);
		JPMethod* method = getMethod(name);
		if (method == NULL)
		{
			method = new JPMethod(this, name, false);
			m_Methods[name] = method;
		}

		method->addOverload(this, *it);
	}
	TRACE_OUT;
}

void JPObjectClass::loadConstructors()
{
	JPLocalFrame frame(32);
	TRACE_IN("JPObjectClass::loadMethods");
	m_Constructors = new JPMethod(this, "[init", true);

	if (JPJni::isAbstract(m_Class))
	{
		// NO constructor ...
		return;
	}


	vector<jobject> methods = JPJni::getDeclaredConstructors(frame, m_Class);

	for (vector<jobject>::iterator it = methods.begin(); it != methods.end(); it++)
	{
		if (JPJni::isMemberPublic(*it))
		{
			m_Constructors->addOverload(this, *it);
		}
	}
	TRACE_OUT;
}

JPField* JPObjectClass::getInstanceField(const string& name)
{
	map<string, JPField*>::iterator it = m_InstanceFields.find(name);
	if (it == m_InstanceFields.end())
	{
		return NULL;
	}
	return it->second;
}

JPField* JPObjectClass::getStaticField(const string& name)
{
	map<string, JPField*>::iterator it = m_StaticFields.find(name);
	if (it == m_StaticFields.end())
	{
		return NULL;
	}
	return it->second;
}


JPMethod* JPObjectClass::getMethod(const string& name)
{
	map<string, JPMethod*>::iterator it = m_Methods.find(name);
	if (it == m_Methods.end())
	{
		return NULL;
	}

	return it->second;
}

PyObject* JPObjectClass::getStaticAttribute(const string& name)
{
	JPField* fld = getStaticField(name);
	if (fld != NULL)
	{
		return fld->getStaticAttribute();
	}

	JPyErr::setAttributeError(name.c_str());
	JPyErr::raise("getAttribute");
	return NULL; // never reached
}

void JPObjectClass::setStaticAttribute(const string& name, PyObject* val)
{
	JPField* fld = getStaticField(name);
	if (fld != NULL)
	{
		fld->setStaticAttribute(val);
		return;
	}
	JPyErr::setAttributeError(name.c_str());
	JPyErr::raise("__setattr__");
}

PyObject* JPObjectClass::asHostObject(jvalue obj)
{
	TRACE_IN("JPObjectClass::asHostObject");
	if (obj.l == NULL)
	{
		return JPPyni::getNone();
	}

	jclass cls = JPJni::getClass(obj.l);
	if (JPJni::isArray(cls))
	{
		JPArrayClass* arrayType = (JPArrayClass*)JPTypeManager::findClass(cls);
		return arrayType->asHostObject(obj);
	}

	return JPPyni::newObject(JPValue(JPTypeManager::findClass(cls), obj));
	TRACE_OUT;
}

EMatchType JPObjectClass::canConvertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	JPLocalFrame frame;
	TRACE_IN("JPObjectClass::canConvertToJava");
	if (obj.isNone())
	{
		return _implicit;
	}

	if (obj.isJavaValue())
	{
		const JPValue& value = obj.asJavaValue();
		JPClass* oc = value.getClass(); 
		TRACE2("Match name", oc->m_Name.getSimpleName());

		if (oc == this)
		{
			// hey, this is me! :)
			return _exact;
		}

		if (JPEnv::getJava()->IsAssignableFrom(oc->getNativeClass(), getNativeClass()))
		{
			return _implicit;
		}
	}

	if (obj.isProxy())
	{
		JPProxy* proxy = obj.asProxy();
		if (proxy->implements(this))
		{
				TRACE1("implicit proxy");
				return _implicit;
    }
	}

	return _none;
	TRACE_OUT;
}

jvalue JPObjectClass::convertToJava(PyObject* pyobj)
{
	JPyAdaptor obj(pyobj);

	TRACE_IN("JPObjectClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;

	// assume it is convertible;
	if (obj.isNone())
	{
		res.l = NULL;
		return res;
	}

	if (obj.isJavaValue())
	{
		const JPValue& value = obj.asJavaValue();
		res = value.getValue();
		res.l = frame.keep(res.l);
		return res;
	}

	if (obj.isProxy())
	{
		JPProxy* proxy = obj.asProxy();
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	return res;
	TRACE_OUT;
}

JPValue JPObjectClass::newInstance(vector<PyObject*>& args)
{
	return m_Constructors->invokeConstructor(args);
}

string JPObjectClass::describe()
{
	JPLocalFrame frame;
	stringstream out;
	out << "public ";
	if (isAbstract())
	{
		out << "abstract ";
	}
	if (isFinal())
	{
		out << "final ";
	}

	out << "class " << m_Name;
	if (m_SuperClass != NULL)
	{
		out << " extends " << m_SuperClass->getSimpleName();
	}

	if (m_SuperInterfaces.size() > 0)
	{
		out << " implements";
		bool first = true;
		for (vector<JPObjectClass*>::iterator itf = m_SuperInterfaces.begin(); itf != m_SuperInterfaces.end(); itf++)
		{
			if (!first)
			{
				out << ",";
			}
			else
			{
				first = false;
			}
			JPObjectClass* pc = *itf;
			out << " " << pc->getSimpleName();
		}
	}
	out << endl << "{" << endl;

	// Fields
	out << "  // Accessible Static Fields" << endl;
	for (map<string, JPField*>::iterator curField = m_StaticFields.begin(); curField != m_StaticFields.end(); curField++)
	{
		JPField* f = curField->second;
		out << "  public static ";
		if (f->isFinal())
		{
			out << "final ";
		}
		out << f->getClass()->getSimpleName() << " " << f->getName() << ";" << endl;
	}
	out << endl;
	out << "  // Accessible Instance Fields" << endl;
	for (map<string, JPField*>::iterator curInstField = m_InstanceFields.begin(); curInstField != m_InstanceFields.end(); curInstField++)
	{
		JPField* f = curInstField->second;
		out << "  public ";
		if (f->isFinal())
		{
			out << "final ";
		}
		out << f->getClass()->getSimpleName() << " " << f->getName() << ";" << endl;
	}
	out << endl;

	// Constructors
	out << "  // Accessible Constructors" << endl;
	out << m_Constructors->describe("  ") << endl;

	out << "  // Accessible Methods" << endl;
	for (map<string, JPMethod*>::iterator curMethod = m_Methods.begin(); curMethod != m_Methods.end(); curMethod++)
	{
		JPMethod* f = curMethod->second;
		out << f->describe("  ");
		out << endl;
	}
	out << "}";

	return out.str();
}

bool JPObjectClass::isObjectType() const
{
	return true;
}

JPObjectClass* JPObjectClass::getSuperClass() const
{
	return m_SuperClass;
}

const vector<JPObjectClass*>& JPObjectClass::getInterfaces() const
{
	return m_SuperInterfaces;
}

vector<JPMethod*>  JPObjectClass::getMethods() const
{
	vector<JPMethod*> res;
	res.reserve(m_Methods.size());
	for (map<string, JPMethod*>::const_iterator cur = m_Methods.begin(); cur != m_Methods.end(); cur++)
	{
		res.push_back(cur->second);
	}
	return res;
}


