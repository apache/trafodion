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
#ifndef ELEMDDLCOLREFARRAY_H
#define ELEMDDLCOLREFARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColRefArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class ElemDDLColRef
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
#include "ElemDDLColRef.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColRefArray;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Definition of class ElemDDLColRefArray
// -----------------------------------------------------------------------
class ElemDDLColRefArray : public LIST(ElemDDLColRef *)
{

public:

  // constructor
  ElemDDLColRefArray(CollHeap *heap = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLColRefArray();

  // see if the this ElemDDLColRefArray has other ElemDDLColRefArray
  // as a prefix.
  ComBoolean hasPrefix(ElemDDLColRefArray &other);

  // See if this columnName is in a ElemDDLColRefArray.  Returns the index,
  // -1 if not found.
  Int32 getColumnIndex(const NAString & columnName);

  // see if the this ElemDDLColRefArray contains other ElemDDLColRefArray.
  // The columns need not be in the same order.
  ComBoolean contains(ElemDDLColRefArray &other
                      ,Int32 &firstUnmatchedEntry);

  // see if the ElemDDLColRefArray matches the other ElemDDLColRefArray.
  // The columns need not be in the same order.
  ComBoolean matches(ElemDDLColRefArray &other) ;

  // see if the this ElemDDLColRefArray has the ElemDDLColRef as an entry.
  ComBoolean hasEntry(ElemDDLColRef &colRef);
 
  // Compare column names and their order with other.
  ComBoolean operator == (ElemDDLColRefArray &other);

  // Compare column names and their order with other.
  ComBoolean operator != (ElemDDLColRefArray &other);
private:

}; // class ElemDDLColRefArray

#endif /* ELEMDDLCOLREFARRAY_H */
