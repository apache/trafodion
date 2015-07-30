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
#ifndef ELEMDDLFILEATTRMAXSIZE_H
#define ELEMDDLFILEATTRMAXSIZE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrMaxSize.h
 * Description:  class for MaxSize File Attribute (parse node) elements
 *               in MaxSize clause in DDL statements
 *               
 * Created:      4/20/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComUnits.h"
#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrMaxSize;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrMaxSize
// -----------------------------------------------------------------------
class ElemDDLFileAttrMaxSize : public ElemDDLFileAttr
{

public:

  enum { DEFAULT_MAX_SIZE_IN_BYTES = COM_MAX_PART_SIZE_IN_BYTES };
  enum { DEFAULT_MAX_SIZE_UNIT = COM_BYTES };

  // constructors
  ElemDDLFileAttrMaxSize();
  ElemDDLFileAttrMaxSize(Lng32 maxSize);
  ElemDDLFileAttrMaxSize(Lng32 maxSize, ComUnits maxSizeUnit);

  // virtual destructor
  virtual ~ElemDDLFileAttrMaxSize();

  // cast
  virtual ElemDDLFileAttrMaxSize * castToElemDDLFileAttrMaxSize();

  // accessors
  inline ULng32 getMaxSize() const;
  inline ComUnits      getMaxSizeUnit() const;
  NAString             getMaxSizeUnitAsNAString() const;
  inline NABoolean isUnbounded() const;

  // methods for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

  //
  // methods
  //

  NABoolean isLegalMaxSizeValue(Lng32 maxSize) const;
  void initializeMaxSize(Lng32 maxSize);

  //
  // data members
  //

  NABoolean isUnbounded_;
  ULng32 maxSize_;
  ComUnits maxSizeUnit_;
  
}; // class ElemDDLFileAttrMaxSize

//
// helpers
//

void ParSetDefaultMaxSize(ULng32 &maxSize,
                          ComUnits &maxSizeUnit);

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrMaxSize
// -----------------------------------------------------------------------

//
// accessors
//

inline ULng32
ElemDDLFileAttrMaxSize::getMaxSize() const
{
  return maxSize_;
}

inline ComUnits
ElemDDLFileAttrMaxSize::getMaxSizeUnit() const
{
  return maxSizeUnit_;
}

inline NABoolean
ElemDDLFileAttrMaxSize::isUnbounded() const
{
  return isUnbounded_;
}

#endif // ELEMDDLFILEATTRMAXSIZE_H
