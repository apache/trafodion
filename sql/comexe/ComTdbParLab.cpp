/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2005-2014 Hewlett-Packard Development Company, L.P.
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
****************************************************************************
*
* File:         ComTdbParLab.cpp
* Description:  
*
* Created:      6/1/2005
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbParLab.h"
#include "ComTdbCommon.h"
#include "ComQueue.h"
#include "str.h"
#include "SqlTableOpenInfo.h" // for SqlTableOpenInfo

ParallelLabelOpPartInfo::ParallelLabelOpPartInfo(void) :
     guardianName_(NULL),
     logicalPartName_(NULL),
     lowKey_(NULL),
     highKey_(NULL),
     priExt_(0),
     secExt_(0),
     maxExt_(0)
{
   memset(filler_, 0, sizeof(filler_));
}

ParallelLabelOpPartInfo::~ParallelLabelOpPartInfo(void)
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
////////////////////////////////////////////////////////////////////////
ComTdbParallelLabelOp::ComTdbParallelLabelOp(void) :
                    ComTdb(ex_PARALLEL_LABEL_OP, eye_PARALLEL_LABEL_OP)
{
   objectName_ = (char *) NULL ; 
   operation_ = -1;
   timeout_ = -1;
   partInfoArray_ = NULL;
   memset(fillerComTdbParLabOp_, 0, sizeof(fillerComTdbParLabOp_));
}

ComTdbParallelLabelOp::ComTdbParallelLabelOp(const char *objectName,
                                             short operation,
                                             Lng32 numParts,
                                             ParallelLabelOpPartInfo *partInfo,
 					     short altOpCode,
					     Lng32 timeout,
                                             ex_cri_desc *given_cri_desc,
                                             ex_cri_desc *returned_cri_desc,
                                             queue_index upQueueSize,
                                             queue_index downQueueSize,
                                             Lng32 numBuffers,
                                             ULng32 bufferSize) :
                    ComTdb(ex_PARALLEL_LABEL_OP, eye_PARALLEL_LABEL_OP,
                           (Cardinality) 0.0, given_cri_desc,
                           returned_cri_desc,
                           downQueueSize,
                           upQueueSize,
                           numBuffers,
                           bufferSize),
		    timeout_(timeout)
{
   objectName_ = objectName;
   operation_ = operation;
   numParts_ = numParts;
   partInfoArray_ = partInfo;
   altOpCode_ = altOpCode;
}

ComTdbParallelLabelOp::~ComTdbParallelLabelOp(void)
{
}

Long ComTdbParallelLabelOp::pack(void * space)
{
   objectName_.pack(space);
   partInfoArray_.packArray(space, numParts_);
   return ComTdb::pack(space);
}

Lng32 ComTdbParallelLabelOp::unpack(void * base, void * reallocator)
{
   if (objectName_.unpack(base)) return -1;
   if (partInfoArray_.unpackArray(base, numParts_, reallocator)) return -1; 
   return ComTdb::unpack(base, reallocator);
}

void ComTdbParallelLabelOp::displayContents(Space *space, ULng32 flag)
{
   ComTdb::displayContents(space, flag & 0xFFFFFFFE);
   if (flag & 0x00000008)
   {
      char buf[100];
      str_sprintf(buf,"\nFor ComTdbParallelLabelOp: ");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf,"Operation = %d", operation_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
   }

   if (flag & 0x00000001)
   {
      displayExpression(space, flag);
      displayChildren(space, flag);
   }
}

ComTdbDiskLabelStatistics::ComTdbDiskLabelStatistics (
     const char* objectName,
     Queue * tableOpenInfoList,
     Lng32 returnedRowLen,
     Lng32 numParts,
     ParallelLabelOpPartInfo *partInfo,
     Lng32 timeout,
     ex_cri_desc * given_cri_desc,
     ex_cri_desc * returned_cri_desc,
     queue_index upQueueSize,
     queue_index downQueueSize,
     Lng32 numBuffers,
     ULng32 bufferSize)
     : ComTdbParallelLabelOp(objectName, DISK_COUNT_OP,
                             numParts, partInfo,
			     0, timeout, 
			     given_cri_desc, returned_cri_desc,
			     upQueueSize, downQueueSize,
			     numBuffers, bufferSize),
       tableOpenInfoList_(tableOpenInfoList),
       returnedRowLen_(returnedRowLen),
       flags_(0)
{
  setNodeType(ComTdb::ex_DISK_LABEL_STATISTICS);
  
  tuppIndex_ = returned_cri_desc->noTuples() - 1;
}

Long ComTdbDiskLabelStatistics::pack(void * space)
{
  PackQueueOfNAVersionedObjects(tableOpenInfoList_,space,SqlTableOpenInfo);

  return ComTdbParallelLabelOp::pack(space);
}

Lng32 ComTdbDiskLabelStatistics::unpack(void * base, void * reallocator)
{
  UnpackQueueOfNAVersionedObjects(tableOpenInfoList_,base,SqlTableOpenInfo,reallocator);

  return ComTdbParallelLabelOp::unpack(base, reallocator);
}


