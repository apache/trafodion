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
#ifndef MDAMINTERVALLISTMERGER_H
#define MDAMINTERVALLISTMERGER_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalListMerger.h
* Description:  MDAM Interval List Merger
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

#include "MdamEnums.h"
#include "MdamIntervalList.h"
#include "MdamEndPoint.h"
#include "MdamIntervalIterator.h"

// *****************************************************************************
// MdamIntervalListMerger : MDAM Interval List Merger
// The iteration operator in this class merges two interval lists.
// It eliminates duplicate points in the process. 
// If a point appears as both a begin and an end point,
// it is returned twice.  As each point is returned, state is updated:
//   + number of intervals currently active,
//   + pointers to intervals from which the end points came.
// The two interval lists are identified by logical numbers 0 and 1.
// *****************************************************************************

class MdamIntervalListMerger
{

public:

  // Constructor.
  MdamIntervalListMerger
    (const MdamIntervalList * intervalListPtr0,
     const MdamIntervalList * intervalListPtr1,
     const ULng32 keyLen)
     : intervalIterator0_(intervalListPtr0, 0),
       intervalIterator1_(intervalListPtr1, 1),
       activeIntervals_(0),
       previousActiveIntervals_(0),
       endPoint0_(),
       endPoint1_(),
       keyLen_(keyLen)
  {
    // Get the first endpoint from each of the two interval lists.
    intervalIterator0_(endPoint0_);
    intervalIterator1_(endPoint1_);
  }


  // Destructor.
  ~MdamIntervalListMerger();

  // Iteration operator returns the next endpoint on each call.
  NABoolean operator()(MdamEndPoint & mdamEndPoint);

  // Get the number of active intervals for the most recent iteration call.
  inline Int32 getActiveIntervals() const;

  // Get the number of active intervals for the previous iteration call.
  inline Int32 getPreviousActiveIntervals() const;

private:

  // Iterators for the two interval lists being merged.
  MdamIntervalIterator intervalIterator0_;
  MdamIntervalIterator intervalIterator1_;

  // Number of intervals currently active.  Possible values are zero to two,
  // inclusive.
  Int32 activeIntervals_;

  // Number of active intervals on the previous call.
  Int32 previousActiveIntervals_;

  // End points.  One from each of the two lists being merged.
  MdamEndPoint endPoint0_;
  MdamEndPoint endPoint1_;

  // Key length.
  ULng32 keyLen_;

}; // class MdamIntervalListMerger


// *****************************************************************************
// Inline member functions for class MdamIntervalListMerger
// *****************************************************************************

// Get the number of active intervals for the most recent iteration call.
inline Int32 MdamIntervalListMerger::getActiveIntervals() const
{
  return activeIntervals_;
}


// Get the number of active intervals for the previous iteration call.
inline Int32 MdamIntervalListMerger::getPreviousActiveIntervals() const
{
  return previousActiveIntervals_;
}


#endif /* MDAMINTERVALLISTMERGER_H */
