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
#ifndef ELEMDDLFILEATTRMAXEXTENTS_H
#define ELEMDDLFILEATTRMAXEXTENTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrMaxExtents.h
 * Description:  class for MaxExtents File Attribute (parse node) elements
 *               in MaxExtents clause in DDL statements
 *               
 * Created:      09/12/01 
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
class ElemDDLFileAttrMaxExtents;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrExtents
// -----------------------------------------------------------------------
class ElemDDLFileAttrMaxExtents : public ElemDDLFileAttr
{

public:

  enum { DEFAULT_MAX_EXTENT = COM_MAX_EXTENT };
  
  // constructors
  ElemDDLFileAttrMaxExtents();
  ElemDDLFileAttrMaxExtents(Lng32 maxExtent);

  // virtual destructor
  virtual ~ElemDDLFileAttrMaxExtents();

  // cast
  virtual ElemDDLFileAttrMaxExtents * castToElemDDLFileAttrMaxExtents();

  // accessors
  inline ULng32 getMaxExtents() const;

  // methods for tracing
//  virtual const NAString getText() const;
//  virtual const NAString displayLabel1() const;
//  virtual const NAString displayLabel2() const;
//  virtual const NAString displayLabel3() const;


private:

  //
  // methods
  //

  NABoolean isLegalMaxExtentValue(Lng32 maxExtent) const;
  void initializeMaxExtents(Lng32 maxExtent);

  //
  // data members
  //

  ULng32 maxExt_;
  
}; // class ElemDDLFileAttrMaxExtents

//
// helpers
//

void ParSetDefaultMaxExtents(ULng32 &maxExt);

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrMaxExtents
// -----------------------------------------------------------------------

//
// accessors
//

inline ULng32
ElemDDLFileAttrMaxExtents::getMaxExtents() const
{
  return maxExt_;
}


#endif // ELEMDDLFILEATTRMAXEXTENTS_H
