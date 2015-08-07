#ifndef EHEXCEPTIONTYPENODE_H
#define EHEXCEPTIONTYPENODE_H
/* -*-C++-*-
******************************************************************************
*
* File:         EHExceptionTypeNode.h
* Description:  class for a node in a singular linked list.  The node
*               contains the type of an exception associating with a
*               try block.
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
#include "EHExceptionTypeEnum.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class EHExceptionTypeNode;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// class for a node in a singular linked list.  The node contains the
// type of an exception associating with a try block.
// -----------------------------------------------------------------------
class EHExceptionTypeNode
{

public:

  // default constructor

  EHExceptionTypeNode(EHExceptionTypeEnum exceptionType = EH_NORMAL,
                      EHExceptionTypeNode * pNextNode = NULL)
    : exceptionType_(exceptionType),
      pNextNode_(pNextNode)
  {
  }

  // destructor
  //
  //   Use the destructor provided by the C++ compiler.
  //   We don't need to call this destructor when we call
  //   longjmp() to cut back the runtime stack.


  // accessors

  EHExceptionTypeEnum
  getExceptionType() const
  {
    return exceptionType_;
  }
  
  EHExceptionTypeNode *
  getNextNode() const
  {
    return pNextNode_;
  }

  // mutators

  void
  setExceptionType(EHExceptionTypeEnum exceptionType)
  {
    exceptionType_ = exceptionType;
  }

  void
  setNextNode(EHExceptionTypeNode * pNextNode)
  {
    pNextNode_ = pNextNode;
  }

private:

  // data

  EHExceptionTypeEnum exceptionType_;

  // link

  EHExceptionTypeNode * pNextNode_;

};  // class EHExceptionTypeNode

#endif // EHEXCEPTIONTYPENODE_H
