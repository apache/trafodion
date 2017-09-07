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
#ifndef MDAMPOINT_H
#define MDAMPOINT_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamPoint.h
* Description:  MDAM Point
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

#include "Platform.h"
#include "MdamEnums.h"
#include "ExpSqlTupp.h"
#include "str.h"

// *****************************************************************************
// MdamPoint represents one end of an interval.  An MdamPoint consists of a
// value and inclusion, an indicatation of whether the point is included or
// excluded from the interval.
// *****************************************************************************

// Forward declarations.
class MdamEndPoint;
// End of forward declarations.

#pragma nowarn(1103)   // warning elimination 
class MdamPoint
{

public:
  
  // Constructor that accepts tupp and inclusion.
  MdamPoint(const tupp & tupp,
	    const MdamEnums::MdamInclusion inclusion);

  // Constructor that accepts an MdamEndPoint.
  MdamPoint(MdamEndPoint & endPoint);

  // Determine if v could be within an interval for which this MdamPoint
  // is the begin endpoint.
  NABoolean beginContains
    (const ULng32 keyLen, const char * v) const;

  // Compare function.
  MdamEnums::MdamOrder compare(const MdamPoint * other,
					  const ULng32 keyLen) const;

  // Determine if v could be within an interval for which this MdamPoint
  // is the end endpoint.
  NABoolean endContains
    (const ULng32 keyLen, const char * v) const;

  // Get function for data pointer.
  inline char * getDataPointer() const;

  // Get function for inclusion_.
  MdamEnums::MdamInclusion getInclusion() const;

  // Determine if the MdamPoint is included.
  NABoolean included() const;

  // Reverse inclusion.
  void reverseInclusion();

  // Release tupp storage
  inline void release();

  // Print functions.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  void printBrief() const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */
  
private:

  // The point's value.
  tupp tupp_;

  // Defines whether the point is included or excluded.
  MdamEnums::MdamInclusion inclusion_;

}; // class MdamPoint


#pragma warn(1103)   // warning elimination 

// *****************************************************************************
// Inline member functions for class MdamPoint
// *****************************************************************************


// $$$$ Possibly not used except for testing.
// Constructor.
inline MdamPoint::MdamPoint(const tupp & tupp,
                            const MdamEnums::MdamInclusion inclusion)
     : tupp_ (tupp),
       inclusion_ (inclusion) {}


// The comparision two MdamPoints.
inline MdamEnums::MdamOrder MdamPoint::compare
             (const MdamPoint * other, const ULng32 keyLen) const
{
#pragma nowarn(1506)   // warning elimination 
  short retVal = str_cmp(tupp_.getDataPointer(), 
                                      other->tupp_.getDataPointer(),
                                      Int32(keyLen));
#pragma warn(1506)  // warning elimination 
  if (retVal < 0)
     return MdamEnums::MDAM_LESS;
  else
     if (retVal > 0)
        return MdamEnums::MDAM_GREATER;
       else
          return MdamEnums::MDAM_EQUAL;
}


// Get function for data pointer.
inline char * MdamPoint::getDataPointer() const
{
  return tupp_.getDataPointer();
}


// Get function for inclusion_.
inline MdamEnums::MdamInclusion MdamPoint::getInclusion() const
{
  return inclusion_;
}


// Determine if the MdamPoint is included.
inline NABoolean MdamPoint::included() const
{
  return (inclusion_ == MdamEnums::MDAM_INCLUDED);
}


// Reverse inclusion.  MDAM_INCLUDED becomes MDAM_EXCLUDED and
// MDAM_EXCLUDED becomes IMDAM_NCLUDED.
inline void MdamPoint::reverseInclusion()
{
  if (inclusion_ == MdamEnums::MDAM_INCLUDED)
    {
      inclusion_ = MdamEnums::MDAM_EXCLUDED;
    }
  else
    {
      inclusion_ = MdamEnums::MDAM_INCLUDED;
    };
}

// Release tupp storage associated with the point
inline void MdamPoint::release()
{
	tupp_.release();
}

#endif /* MDAMPOINT_H */
