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
* File:         ComTdbHashGrby.cpp
* Description:  Methods for the tdb of hash aggregate/grby operator
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbHashGrby.h"
#include "ComTdbCommon.h"
#include "ExpSqlTupp.h"          // for sizeof(tupp_descriptor).

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Constructor
ComTdbHashGrby::ComTdbHashGrby()
  : ComTdb(ComTdb::ex_HASH_GRBY, eye_HASH_GRBY) {};

ComTdbHashGrby::ComTdbHashGrby(ComTdb * childTdb,
			       ex_cri_desc * givenDesc,
			       ex_cri_desc * returnedDesc,
			       ex_expr * hashExpr,
			       ex_expr * bitMuxExpr,
			       ex_expr * bitMuxAggrExpr,
			       ex_expr * hbMoveInExpr,
			       ex_expr * ofMoveInExpr,
			       ex_expr * resMoveInExpr,
			       ex_expr * hbAggrExpr,
			       ex_expr * ofAggrExpr,
			       ex_expr * resAggrExpr,
			       ex_expr * havingExpr,
			       ex_expr * moveOutExpr,
			       ex_expr * hbSearchExpr,
			       ex_expr * ofSearchExpr,
			       ULng32 keyLength,
			       ULng32 resultRowLength,
			       ULng32 extGroupedRowLength,
			       ex_cri_desc * workCriDesc,
			       short hbRowAtpIndex,
			       short ofRowAtpIndex,
			       short hashValueAtpIndex,
			       short bitMuxAtpIndex,
			       short bitMuxCountOffset,
			       short resultRowAtpIndex,
			       short returnedAtpIndex,
			       unsigned short memUsagePercent,
			       short pressureThreshold,
                               short scratchThresholdPct,
			       queue_index fromParent,
			       queue_index toParent,
			       NABoolean isPartialGroup,
			       Cardinality estimatedRowCount,
			       Lng32 numBuffers,
			       ULng32 bufferSize,
			       ULng32 partialGrbyFlushThreshold,
			       ULng32 partialGrbyRowsPerCluster,
			       ULng32 initialHashTableSize,
			       unsigned short minBuffersToFlush,
			       ULng32 numInBatch,
                               short hgbGrowthPercent
)
  : ComTdb(ComTdb::ex_HASH_GRBY,
	   eye_HASH_GRBY,
	   estimatedRowCount,
           givenDesc,
	   returnedDesc,
           fromParent,
	   toParent,
           numBuffers,
	   bufferSize),
    childTdb_(childTdb),
    hashExpr_(hashExpr),
    bitMuxExpr_(bitMuxExpr),
    bitMuxAggrExpr_(bitMuxAggrExpr),
    hbMoveInExpr_(hbMoveInExpr),
    ofMoveInExpr_(ofMoveInExpr),
    resMoveInExpr_(resMoveInExpr),
    hbAggrExpr_(hbAggrExpr),
    ofAggrExpr_(ofAggrExpr),
    resAggrExpr_(resAggrExpr),
    havingExpr_(havingExpr),
    moveOutExpr_(moveOutExpr),
    hbSearchExpr_(hbSearchExpr),
    ofSearchExpr_(ofSearchExpr),
    keyLength_(keyLength),
    resultRowLength_(resultRowLength),
    extGroupedRowLength_(extGroupedRowLength),
    workCriDesc_(workCriDesc),
    hbRowAtpIndex_(hbRowAtpIndex),
    ofRowAtpIndex_(ofRowAtpIndex),
    hashValueAtpIndex_(hashValueAtpIndex),
    bitMuxAtpIndex_(bitMuxAtpIndex),
    bitMuxCountOffset_(bitMuxCountOffset),
    resultRowAtpIndex_(resultRowAtpIndex),
    returnedAtpIndex_(returnedAtpIndex),
    memUsagePercent_(memUsagePercent),
    pressureThreshold_(pressureThreshold),
    scratchThresholdPct_(scratchThresholdPct),
    hashGroupByFlags_(0),
    memoryQuotaMB_(0),
    isPartialGroup_(isPartialGroup),
    partialGrbyFlushThreshold_(partialGrbyFlushThreshold),
    partialGrbyRowsPerCluster_(partialGrbyRowsPerCluster),
    initialHashTableSize_(initialHashTableSize),
    minBuffersToFlush_(minBuffersToFlush),
    numInBatch_(numInBatch),
    bmoMinMemBeforePressureCheck_(0),
    bmoMaxMemThresholdMB_(0),
    hgbGrowthPercent_(hgbGrowthPercent)
{
      memset(fillersComTdbHashGrby_, 0, sizeof(fillersComTdbHashGrby_));

      // This code is moved to the HashGroupBy::codeGen() right before
      // we call this constructor. This is becasue the minimum estimated 
      // row count is now controlled by CQD GEN_HGBY_MIN_ESTIMATED_ROW_COUNT.
      

      // for the estimates, we get at least one result row. Otherwise
      // the calculations for #buckets/#clusters might fail
      // if (getEstimatedRowCount() < 1.0)
      // setEstimatedRowCount((float)1.0);

      // also, make sure that we don't run into overflow problems.
      // estimatedRowCount_ is an estimate. Therfore it is ok to limit it

      // the c89 doesn't handle the direct comparison of float and
      // UINT_MAX correctly. For now we use 4294967295.0!!!!!!!
      // if (getEstimatedRowCount() > 4294967295.0)
      // setEstimatedRowCount((float)4294967295.0);

};

ComTdbHashGrby::~ComTdbHashGrby(){
};

void ComTdbHashGrby::display() const {};

Int32 ComTdbHashGrby::orderedQueueProtocol() const {
  return -1;
}

Long ComTdbHashGrby::pack(void * space) {
  workCriDesc_.pack(space);
  childTdb_.pack(space);
  hashExpr_.pack(space);
  bitMuxExpr_.pack(space);
  bitMuxAggrExpr_.pack(space);
  hbMoveInExpr_.pack(space);
  ofMoveInExpr_.pack(space);
  resMoveInExpr_.pack(space);
  hbAggrExpr_.pack(space);
  ofAggrExpr_.pack(space);
  resAggrExpr_.pack(space);
  havingExpr_.pack(space);
  moveOutExpr_.pack(space);
  hbSearchExpr_.pack(space);
  ofSearchExpr_.pack(space);
  return ComTdb::pack(space);
};

Lng32 ComTdbHashGrby::unpack(void * base, void * reallocator) {
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(childTdb_.unpack(base, reallocator)) return -1;
  if(hashExpr_.unpack(base, reallocator)) return -1;
  if(bitMuxExpr_.unpack(base, reallocator)) return -1;
  if(bitMuxAggrExpr_.unpack(base, reallocator)) return -1;
  if(hbMoveInExpr_.unpack(base, reallocator)) return -1;
  if(ofMoveInExpr_.unpack(base, reallocator)) return -1;
  if(resMoveInExpr_.unpack(base, reallocator)) return -1;
  if(hbAggrExpr_.unpack(base, reallocator)) return -1;
  if(ofAggrExpr_.unpack(base, reallocator)) return -1;
  if(resAggrExpr_.unpack(base, reallocator)) return -1;
  if(havingExpr_.unpack(base, reallocator)) return -1;
  if(moveOutExpr_.unpack(base, reallocator)) return -1;
  if(hbSearchExpr_.unpack(base, reallocator)) return -1;
  if(ofSearchExpr_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
};

void ComTdbHashGrby::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
      char buf[100];
      str_sprintf(buf, "\nFor ComTdbHashGrby :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"resultRowLength = %d, extGroupedRowLength = %d",
		  resultRowLength_,extGroupedRowLength_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"keyLength = %d, isPartialGroup = %d, initialHashTableSize = %d",
		  keyLength_,isPartialGroup_,initialHashTableSize_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf,"memUsagePercent = %d, pressureThreshold = %d, minBuffersToFlush = %d",
		  memUsagePercent_,pressureThreshold_, minBuffersToFlush_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf,"logDiagnostics = %d", logDiagnostics());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "memoryQuotaMB = %d", memoryQuotaMB());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }
  
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}


