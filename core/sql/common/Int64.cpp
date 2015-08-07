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
**************************************************************************
*
* File:         Int64.C
* Description:  64-bit integer
* Created:      3/5/96
* Language:     C++
*
*
*
*
**************************************************************************
*/


#include "Int64.h"
#include "NABoolean.h"
#include "str.h"
#include "NAStdlib.h"

Int64 uint32ToInt64(UInt32 value)
{
  return (Int64) value;
}

Int32 int64ToInt32(Int64 value)
{
  UInt32 val32u;
  Int32 val32;

  val32u = (UInt32) value;
  val32 = (Int32)val32u;

  return val32;
}

double convertInt64ToDouble(const Int64 &src)
{
  return (double) src;
}


Int64 uint32ArrayToInt64(const UInt32 array[2])
{
  Int64 result = uint32ToInt64(array[0]);
  Int64 array1 = uint32ToInt64(array[1]);
  Int64 shift = INT_MAX;	// 2^31 - 1
  shift  += 1;			// 2^31
  result *= shift;
  result *= 2;			// 2*32, so result now has array[0] in high word
  result += array1;		// and array[1] in low word
  return result;
}

Int32 aToInt32(const char* src)
{
  NABoolean isNeg = FALSE;
  if (*src == '-')
  {
    isNeg = TRUE;
    src++;
  }

  Int32 tgt = 0;
  while ((*src >= '0') && (*src <= '9')) {
    tgt = tgt * 10 + (*src - '0');
    src++;
  }
  
  if (isNeg)
    return -tgt;
  else
    return tgt;
}

Int64 atoInt64(const char* src)
{
  NABoolean isNeg = FALSE;
  if (*src == '-')
  {
    isNeg = TRUE;
    src++;
  }

  Int64 tgt = 0;
  while ((*src >= '0') && (*src <= '9')) {
    tgt = tgt * 10 + (*src - '0');
    src++;
  }
  
  if (isNeg)
    return -tgt;
  else
    return tgt;

 
}

void convertInt64ToAscii(const Int64 &src, char* tgt)
{
  Int64 temp = src;  // (src >= 0) ? src : - src;
  char buffer[21];
  char *s = &buffer[21];
  *--s = '\0';
  do {
    char c = (char) (temp % 10);
    if (c < 0)
      c = -c;
    *--s = (char)(c + '0');
    temp /= 10;
  } while (temp != 0);
  if (src < 0)
    *--s = '-';
  strcpy(tgt, s);
}

void convertInt64ToUInt32Array(const Int64 &src, UInt32 *tgt)
{
  Lng32 *tPtr = (Lng32 *) &src;
#ifdef NA_LITTLE_ENDIAN
  tgt[0] = tPtr[1];
  tgt[1] = tPtr[0];
#else
  tgt[0] = tPtr[0];
  tgt[1] = tPtr[1];
#endif
}

//
// End of File
//
