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
#ifndef ELEMDDLUDRISOLATE_H
#define ELEMDDLUDRISOLATE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdrIsolate.h
* Description:  class for Audit File Attribute (parse node) elements in
*               DDL statements
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

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdrIsolate : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdrIsolate(NABoolean theIsolate);

  // virtual destructor
  virtual ~ElemDDLUdrIsolate(void);

  // cast
  virtual ElemDDLUdrIsolate * castToElemDDLUdrIsolate(void);

  // accessor
  inline const NABoolean getIsolate(void) const
  {
    return isolate_;
  }

  //
  // methods for tracing
  //
  
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  NABoolean isolate_;

}; // class ElemDDLUdrIsolate

#endif /* ELEMDDLUDRISOLATE_H */
