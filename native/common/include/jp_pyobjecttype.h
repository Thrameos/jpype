/*
 * Copyright 2020 nelson85.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * File:   jp_pyobjecttype.h
 * Author: nelson85
 *
 * Created on June 30, 2020, 6:45 AM
 */
#ifndef _JPPYOBJECTCLASS_H_
#define _JPPYOBJECTCLASS_H_

class JPPyObjectType : public JPClass
{
public:
	JPPyObjectType(JPJavaFrame& frame,
			jclass clss,
			const string& name,
			JPClass* super,
			JPClassList& interfaces,
			jint modifiers);
	virtual ~JPPyObjectType();
	virtual JPPyObject convertToPythonObject(JPJavaFrame& frame, jvalue val, bool cast) override;

protected:
	jfieldID        m_SelfID;
} ;

#endif // _JPPYOBJECTCLASS_H_