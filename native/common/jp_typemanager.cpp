/*****************************************************************************
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   See NOTICE file for details.
 *****************************************************************************/
#include "jpype.h"
#include "jp_classloader.h"

/**
 * Constructor for JPTypeManager.
 * Initializes the JPTypeManager by loading the necessary Java methods
 * from the TypeManager class in the JVM.
 * 
 * @param frame - A reference to the current JPJavaFrame.
 */
JPTypeManager::JPTypeManager(JPJavaFrame& frame)
{
	JP_TRACE_IN("JPTypeManager::init");
	jclass cls = frame.getContext()->getClassLoader()->findClass(frame, "org.jpype.manager.TypeManager");
	m_FindClass = frame.GetMethodID(cls, "findClass", "(Ljava/lang/Class;)J");
	m_FindClassByName = frame.GetMethodID(cls, "findClassByName", "(Ljava/lang/String;)J");
	m_FindClassForObject = frame.GetMethodID(cls, "findClassForObject", "(Ljava/lang/Object;)J");
	m_PopulateMethod = frame.GetMethodID(cls, "populateMethod", "(JLjava/lang/reflect/Executable;)V");
	m_PopulateMembers = frame.GetMethodID(cls, "populateMembers", "(Ljava/lang/Class;)V");
    m_InterfaceParameterCount = frame.GetMethodID(cls, "interfaceParameterCount", "(Ljava/lang/Class;)I");

	// The object instance will be loaded later
	JP_TRACE_OUT;
}

/**
 * Finds a JPClass object for a given Java class object.
 * 
 * @param obj - The Java class object to find.
 * @return A pointer to the JPClass object corresponding to the Java class.
 */
JPClass* JPTypeManager::findClass(jclass obj)
{
	JP_TRACE_IN("JPTypeManager::findClass");
	JPJavaFrame frame = JPJavaFrame::outer();
if (obj == nullptr)
{
    JP_RAISE(PyExc_RuntimeError, "Null class object passed to findClass");
}
	jvalue val;
	val.l = obj;
	return (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClass, &val));
	JP_TRACE_OUT;
}

/**
 * Finds a JPClass object by its name.
 * 
 * @param name - The name of the Java class to find.
 * @return A pointer to the JPClass object corresponding to the class name.
 * @throws PyExc_TypeError if the class cannot be found.
 */
JPClass* JPTypeManager::findClassByName(const string& name)
{
	JP_TRACE_IN("JPTypeManager::findClassByName");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (name.empty())
		JP_RAISE(PyExc_ValueError, "Empty class name passed to findClassByName");

	jvalue val;
	val.l = (jobject) frame.fromStringUTF8(name);
	auto* out = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassByName, &val));
	if (out == nullptr)
	{
		std::stringstream err;
		err << "Class " << name << " is not found";
		JP_RAISE(PyExc_TypeError, err.str());
	}
	return out;
	JP_TRACE_OUT;
}

/**
 * Finds a JPClass object for a given Java object instance.
 * 
 * @param obj - The Java object instance to find the class for.
 * @return A pointer to the JPClass object corresponding to the object's class.
 */
JPClass* JPTypeManager::findClassForObject(jobject obj)
{
	JP_TRACE_IN("JPTypeManager::findClassForObject");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (obj == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null object passed to findClassForObject");
	jvalue val;
	val.l = obj;
	auto *cls = (JPClass*) (frame.CallLongMethodA(m_JavaTypeManager.get(), m_FindClassForObject, &val));
	frame.check();
	JP_TRACE("ClassName", cls == nullptr ? "null" : cls->getCanonicalName());
	return cls;
	JP_TRACE_OUT;
}

/**
 * Populates a method with details from a Java executable object.
 * 
 * @param method - Pointer to the method to populate.
 * @param obj - The Java executable object to populate the method from.
 */
void JPTypeManager::populateMethod(void* method, jobject obj)
{
	JP_TRACE_IN("JPTypeManager::populateMethod");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (method == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null method pointer passed to populateMethod");
	if (obj == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null Java object passed to populateMethod");
	jvalue val[2];
	val[0].j = (jlong) method;
	val[1].l = obj;
	JP_TRACE("Method", method);
	frame.CallVoidMethodA(m_JavaTypeManager.get(), m_PopulateMethod, val);
	JP_TRACE_OUT;
}

/**
 * Populates the members of a JPClass object.
 * 
 * @param cls - The JPClass object to populate members for.
 */
void JPTypeManager::populateMembers(JPClass* cls)
{
	JP_TRACE_IN("JPTypeManager::populateMembers");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (cls == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null JPClass object passed to populateMembers");
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass();
	frame.CallVoidMethodA(m_JavaTypeManager.get(), m_PopulateMembers, val);
	JP_TRACE_OUT;
}

/**
 * Counts the number of parameters for a given class interface.
 * 
 * @param cls - The JPClass object representing the interface.
 * @return The number of parameters for the interface.
 */
int JPTypeManager::interfaceParameterCount(JPClass *cls)
{
	JP_TRACE_IN("JPTypeManager::interfaceParameterCount");
	JPJavaFrame frame = JPJavaFrame::outer();
	if (cls == nullptr)
		JP_RAISE(PyExc_RuntimeError, "Null JPClass object passed to interfaceParameterCount");
	jvalue val[1];
	val[0].l = (jobject) cls->getJavaClass();
	return frame.CallIntMethodA(m_JavaTypeManager.get(), m_InterfaceParameterCount, val);
	JP_TRACE_OUT;
}
