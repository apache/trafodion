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
#ifndef ELEMDDLFILEATTRLOCKLENGTH_H
#define ELEMDDLFILEATTRLOCKLENGTH_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrLockLength.h
* Description:  class for Generic Lock Length (parse node) elements
*               in DDL statements
*
*               
* Created:      4/21/95
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrLockLength;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Generic Lock Length (parse node) elements in DDL statements
// -----------------------------------------------------------------------
class ElemDDLFileAttrLockLength : public ElemDDLFileAttr
{

public:

  // constructor
  ElemDDLFileAttrLockLength(unsigned short lockLengthInBytes)
  : ElemDDLFileAttr(ELM_FILE_ATTR_LOCK_LENGTH_ELEM),
  lockLengthInBytes_(lockLengthInBytes)
  { }

  // virtual destructor
  virtual ~ElemDDLFileAttrLockLength();

  // cast
  virtual ElemDDLFileAttrLockLength * castToElemDDLFileAttrLockLength();

  // accessor
  inline unsigned short getLockLength() const;

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;


private:

  unsigned short lockLengthInBytes_;

}; // class ElemDDLFileAttrLockLength

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrLockLength
// -----------------------------------------------------------------------
//
// accessor
//

inline unsigned short
ElemDDLFileAttrLockLength::getLockLength() const
{
  return lockLengthInBytes_;
}

#endif // ELEMDDLFILEATTRLOCKLENGTH_H
