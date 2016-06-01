#ifndef ELEMDDLCOLNAMEARRAY_H
#define ELEMDDLCOLNAMEARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColNameArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class ElemDDLColName
 *               
 *               
 * Created:      5/26/95
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




#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "Collections.h"
#include "ElemDDLColName.h"
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColNameArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class NAMemory;

// -----------------------------------------------------------------------
// Definition of class ElemDDLColNameArray
// -----------------------------------------------------------------------
class ElemDDLColNameArray : public LIST(ElemDDLColName *)
{

public:

  // constructor
  ElemDDLColNameArray(NAMemory *heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLColNameArray();

private:

}; // class ElemDDLColNameArray

#endif /* ELEMDDLCOLNAMEARRAY_H */
