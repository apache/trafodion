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
#ifndef STMTDDLCREATEVIEWARRAY_H
#define STMTDDLCREATEVIEWARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateViewArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class StmtDDLCreateView
 *               
 *               
 * Created:      1/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Collections.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
#include "StmtDDLCreateView.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateViewArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class StmtDDLCreateViewArray
// -----------------------------------------------------------------------
class StmtDDLCreateViewArray : public LIST(StmtDDLCreateView *)
{

public:

  // constructor
  StmtDDLCreateViewArray(CollHeap *heap = PARSERHEAP())
   : LIST(StmtDDLCreateView *)(heap)
  { }

  // virtual destructor
  virtual ~StmtDDLCreateViewArray();

private:

}; // class StmtDDLCreateViewArray
 

#endif // STMTDDLCREATEVIEWARRAY_H
