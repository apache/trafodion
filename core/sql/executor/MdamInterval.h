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
#ifndef MDAMINTERVAL_H
#define MDAMINTERVAL_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamInterval.h
* Description:  MDAM Interval
*
*
* Created:      9/11/96
* Language:     C++
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "MdamEnums.h"
#include "NABoolean.h"
#include "MdamPoint.h"
#include "MdamRefList.h"
#include "FixedSizeHeapManager.h"

// Forward references...
class MdamEndPoint;
class tupp;

// *****************************************************************************
// MdamInterval represents a range of values from beginPoint_ to endPoint_.
// An invarient relation is that beginPoint_ <= endPoint_.  Associated with
// an interval is a reference list.  It specifies the disjuncts the interval
// satisfies.
// *****************************************************************************
class MdamInterval
{

friend class MdamIntervalIterator;

public:

  // Constructor.
  // $$$$ Possibly testing only.
  inline MdamInterval(const tupp & beginTupp,
				 const MdamEnums::MdamInclusion beginInclusion,
				 const tupp & endTupp,
				 const MdamEnums::MdamInclusion endInclusion,
				 const Int32 disjunctNum,
				 FixedSizeHeapManager & mdamRefListEntryHeap);

  // Constructor without a disjunctNum.  Building of MdamRefList is deferred.
   MdamInterval(const tupp & beginTupp,
				 const MdamEnums::MdamInclusion beginInclusion,
				 const tupp & endTupp,
				 const MdamEnums::MdamInclusion endInclusion);

  // Constructor without a disjunctNum.  Building of MdamRefList is deferred.
   MdamInterval(MdamEndPoint & beginEndPoint,
				 MdamEndPoint & endEndPoint);

  // This constructor meets the special requirements of
  // MdamIntervalList::unionSeparateDisjuncts.  These requirements are:
  //  + reverse the inclusion of endEndPoint if it is of type BEGIN, and
  //  + build a reference list.
  MdamInterval(MdamEndPoint & beginEndPoint,
			  MdamEndPoint & endEndPoint,
			  MdamInterval * intervalPtr0,
			  MdamInterval * intervalPtr1,
			  const Int32 disjunctNum,
			  FixedSizeHeapManager & mdamRefListEntryHeap);

  // Destructor.
  ~MdamInterval();

  // Operator new.
  inline void * operator new(size_t size,
					FixedSizeHeapManager & mdamIntervalHeap);

  // Operator new with just size_t.  This should never be called.
   void * operator new(size_t size);

  // Operator delete.  This should never be called.
   void operator delete(void *);


  // Determines if a value is contained within this interval.
  NABoolean contains(const ULng32 keyLen, const char * v)
       const;

  // Create the Reference list for an interval.  The new reference list is
  // a copy of the reference list associated with interval1Ptr.  disjunctNum
  // is inserted into the new list if the interval pointed to by interval2Ptr
  // is active.
  void createRefList(const MdamInterval * interval1Ptr,
				const MdamInterval * interval2Ptr,
				const Int32 disjunctNum,
				FixedSizeHeapManager & mdamRefListEntryHeap);

  // Get function that obtains the first value in an interval.
  // The value is stored at the location specified by s.
  // If the operation is successful, the function returns true.
  // Otherwise, it returns false.  Failure can occur if the begin
  // end point is excluded and incrementing the value results
  // in an error or a value that is outside the interval.
  NABoolean getFirstValue(const ULng32 keyLen, char * s)
       const;

  // Const function to get nextMdamIntervalPtr_.
  inline MdamInterval * getNextMdamIntervalPtr() const;

  // Get function that obtains the next value in an interval.  The
  // value is stored at the location specified by s.  If the operation
  // is successful, the function returns true.  Otherwise, it returns
  // false.  Failure can occur if incrementing the value results in an
  // error or a value that is outside the interval.
  NABoolean getNextValue(const ULng32 keyLen, char * s) const;

  // Const function to get the address of the beginning or ending point.
  MdamPoint * getPointPtr
    (const MdamEnums::MdamEndPointType endPointType);

  // Function to get the address of the MdamRefList.
  inline MdamRefList * getRefListPtr();

  // This function inserts a single disjunct number into the reference list
  // associated with this MdamInterval.
  inline void insertDisjunctNum(const Int32 disjunctNum,
				FixedSizeHeapManager & mdamRefListEntryHeap);

  // Release resources associated with this interval.
  // This is used prior to returning the interval to the free list.
  inline void release(FixedSizeHeapManager & mdamRefListEntryHeap);

  // Release reference list entries associated with this interval.
  inline void releaseRefListEntries(FixedSizeHeapManager & mdamRefListEntryHeap);

  // Release tupp storage associated with this interval.
  inline void releaseTupps();

  // Print functions.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  void printBrief() const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

  // Mutator function to set nextMdamIntervalPtr_.
  void setNextMdamIntervalPtr(MdamInterval * nextMdamIntervalPtr);


private:

  // MdamPoint that marks the beginning of the interval.
  MdamPoint beginPoint_;

  // MdamPoint that marks the end of the interval.
  MdamPoint endPoint_;

  // MdamRefList associated with the interval.
  MdamRefList mdamRefList_;

  // This forward pointer is used to form a singlely-linked list of intervals.
  MdamInterval * nextMdamIntervalPtr_;

}; // class MdamInterval


// *****************************************************************************
// Inline member functions for class MdamInterval
// *****************************************************************************


// Constructor.
inline MdamInterval::MdamInterval(const tupp & beginTupp,
                                  const MdamEnums::MdamInclusion beginInclusion,
                                  const tupp & endTupp,
                                  const MdamEnums::MdamInclusion endInclusion,
                                  const Int32 disjunctNum,
				  FixedSizeHeapManager & mdamRefListEntryHeap)
     : beginPoint_(beginTupp, beginInclusion),
       endPoint_(endTupp, endInclusion),
       mdamRefList_(disjunctNum, mdamRefListEntryHeap),
       nextMdamIntervalPtr_(0)
{}

// Operator new.
inline void * MdamInterval::operator new
  (size_t size,
   FixedSizeHeapManager & mdamIntervalHeap)
{
  return mdamIntervalHeap.allocateElement(size);
}


// Function to get the address of the MdamRefList.
inline MdamRefList * MdamInterval::getRefListPtr()
{
  return &mdamRefList_;
}


// This function inserts a single disjunct number into the reference list
// associated with this MdamInterval.
inline void MdamInterval::insertDisjunctNum(const Int32 disjunctNum,
			     FixedSizeHeapManager & mdamRefListEntryHeap)
{
  mdamRefList_.insert(disjunctNum, mdamRefListEntryHeap);
}


// Const function to get nextMdamIntervalPtr_.
inline MdamInterval * MdamInterval::getNextMdamIntervalPtr () const
{
  return nextMdamIntervalPtr_;
}


// Release resources associated with this interval.
// This is used prior to returning the interval to the free list.
inline void MdamInterval::release(FixedSizeHeapManager & mdamRefListEntryHeap)
{
  releaseTupps();
  releaseRefListEntries(mdamRefListEntryHeap);
}


// Release reference list entries associated with this interval.
inline void MdamInterval::releaseRefListEntries(FixedSizeHeapManager & mdamRefListEntryHeap)
{
  mdamRefList_.deleteEntries(mdamRefListEntryHeap);
}


// Release tupp storage associated with this interval.
inline void MdamInterval::releaseTupps()
{
  beginPoint_.release();
  endPoint_.release();
}

#endif /* MDAMINTERVAL_H */
