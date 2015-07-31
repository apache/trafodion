/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComExtents.h
 * Description:  Provides conversion functions to convert Maxsize and Allocate
 *               attributes to primary-extents, secondary-extents and max-extents
 *               
 *               
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

// -----------------------------------------------------------------------


#ifndef _COM_EXTENTS_H_
#define _COM_EXTENTS_H_

#include "ComUnits.h"
#include "ComSmallDefs.h"
#include "Int64.h"

class ComExtents
{
  public:
  // ---------------------------------------------------------------------
  // Constructors & destructor
  // ---------------------------------------------------------------------
   ComExtents (Int64  maxSize,
	       ComUnits   units);

   ComExtents ( Int64 maxSize);

   ComExtents (const ComExtents &rhs);

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------

   
   Int64    getMaxSize             (void) const;
   ComUnits getMaxSizeUnits        (void) const;
   Int64 getSizeInBytes         (Int64 sizeToConvert
  		                      ,ComUnits  units
	  		              );  


  protected:

  private:
  // ---------------------------------------------------------------------
  // Private data members
  // ---------------------------------------------------------------------
   Int64      maxSize_;
   ComUnits   units_;   

};

inline
Int64 ComExtents::getMaxSize (void) const
{
   return maxSize_;
};

inline
ComUnits ComExtents::getMaxSizeUnits (void) const
{
   return units_;
};



#endif //_COM_EXTENTS_H_
