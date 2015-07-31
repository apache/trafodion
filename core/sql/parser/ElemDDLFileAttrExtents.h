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
#ifndef ELEMDDLFILEATTREXTENTS_H
#define ELEMDDLFILEATTREXTENTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrExtents.h
 * Description:  class for Extents File Attribute (parse node) elements
 *               in Extents clause in DDL statements
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
class ElemDDLFileAttrExtents;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrExtents
// -----------------------------------------------------------------------
class ElemDDLFileAttrExtents : public ElemDDLFileAttr
{

public:

  enum { DEFAULT_PRI_EXTENT = COM_PRI_EXTENT };
  enum { DEFAULT_SEC_EXTENT = COM_SEC_EXTENT };
  
  // constructors
  ElemDDLFileAttrExtents();
  ElemDDLFileAttrExtents(Lng32 priExt);
  ElemDDLFileAttrExtents(Lng32 priExt, Lng32 secExt);

  // virtual destructor
  virtual ~ElemDDLFileAttrExtents();

  // cast
  virtual ElemDDLFileAttrExtents * castToElemDDLFileAttrExtents();

  // accessors
  inline ULng32 getPriExtents() const;
  inline ULng32 getSecExtents() const;

  // methods for tracing
 // virtual const NAString getText() const;
 // virtual const NAString displayLabel1() const;
 // virtual const NAString displayLabel2() const;
 // virtual const NAString displayLabel3() const;


private:

  //
  // methods
  //

  NABoolean isLegalExtentValue(Lng32 extent) const;
  void initializeExtents(Lng32 priExt, Lng32 secExt);
  void initializePriExtent(Lng32 priExt);

  //
  // data members
  //

  ULng32 priExt_;
  ULng32 secExt_;
  
}; // class ElemDDLFileAttrExtents

//
// helpers
//

void ParSetDefaultExtents(ULng32 &priExt,
                          ULng32 &secExt);
void ParSetDefaultPriExtent(ULng32 &priExt);

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrExtents
// -----------------------------------------------------------------------

//
// accessors
//

inline ULng32
ElemDDLFileAttrExtents::getPriExtents() const
{
  return priExt_;
}

inline ULng32
ElemDDLFileAttrExtents::getSecExtents() const
{
  return secExt_;
}

#endif // ELEMDDLFILEATTREXTENTS_H
