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
* File:         MdamPoint.C
* Description:  Implimentation for MDAM Point
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
#include <cctype>
#include <cstdio>
#endif /* NA_MDAM_EXECUTOR_DEBUG */
#include "NABoolean.h"
#include "MdamEndPoint.h"
#include "MdamPoint.h"
#include "str.h"

// *****************************************************************************
// Member functions for class MdamPoint
// *****************************************************************************

// Constructor that accepts an MdamEndPoint.
MdamPoint::MdamPoint(MdamEndPoint & endPointRef)
     : tupp_ (endPointRef.getPointPtr()->tupp_),
       inclusion_ (endPointRef.getInclusion()) { }

// Determine if v could be within an interval for which this MdamPoint
// is the begin endpoint.
NABoolean MdamPoint::beginContains(const ULng32 keyLen, const char * v)
  const
{
  Int32 cmpCode = str_cmp(tupp_.getDataPointer(), v, Int32(keyLen));
  MdamEnums::MdamOrder tempOrder = 
    MdamEnums::MdamOrder((cmpCode > 0)? 1 : ((cmpCode == 0) ? 0 : -1));
  if (tempOrder == MdamEnums::MDAM_LESS)
    {
      return TRUE;
    }
  else if (tempOrder == MdamEnums::MDAM_GREATER)
    {
      return FALSE;
    } 
  // The two values are equal so consider inclusion.
  else if (included())
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    };
  return TRUE;  // To get rid of compiler warning.
}


// Determine if v could be within an interval for which this MdamPoint
// is the end endpoint.
NABoolean MdamPoint::endContains(const ULng32 keyLen, const char * v) const
{
  Int32 cmpCode = str_cmp(tupp_.getDataPointer(), v, Int32(keyLen));
  MdamEnums::MdamOrder tempOrder = 
    MdamEnums::MdamOrder((cmpCode > 0)? 1 : ((cmpCode == 0) ? 0 : -1));
  if (tempOrder == MdamEnums::MDAM_LESS)
    {
      return FALSE;
    }
  else if (tempOrder == MdamEnums::MDAM_GREATER)
    {
      return TRUE;
    } 
  // The two values are equal so consider inclusion.
  else if (included())
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    };
  return TRUE;  // To get rid of compiler warning.
}


// Print functions.
#ifdef NA_MDAM_EXECUTOR_DEBUG
void MdamPoint::print(const char * header) const
{
  cout << header << endl;
  char * dataPointer = getDataPointer();
  Lng32 keyLen = tupp_.getAllocatedSize();
  cout << "  Key length: " << keyLen << endl;
  cout << "  Data pointer: " << (void *)dataPointer << endl;
  cout << "  Inclusion: "
       << (inclusion_ ? "INCLUDED" : "EXCLUDED")
       << endl;
}

void MdamPoint::printBrief() const
{
  char * dataPointer = getDataPointer();
  Lng32 keyLen = tupp_.getAllocatedSize();

  ::printBrief(dataPointer, keyLen, FALSE /* no end of line wanted */);
}
#endif
