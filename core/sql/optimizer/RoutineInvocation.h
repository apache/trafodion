/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef ROUTINEINVOCATION_H
#define ROUTINEINVOCATION_H
#include "NABasicObject.h"

class RoutineInvocation;

class RoutineInvocation : public NABasicObject
{
public:
    RoutineInvocation ( void );
    
    inline RoutineInvocation ( NAString *name, Int32 type, Int32 argc )
    {
	routineName_ = name;
	routineType_ = type;
	numArgs_ = argc;
    }
    
    ~RoutineInvocation ();

    const inline NAString &getRoutineName ()
    {
	return routineName_;
    }

    inline Int32 getRoutineType ()
    {
	return routineType_;
    }

    inline Int32 getNumArgs ()
    {
	return numArgs_;
    }

    inline void setRoutineName ( NAString *name )
    {
	routineName_ = name;
    }

    inline void setRoutineType ( Int32 type )
    {
	routineType_ = type;
    }

    inline void setNumArgs ( Int32 argc )
    {
	numArgs_ = args;
    }
    
private:
    NAString	*routineName_;
    Int32		routineType_;
    Int32		numArgs_;
}

#endif



