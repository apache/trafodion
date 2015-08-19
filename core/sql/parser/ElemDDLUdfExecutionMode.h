/* -*-C++-*- */
#ifndef ELEMDDLUDFEXECUTIONMODE_H
#define ELEMDDLUDFEXECUTIONMODE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfExecutionMode.h
* Description:  class for routine execution mode (parse node)
*               elements in DDL statements
*
*
* Created:      1/19/2010
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
******************************************************************************
*/

#include "ElemDDLNode.h"

class ElemDDLUdfExecutionMode : public ElemDDLNode
{

public:

  // constructor
  ElemDDLUdfExecutionMode(ComRoutineExecutionMode theExecutionMode);

  // virtual destructor
  virtual ~ElemDDLUdfExecutionMode(void);

  // cast
  virtual ElemDDLUdfExecutionMode * castToElemDDLUdfExecutionMode(void);

  // accessor
  inline const ComRoutineExecutionMode getExecutionMode(void) const
  {
    return executionMode_;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  ComRoutineExecutionMode executionMode_;

}; // class ElemDDLUdfExecutionMode

#endif /* ELEMDDLUDFEXECUTIONMODE_H */
