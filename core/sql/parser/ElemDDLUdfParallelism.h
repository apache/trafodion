/* -*-C++-*- */
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
#ifndef ELEMDDLUDFPARALLELISM_H
#define ELEMDDLUDFPARALLELISM_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfParallelism.h
* Description:  class for Udf Parallelism Attribute (parse node) elements in
*               DDL statements
*
*
* Created:      7/14/09
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ElemDDLNode.h"

class ElemDDLUdfParallelism : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdfParallelism(ComRoutineParallelism parallelismChoice);

  // virtual destructor
  virtual ~ElemDDLUdfParallelism(void);

  // cast
  virtual ElemDDLUdfParallelism * castToElemDDLUdfParallelism(void);

  // accessors
  inline const ComRoutineParallelism getParallelism(void) const
  {
    return parallelism_;
  }

  inline NABoolean getCanBeParallel(void) const
  {
    return parallelism_ NEQ COM_ROUTINE_NO_PARALLELISM;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  ComRoutineParallelism parallelism_;

}; // class ElemDDLUdfParallelism

#endif /* ELEMDDLUDFPARALLELISM_H */
