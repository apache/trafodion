#ifndef ELEMDDLCOLDEFARRAY_H
#define ELEMDDLCOLDEFARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColDefArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class ElemDDLColDef
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


#include "Collections.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
#include "ElemDDLColDef.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColDefArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class ElemDDLColDefArray
// -----------------------------------------------------------------------
class ElemDDLColDefArray : public LIST(ElemDDLColDef *)
{

public:

  // constructor
  ElemDDLColDefArray(CollHeap *heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLColDefArray();

  // See if this columnName is in a ElemDDLColRefArray.  Returns the index,
  // -1 if not found.
  Int32 getColumnIndex(const NAString & internalColumnName);

  // Returns -1 if the colDefParseNodeArray does not contain any division columns.
  // All division columns (if exist) are at the end of the list.
  ComSInt32 getIndexToFirstDivCol() const;

private:

}; // class ElemDDLColDefArray

#endif /* ELEMDDLCOLDEFARRAY_H */
