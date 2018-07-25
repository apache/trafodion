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
#ifndef MDAMINTERVALITERATOR_H
#define MDAMINTERVALITERATOR_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalIterator.h
* Description:  MDAM Interval Iterator
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
#include "MdamIntervalListIterator.h"

// *****************************************************************************
// MdamIntervalIterator : MDAM Interval Iterator
//
// This iterator returns the next endpoint on each call.  Within an interval
// it returns the begin endpoint followed by the end endpoint. Then it
// continues with the next interval in an interval list until the interval 
// list is exhausted.
// So, this iterator makes an interval list appear to be an ordered set of
// endpoints and presents the endpoints one at a time.
// *****************************************************************************

class MdamIntervalIterator
{

public:

  // Constructor.
  MdamIntervalIterator(const MdamIntervalList * intervalListPtr,
					 const Int32 logicalIntervalListNumber);

  // Destructor.
  ~MdamIntervalIterator();

  // Iteration operator returns the next endpoint on each call.
  NABoolean operator()(MdamEndPoint & mdamEndPoint);

private:

  // Logical interval list number.
  const Int32 logicalIntervalListNumber_;

  // Pointer to the current interval.
  MdamInterval * intervalPtr_;

  // endPointType_ is used to keep track of which of the two endpoints
  // is the current one for iteration purposes.
  MdamEnums::MdamEndPointType endPointType_;

  // Iterator to return intervals from the interval list.
  MdamIntervalListIterator intervalListIterator_;

}; // class MdamIntervalIterator



#endif /* MDAMINTERVALITERATOR_H */
