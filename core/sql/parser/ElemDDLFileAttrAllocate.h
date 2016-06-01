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
#ifndef ELEMDDLFILEATTRALLOCATE_H
#define ELEMDDLFILEATTRALLOCATE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrAllocate.h
 * Description:  class for File Disk Space (parse node) elements in
 *               Allocate clause in DDL statements
 *
 *               
 * Created:      4/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrAllocate;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrAllocate
// -----------------------------------------------------------------------
class ElemDDLFileAttrAllocate : public ElemDDLFileAttr
{

public:

  enum { DEFAULT_EXTENTS_TO_ALLOCATE = 0 };

  // constructors
  ElemDDLFileAttrAllocate(ComSInt16 size);

  // virtual destructor
  virtual ~ElemDDLFileAttrAllocate();

  // cast
  virtual ElemDDLFileAttrAllocate * castToElemDDLFileAttrAllocate();

  // accessors
  inline const ComSInt16 getExtentsToAllocate() const;

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;


private:

  //
  // methods
  //

  NABoolean isLegalAllocateValue(ComSInt16 extentsToAllocate) const;
  void initializeExtentsToAllocate(ComSInt16 extentsToAllocate);

  //
  // data members
  //
  short extentsToAllocate_;

}; // class ElemDDLFileAttrAllocate

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrAllocate
// -----------------------------------------------------------------------

//
// accessors
//

inline const ComSInt16
ElemDDLFileAttrAllocate::getExtentsToAllocate() const
{
  return extentsToAllocate_;
}


#endif // ELEMDDLFILEATTRALLOCATE_H
