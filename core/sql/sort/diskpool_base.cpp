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

#include "Platform.h"
#include "DiskPool.h"
#include "ComDistribution.h"

#include <assert.h>
#if !defined( FORDEBUG ) && !defined( NDEBUG )
#define NDEBUG
#endif

//--------------------------------------------------------------------------
//DiskPool()
//    The constructor.
//--------------------------------------------------------------------------
DiskPool::DiskPool(CollHeap *heap) 
          :numberOfDisks_(0),
           numberOfLocalDisks_(0),
           diskTablePtr_(NULL),
           localDisksPtr_(NULL),
           currentDisk_(-1),
           scratchSpace_(NULL),
           heap_(heap)
{
}

//--------------------------------------------------------------------------
//~DiskPool()
//    The destructor, it will destroy array of DiskDetailsPtr if it
//    is created.  
//--------------------------------------------------------------------------
DiskPool::~DiskPool()
{
  // This dtor is pure-virtual: The derived classes would do the deletions.
  //  NADELETEARRAY(diskTablePtr_, numberOfDisks_, DiskDetailsPtr, heap_ );
  diskTablePtr_ = NULL;
  localDisksPtr_ = NULL;
}

