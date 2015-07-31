#ifndef EHJMPBUFNODE_H
#define EHJMPBUFNODE_H
/* -*-C++-*-
******************************************************************************
*
* File:         EHJmpBufNode.h
* Description:  class for records in the JmpBuf stack to support
*               exception handling
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


#include <setjmp.h>

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class EHExceptionJmpBufNode;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class EHExceptionTypeNode;

// -----------------------------------------------------------------------
// class for records of the shadow runtime stack
// -----------------------------------------------------------------------
class EHExceptionJmpBufNode
{

public:

  struct env
  {
    jmp_buf jmpBuf;
  } environment;
  
  // default constructor

  EHExceptionJmpBufNode(
       EHExceptionTypeNode * firstExceptionTypeNode = NULL,
       EHExceptionJmpBufNode * pPrevRecord = NULL)
    : firstExceptionTypeNode_(firstExceptionTypeNode), link_(pPrevRecord)
  {
  }

  // destructor
  //
  //   Use the destructor provided by the C++ compiler.
  //   We don't need to call this destructor when we call
  //   longjmp() to cut back the runtime stack.

  // accessors

  EHExceptionTypeNode *
  getExceptionTypeList() const
  {
    return firstExceptionTypeNode_;
  }

  EHExceptionJmpBufNode *
  getLink() const
  {
    return link_;
  }

  // mutators

  void setEnv(const env & envStruct);

  void
  setExceptionTypeList(EHExceptionTypeNode * pExceptionTypeList)
  {
    firstExceptionTypeNode_ = pExceptionTypeList;
  }

  void
  setLink(EHExceptionJmpBufNode * pPrevRecord)
  {
    link_ = pPrevRecord;
  }

private:

  // pointer pointing to a singular linked list containing
  // a list of types of exceptions associating with a try block
  
  EHExceptionTypeNode * firstExceptionTypeNode_;

  // pointer to the record (node) right below this record

  EHExceptionJmpBufNode * link_;

};  // class EHExceptionJmpBufNode

#endif // EHJMPBUFNODE_H
