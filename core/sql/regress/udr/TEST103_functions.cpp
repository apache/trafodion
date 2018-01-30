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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqludr.h"

extern "C" {

/* genPhoneNumber */
SQLUDR_LIBFUNC SQLUDR_INT32 genPhoneNumber(SQLUDR_INT32 *in1, // seed
                                           SQLUDR_CHAR *in2, // areacode 
                                           SQLUDR_CHAR *out,
                                           SQLUDR_INT16 *in1Ind,
                                           SQLUDR_INT16 *in2Ind,
                                           SQLUDR_INT16 *outInd,
                                           SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  std::string result (in2);
  if (*in1Ind == SQLUDR_NULL || *in2Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    srand(*in1);
    int number = 7;  // 7 digit phone number
    for (int i = 0; i < number; i++)
    {
      int randNumber = rand() %10;
      if (i == 0 && randNumber == 0)
        randNumber++;
      switch (randNumber)
      {
         case 0: result += '0'; break;
         case 1: result += '1'; break;
         case 2: result += '2'; break;
         case 3: result += '3'; break;
         case 4: result += '4'; break;
         case 5: result += '5'; break;
         case 6: result += '6'; break;
         case 7: result += '7'; break;
         case 8: result += '8'; break;
         default : result += '9'; break;
      }
    }
  }

  memcpy(out, result.c_str(), result.length());
  return SQLUDR_SUCCESS;
}



/* genRandomNumber */
SQLUDR_LIBFUNC SQLUDR_INT32 genRandomNumber(SQLUDR_INT32 *in1,
                                            SQLUDR_INT32 *in2,
                                            SQLUDR_CHAR *out,
                                            SQLUDR_INT16 *in1Ind,
                                            SQLUDR_INT16 *in2Ind,
                                            SQLUDR_INT16 *outInd,
                                            SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  std::string result;
  if (*in1Ind == SQLUDR_NULL || *in2Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    srand(*in1); 
    int number = *in2;
    for (int i = 0; i < number; i++)
    {
      int randNumber = rand() %10;
      if (i == 0 && randNumber == 0)
        randNumber++;
      switch (randNumber)
      {
         case 0: result += '0'; break;
         case 1: result += '1'; break;
         case 2: result += '2'; break;
         case 3: result += '3'; break;
         case 4: result += '4'; break;
         case 5: result += '5'; break;
         case 6: result += '6'; break;
         case 7: result += '7'; break;
         case 8: result += '8'; break;
         default : result += '9'; break;
      }
    }
  }
  
  memcpy(out, result.c_str(), result.length());
  return SQLUDR_SUCCESS;
}


SQLUDR_LIBFUNC SQLUDR_INT32 nonDeterministicRandom(SQLUDR_INT32 *out1,
                                                   SQLUDR_INT16 *outInd1,
                                                   SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  // pointer to the buffer in the state area that is
  // available for the lifetime of this statement,
  // this can be used by the UDF to maintain state
  int *my_state = (int *) statearea->stmt_data.data;

  if (calltype == SQLUDR_CALLTYPE_INITIAL && *my_state == 0)
    {
      *my_state = 555;
    }
  else
    // Use a simple linear congruential generator, we want
    // deterministic regression results, despite the name of this
    // function. Note that this UDF is still "not deterministic",
    // since it returns different results when called with the same
    // (empty) inputs.
    *my_state = (13 * (*my_state) + 101) % 1000;

  (*out1) = *my_state;

  return SQLUDR_SUCCESS;
}

SQLUDR_LIBFUNC SQLUDR_INT32 canAccessView(SQLUDR_CHAR *inZoneNeeded,
                                          SQLUDR_CHAR *inZoneHas,
                                          SQLUDR_INT32 *inPackageNeeded,
                                          SQLUDR_INT32 *inPackageHas,
                                          SQLUDR_INT32 *out,
                                          SQLUDR_INT16 *in1Ind,
                                          SQLUDR_INT16 *in2Ind,
                                          SQLUDR_INT16 *in3Ind,
                                          SQLUDR_INT16 *in4Ind,
                                          SQLUDR_INT16 *outInd,
                                          SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  int zoneFound = 0;
  int hasPackage = 0;

  if (*in1Ind == SQLUDR_NULL ||
      *in2Ind == SQLUDR_NULL ||
      *in3Ind == SQLUDR_NULL ||
      *in4Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
     // verify that the requester has privileges to view
     // rows in the requested geographic zone
     std::string theZone(inZoneNeeded);
     std::string zonesAllowed(inZoneHas);
     for (size_t i = 0; i < zonesAllowed.size(); i++)
     {
        if (theZone[0] == zonesAllowed[i])
        {
          zoneFound = 1;
          break;
        }
     }

    // verify that the requester has bought the correct package
    switch (*inPackageNeeded)
    {
      case 1:
        if (*inPackageHas == 1 || *inPackageHas == 3 ||
            *inPackageHas == 5 ||*inPackageHas == 7)
          hasPackage = 1;
          break;
      case 2:
        if (*inPackageHas == 2 || *inPackageHas == 3 ||
            *inPackageHas == 6 || *inPackageHas == 7)
          hasPackage = 1;
          break;
      case 3:
        if (*inPackageHas == 4 || *inPackageHas == 5 ||
            *inPackageHas == 6 || *inPackageHas == 7)
          hasPackage = 1;
          break;
      default:
          hasPackage = 0;
    }
  }
  *out = (zoneFound && hasPackage) ? 1 : 0;
  return SQLUDR_SUCCESS;
}

} /* extern "C" */
