#ifndef STMTDDLSCHGRANTARRAY_H
#define STMTDDLSCHGRANTARRAY_H
/* -*-C++-*- 
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

    * File:         StmtDDLSchGrantArray.h
    * Description:  class for an array of pointers pointing to instances of
    *               class StmtDDLSchGrant
    *               
    * Created:      03/06/2007
    * Language:     C++
**********************************************************************/

#include "Collections.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
#include "StmtDDLSchGrant.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLSchGrantArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class StmtDDLSchGrantArray
// -----------------------------------------------------------------------
class StmtDDLSchGrantArray : public LIST(StmtDDLSchGrant *)
{
public:

  // constructor
  StmtDDLSchGrantArray(CollHeap *heap = PARSERHEAP())
   : LIST(StmtDDLSchGrant *)(heap)
  { }

  // virtual destructor
  virtual ~StmtDDLSchGrantArray();

private:

}; // class StmtDDLSchGrantArray

#endif // STMTDDLSCHGRANTARRAY_H
