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
#ifndef INT64_H
#define INT64_H
/* -*-C++-*-
**************************************************************************
*
* File:         Int64.h
* Description:  64-bit integer
* Created:      3/5/96
* Language:     C++
*
*
**************************************************************************
*/


#include <limits.h>
#include "Platform.h"

#if   defined( __linux__ ) && defined( NA_64BIT )

  #ifndef LLONG_MAX
    #define LLONG_MAX LONG_MAX
  #endif

  #ifndef LLONG_MIN
    #define LLONG_MIN LONG_MIN
  #endif


#else

#ifndef LLONG_MAX
#define LLONG_MAX _I64_MAX
#endif


#endif

// ***********************************************************************
// Ancillary global functions
// ***********************************************************************

// -----------------------------------------------------------------------
// Convert an unsigned int to Int64.
// ----------------------------------------------------------------------- 
NA_EIDPROC
Int64 uint32ToInt64(UInt32 value);

// -----------------------------------------------------------------------
// Convert an Int64 to long.
// -----------------------------------------------------------------------
NA_EIDPROC
Int32 int64ToInt32(Int64 value);

// -----------------------------------------------------------------------
// Convert the integer from array of two longs, most significant first
// (Guardian-style LARGEINT datatype).
// -----------------------------------------------------------------------
NA_EIDPROC
Int64 uint32ArrayToInt64(const UInt32 array[2]);

// -----------------------------------------------------------------------
// Convert an array of two unsigned longs to the integer, most
// significant first.  This routine also takes care of the little
// endian and big endian problems.  Parameter tgt must point to
// an array of two (2) unsigned long elements.
// -----------------------------------------------------------------------
NA_EIDPROC
void convertInt64ToUInt32Array(const Int64 &src, UInt32 *tgt);

// -----------------------------------------------------------------------
// Convert the integer from ascii.
// -----------------------------------------------------------------------
NA_EIDPROC
Int32 aToInt32(const char* src);

// -----------------------------------------------------------------------
// Convert the integer from ascii.
// -----------------------------------------------------------------------
NA_EIDPROC
Int64 atoInt64(const char* src);

// -----------------------------------------------------------------------
// Convert the integer to ascii.
// -----------------------------------------------------------------------
NA_EIDPROC
void convertInt64ToAscii(const Int64 &src, char* tgt);

// -----------------------------------------------------------------------
// Convert the integer to double.
// -----------------------------------------------------------------------
NA_EIDPROC
double convertInt64ToDouble(const Int64 &src);

#endif /* INT64_H */
