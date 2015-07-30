#ifndef EHJMPBUFSTACK_H
#define EHJMPBUFSTACK_H
/* -*-C++-*-
******************************************************************************
*
* File:         EHJmpBufStack.h
* Description:  class for the shadow runtime stack
*
*               
* Created:      5/16/95
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


#include "EHBaseTypes.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class EHExceptionJmpBufStack;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class EHExceptionJmpBufNode;

// -----------------------------------------------------------------------
// class for shadow runtime stack
// -----------------------------------------------------------------------
class EHExceptionJmpBufStack
{
public:

  // constructor
  EHExceptionJmpBufStack() : pTopNode_(NULL)
  {
  }
  
  // virtual destructor
  virtual ~EHExceptionJmpBufStack();

  // push
  //
  //   pJmpBufNode must point to a space allocated
  //   via the new operator
  //
  void push(EHExceptionJmpBufNode * pJmpBufNode);

  // pop
  // 
  //   the pointer returned by pop() points to space
  //   allocated via the new operator
  //
  EHExceptionJmpBufNode * pop();

private:

  EHExceptionJmpBufNode * pTopNode_;
  
};  // class EHExceptionJmpBufStack

#endif // EHJMPBUFSTACK_H
