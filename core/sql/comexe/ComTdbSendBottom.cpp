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
* File:         ComTdbSendBottom.cpp
* Description:  Send bottom node (server part of a point to point connection)
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

// -----------------------------------------------------------------------

#include "ComTdbSendBottom.h"
#include "ComTdbCommon.h"

// -----------------------------------------------------------------------
// Methods for class ComTdbSendBottom
// -----------------------------------------------------------------------

ComTdbSendBottom::ComTdbSendBottom(
				   ex_expr      *moveOutputValues,
				   queue_index  downSize,
				   queue_index  upSize,
				   ex_cri_desc  *criDescDown,
				   ex_cri_desc  *criDescUp,
				   ex_cri_desc  *workCriDesc,
				   Lng32         moveExprTuppIndex,
				   Lng32         downRecordLength,
				   Lng32         upRecordLength,
				   Lng32         requestBufferSize,
				   Lng32         numRequestBuffers,
				   Lng32	  replyBufferSize,
				   Lng32         numReplyBuffers,
				   Cardinality  estNumRowsRequested,
				   Cardinality  estNumRowsReplied) : 
  ComTdb(ex_SEND_BOTTOM,
	 eye_SEND_BOTTOM,
	 estNumRowsReplied,
	 criDescDown,
	 criDescUp,
	 downSize,
	 upSize)
{
  moveOutputValues_    = moveOutputValues;
  workCriDesc_         = workCriDesc;
  moveExprTuppIndex_   = moveExprTuppIndex;
  downRecordLength_    = downRecordLength;
  upRecordLength_      = upRecordLength;
  requestBufferSize_   = requestBufferSize;
  numRequestBuffers_   = numRequestBuffers;
  replyBufferSize_     = replyBufferSize;
  numReplyBuffers_     = numReplyBuffers;
  p_estNumRowsRequested_ = estNumRowsRequested;
  p_estNumRowsReplied_   = estNumRowsReplied;
  sendBottomFlags_       = 0;
  smTag_                 = 0;
}
  
Int32 ComTdbSendBottom::orderedQueueProtocol() const
{
  return TRUE;
} // these 3 lines won't be covered, obsolete
  
void ComTdbSendBottom::display() const
{
} // ignore these lines, used by Windows GUI only

const ComTdb* ComTdbSendBottom::getChild(Int32 pos) const
{
  return NULL;
} // these lines won't be covered, it's never called as it has no child opr

Int32 ComTdbSendBottom::numChildren() const
{
  return 0;
}

Int32 ComTdbSendBottom::numExpressions() const
{
  return 1;
}

ex_expr* ComTdbSendBottom::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return moveOutputValues_;
  return NULL;
}

const char * ComTdbSendBottom::getExpressionName(Int32 pos) const
{
  if (pos == 0)
    return "moveOutputValues_";
  return NULL;
}

Long ComTdbSendBottom::pack(void * space)
{
  moveOutputValues_.pack(space);
  workCriDesc_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbSendBottom::unpack(void * base, void * reallocator)
{
  if(moveOutputValues_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbSendBottom::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space, flag & 0xFFFFFFFE);
  
  if (flag & 0x00000008)
  {
    char buf[256];
    
    str_sprintf(buf, "\nFor ComTdbSendBottom :");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    
    str_sprintf(buf, "sendBottomFlags_ = %x", (Lng32) sendBottomFlags_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "downRecordLength_ = %d, upRecordLength_ = %d",
                downRecordLength_, upRecordLength_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "requestBufferSize_ = %d, replyBufferSize_ = %d",
                requestBufferSize_, replyBufferSize_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "numRequestBuffers_ = %d, numReplyBuffers_ = %d",
                numRequestBuffers_, numReplyBuffers_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "Exchange uses SeaMonster: %s",
                getExchangeUsesSM() ? "yes" : "no");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
  }

  if (flag & 0x00000001)
  {
    displayExpression(space,flag);
    displayChildren(space,flag);
  }
}
