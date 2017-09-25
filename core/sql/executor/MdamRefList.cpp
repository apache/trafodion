/* -*-C++-*-
********************************************************************************
*
* File:         MdamRefList.C
* Description:  Implementation for MDAM Reference Lists
*               
*               
* Created:      6/28/96
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
#ifdef NA_MDAM_EXECUTOR_DEBUG
#include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */

#include "ex_stdh.h"
#include "NABoolean.h"
#include "MdamRefList.h"
#include "MdamRefListIterator.h"

#ifdef INCLUDETESTCODE
#include "testCounters.C"
#endif

// The following enum is used to initialize temporary disjunct number variables.
// The value, -1, is not special in any way.  In all cases, the initialized
// variable is assigned a value in a source code line that closely follows
// the initialization.  The intention of initialization is to promote repeatable
// behavior should there be a bug in the code that assigns a value.
enum {disjunctNumInitialValue = -1};

// *****************************************************************************
// Member functions for class MdamRefList
// *****************************************************************************

// this desctor is not called as the instance is part of MdamColumn class and
// would be constructed and destructed together.
//Destructor.
MdamRefList::~MdamRefList()
{
  // The destructor should only be called after deleteEntries has been
  // called to delete all the entries.  This destructor can't do this
  // because access to the heap manager is needed.
  ex_assert(isEmpty(),
    "MdamRefList::~MdamRefList() called for a non-empty reference list.");
}

// end of excluding the destructor from coverage checking.

// this operator was designed for completeness but is not currently used
// Compare two reference lists.  If they are equal, return true.
// Otherwise, return false. Comparision stops at first non-match.
NABoolean MdamRefList::operator==(const MdamRefList & otherList) const
{
  // Obtain the first disjunct number from each list.
  Int32 thisDisjunctNum = disjunctNumInitialValue;
  Int32 otherDisjunctNum = disjunctNumInitialValue;
  MdamRefListIterator thisIterator(this);
  MdamRefListIterator otherIterator(&otherList); 
  NABoolean thisMore = thisIterator(thisDisjunctNum);
  NABoolean otherMore = otherIterator(otherDisjunctNum);  
  // Compare disjunct numbers.  Stop when either list is exhausted
  while (thisMore && otherMore)
    {
      if (thisDisjunctNum == otherDisjunctNum)
        {
          // Disjunct numbers the same.  Advance both lists.
          thisMore  = thisIterator(thisDisjunctNum);
          otherMore = otherIterator(otherDisjunctNum);
        }
      else 
        {
          // Disjunct numbers differ.  So, return false.
          return FALSE;
        }
    }
  // Check if either list has additional entries.  If so, return false.
  if (thisMore || otherMore) return FALSE;
  // Lists must be equal so return true.
  return TRUE;
}

// end of excluding this equal operator from coverage checking

// Copy the entries from one reference list to another.
// Source list is otherList.  Target list is this list.
// The insert member function is used so list order is maintained
// and duplicates are discarded without error.
void MdamRefList::copyEntries(const MdamRefList & otherList,
			      FixedSizeHeapManager & mdamRefListEntryHeap)
{
  MdamRefListIterator iterator(&otherList);
  Int32 disjunctNum = disjunctNumInitialValue;
  while(iterator(disjunctNum))
    {
      insert(disjunctNum, mdamRefListEntryHeap);
    }
}


// This member function deletes all the Reference list entries from
// the list for which it was called.  The reference list private data member
// is updated to reflect an empty list.
void MdamRefList::deleteEntries(FixedSizeHeapManager & mdamRefListEntryHeap)
{
  MdamRefListIterator iterator(this);
  MdamRefListEntry * currentPtr = 0;
  while((currentPtr = iterator()) != 0)
    {
      mdamRefListEntryHeap.releaseElement(currentPtr);
    }
  lastEntryPtr_ = 0;
}


// Insert an entry into the reference list.
// This member function inserts a node into a circular linked list. Each node
// contains a unique disjunct number.  The nodes are maintained in ascending
// order by disjunct number.  A duplicate is discarded without error. 
MdamRefList & MdamRefList::insert(const Int32 disjunctNum,
				  FixedSizeHeapManager & mdamRefListEntryHeap)
{
  if (lastEntryPtr_ == 0)  // Empty list?
    {        
      // Empty list.  Allocate the first node.     
      lastEntryPtr_ = new(mdamRefListEntryHeap)
	MdamRefListEntry(disjunctNum);
    }         
  else
    {             
      // Non-empty list.  Position beforePtr for insertion.
      MdamRefListEntry * beforePtr
        = lastEntryPtr_->positionBeforePtr(disjunctNum);
      // A duplicate disjunct number is discarded without error.
      if (disjunctNum == beforePtr->getDisjunctNum()) {return *this;};
      // Allocate a new node.
      MdamRefListEntry * newPtr = new(mdamRefListEntryHeap)
	MdamRefListEntry(disjunctNum, beforePtr);
      // If the newly-inserted node is last, adjust lastEntryPtr_ to
      // point to it.
      if (lastEntryPtr_->getDisjunctNum() < newPtr->getDisjunctNum())
        {
          lastEntryPtr_ = newPtr;
        };
    }
    return *this;
}    


// Calculate the intersection of two reference lists.
void MdamRefList::intersect(const MdamRefList & refList0Ref,
                            const MdamRefList & refList1Ref,
			    FixedSizeHeapManager & mdamRefListEntryHeap)
{
  // Delete all the entries from this list.
  deleteEntries(mdamRefListEntryHeap);

  // Obtain the first disjunct number from each list.
  Int32 disjunctNum0 = disjunctNumInitialValue;
  Int32 disjunctNum1 = disjunctNumInitialValue;
  MdamRefListIterator iterator0(&refList0Ref);
  MdamRefListIterator iterator1(&refList1Ref); 
  NABoolean more0 = iterator0(disjunctNum0);
  NABoolean more1 = iterator1(disjunctNum1);
  
  // Iterate finding disjunct numbers common to both lists.
  // Stop when either list is exhausted.
  while (more0 && more1)
    {
      if (disjunctNum0 == disjunctNum1)
        {
          // Disjunct numbers are equal.  Insert into result list.
          insert(disjunctNum0, mdamRefListEntryHeap);
          // Advance both lists.
          more0 = iterator0(disjunctNum0);
          more1 = iterator1(disjunctNum1);
        }
      else if (disjunctNum0 < disjunctNum1)
        {
          // Disjunct number from list 0 is less.
          // Advance list 0. 
          more0 = iterator0(disjunctNum0);
        }
      else
        {
          // Disjunct number from list 1 is less.
          // Advance list 1. 
          more1 = iterator1(disjunctNum1);
        }
    }
}
 
// Determine if the intersection of two reference lists is empty.
NABoolean MdamRefList::intersectEmpty(const MdamRefList & otherList)
{
  // Obtain the first disjunct number from each list.
  Int32 thisDisjunctNum = disjunctNumInitialValue;
  Int32 otherDisjunctNum = disjunctNumInitialValue;
  MdamRefListIterator thisIterator(this);
  MdamRefListIterator otherIterator(&otherList); 
  NABoolean thisMore  = thisIterator(thisDisjunctNum);
  NABoolean otherMore = otherIterator(otherDisjunctNum);
  
  // Loop looking for a disjunct number common to both lists.
  // Stop when such a disjunct number is found or when
  // either list is exhausted.
  while (thisMore && otherMore)
    {
      if (thisDisjunctNum == otherDisjunctNum)
        {
          // Match!  Return false because the intersection is not empty.
          return FALSE;
        }
      else if (thisDisjunctNum < otherDisjunctNum)
        {
          // Disjunct number from this list is less.
          // Advance this list.
          thisMore = thisIterator(thisDisjunctNum);
        }
      else
        {
          // Disjunct number from otherList is less.
          // Advance otherList.
          otherMore = otherIterator(otherDisjunctNum);
        }
    }
  return TRUE;  // Return true because the intersection is empty.
}


// Determine if the intersection of three reference lists is empty.
// - this (which we will refer as refList0), refList1 and reflist2.
NABoolean MdamRefList::intersectEmpty(const MdamRefList & refList1,
				    const MdamRefList & refList2)
{

  // For readability purpose, let's refer to this pointer as reflist0.
  MdamRefList *refList0 = this;

  // Obtain the first disjunct number from each list.
  Int32 disjunctNum0 = disjunctNumInitialValue;
  Int32 disjunctNum1 = disjunctNumInitialValue;
  Int32 disjunctNum2 = disjunctNumInitialValue;

  MdamRefListIterator iterator0(refList0);
  MdamRefListIterator iterator1(&refList1); 
  MdamRefListIterator iterator2(&refList2); 

  NABoolean more0 = iterator0(disjunctNum0);
  NABoolean more1 = iterator1(disjunctNum1);
  NABoolean more2 = iterator2(disjunctNum2);
  
  // Iterate finding disjunct numbers common to all three lists.
  // Stop when one is found or when one of the refLists is exhausted.
  while (more0 && more1 && more2)
    {
      if (disjunctNum0 == disjunctNum1)
        {
	  if (disjunctNum0 == disjunctNum2)
	    return FALSE; // Found a match in refList0 refList1 and refList2!
	                  // intersection is non-empty.
  	  else if (disjunctNum0 < disjunctNum2)
	    {
	      // We already know disjunctNum0=disjunctNum1. 
	      // Advance both refList0 and refList1.
	      more0 = iterator0(disjunctNum0);
	      more1 = iterator1(disjunctNum1);
	    }
	  else 
	    {
	      // Must be disjunctNum2<disjunctNum0 and 
	      // disjunctNum2<disjunctNum1
	      more2 = iterator2(disjunctNum2);
	    }
	} 
      else if (disjunctNum0 < disjunctNum1)
	{
	  if (disjunctNum0 < disjunctNum2)
	    {
	      // Disjunct number from list 0 is less than 1 and 2.
	      // Advance list 0. 
	      more0 = iterator0(disjunctNum0);
	    }
	  else
	    {
	      // We know disjunctNum2 <= disjunctNum0 < disjunctNum1
	      more0 = iterator0(disjunctNum0);
	      more2 = iterator2(disjunctNum2);
	    }
	}
      else // (disjunctNum1 < disjunctNum0)
        {
	  if (disjunctNum1 < disjunctNum2)
	    {
	      // Disjunct number from list 1 is less than from 0 and 2.
	      // Advance list 1. 
	      more1 = iterator1(disjunctNum1);
	    }
	  else
	    {
	      // We know that disjunctNum2 <= disjunctNum1 < disjunctNum0 
	      more1 = iterator1(disjunctNum1);
	      more2 = iterator2(disjunctNum2);
	    }
        }
      
    }//While
  
  return TRUE; // Return true because the intersection is empty.
}


#ifdef NA_MDAM_EXECUTOR_DEBUG
// Print functions.
void MdamRefList::print(const char *header) const
{
  cout << endl << "Header: " << header << endl;
  MdamRefListIterator iterator(this);
  Int32 disjunctNum = disjunctNumInitialValue;
  while(iterator(disjunctNum))
    {
       cout << "  " << disjunctNum << endl;
    }
}

void MdamRefList::printBrief() const
{
  cout << "{";
  MdamRefListIterator iterator(this);
  Int32 disjunctNum = disjunctNumInitialValue;
  if (iterator(disjunctNum))
    {
      cout << disjunctNum;
      while(iterator(disjunctNum))
	{
	  cout << "," << disjunctNum;
	}
    }
  cout << "} ";
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */


// Calculate the union of two reference lists.
void MdamRefList::unionx(const MdamRefList & refList0Ref,
                         const MdamRefList & refList1Ref,
			 FixedSizeHeapManager & mdamRefListEntryHeap)
{
  // Delete all the entries from the target list.
  deleteEntries(mdamRefListEntryHeap);

  // Obtain the first disjunct number from each source list.
  Int32 disjunctNum0 = disjunctNumInitialValue;
  Int32 disjunctNum1 = disjunctNumInitialValue;
  MdamRefListIterator iterator0(&refList0Ref);
  MdamRefListIterator iterator1(&refList1Ref); 
  NABoolean more0 = iterator0(disjunctNum0);
  NABoolean more1 = iterator1(disjunctNum1);
  
  // Loop through both lists until either one is empty.
  while (more0 && more1)
    {
      if (disjunctNum0 == disjunctNum1)
        {
          // Disjunct numbers are equal.  Keep one copy.
          insert(disjunctNum0, mdamRefListEntryHeap);
          // Advance both lists.
          more0 = iterator0(disjunctNum0);
          more1 = iterator1(disjunctNum1);
        }
      else if (disjunctNum0 < disjunctNum1)
        {
          // Disjunct number from list 0 is less.
          // Insert it into the result and advance list 0. 
          insert(disjunctNum0, mdamRefListEntryHeap);
          more0 = iterator0(disjunctNum0);
        }
      else
        {
          // Disjunct number from list 1 is less.
          // Insert it into the result and advance list 1.  
          insert(disjunctNum1, mdamRefListEntryHeap);
          more1 = iterator1(disjunctNum1);
        }
    }

  // Copy any remaining disjunct numbers from refList0Ref.
  while (more0)
    {
      insert(disjunctNum0, mdamRefListEntryHeap);
      more0 = iterator0(disjunctNum0);
    }

  // Copy any remaining disjunct numbers from refList1Ref.
  while (more1)
    {
      insert(disjunctNum1, mdamRefListEntryHeap);
      more1 = iterator1(disjunctNum1);
    }
} 
