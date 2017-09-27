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
****************************************************************************
*
* File:         ComTdbSort.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/


#include "ComTdbSort.h"
#include "ComTdbCommon.h"


// Constructor
ComTdbSort::ComTdbSort()
: ComTdb(ComTdb::ex_SORT, eye_SORT),
  tuppIndex_(0)
{
};

ComTdbSort::ComTdbSort(ex_expr * sort_key_expr,
		       ex_expr * sort_rec_expr,
		       ULng32 sort_key_len,
		       ULng32 sort_rec_len,
	               ULng32 sort_partial_key_len,
		       const unsigned short tupp_index,
		       ComTdb * child_tdb,
		       ex_cri_desc * given_cri_desc,
		       ex_cri_desc * returned_cri_desc,
		       ex_cri_desc * work_cri_desc,
		       queue_index down,
		       queue_index up,
		       Cardinality estimatedRowCount,
		       Lng32 num_buffers,
		       ULng32 buffer_size,
		       ULng32 maxNumBuffers,
		       SortOptions * sort_options,
               short sortGrowthPercent)
  : ComTdb(ComTdb::ex_SORT,
	   eye_SORT,
	   estimatedRowCount,
	   given_cri_desc,
	   returned_cri_desc,
	   down,
	   up,
	   num_buffers,
	   buffer_size),
  sortKeyExpr_(sort_key_expr),
  sortRecExpr_(sort_rec_expr),
#pragma nowarn(1506)   // warning elimination
  sortRecLen_(sort_rec_len),
  sortKeyLen_(sort_key_len),
  sortPartialKeyLen_(sort_partial_key_len),
  minimalSortRecs_(0),
#pragma warn(1506)  // warning elimination
  tuppIndex_(tupp_index),
  tdbChild_(child_tdb),
  workCriDesc_(work_cri_desc),
#pragma nowarn(1506)   // warning elimination 
  maxNumBuffers_(maxNumBuffers),
#pragma warn(1506)  // warning elimination 
  sortOptions_(sort_options),
  flags_(0),
  sortMemEstInKBPerNode_(0),
  sortGrowthPercent_(sortGrowthPercent),
  bmoCitizenshipFactor_(0),
  pMemoryContingencyMB_(0),
  topNThreshold_(-1) 
{

}

ComTdbSort::~ComTdbSort()
{
}

void ComTdbSort::display() const {};

Long ComTdbSort::pack(void * space)
{
  sortOptions_.pack(space);
  tdbChild_.pack(space);
  workCriDesc_.pack(space);
  sortKeyExpr_.pack(space);
  sortRecExpr_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbSort::unpack(void * base, void * reallocator)
{
  if(sortOptions_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(tdbChild_.unpack(base, reallocator)) return -1;
  if(sortKeyExpr_.unpack(base, reallocator)) return -1;
  if(sortRecExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbSort::displayContents(Space * space,ULng32 flag)
{
      ComTdb::displayContents(space,flag & 0xFFFFFFFE);

      if(flag & 0x00000008)
        {
            char buf[100];
   	    str_sprintf(buf, "\nFor ComTdbSort :");
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                    sizeof(short));

            str_sprintf(buf,"Flags = %x, sortRecLen = %d, sortKeyLen = %d",
		      	  flags_,sortRecLen_,sortKeyLen_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                    sizeof(short));

            str_sprintf(buf,"sortPartialKeyLen = %d, maxNumBuffers = %d",
		      	  sortPartialKeyLen_,maxNumBuffers_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                    sizeof(short));

            str_sprintf(buf,"minimalSortRecs = %d",minimalSortRecs_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	    str_sprintf(buf,"tuppIndex_ = %d", tuppIndex_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

            str_sprintf(buf,"sortMemEstInKBPerNode_ = %f, estimateErrorPenalty = %d ",
                sortMemEstInKBPerNode_, sortGrowthPercent_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

            str_sprintf(buf,"bmoCitizenshipFactor = %f, PhyMemoryContingencyMB = %d ",
                bmoCitizenshipFactor_, pMemoryContingencyMB_);
            space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

            sortOptions_->displayContents(space);
        }

      if(flag & 0x00000001)
        {
        		displayExpression(space,flag);
            displayChildren(space,flag);
        }
}

Int32 ComTdbSort::orderedQueueProtocol() const {return -1;};

void SortOptions::displayContents(Space *space)
{
	char buf[100];

   str_sprintf(buf, "\nFor SortOptions:\nsortNRows = %d, sortType = %d, internalSort = %d",
   							sortNRows_,sortType_,internalSort_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

   str_sprintf(buf,"Sort Option Flags = %x", flags_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf),sizeof(short));
   
   str_sprintf(buf, "sortMaxHeapSizeMB = %d, scratchFreeSpaceThreshold = %d",
                            sortMaxHeapSizeMB_,scratchFreeSpaceThreshold_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

   str_sprintf(buf,"dontOverflow = %d, memoryQuotaMB = %d",
                dontOverflow_, memoryQuotaMB_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   
   str_sprintf(buf,"pressureThreshold = %d, mergeBufferUnit = %d",pressureThreshold_, mergeBufferUnit_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

   str_sprintf(buf,"scratchIOBlockSize = %d",scratchIOBlockSize_);
   space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}

