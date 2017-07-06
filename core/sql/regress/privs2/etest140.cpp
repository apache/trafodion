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
#include <bitset>
#include "sqludr.h"

extern "C" {

/* TRANSLATEBITMAP */
SQLUDR_LIBFUNC SQLUDR_INT32 translateBitmap(SQLUDR_INT32 *in1,
                                            SQLUDR_CHAR *out,
                                            SQLUDR_INT16 *in1Ind,
                                            SQLUDR_INT16 *outInd,
                                            SQLUDR_TRAIL_ARGS)
{
  enum UDR_PRIVILEGE { SELECT_PRIV = 0,
                       INSERT_PRIV,
                       DELETE_PRIV,
                       UPDATE_PRIV,
                       USAGE_PRIV,
                       REFERENCES_PRIV,
                       EXECUTE_PRIV };

  if (calltype == SQLUDR_CALLTYPE_FINAL)
    return SQLUDR_SUCCESS;

  std::string result;
  if (*in1Ind == SQLUDR_NULL)
  {
    *outInd = SQLUDR_NULL;
  }
  else
  {
    std::bitset<7> privs;
    privs = *in1;
    if (privs.none())
      result = "NONE";
    else
    {
      if (privs.test(SELECT_PRIV))
        result += "S";
      else
        result += '-';
      if (privs.test(INSERT_PRIV))
        result += "I";
      else
        result += '-';
      if (privs.test(DELETE_PRIV))
        result += "D";
      else
        result += '-';
      if (privs.test(UPDATE_PRIV))
        result += "U";
      else
        result += '-';
      if (privs.test(USAGE_PRIV))
        result += "G";
      else
        result += '-';
      if (privs.test(REFERENCES_PRIV))
        result += "R";
      else
        result += '-';
      if (privs.test(EXECUTE_PRIV))
        result += "E";
      else
        result += '-';
    }
  }

  memcpy(out, result.c_str(), result.length());
  return SQLUDR_SUCCESS;
}


} /* extern "C" */
