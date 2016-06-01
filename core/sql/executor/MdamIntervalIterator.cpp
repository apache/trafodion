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
* File:         MdamIntervalIterator.C
* Description:  Implimentation for MDAM Interval Iterator
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
#include "MdamEndPoint.h"
#include "MdamIntervalIterator.h"

// *****************************************************************************
// Member functions for class MdamIntervalIterator
// *****************************************************************************

// Constructor. 
MdamIntervalIterator::MdamIntervalIterator
                                    (const MdamIntervalList * intervalListPtr,
                                     const Int32 logicalIntervalListNumber)
      : logicalIntervalListNumber_(logicalIntervalListNumber),
        intervalPtr_(0),
        endPointType_(MdamEnums::MDAM_BEGIN),
        intervalListIterator_(*intervalListPtr)
{
  intervalPtr_ = intervalListIterator_();  // Get the first interval.
}

// Destructor.
MdamIntervalIterator::~MdamIntervalIterator() {}


// Iteration operator returns the next endpoint on each call.
NABoolean MdamIntervalIterator::operator()(MdamEndPoint & mdamEndPointRef)
{
  if (intervalPtr_ == 0)  // No intervals remaining.
    {
      mdamEndPointRef.reset();
      return FALSE;
    };
  mdamEndPointRef.set(intervalPtr_, endPointType_, logicalIntervalListNumber_ );
  if (endPointType_ == MdamEnums::MDAM_BEGIN)
    {
      endPointType_ = MdamEnums::MDAM_END;  // Advance to end endpoint.
    }
  else  // Must be that endPointType_ == MdamEnums::MDAM_END.
    {
      intervalPtr_ = intervalListIterator_();  // Advance to the next interval.
      endPointType_ = MdamEnums::MDAM_BEGIN;  // Reset to the begin endpoint.
    };
  return TRUE;
}
