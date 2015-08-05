/* -*-C++-*-
********************************************************************************
*
* File:         MdamRefListEntry.C
* Description:  Implementation for MDAM Reference List Entry
*               
*               
* Created:      9/12/96
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

#include "ex_stdh.h"
#include "MdamRefListEntry.h"

// *****************************************************************************
// Member functions for class MdamRefListEntry
// *****************************************************************************

// Position the beforePtr to prepare for an insertion.
MdamRefListEntry * MdamRefListEntry::positionBeforePtr(const Int32 disjunctNum)
{
  // Assume the new node will go between the head and the tail.
  MdamRefListEntry * beforePtr = this;
  // Check if the above assumption is incorrect.
  if (disjunctNum >= nextEntryPtr_->disjunctNum_ && disjunctNum < disjunctNum_)
    {
      // Move the beforePtr so that it points to the node that immediately
      // preceeds the new node's location.
      // If disjunctNum is already in the list (that is, the insertion is a
      // duplicate), beforePtr will end up pointing to the node with the
      // duplicated value.
      beforePtr = beforePtr->nextEntryPtr_;
      while (disjunctNum >= beforePtr->nextEntryPtr_->disjunctNum_)
        {
          beforePtr = beforePtr->nextEntryPtr_;
        };
    };
  return beforePtr;
}
