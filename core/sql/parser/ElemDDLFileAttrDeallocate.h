#ifndef ELEMDDLFILEATTRDEALLOCATE_H
#define ELEMDDLFILEATTRDEALLOCATE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrDeallocate.h
 * Description:  class representing Deallocate file attribute clause
 *               in DDL statements
 *
 *               
 * Created:      9/29/95
 * Language:     C++
 *
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
 *
 *****************************************************************************
 */


#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrDeallocate;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrDeallocate
// -----------------------------------------------------------------------
class ElemDDLFileAttrDeallocate : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrDeallocate();

  // virtual destructor
  virtual ~ElemDDLFileAttrDeallocate();

  // cast
  virtual ElemDDLFileAttrDeallocate * castToElemDDLFileAttrDeallocate();

  // methods for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLFileAttrDeallocate

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrDeallocate
// -----------------------------------------------------------------------

// constructor
inline
ElemDDLFileAttrDeallocate::ElemDDLFileAttrDeallocate()
: ElemDDLFileAttr(ELM_FILE_ATTR_DEALLOCATE_ELEM)
{
}

#endif // ELEMDDLFILEATTRDEALLOCATE_H
