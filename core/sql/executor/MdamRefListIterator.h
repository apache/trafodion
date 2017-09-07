#ifndef MDAMREFLISTITERATOR_H
#define MDAMREFLISTITERATOR_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamRefListIterator.h
* Description:  MDAM Reference List Iterator
*               
*               
* Created:      9/11/96
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
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "MdamRefList.h"

// *****************************************************************************
// MdamRefListIterator is an iterator class for an Mdam Reference List.
// Two iteration member functions are provided.
// The first returns a boolean to indicate the success of the interation
// call and, if successful, returns the disjunct number by reference.
// The second returns a pointer.  If the iteration call is not successful,
// the pointer is null.
// This iterator is safe to use even if the reference list entries are
// being deleted.
// *****************************************************************************
class MdamRefListIterator 
{

public:

  // Constructor.
  MdamRefListIterator(const MdamRefList * RefListPtr);
  
  // Iteration operator returns a boolean that indicates if there was or
  // was not a "next" entry and the corresponding disjunct number.
  NABoolean operator () (Int32 & disjunctNum_);

  // Iteration operator returns a pointer to the next entry.
  MdamRefListEntry * operator()();

private:

  // Pointer to the reference list.
  const MdamRefList * currentListPtr_;

  // Pointer to the current entry on the list.
  MdamRefListEntry * currentEntryPtr_;

}; // class MdamRefListIterator


#endif /* MDAMREFLISTITERATOR_H */

