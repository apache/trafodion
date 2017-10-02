/* -*-C++-*-
******************************************************************************
*
* File:         EHJmpBufStack.C
* Description:  member functions of class EHExceptionJmpBufStack
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


#include <stdio.h> 
#include "EHCommonDefs.h"


#include "EHJmpBufNode.h"
#include "EHJmpBufStack.h"

// -----------------------------------------------------------------------
// methods for class EHExceptionJmpBufStack
// -----------------------------------------------------------------------

// virtual destructor
EHExceptionJmpBufStack::~EHExceptionJmpBufStack()
{
}

// push
//
//   pJmpBufNode must point to a space allocated
//   via the new operator
//
void
EHExceptionJmpBufStack::push(EHExceptionJmpBufNode * pJmpBufNode)
{
  EH_ASSERT(pJmpBufNode->getLink() == NULL);

  pJmpBufNode->setLink(pTopNode_);
  pTopNode_ = pJmpBufNode;
}

// pop
// 
//   the pointer returned by pop() points to space allocated
//   via the new operator
//
EHExceptionJmpBufNode *
EHExceptionJmpBufStack::pop()
{
  EHExceptionJmpBufNode * pNode = pTopNode_;

  if (pTopNode_ != NULL)
    pTopNode_ = pNode->getLink();
  return pNode;
}
