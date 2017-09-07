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
#ifndef COM_SYSUTILS_H
#define COM_SYSUTILS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComSysUtils.h
 * Description:
 *
 * Created:      4/10/96 evening
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "SQLTypeDefs.h"
#include "Int64.h"
#include "Platform.h"


#include <sys/time.h>


#ifdef NA_LITTLE_ENDIAN
#define EXTRACT_SIGN_INT64(sign, ptr) sign = *((Int32 *)ptr +1) & 0x80000000;
#else
#define EXTRACT_SIGN_INT64(sign, ptr) sign = *((Int32 *)ptr) & 0x80000000;
#endif

// **************************************************************************
// *                                                                        *
// *            Structures used by the gettimeofday function.               *
// *                                                                        *
// **************************************************************************
extern "C" {

  // Real Unix need not emulate its native "gettimeofday" (see ComSysUtils.cpp)
#define NA_timeval timeval

struct NA_timezone   /* Only here for the compiler. */
   {
   };

typedef struct NA_timeval TimeVal;
typedef struct NA_timezone TimeZone;
  Int32 NA_gettimeofday(struct NA_timeval *, struct NA_timezone *);

#define GETTIMEOFDAY(tv,tz) NA_gettimeofday(tv,tz)
}

#if   defined(NA_LITTLE_ENDIAN)


inline unsigned short reversebytes( unsigned short s )
{
  return (s>>8 | s<<8);
}

inline unsigned short reversebytesUS( unsigned short s )
{
  return (s>>8 | s<<8);
}

inline ULng32 reversebytes( ULng32 x )
{
  return ((x<<24)
	  |(x<<8 & 0x00ff0000)
	  |(x>>8 & 0x0000ff00)
	  |(x>>24)
	  );
}

//SQ TBD NGG
#if 0
inline Int64 reversebytes( Int64 x )
{
  return ((x<<56)
	  |(x<<40 & 0x00ff000000000000LL)
	  |(x>>40 & 0x000000000000ff00)
	  |(x<<24 & 0x0000ff0000000000LL)
	  |(x>>24 & 0x0000000000ff0000)
	  |(x<<8 & 0x000000ff00000000LL)
	  |(x>>8 & 0x00000000ff000000)
	  |(x>>56)
	  );
}
#endif

inline Int64 reversebytes( Int64 xx )
{
    union {
        Int64 xx;
        char c[8];
    } source;

    union {
        Int64 xx;
        char c[8];
    } sink;

    source.xx = xx;

    for (Int32 i=0; i<8; i++)
        sink.c[i] = source.c[7-i];

    return sink.xx;
}

inline UInt64 reversebytes( UInt64 xx )
{
    union {
        UInt64 xx;
        char c[8];
    } source;

    union {
        UInt64 xx;
        char c[8];
    } sink;

    source.xx = xx;

    for (Int32 i=0; i<8; i++)
        sink.c[i] = source.c[7-i];

    return sink.xx;
}

#endif  // NA_LITTLE_ENDIAN
//----------------------------------------------------------------

#endif


void copyInteger (void *destination, Int32 targetLength, 
		  void *sourceAddress, Int32 sourceLength);

void copyToInteger1 (Int8 *destination, void *sourceAddress, Int32 sourceSize);

void copyToInteger2 (short *destination, void *sourceAddress, Int32 sourceSize);

void copyToInteger4 (Lng32 *destination, void *sourceAddress, Int32 sourceSize);

void copyToInteger8 (Int64 *destination, void *sourceAddress, Int32 sourceSize);
