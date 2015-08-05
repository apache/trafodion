/* -*-C++-*-
******************************************************************************
*
* File:         HvTypes.h
* Description:  HostVar argument types for Parser
* Language:     C++
*
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
*
******************************************************************************
*/

#ifndef HVTYPES_H
#define HVTYPES_H

#include "Collections.h"
#include "NAString.h"
#include "NAType.h"

class HVArgType : public NABasicObject
{
public:
  HVArgType(NAString *name, NAType *type)
    : name_(name), type_(type),
      useCount_(0), intoCount_(0)
    {}

  const NAString *getName() const	{ return name_; }
  const NAType   *getType() const	{ return type_; }
        NAType   *getType()      	{ return type_; }
	Int32	 &useCount()		{ return useCount_; }
	Int32	 &intoCount()		{ return intoCount_; }

  // Methods needed for NAKeyLookup collections template

  const NAString *getKey() const	{ return name_; }

  NABoolean operator==(const HVArgType &other)
  { return *getName() == *other.getName() &&
  	   *getType() == *other.getType();
  }

private:
  NAString *name_;
  NAType   *type_;
  Int32 	   useCount_;
  Int32 	   intoCount_;
};

class HVArgTypeLookup : public NAKeyLookup<NAString,HVArgType>
{
public:
  #define HVARGTYPELKP_INIT_SIZE 29
  HVArgTypeLookup(CollHeap *h) :
    NAKeyLookup<NAString,HVArgType> (HVARGTYPELKP_INIT_SIZE,
    				     NAKeyLookupEnums::KEY_INSIDE_VALUE,
				     h)
    {}
  
  ~HVArgTypeLookup() { clearAndDestroy(); }
private:
};

#endif // HVTYPES_H
