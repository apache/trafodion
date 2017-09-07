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
#ifndef MDAMINTERVALLIST_H
#define MDAMINTERVALLIST_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalList.h
* Description:  MDAM Interval List
*               
*               
* Created:      9/11/96
* Language:     C++
*
*
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "NABoolean.h"

#if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  #include "Int64.h"
  #include "ExCextdecs.h"
#endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

// *****************************************************************************
// MdamIntervalList : MDAM Interval List
// An MdamIntervalList is a singlely-linked list of MDAM intervals.
// An invariant property of an MdamIntervalList is that it is a list
// of disjoint intervals in order.  Ordering is by the beginEndPoint value.
// When two intervals have the same value for beginEndPoint, ordering
// is determined by inclusion.  An interval with an included beginEndPoint
// preceeds one with an excluded beginEndPoint.
// 
// Either both pointers, firstIntervalPtr_ and lastIntervalPtr_, are null
// or neither of them is null.  If these pointers are null it means that the
// list is empty.
// *****************************************************************************

// Forward declarations.
class MdamInterval;
// End of forward declarations.

class MdamIntervalList
{

friend class MdamIntervalListIterator;

public:

  // Default constructor creates an empty list.
  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  MdamIntervalList(const Lng32 callerTag = -1);
  #else
   MdamIntervalList();
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // Destructor.
  ~MdamIntervalList();

  // Appends an MdamInterval onto an MdamIntervalList.  Warning: caller
  // must ensure that the new interval is disjoint and in order.  No
  // integrity checks are performed.
  MdamIntervalList & append(MdamInterval * interval);

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  Lng32 countIntervals() const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // Delete all intervals in the list.
  void deleteAllIntervals(FixedSizeHeapManager & mdamIntervalHeap,
                                     FixedSizeHeapManager & mdamRefListEntryHeap);

  // This function inserts a single disjunct number into the reference list
  // associated with each MdamInterval on the MdamIntervalList.
  void insertDisjunctNum(const Int32 disjunctNum,
                              FixedSizeHeapManager & mdamRefListEntryHeap);

  // Performs an intersect operation on two interval lists to produce a
  // result list.  The this list and the otherList are inputs to the
  // intersect operation.  The result replaces the this list.  The
  // original interval list entries for the this list are deleted.
  MdamIntervalList & intersect
    (const MdamIntervalList & otherList,
     const ULng32 keyLen,
     FixedSizeHeapManager & mdamIntervalHeap,
     FixedSizeHeapManager & mdamRefListEntryHeap);

  // Determine if an interval list is empty.
  inline NABoolean isEmpty() const;

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  void logEvent(const Lng32 functionId,
                           const Lng32 intervalCount = 0,
                           const Int64 otherListId = -1,
                           const Lng32 callerTag = -1) const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

  // Print functions.
  #if defined ( NA_MDAM_EXECUTOR_DEBUG )
  void print(const char * header = "") const;
  void printBrief() const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

  // Performs an union operation on two interval lists to produce a
  // result list.  The source lists have no MdamRefLists associated
  // with them.  The this list and the otherList are inputs to the
  // union operation.  The result replaces the this list.  The
  // original interval list entries for this list are deleted.
  MdamIntervalList & unionSameDisjunct
    (const MdamIntervalList & otherList,
     const ULng32 keyLen,
     FixedSizeHeapManager & mdamIntervalHeap,
     FixedSizeHeapManager & mdamRefListEntryHeap);

  // Performs a union operation on two interval lists to produce a
  // result list.  The list's intervals may have different
  // MdamRefList*s associated with them.  Any reference lists
  // associated with otherList are ignored and disjunctNum is used
  // instead.  The this list and the otherList are inputs to the
  // intersect operation.  The result replaces the this list.  The
  // original interval list entries for the this list are deleted.
  MdamIntervalList & unionSeparateDisjuncts
    (const MdamIntervalList & otherList,
     const Int32 disjunctNum,
     const ULng32 keyLen,
     FixedSizeHeapManager & mdamIntervalHeap,
     FixedSizeHeapManager & mdamRefListEntryHeap);


private:

  // Points to the first MdamInterval on the list.
  MdamInterval * firstIntervalPtr_;

  // Points to the last MdamInterval on the list.
  MdamInterval * lastIntervalPtr_;

  // Give all intervals from this list and put them in otherList.
  // this list becomes empty.
  void giveAllIntervals(MdamIntervalList & otherList);

  #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
  const Int64 intervalListId_;
  #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

}; // class MdamIntervalList


// *****************************************************************************
// Inline member functions for class MdamIntervalList
// *****************************************************************************


// Determine if an interval list is empty.
inline NABoolean MdamIntervalList::isEmpty() const
{
  return firstIntervalPtr_ == 0;
}


#endif /* MDAMINTERVALLIST_H */
