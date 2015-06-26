/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
#pragma nowarn(203)   // warning elimination 
  return TRUE;  // To get rid of compiler warning.
#pragma warn(203)  // warning elimination 
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
#pragma nowarn(203)   // warning elimination 
  return TRUE;  // To get rid of compiler warning.
#pragma warn(203)  // warning elimination 
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

  // We don't know what the data type is, but we do know how
  // long the field is. So we will guess the data type.

  // The Generator transforms varchars to chars, so we don't
  // have to worry about varchar length fields.

  // We might have a null indicator, but we have no way of knowing
  // that here. So we will ignore that possibility. (Sorry!)

  // If the length is 2 or 4 or 8, we'll guess that it is an
  // integer and print a signed integer interpretation.

  // If the length is 7 and the first two bytes, when interpreted
  // as Big Endian, looks like a year within 100 years of 2000,
  // we'll interpret it as a TIMESTAMP(0).

  // There are other possibilities of course which can be added
  // over time but a better solution would be to change the 
  // Generator and Executor to simply give us the data type info.

  char local[1001];  // will assume our length is <= 1000
  local[0] = '\0';

  if (dataPointer)
    {
    bool allNulls = true;
    bool allFFs = true;
    bool allPrintable = true;
    size_t i = 0;
    while (i < keyLen && (allNulls || allFFs))
      {
      if (dataPointer[i] != '\0') allNulls = false;
      if (dataPointer[i] != -1) allFFs = false;
      if (!isprint(dataPointer[i])) allPrintable = false;
      i++;
      }
    if (allNulls)
      {
      strcpy(local,"*lo*");  // hopefully there won't be a legitimate value of *lo*
      }
    else if (allFFs)
      {
      strcpy(local,"*hi*");  // hopefully there won't be a legitimate value of *hi*
      }
    else if (allPrintable)
      {
      size_t lengthToMove = sizeof(local) - 1;
      if (keyLen < lengthToMove)
        lengthToMove = keyLen;
      strncpy(local,dataPointer,lengthToMove);
      local[lengthToMove] = '\0';
      }
    else  
      {
      // create a hex representation of the first 498 characters
      strcpy(local,"hex ");
      char * nextTarget = local + strlen(local);
      size_t repdChars = ((sizeof(local) - 1)/2) - 4; // -4 to allow for "hex "
      if (keyLen < repdChars)
        repdChars = keyLen;

      for (size_t i = 0; i < repdChars; i++)
        {
        unsigned char nibbles[2];
        nibbles[0] = ((unsigned char)dataPointer[i] & 
                      (unsigned char)0xf0)/16;
        nibbles[1] = (unsigned char)dataPointer[i] & (unsigned char)0x0f;
        for (size_t j = 0; j < 2; j++)
          {
          if (nibbles[j] < 10)
            *nextTarget = '0' + nibbles[j];
          else
            *nextTarget = 'a' + (nibbles[j] - 10);
          nextTarget++;
          }  // for j
        }  // for i

      *nextTarget = '\0';         
      }

    if (keyLen == 2)  // if it might be a short
      {
      // append an interpretation as a short (note that there
      // is room in local for this purpose)

      // the value is big-endian hence the weird computation
      long value = 256 * dataPointer[0] + 
                   (unsigned char)dataPointer[1];                  
      sprintf(local + strlen(local), " (short %ld)",value);
      }
    else if (keyLen == 4)  // if it might be a long
      {
      // append an interpretation as a long (note that there
      // is room in local for this purpose)

      // the value is big-endian hence the weird computation
      long value = 256 * 256 * 256 * dataPointer[0] + 
                   256 * 256 * (unsigned char)dataPointer[1] +  
                   256 * (unsigned char)dataPointer[2] + 
                   (unsigned char)dataPointer[3];           
      sprintf(local + strlen(local), " (long %ld)",value);
      }
    else if (keyLen == 8)  // if it might be a 64-bit integer
      {
      // append an interpretation as a short (note that there
      // is room in local for this purpose)

      // the value is big-endian hence the weird computation
      long long value = 256 * 256 * 256 * dataPointer[0] + 
                   256 * 256 * (unsigned char)dataPointer[1] +  
                   256 * (unsigned char)dataPointer[2] + 
                   (unsigned char)dataPointer[3]; 
      value = (long long)256 * 256 * 256 * 256 * value +   
                   256 * 256 * 256 * (unsigned char)dataPointer[4] + 
                   256 * 256 * (unsigned char)dataPointer[5] +  
                   256 * (unsigned char)dataPointer[6] + 
                   (unsigned char)dataPointer[7];        
      sprintf(local + strlen(local), " (long long %lld)",value);
      }
    else if (keyLen == 7)  // a TIMESTAMP(0) perhaps?
      {
      long year = 256 * dataPointer[0] +
                          (unsigned char)dataPointer[1];
      if ((year >= 1900) && (year <= 2100))
        {
        // looks like a TIMESTAMP(0); look further
        long month = (unsigned char)dataPointer[2];
        long day = (unsigned char)dataPointer[3];
        long hour = (unsigned char)dataPointer[4];
        long minute = (unsigned char)dataPointer[5];
        long second = (unsigned char)dataPointer[6];

        if ((month >= 1) && (month <= 12) &&
            (day >= 1) && (day <= 31) &&
            (hour >= 0) && (hour <= 23) &&
            (minute >= 0) && (minute <= 59) &&
            (second >= 0) && (second <= 59))
          {
          sprintf(local + strlen(local), 
                  " (TIMESTAMP(0) %ld-%02ld-%02ld %02ld:%02ld:%02ld)",
                  year,month,day,hour,minute,second);
          }
        }
      }
    }    
  cout << local;
  // cout << *(Lng32 *)dataPointer;
  // End test change
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */
