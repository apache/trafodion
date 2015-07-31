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
#ifndef ITEMCONSTVALUEARRAY_H
#define ITEMCONSTVALUEARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ItemConstValueArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class ConstValue (defined in header file ItemColRef.h)
 *               
 *               
 * Created:      5/26/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Collections.h"
#include "ItemExpr.h"
#include "ItemColRef.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ItemConstValueArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of typedef ItemConstValueArray
// -----------------------------------------------------------------------
class ItemConstValueArray : public LIST(ConstValue *)
{

public:

  // constructor
  ItemConstValueArray(CollHeap *heap = PARSERHEAP())
    : LIST(ConstValue *)(heap)
  { }

  // virtual destructor
  virtual ~ItemConstValueArray();

private:

}; // class ItemConstValueArray
  

#endif // ITEMCONSTVALUEARRAY_H
