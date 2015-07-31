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
* File:         MdamInterval.C
* Description:  Implimentation for MDAM Interval
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
#ifdef NA_MDAM_EXECUTOR_DEBUG
#include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */
#include "MdamInterval.h"
#include "MdamEndPoint.h"

// *****************************************************************************
// Member functions for class MdamInterval
// *****************************************************************************

// Constructor without a disjunctNum.  Building of MdamRefList is deferred.
MdamInterval::MdamInterval(const tupp & beginTupp,
                                  const MdamEnums::MdamInclusion beginInclusion,
                                  const tupp & endTupp,
                                  const MdamEnums::MdamInclusion endInclusion)
     : beginPoint_(beginTupp, beginInclusion), 
       endPoint_(endTupp, endInclusion),
       mdamRefList_(),
       nextMdamIntervalPtr_(0)
{}


// Constructor without a disjunctNum.  Building of MdamRefList is deferred.
MdamInterval::MdamInterval(MdamEndPoint & beginEndPointRef,
                                  MdamEndPoint & endEndPointRef)
     : beginPoint_(beginEndPointRef),
       endPoint_(endEndPointRef),
       mdamRefList_(),
       nextMdamIntervalPtr_(0)
{}


// This constructor meets the special requirements of
// MdamIntervalList::unionSeparateDisjuncts.  These requirements are:
//  + reverse the inclusion of endEndPoint if it is of type BEGIN, and
//  + build a reference list.
MdamInterval::MdamInterval(MdamEndPoint & beginEndPoint,
			   MdamEndPoint & endEndPoint,
			   MdamInterval * intervalPtr0,
			   MdamInterval * intervalPtr1,
			   const Int32 disjunctNum,
			   FixedSizeHeapManager & mdamRefListEntryHeap)
     : beginPoint_(beginEndPoint),
       endPoint_(endEndPoint),
       mdamRefList_(),
       nextMdamIntervalPtr_(0)
{
  // Reverse the inclusion of endEndPoint if it is of type BEGIN.
  if (endEndPoint.begin())
    {
      endPoint_.reverseInclusion();
    };
  // Build a reference list.
  createRefList(intervalPtr0, intervalPtr1, disjunctNum, mdamRefListEntryHeap);
}


// Destructor.
MdamInterval::~MdamInterval()
{}

// Operator new with just size_t.  This should never be called.
void * MdamInterval::operator new(size_t)
{
  ex_assert(0,"MdamInterval::operator new(size_t) called.");
  return 0;
}


// Operator delete.  This should never be called. 
void MdamInterval::operator delete(void *)
{
  ex_assert(0,"MdamInterval::operator delete(void *) called.");
}

// Determines if the value specified by the arguement, v, is contained
// within this interval.
NABoolean MdamInterval::contains
  (const ULng32 keyLen, const char * v) const
{
  if(beginPoint_.beginContains(keyLen, v)
         && endPoint_.endContains(keyLen, v))
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}


// Get function that obtains the first value in an interval.
// The value is stored at the location specified by s.
// If the operation is successful, the function returns true.
// Otherwise, it returns false.  Failure can occur if the begin
// end point is excluded and incrementing the value results
// in an error or a value that is outside the interval.
NABoolean MdamInterval::getFirstValue
  (const ULng32 keyLen, char * s) const
{
  // Move the string.
  str_cpy_all(s, beginPoint_.getDataPointer(), Lng32(keyLen));

  // If the begin endpoint is included, then it is the first value.
  if (beginPoint_.included())
    {
      return TRUE;
    }
  else
    // The begin endpoint is excluded so calculate the next higher
    // value.
    {
      Int32 returnCode = str_inc(keyLen, s);
      // Check if the string was successfully incremented.
      if (returnCode == 0)
        {
	  // Check that the incremented value is within the interval.
          if (contains(keyLen, s))  // $$$$ This test is over-kill.
	    {
              return TRUE;
	    }
          else
	    // Incremented value not within interval.
            {
              return FALSE;
	    }
        }
      else
	// Value not successfully incremented.
        {
          return FALSE;
        }
    }
}


// Get function that obtains the next value in an interval.  The
// value is stored at the location specified by s.  If the operation
// is successful, the function returns true.  Otherwise, it returns
// false.  Failure can occur if incrementing the value results in an
// error or a value that is outside the interval.
NABoolean MdamInterval::getNextValue(const ULng32 keyLen, char * s) const
{
  // Increment the string.
  Int32 returnCode = str_inc(keyLen, s);
  // Check if the string was successfully incremented.
  if (returnCode == 0)
    {
      // Check that the incremented value is within the interval.
      if (contains(keyLen, s))  // $$$$ This test is over-kill.
	{
          return TRUE;
	}
      else
	// Incremented value not within interval.
        {
          return FALSE;
	}
    }
    else
    // Value not successfully incremented.
    {
      return FALSE;
    }
}


// $$$$ try making this inline
// Function to get the address of the beginning or ending point.
MdamPoint * MdamInterval::getPointPtr
  (const MdamEnums::MdamEndPointType endPointType)
{
  return ((endPointType == MdamEnums::MDAM_BEGIN) ? &beginPoint_ : &endPoint_);
}


// Mutator function to set nextMdamIntervalPtr_.
void MdamInterval::setNextMdamIntervalPtr(MdamInterval * nextMdamIntervalPtr)
{
  nextMdamIntervalPtr_ = nextMdamIntervalPtr;
}


// Create the Reference list for an interval.  The new reference list is
// a copy of the reference list associated with interval1Ptr.  disjunctNum
// is inserted into the new list if the interval pointed to by interval2Ptr
// is active. 
void MdamInterval::createRefList(const MdamInterval * interval1Ptr, 
                                 const MdamInterval * interval2Ptr,
                                 const Int32 disjunctNum,
				 FixedSizeHeapManager & mdamRefListEntryHeap)
{
  if (interval1Ptr != 0)
    {
      mdamRefList_.copyEntries(interval1Ptr->mdamRefList_, mdamRefListEntryHeap);
    };

  if (interval2Ptr != 0)
    {
      mdamRefList_.insert(disjunctNum, mdamRefListEntryHeap);
    };
}


#ifdef NA_MDAM_EXECUTOR_DEBUG
// Print functions.
void MdamInterval::print(const char * header) const
{
  cout << header << endl;
  beginPoint_.print("begin point:");
  endPoint_.print("end point:");
  mdamRefList_.print("reference list:");
  cout << "nextMdamIntervalPtr = " << (void *)nextMdamIntervalPtr_ << endl;
}

void MdamInterval::printBrief() const
{
  cout << " ";
  if (beginPoint_.included())
    cout << "[";
  else
    cout << "(";
  beginPoint_.printBrief();
  cout << ",";
  endPoint_.printBrief();
  if (endPoint_.included())
    cout << "]";
  else
    cout << ")";
  cout << " ";
  mdamRefList_.printBrief();
  cout << endl;
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */
