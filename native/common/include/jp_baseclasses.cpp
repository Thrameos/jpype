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
#ifndef _JPBASECLASS_H_
#define _JPBASECLASS_H_

/**
 * Wrapper for Class<java.lang.Object>
 *
 * Primitive types can implicitely cast to this type as
 * well as class wrappers, thus we need a specialized
 * wrapper.  This class should not be used outside of
 * the JPTypeManager.
 *
 */
class JPObjectSpecializationClass : public JPObjectClass
{
public :
	JPObjectSpecializationClass();
	virtual~ JPObjectSpecializationClass();

public : // JPClass implementation
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
};

/**
 * Wrapper for Class<java.lang.Class>
 *
 * Class wrappers need to be able to cast to this type,
 * thus we need a specialized version.
 * This class should not be used outside of
 * the JPTypeManager.
  */
class JPClassBaseClass : public JPObjectClass
{
public :
	JPClassBaseClass();
	virtual~ JPClassBaseClass();

public : // JPClass implementation
	virtual EMatchType canConvertToJava(HostRef* obj);
	virtual jvalue     convertToJava(HostRef* obj);
};

#endif // _JPBASECLASS_H_
