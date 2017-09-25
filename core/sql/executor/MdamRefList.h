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
#ifndef MDAMREFLIST_H
#define MDAMREFLIST_H
 
/* -*-C++-*-
********************************************************************************
*
* File:         MdamRefList.h
* Description:  MDAM Reference List 
*               
*               
* Created:      5/21/96
* Language:     C++
*
*
*
*
********************************************************************************
*/
// -----------------------------------------------------------------------------

#include "MdamRefListEntry.h"
#include "NABoolean.h"

// *****************************************************************************
// MdamRefList - MDAM Reference List
// MdamRefList represents a list of disjunct numbers. A disjunct number is
// is stored in a class MdamRefListEntry object.  The MdamRefListEntry class
// object also contains a pointer that is used to form a circular, linked
// list.  The present class, MdamRefList, has a pointer to the last entry.
// Invariant relations:
//   + A list is maintained in ascending order by disjunct number.
//   + A list contains no duplicates.
// *****************************************************************************

// Forward declarations.
class FixedSizeHeapManager;
// End of forward declarations.

class MdamRefList
{

  friend class MdamRefListIterator;

  public:

    // Default constructor.  Creates an empty list.
    MdamRefList() : lastEntryPtr_(0) {}

    // Constructor.  Creates a list with one entry.
    inline MdamRefList(const Int32 disjunctNum,
				  FixedSizeHeapManager & mdamRefListEntryHeap);

    // Copy constructor is not supported.

    // Destructor.
    ~MdamRefList();

    // Assignment operator is not supported.

// this operator was defined for completeness but is not currently used
    // Test for equality.
    NABoolean operator==(const MdamRefList & otherList) const;
// end of excluding this equal operator from coverage checking

    // Copy the entries from one reference list to another.
    // Source list is otherList.  Target list is this list.
    void copyEntries(const MdamRefList & otherList,
				FixedSizeHeapManager & mdamRefListEntryHeap);

    // Delete all reference list entries.
    void deleteEntries(FixedSizeHeapManager & mdamRefListEntryHeap);

    // Insert an entry into the reference list.
    MdamRefList & insert(const Int32 disjunctNum,
				    FixedSizeHeapManager & mdamRefListEntryHeap);

    // Calculate the intersection of two reference lists.
    void intersect(const MdamRefList & refList0Ref,
			      const MdamRefList & refList1Ref,
			      FixedSizeHeapManager & mdamRefListEntryHeap);

    // Determine if the intersection of two reference lists is empty.
    NABoolean intersectEmpty(const MdamRefList & otherList);


    // Determine if the intersection of three reference lists (this, refList1
    // and refList2) is empty.
    NABoolean intersectEmpty(const MdamRefList & refList1,
					const MdamRefList & refList2);

    // Determine if the list is empty.
    inline NABoolean isEmpty() const;

    // Print functions.
    #ifdef NA_MDAM_EXECUTOR_DEBUG
    void print(const char * header = "") const;
    void printBrief() const;
    #endif /* NA_MDAM_EXECUTOR_DEBUG */

    // Calculate the union of two reference lists.
    void unionx(const MdamRefList & refList0Ref,
			   const MdamRefList & refList1Ref,
			   FixedSizeHeapManager & mdamRefListEntryHeap);

private:

    // Pointer to the last MDAM Reference List entry, if any.
    MdamRefListEntry * lastEntryPtr_;

    // Assignment operator is not supported.
    MdamRefList & operator=(const MdamRefList & otherList);


};  // class MdamRefList


// *****************************************************************************
// Inline member functions for class MdamRefList
// *****************************************************************************

// Constructor.  Creates a list with one entry.
inline MdamRefList::MdamRefList(const Int32 disjunctNum,
				FixedSizeHeapManager & mdamRefListEntryHeap)
  : lastEntryPtr_(0)
{
  insert(disjunctNum, mdamRefListEntryHeap);
}

// this method is only called in the destruct, see MdamRefList.cpp for reason
// Determine if the list is empty.
inline NABoolean MdamRefList::isEmpty() const
{
  return lastEntryPtr_ == 0;
}

// end of excluding isEmpty from coverage checking
#endif /* MDAMREFLIST_H */
