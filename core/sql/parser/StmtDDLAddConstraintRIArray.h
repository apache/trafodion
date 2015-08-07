#ifndef STMTDDLADDCONSTRAINTRIARRAY_H
#define STMTDDLADDCONSTRAINTRIARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAddConstraintRIArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class StmtDDLAddConstraintRI
 *               
 *               
 * Created:      6/21/95
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


#include "Collections.h"
#include "StmtDDLAddConstraintRI.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLAddConstraintRIArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class StmtDDLAddConstraintRIArray
// -----------------------------------------------------------------------
class StmtDDLAddConstraintRIArray : public LIST(StmtDDLAddConstraintRI *)
{

public:

  // constructor
  StmtDDLAddConstraintRIArray(CollHeap *heap = PARSERHEAP())
    : LIST(StmtDDLAddConstraintRI *)(heap)
  {
  }

  // virtual destructor
  virtual ~StmtDDLAddConstraintRIArray();

private:

}; // class StmtDDLAddConstraintRIArray

#endif /* STMTDDLADDCONSTRAINTRIARRAY_H */
