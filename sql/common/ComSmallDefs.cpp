/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         ComSmallDefs.C
 * Description:  Small definitions are declared here that are used throughout
 *               the SQL/ARK product.
 *
 * Created:      10/27/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"

// this file needed for getuid/getpid for UNIX implementation of ComUID
#include <sys/unistd.h>
#include <sys/time.h>



#include <stdio.h>
#include <time.h>

#include <cextdecs/cextdecs.h>



#include "BaseTypes.h"
#include "ComLocationNames.h"
#include "ComOperators.h"
#include "ComSmallDefs.h"
#include "ComASSERT.h"
#include "ComRegAPI.h"

#include "CatSQLShare.h"

#ifdef _DEBUG
#include "ComRtUtils.h"
#endif

// This function now for non-NSKLite platforms only (UNIX)
Int64 ComSmallDef_local_GetTimeStamp(void)
{
  //#if defined(NA_HSC_LINUX) || defined(NA_LINUX)
#if defined(NA_HSC_LINUX)
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return(Int64(tv.tv_usec) + (Int64(tv.tv_sec)*Int64(1000000L)));

#else
  return(JULIANTIMESTAMP());

#endif
}


//**********************************************************************************
//
//  UID and funny name stuff
//
//


void ComUID::make_UID(void)
{

// A UID is based on a 64-bit unique value
//
// For packaging purposes, the generation happens in CatSQLShare, see sqlshare/CatSQLShare.cpp
#ifdef _DEBUG
  // Debug code, to read the UID values from a file specified by an envvar
  char lineFromFile[80];

  if (ComRtGetValueFromFile ("MX_FAKE_UID_FILE", lineFromFile, sizeof(lineFromFile)))
  {
    this->data = atoInt64 (lineFromFile);
    return;
  }
#endif
  this->data = generateUniqueValue ();

}


// This method was adapted from the definition of function
// void convertInt64ToAscii(const Int64 &src, char* tgt)
// defined in w:/common/Int64.cpp
void ComUID::convertTo19BytesFixedWidthStringWithZeroesPrefix (ComString &tgt) const
{
  Int64 temp = get_value();
  char buffer[20];
  char *s = &buffer[occurs(buffer) - 1];
  memset(buffer, '0', occurs(buffer) - 1);
  *s = '\0';
  do {
    unsigned char c = (unsigned char) (temp % 10);
    *--s = (char)(c + '0');
    temp /= 10;
  } while (temp != 0);
  tgt = buffer;
}


// friend
ostream & operator << (ostream &s, const ComUID &uid)
{
  char buf[21];
  Int64 num;
  Int32 i;
  Int32 digit;

  // "buf" is big enough to hold the biggest num possible.   We fill it
  // from right to left 'cause its easier then print from the first
  // (leftmost) valid character of the array
  buf[20] = 0;
  i=19;
  num = uid.data;
  while (num > 0)
  {
    digit = (Int32) (num % 10);          // NT_PORT (10/17/96)
    num = num / 10;
#pragma nowarn(1506)   // warning elimination
    buf[i--] = digit+'0';
#pragma warn(1506)  // warning elimination
  }
  i++;
  while (buf[i] != 0)
  {
    s << buf[i];
    i++;
  }

  return s;
}

