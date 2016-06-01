/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComExtents.C
 * Description:  Provides conversion functions to convert Maxsize and Allocate
 *               attributes to primary-extents, secondary-extents and max-extents
 *
 * Created:      11/28/94
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "ComExtents.h"
#include "Int64.h"
#include "ComASSERT.h"

ComExtents::ComExtents (Int64      maxSize,
	                ComUnits   units)
: maxSize_(maxSize)
, units_(units)
{
  // ---------------------------------------------------------------------
  // Calculate the extent size:
  //
  // maxFileSize = MAX(maxSizeInBytes, 20MB)
  //
  // maxFileSize = MIN(maxFileSize, 2gbytes) -- make sure this size
  //   does not go over 2gbytes (largest supported by NSK)
  //
  // Extent sizes are no longer calculated because DP2 decides on
  // the extent sizes to use.
  // ---------------------------------------------------------------------

   const Int64 maxSizeInBytes = getSizeInBytes ( maxSize_
                                               , units_
                                               );

   Int64 maxFileSize = maxSizeInBytes;
   // If maxSize_ is too small, set it to the minimum allowed (in bytes).
   if (maxFileSize < COM_MIN_PART_SIZE_IN_BYTES)
   {
     maxSize_ = COM_MIN_PART_SIZE_IN_BYTES;
     units_ = COM_BYTES;
   }
   // If maxSize_ is too large, set it to the maximum allowed (in bytes).
   else if (maxFileSize > COM_MAX_PART_SIZE_IN_BYTES)
   {
     maxSize_ = COM_MAX_PART_SIZE_IN_BYTES;
     units_ = COM_BYTES;
   }
   // If maxSize_ is within the allowed range, leave it and units_ unchanged.
};

ComExtents::ComExtents (Int64 maxSize)
:  maxSize_(maxSize)
{
  // Since units_ was unspecified, maxSize is bytes.
  units_ = COM_BYTES;

  // If maxSize_ is too small, set it to the minimum allowed.
  if (maxSize < COM_MIN_PART_SIZE_IN_BYTES)
     maxSize_ = COM_MIN_PART_SIZE_IN_BYTES;
  // If maxSize_ is too large, set it to the maximum allowed.
  else if (maxSize > COM_MAX_PART_SIZE_IN_BYTES)
    maxSize_ = COM_MAX_PART_SIZE_IN_BYTES;
};

// -----------------------------------------------------------------------
// getSizeInBytes:
//
// This function calculates the size of the input parameter in bytes
// -----------------------------------------------------------------------
Int64 ComExtents::getSizeInBytes         ( Int64 sizeToConvert
		                         , ComUnits  units
					 )
{
  Int64 convertedSize = 0;

  switch (units)
    {
    case COM_BYTES:
      convertedSize = (sizeToConvert);
      break;
    case COM_KBYTES:
      convertedSize = (sizeToConvert * 1024);
      break;
    case COM_MBYTES:
      convertedSize = (sizeToConvert * 1024 * 1024);
      break;
    case COM_GBYTES:
      convertedSize = (sizeToConvert * (1024 * 1024 * 1024));
      break;
    default:
      ComASSERT( FALSE ); // raise an exception
      break;
    };
  return convertedSize;
};


