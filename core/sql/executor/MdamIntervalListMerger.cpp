/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalListMerger.C
* Description:  Implimentation for MDAM Interval List Merger
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
#include "MdamEndPoint.h"
#include "MdamIntervalListMerger.h"

// *****************************************************************************
// Member functions for class MdamIntervalListMerger
// *****************************************************************************

// Destructor.
MdamIntervalListMerger::~MdamIntervalListMerger() {}


// Iteration operator returns the next endpoint on each call.
NABoolean MdamIntervalListMerger::operator()(MdamEndPoint & mdamEndPointRef)
{
  previousActiveIntervals_ = activeIntervals_;
  Int32 activeIntervalsDelta = 1;
  if (endPoint0_.exists())
    {
      if (endPoint1_.exists())
        {
          // Both endpoints exist, so compare them and return the LESSer.
          switch (endPoint0_.compare(&endPoint1_, keyLen_))
            {
	      case MdamEnums::MDAM_LESS:
                {
                  // endPoint0_ is LESS, so return it.
                  mdamEndPointRef = endPoint0_;
                  intervalIterator0_(endPoint0_);    
                  break;
                }
	      case MdamEnums::MDAM_EQUAL:
                {
                  // The endpoints are equal so arbitrarily return endPoint0_.
                  mdamEndPointRef = endPoint0_;
                  // Include the interval pointer from endpoint 1.  The returned
                  // endpoint represents points from both interval lists.
                  mdamEndPointRef.assignIntervalPtr1(&endPoint1_);
                  // Advance both lists.
                  intervalIterator0_(endPoint0_);
                  intervalIterator1_(endPoint1_);
                  activeIntervalsDelta = 2;
                  break;
                }
              case MdamEnums::MDAM_GREATER:
                {
                  // endPoint1_ is LESS, so return it.
                  mdamEndPointRef = endPoint1_;
                  intervalIterator1_(endPoint1_);
                  break;
                }
	    };
        }
      else
        {
          // Only endPoint0_ exists, so return it.
          mdamEndPointRef = endPoint0_;
          intervalIterator0_(endPoint0_);    
        };
    }
  else
    {
      if (endPoint1_.exists())
        {
          // Only endPoint1_ exists, so return it.
          mdamEndPointRef = endPoint1_;
          intervalIterator1_(endPoint1_);
        }
      else
        {
          // Out of points from both lists.  Return nothing.
          mdamEndPointRef.reset();
          return FALSE;
        };
    };

  // Adjust activeIntervals_.  If the endpoint being returned is a begin
  // endpoint, then the adjustment is positive.  If it is an end endpoint,
  // then the adjustment is negative.
  if (mdamEndPointRef.begin())
    {
      activeIntervals_ += activeIntervalsDelta;
    }
  else
    {
      activeIntervals_ -= activeIntervalsDelta;
    };

  return TRUE;
}
