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
/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalList.C
* Description:  Implimentation for MDAM Interval List
*               
*               
* Created:      9/12/96
* Language:     C++
*
*
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "ex_stdh.h"
#if defined ( NA_MDAM_EXECUTOR_DEBUG )
  #include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */
#include "MdamEndPoint.h"
#include "MdamIntervalList.h"
#include "MdamIntervalListIterator.h"
#include "MdamIntervalListMerger.h"

#if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  #include <stdio.h>
#endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

// *****************************************************************************
// Member functions for class MdamIntervalList
// *****************************************************************************

// Constructor.
#if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
MdamIntervalList::MdamIntervalList(const Lng32 callerTag)
         : firstIntervalPtr_(0), lastIntervalPtr_(0),
           intervalListId_(NA_JulianTimestamp())
{
  logEvent(5,0,-1,callerTag);
}
#else 

MdamIntervalList::MdamIntervalList() 
         : firstIntervalPtr_(0), lastIntervalPtr_(0)
{}
#endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */


// Destructor.
MdamIntervalList::~MdamIntervalList()
{
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  logEvent(6);
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // This destructor should only be called after deleteAllIntervals() has been
  // called to delete all the intervals.  This destructor can't do this
  // because access to the heap manager is needed.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  ex_assert(isEmpty(),
  "MdamIntervalList::~MdamIntervalList() called for a non-empty interval list.");
  #endif /* NA_MDAM_EXECUTOR_DEBUG */
}

// Appends an MdamInterval onto an MdamIntervalList.  Warning: caller
// must ensure that the new interval is disjoint and in order.  No
// integrity checks are performed.
MdamIntervalList & MdamIntervalList::append(MdamInterval * newIntervalPtr)
{

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  // logEvent(4,1);
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  if (firstIntervalPtr_ == 0)  // Empty list?
    {
      firstIntervalPtr_ = newIntervalPtr;
    }
  else
    {
      lastIntervalPtr_->setNextMdamIntervalPtr(newIntervalPtr);
    };
  lastIntervalPtr_ = newIntervalPtr;
  newIntervalPtr->setNextMdamIntervalPtr(0);
  return *this;
}


#if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
Lng32 MdamIntervalList::countIntervals() const
{
  Lng32 intervalCount = 0;
  MdamIntervalListIterator iterator(*this);
  while(iterator() != 0)
    {
      intervalCount++;
    };
  return intervalCount;
}
#endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

// Delete all the intervals in the list.
void MdamIntervalList::deleteAllIntervals
       (FixedSizeHeapManager & mdamIntervalHeap,
        FixedSizeHeapManager & mdamRefListEntryHeap)
{
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  logEvent(1,-countIntervals());
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  MdamIntervalListIterator iterator(*this);
  MdamInterval * intervalPtr = 0;
  while((intervalPtr = iterator()) != 0)
    {
      intervalPtr->release(mdamRefListEntryHeap);  // release the interval's resources
      mdamIntervalHeap.releaseElement(intervalPtr); // return interval to free list
    };
  firstIntervalPtr_ = 0;
  lastIntervalPtr_ = 0;
}


// Give all intervals from this list and put them in otherList.
// this list becomes empty.
void MdamIntervalList::giveAllIntervals(MdamIntervalList & otherList)
{
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  // Log for this list giving intervals.
  logEvent(2,-countIntervals(),otherList.intervalListId_);
  // Log for otherList receiving intervals.
  otherList.logEvent(3,countIntervals(),intervalListId_);
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  otherList.firstIntervalPtr_ = firstIntervalPtr_;
  otherList.lastIntervalPtr_ = lastIntervalPtr_;
  firstIntervalPtr_ = 0;
  lastIntervalPtr_ = 0;
}


// This function inserts a single disjunct number into the reference list
// associated with each MdamInterval on the MdamIntervalList.
void MdamIntervalList::insertDisjunctNum(const Int32 disjunctNum,
			     FixedSizeHeapManager & mdamRefListEntryHeap)
{
  MdamIntervalListIterator iterator(*this);
  MdamInterval * intervalPtr = 0;
  while((intervalPtr = iterator()) != 0)
    {
      intervalPtr->insertDisjunctNum(disjunctNum, mdamRefListEntryHeap);
    };
}


// Performs an intersect operation on two interval lists to produce a
// result list.  The this list and the otherList are inputs to the
// intersect operation.  The result replaces the this list.  The
// original interval list entries for the this list are deleted.
MdamIntervalList & MdamIntervalList::intersect
                (const MdamIntervalList & otherList,
                 const ULng32 keyLen,
		 FixedSizeHeapManager & mdamIntervalHeap,
                 FixedSizeHeapManager & mdamRefListEntryHeap)
{
  // Move entries from this into tempIntervalList.
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  MdamIntervalList tempIntervalList(1);
  #else
  MdamIntervalList tempIntervalList;
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  giveAllIntervals(tempIntervalList);

  MdamIntervalListMerger getNextPoint(&tempIntervalList,
                                      &otherList,
                                      keyLen);
  MdamEndPoint endPoint1;
  MdamEndPoint endPoint2;
  getNextPoint(endPoint1);
  while(endPoint1.exists())
    {
      if (getNextPoint.getActiveIntervals() == 2)
        {
          getNextPoint(endPoint2);
          MdamInterval * tempIntervalPtr = new(mdamIntervalHeap)
	    MdamInterval(endPoint1, endPoint2);
          append(tempIntervalPtr);
          endPoint1 = endPoint2;
        }
      else
        {
          getNextPoint(endPoint1);
        };  // end if
    };  // end while

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  // Log entry for intervals resulting from the intersect operation.
  logEvent(7,countIntervals(),otherList.intervalListId_);
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // Delete the original this list entries because the destructor
  // can't.
  tempIntervalList.deleteAllIntervals(mdamIntervalHeap,
                                      mdamRefListEntryHeap);

  return *this;
}

#if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
void MdamIntervalList::logEvent(const Lng32 functionId,
                                const Lng32 intervalCount,
                                const Int64 otherListId,
                                const Lng32 callerTag) const
{
  Int64 eventTimestamp = NA_JulianTimestamp();

  printf("  %I64i  %I64i  %li  %li  %I64i  %li\n",
    eventTimestamp,
    intervalListId_,
    functionId,
    intervalCount,
    otherListId,
    callerTag);
}
#endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */


// Print functions.
#if defined ( NA_MDAM_EXECUTOR_DEBUG )
void MdamIntervalList::print(const char * header) const
{
  cout << header << endl;
  MdamIntervalListIterator iterator(*this);
  MdamInterval * intervalPtr = 0;
  while((intervalPtr = iterator()) != 0)
    {
      intervalPtr->print("Interval...");
    };
}

void MdamIntervalList::printBrief() const 
{
  MdamIntervalListIterator iterator(*this);
  MdamInterval * intervalPtr = 0;
  while((intervalPtr = iterator()) != 0)
    {
      intervalPtr->printBrief();
    };
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */


// Performs a union operation on two interval lists to produce a
// result list.  The list's intervals may have different
// MdamRefList*s associated with them.  Any reference lists
// associated with otherList are ignored and disjunctNum is used
// instead.  The this list and the otherList are inputs to the
// intersect operation.  The result replaces the this list.  The
// original interval list entries for the this list are deleted.
MdamIntervalList & MdamIntervalList::unionSeparateDisjuncts
                          (const MdamIntervalList & otherList,
                           const Int32 disjunctNum,
                           const ULng32 keyLen,
			   FixedSizeHeapManager & mdamIntervalHeap,
			   FixedSizeHeapManager & mdamRefListEntryHeap)
{
  // Move entries from this into tempIntervalList.
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  MdamIntervalList tempIntervalList(2);
  #else
  MdamIntervalList tempIntervalList;
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  giveAllIntervals(tempIntervalList);

  MdamIntervalListMerger getNextPoint(&tempIntervalList,
                                      &otherList,
                                      keyLen);
  MdamEndPoint currentEndPoint;
  MdamEndPoint previousEndPoint;

  // Track the active interval for each interval list.  Zero means
  // there is no active interval.  This mechanism permits access
  // to the corresponding reference list.
  MdamInterval * intervalPtr0 = 0;
  MdamInterval * intervalPtr1 = 0;

  while(getNextPoint(currentEndPoint))
    {
      if (getNextPoint.getPreviousActiveIntervals() != 0
          && currentEndPoint.givesNonEmptyInterval(&previousEndPoint,keyLen))
        {
	  // Create a new interval.  The constructor will reverse the 
	  // inclusion of end endpoint, if appropriate, and build
	  // the reference list.
          MdamInterval * tempIntervalPtr = new(mdamIntervalHeap) 
	    MdamInterval(previousEndPoint,
			 currentEndPoint,
			 intervalPtr0,
			 intervalPtr1,
			 disjunctNum,
			 mdamRefListEntryHeap);
          // Append the new interval onto this list.
          append(tempIntervalPtr);
        }; // end if

      // Adjust active reference lists.
      currentEndPoint.adjustIntervalPtr(intervalPtr0, 0);
      currentEndPoint.adjustIntervalPtr(intervalPtr1, 1);

      // Save the current endpoint.
      previousEndPoint = currentEndPoint;
      if (currentEndPoint.end()){previousEndPoint.reverseInclusion();};
    }; // end while

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  // Log entry for intervals resulting from the union operation.
  logEvent(8,countIntervals(),otherList.intervalListId_);
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // Delete the original this list entries because the destructor
  // can't.
  tempIntervalList.deleteAllIntervals(mdamIntervalHeap,
                                      mdamRefListEntryHeap);

  return *this;
}


// Performs an union operation on two interval lists to produce a
// result list.  The source lists have no MdamRefLists associated
// with them.  The this list and the otherList are inputs to the
// union operation.  The result replaces the this list.  The
// original interval list entries for this list are deleted.
MdamIntervalList & MdamIntervalList::unionSameDisjunct
                    (const MdamIntervalList & otherList,
                     const ULng32 keyLen,
		     FixedSizeHeapManager & mdamIntervalHeap,
                     FixedSizeHeapManager & mdamRefListEntryHeap)
{
  // Move entries from this into tempIntervalList.
  MdamIntervalList tempIntervalList;
  giveAllIntervals(tempIntervalList);

  MdamIntervalListMerger getNextPoint(&tempIntervalList,
                                      &otherList,
                                      keyLen);
  MdamEndPoint endPoint1;
  MdamEndPoint endPoint2;
  getNextPoint(endPoint1);

  // If endPoint1 exists, it is a BEGIN endpoint.
  while(endPoint1.exists())
    {
      while(getNextPoint.getActiveIntervals() >= 1)
        {
          getNextPoint(endPoint2);
        };  // end while
      // endPoint2 is an END endpoint. 
      MdamInterval * tempIntervalPtr = new(mdamIntervalHeap)
	MdamInterval(endPoint1, endPoint2);
      append(tempIntervalPtr);
      getNextPoint(endPoint1);
    };  // end while

  // Delete the original this list entries because the destructor
  // can't.
  tempIntervalList.deleteAllIntervals(mdamIntervalHeap,
                                      mdamRefListEntryHeap);

  return *this;
}

