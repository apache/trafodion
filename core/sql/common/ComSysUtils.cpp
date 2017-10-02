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
 *****************************************************************************
 *
 * File:         ComSysUtil.C
 * Description:  Utility functions required in arkcode which do make
 *               system or runtime library calls.
 *
 * Created:      4/10/96, evening
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include "ComSysUtils.h"
#include "NAAssert.h"


//----------------------------------------------------------------
// This section of the code should be removed when the real
// gettimeofday call is available under OSS. Also remember
// to remove the function declaration from SqlciStats.h 


// ****************************************************************************
// *                                                                          *
// * Function: gettimeofday                                                   *
// *                                                                          *
// *    This function partially duplicates the UNIX function by the same      *
// *    name.  In particular, nothing is returned in the timezone argument    *
// *    which is asserted to be zero.                                         *
// *                                                                          *
// * NOTE:     The resolution of the value returned in the timeval argument   *
// *           is 1 MILLIsecond!                                              *
// *              =============                                               *
// *                                                                          *
// ****************************************************************************
// *                                                                          *
// *  Parameters:                                                             *
// *                                                                          *
// *  <tp>                      struct timeval *                Out           *
// *    is used to return the seconds and microseconds since 12:00 am on      *
// *    January 1st, 1970 in the tv_sec and tv_usec fields, repectively, per  *
// *    the UNIX documentation.                                               *
// *                                                                          *
// *  <tzp>                     struct timezone *               In            *
// *    is not used and is asserted to be zero.                               *
// *                                                                          *
// *                                                                          *
// ****************************************************************************
// *                                                                          *
// *  Returns: 0, if all ok, -1, if error.                                    *
// *                                                                          *
// ****************************************************************************

extern "C" {
Int32 NA_gettimeofday(struct NA_timeval *tp, struct NA_timezone *tzp)
  {
    return gettimeofday(tp, 0);


   return(0);
   }
   
} // extern "C"

//----------------------------------------------------------------

void copyInteger (void *destination, Int32 targetLength, 
		  void *sourceAddress, Int32 sourceLength)
{
  switch (targetLength)
    {
    case SQL_TINY_SIZE: {
      Int8 *target = (Int8 *)destination;
      copyToInteger1 (target, sourceAddress, sourceLength);
      break;
    }
    case SQL_SMALL_SIZE: {
      short *target = (short *)destination;
      copyToInteger2 (target, sourceAddress, sourceLength);
      break;
    }
    case SQL_INT_SIZE: {
      Lng32 *target = (Lng32 *)destination;
      copyToInteger4 (target, sourceAddress, sourceLength);
      break;
    }

    case SQL_LARGE_SIZE: {
      Int64 *target = (Int64 *)destination;
      copyToInteger8 (target, sourceAddress, sourceLength);
      break;
    }

    default :
      break;
    }

}

void copyToInteger1 (Int8 *destination, void *sourceAddress, Int32 sourceSize)
{
  switch (sourceSize)
    {
    case SQL_TINY_SIZE: {
      Int8 *source = (Int8 *)sourceAddress;
      *destination = *source;
      break;
    }

    case SQL_SMALL_SIZE: {
      short *source = (short *)sourceAddress;
      *destination = (Int8)*source;
      break;
    }

    case SQL_INT_SIZE: {
      Lng32 *source = (Lng32 *)sourceAddress;
      *destination = (Int8)*source;
      break;
    }

    case SQL_LARGE_SIZE: {
      Int64 *source = (Int64 *)sourceAddress;
      *destination = (Int8)*source;
      break;
    }

    default :
      break;

    }


}

void copyToInteger2 (short *destination, void *sourceAddress, Int32 sourceSize)
{
  switch (sourceSize)
    {
    case SQL_SMALL_SIZE: {
      short *source = (short *)sourceAddress;
	  *destination = *source;
      break;
    }

    case SQL_INT_SIZE: {
      Lng32 *source = (Lng32 *)sourceAddress;
	  *destination = (short)*source;
	  break;
    }

    case SQL_LARGE_SIZE: {
		Int64 *source = (Int64 *)sourceAddress;
	    *destination = (short)*source;
		break;
    }

    default :
      break;

    }


}


void copyToInteger4 (Lng32 *destination, void *sourceAddress, Int32 sourceSize)
{
  switch (sourceSize)
    {
    case SQL_SMALL_SIZE:{
      short *source = (short *)sourceAddress;
	  *destination = (Lng32)*source;
      break;
    }

    case SQL_INT_SIZE: {
      Lng32 *source = (Lng32 *)sourceAddress;
	  *destination = *source;
      break;
    }

    case SQL_LARGE_SIZE: {
      Int64 *source = (Int64 *)sourceAddress;
	  *destination = (Lng32)*source;
      break;
    }

    default :
      break;

    }

  
}

void copyToInteger8 (Int64 *destination, void *sourceAddress, Int32 sourceSize)
{
  switch (sourceSize)
    {
    case SQL_SMALL_SIZE:{
      short *source = (short *)sourceAddress;
	  *destination = (Int64)*source;
      break;
    }

    case SQL_INT_SIZE: {
      Lng32 *source = (Lng32 *)sourceAddress;
	  *destination = (Int64)*source;
      break;
    }

    case SQL_LARGE_SIZE: {
      Int64 *source = (Int64 *)sourceAddress;
	  *destination = *source;
      break;
    }

    default :
      break;
    }

  
}
