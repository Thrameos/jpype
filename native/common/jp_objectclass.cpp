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
	if (!m_IsInterface && m_Name.getSimpleName() != "java.lang.Object")
	{
		jclass baseClass = JPEnv::getJava()->GetSuperclass(m_Class);

		if (baseClass != NULL)
		{
			m_SuperClass = (JPObjectClass*)JPTypeManager::findClass(baseClass);
		}
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

HostRef* JPObjectClass::getStaticAttribute(const string& name)
{
	// static fields
	map<string, JPField*>::iterator fld = m_StaticFields.find(name);
	if (fld != m_StaticFields.end())
	{
		return fld->second->getStaticAttribute();
	}

	JPEnv::getHost()->setAttributeError(name.c_str());
	JPEnv::getHost()->raise("getAttribute");
	return NULL; // never reached
}

HostRef* JPObjectClass::asHostObject(jvalue obj)
{
	TRACE_IN("JPObjectClass::asHostObject");
	if (obj.l == NULL)
	{
		return JPEnv::getHost()->getNone();
	}

	jclass cls = JPJni::getClass(obj.l);
	if (JPJni::isArray(cls))
	{
		JPArrayClass* arrayType = (JPArrayClass*)JPTypeManager::findClass(cls);
		return arrayType->asHostObject(obj);
	}

	return JPEnv::getHost()->newObject(new JPObject((JPObjectClass*)JPTypeManager::findClass(cls), obj.l));
	TRACE_OUT;
}

long JPObjectClass::getClassModifiers()
{
	return JPJni::getClassModifiers(m_Class);
}

EMatchType JPObjectClass::canConvertToJava(HostRef* obj)
{
	JPLocalFrame frame;
	TRACE_IN("JPObjectClass::canConvertToJava");
	if (JPEnv::getHost()->isNone(obj))
	{
		return _implicit;
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* o = JPEnv::getHost()->asObject(obj);
		JPObjectClass* oc = o->getClass(); 
		TRACE2("Match name", oc->m_Name.getSimpleName());

		if (oc == this)
		{
			// hey, this is me! :)
			return _exact;
		}

		if (JPEnv::getJava()->IsAssignableFrom(oc->m_Class, m_Class))
		{
			return _implicit;
		}
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		JPClass* o = JPEnv::getHost()->getWrapperClass(obj);
		if (o == this)
		{
			TRACE1("exact wrapper");
			return _exact;
		}
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		// Check if any of the interfaces matches ...
		vector<jclass> itf = proxy->getInterfaces();
		for (unsigned int i = 0; i < itf.size(); i++)
		{
			if (JPEnv::getJava()->IsAssignableFrom(itf[i], m_Class))
			{
				TRACE1("implicit proxy");
				return _implicit;
			}
		}
	}

	return _none;
	TRACE_OUT;
}

jobject JPObjectClass::buildObjectWrapper(HostRef* obj)
{
	JPLocalFrame frame;

	vector<HostRef*> args(1);
	args.push_back(obj);

	JPObject* pobj = newInstance(args);
	jobject out = pobj->getObject();
	delete pobj;

	return frame.keep(out);
}

jvalue JPObjectClass::convertToJava(HostRef* obj)
{
	TRACE_IN("JPObjectClass::convertToJava");
	JPLocalFrame frame;
	jvalue res;

	res.l = NULL;

	// assume it is convertible;
	if (JPEnv::getHost()->isNone(obj))
	{
		res.l = NULL;
		return res;
	}

	if (JPEnv::getHost()->isObject(obj))
	{
		JPObject* ref = JPEnv::getHost()->asObject(obj);
		res.l = frame.keep(ref->getObject());
		return res;
	}

	if (JPEnv::getHost()->isProxy(obj))
	{
		JPProxy* proxy = JPEnv::getHost()->asProxy(obj);
		res.l = frame.keep(proxy->getProxy());
		return res;
	}

	if (JPEnv::getHost()->isWrapper(obj))
	{
		res = JPEnv::getHost()->getWrapperValue(obj); // FIXME isn't this one global already
		res.l = frame.keep(res.l);
		return res;
	}

	return res;
	TRACE_OUT;
}

JPObject* JPObjectClass::newInstance(vector<HostRef*>& args)
{
	return m_Constructors->invokeConstructor(args);
}

void JPObjectClass::setStaticAttribute(const string& name, HostRef* val)
{
	map<string, JPField*>::iterator it = m_StaticFields.find(name);
	if (it == m_StaticFields.end())
	{
		JPEnv::getHost()->setAttributeError(name.c_str());
		JPEnv::getHost()->raise("__setattr__");
	}

	it->second->setStaticAttribute(val);
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

	out << "class " << m_Name.getSimpleName();
	if (m_SuperClass != NULL)
	{
		out << " extends " << m_SuperClass->getName().getSimpleName();
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
			out << " " << pc->getName().getSimpleName();
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
		out << f->getClass()->getName().getSimpleName() << " " << f->getName() << ";" << endl;
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
		out << f->getClass()->getName().getSimpleName() << " " << f->getName() << ";" << endl;
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

bool JPObjectClass::isArray()
{
	return JPJni::isArray(m_Class);
}

bool JPObjectClass::isAbstract()
{
	return JPJni::isAbstract(m_Class);
}

bool JPObjectClass::isFinal()
{
	return JPJni::isFinal(m_Class);
}

JPObjectClass* JPObjectClass::getSuperClass()
{
	return m_SuperClass;
}

const vector<JPObjectClass*>& JPObjectClass::getInterfaces() const
{
	return m_SuperInterfaces;
}

bool JPObjectClass::isSubclass(JPObjectClass* o)
{
	JPLocalFrame frame;
	jclass jo = o->getNativeClass();
	return JPEnv::getJava()->IsAssignableFrom(m_Class, jo);
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


PyObject* JPObjectClass::getArrayRangeToSequence(jarray, int start, int length)
{
	RAISE(JPypeException, "not valid for objects");
}
	
void JPObjectClass::setArrayRange(jarray, int start, int len, PyObject*) 
{
	RAISE(JPypeException, "not valid for objects");
}

