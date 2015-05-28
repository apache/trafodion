// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include "sqludr.h"

extern "C" {

/* ADD2 */
SQLUDR_LIBFUNC SQLUDR_INT32 add2(SQLUDR_INT32 *in1,
                                 SQLUDR_INT32 *in2,
                                 SQLUDR_INT32 *out,
                                 SQLUDR_INT16 *in1Ind,
                                 SQLUDR_INT16 *in2Ind,
                                 SQLUDR_INT16 *outInd,
                                 SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in1Ind == SQLUDR_NULL || *in2Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    (*out) = (*in1) + (*in2);
  }

  return SQLUDR_SUCCESS;
}


/* SWAP2 */
SQLUDR_LIBFUNC SQLUDR_INT32 swap2(SQLUDR_INT32 *in1,
                                  SQLUDR_INT32 *in2,
                                  SQLUDR_INT32 *out1,
                                  SQLUDR_INT32 *out2,
                                  SQLUDR_INT16 *in1Ind,
                                  SQLUDR_INT16 *in2Ind,
                                  SQLUDR_INT16 *out1Ind,
                                  SQLUDR_INT16 *out2Ind,
                                  SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in1Ind == SQLUDR_NULL)
    *out2Ind = SQLUDR_NULL;
  else
    (*out2) = (*in1);

  if (*in2Ind == SQLUDR_NULL)
      *out1Ind = SQLUDR_NULL;
  else
    (*out1) = (*in2);

  return SQLUDR_SUCCESS;
}

/* SWAP2CHAR */
SQLUDR_LIBFUNC SQLUDR_INT32 swap2char(SQLUDR_CHAR *in1,
                                  SQLUDR_CHAR *in2,
                                  SQLUDR_CHAR *out1,
                                  SQLUDR_CHAR *out2,
                                  SQLUDR_INT16 *in1Ind,
                                  SQLUDR_INT16 *in2Ind,
                                  SQLUDR_INT16 *out1Ind,
                                  SQLUDR_INT16 *out2Ind,
                                  SQLUDR_TRAIL_ARGS)
{
  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  if (*in1Ind == SQLUDR_NULL)
    *out2Ind = SQLUDR_NULL;
  else
    strcpy(out2, in1);

  if (*in2Ind == SQLUDR_NULL)
      *out1Ind = SQLUDR_NULL;
  else
    strcpy(out1,in2);

  return SQLUDR_SUCCESS;
}

} /* extern "C" */
