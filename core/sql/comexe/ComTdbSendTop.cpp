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
* File:         ComTdbSendTop.cpp
* Description:  Send top node (client part of client-server connection)
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

#include "ComTdbSendTop.h"
#include "ComTdbCommon.h"
#include "sql_buffer.h"
#include "sql_buffer_size.h"  // for SqlBuffer::neededSize().

// -----------------------------------------------------------------------
// Methods for class ComTdbSendTop
// -----------------------------------------------------------------------

ComTdbSendTop::ComTdbSendTop(ExFragId           childFragId,
			     ex_expr            *moveInputValues,
			     ex_cri_desc        *criDescDown,
			     ex_cri_desc        *criDescUp,
			     ex_cri_desc        *downRecordCriDesc,
			     ex_cri_desc        *upRecordCriDesc,
			     ex_cri_desc        *workCriDesc,
			     Lng32               moveExprTuppIndex,
			     queue_index        fromParent,
			     queue_index        toParent,
			     Lng32               downRecordLength,
			     Lng32               upRecordLength,
			     Lng32               sendBufferSize,
			     Lng32               numSendBuffers,
			     Lng32               recvBufferSize,
			     Lng32               numRecvBuffers,
			     Cardinality        estNumRowsSent,
			     Cardinality        estNumRowsRecvd,
                             NABoolean          logDiagnostics) :
  ComTdb(ex_SEND_TOP,
	 eye_SEND_TOP,
	 estNumRowsRecvd,
	 criDescDown,
	 criDescUp,
	 fromParent,
	 toParent)
{
  childFragId_            = childFragId;
  moveInputValues_        = moveInputValues;
  downRecordCriDesc_      = downRecordCriDesc;
  upRecordCriDesc_        = upRecordCriDesc;
  workCriDesc_            = workCriDesc;
  moveExprTuppIndex_      = moveExprTuppIndex;
  downRecordLength_       = downRecordLength;
  upRecordLength_         = upRecordLength;
  sendBufferSize_         = sendBufferSize;
  numSendBuffers_         = numSendBuffers;
  recvBufferSize_         = recvBufferSize;
  numRecvBuffers_         = numRecvBuffers;
  p_estNumRowsSent_       = estNumRowsSent;
  p_estNumRowsRecvd_      = estNumRowsRecvd;
  sendTopFlags_           = logDiagnostics ? 
                              LOG_DIAGNOSTICS :
                              NO_FLAGS;
  smTag_                  = 0;
}
  
Int32 ComTdbSendTop::orderedQueueProtocol() const
{
  return TRUE;
} // these lines won't be covered, obsolete but not in the list yet

Lng32 ComTdbSendTop::minSendBufferSize(Lng32 downRecLen, Lng32 numRecs)
{
  // start with the regular size it would take to pack the records
  // into an SqlBuffer
  Lng32 recSpace = SqlBufferNeededSize(numRecs, downRecLen);

  // now add the needed space for the ExpControlInfo struct that goes
  // along with each record
  Lng32 delta = SqlBufferNeededSize(2, sizeof(ControlInfo)) -
    SqlBufferNeededSize(1, sizeof(ControlInfo));

  return recSpace + numRecs * delta;
}

Lng32 ComTdbSendTop::minReceiveBufferSize(Lng32 upRecLen, Lng32 numRecs)
{
  // right now send and receive buffers work the same
  return minSendBufferSize(upRecLen,numRecs);
}

void ComTdbSendTop::display() const
{
} // these 3 lines won't be covered, used by Windows GUI only

const ComTdb* ComTdbSendTop::getChild(Int32 /*pos*/) const
{
  return NULL;
} // these 4 lines won't be covered, used by Windows GUI only

const ComTdb * ComTdbSendTop::getChildForGUI(
	Int32 /*pos*/, 
	Lng32 base,
	void * frag_dir) const 
{
  ExFragDir * fragDir = (ExFragDir *)frag_dir;
  
  return (ComTdb *)((long)base +
       fragDir->getGlobalOffset(childFragId_) +
       fragDir->getTopNodeOffset(childFragId_));
}

Int32 ComTdbSendTop::numChildren() const
{
  return 1;
}

Int32 ComTdbSendTop::numExpressions() const
{
  return 1;
}

ex_expr* ComTdbSendTop::getExpressionNode(Int32 pos)
{
  if (pos == 0)
    return moveInputValues_;
  else
    return NULL;
}

const char * ComTdbSendTop::getExpressionName(Int32 pos) const
{
  if (pos == 0)
    return "moveInputValues_";
  else
    return NULL;
}

Long ComTdbSendTop::pack(void * space)
{
  moveInputValues_.pack(space);
  downRecordCriDesc_.pack(space);
  upRecordCriDesc_.pack(space);
  workCriDesc_.pack(space);
  extractConsumerInfo_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbSendTop::unpack(void * base, void * reallocator)
{
  if (moveInputValues_.unpack(base, reallocator)) return -1;
  if (downRecordCriDesc_.unpack(base, reallocator)) return -1;
  if (upRecordCriDesc_.unpack(base, reallocator)) return -1;
  if (workCriDesc_.unpack(base, reallocator)) return -1;
  if (extractConsumerInfo_.unpack(base, reallocator)) return -1;
  return ComTdb::unpack(base, reallocator);
}

void ComTdbSendTop::displayContents(Space *space, ULng32 flag)
{
  ComTdb::displayContents(space, flag & 0xFFFFFFFE);
  
  if (flag & 0x00000008)
  {
    char buf[256];
    
    str_sprintf(buf, "\nFor ComTdbSendTop :");
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    
    str_sprintf(buf, "childFragId_ = %d, sendTopFlags_ = %x",
                (Lng32) childFragId_, (Lng32) sendTopFlags_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "downRecordLength_ = %d, upRecordLength_ = %d",
                downRecordLength_, upRecordLength_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    str_sprintf(buf, "sendBufferSize_ = %d, recvBufferSize_ = %d",
                sendBufferSize_, recvBufferSize_);
    space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

    if (sendTopFlags_ & EXTRACT_CONSUMER)
    {
      str_sprintf(buf, "This is a parallel extract consumer");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      const char *esp = getExtractEsp();
      str_sprintf(buf, "Extract ESP = %s", (esp ? esp : "(NULL)"));
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      const char *key = getExtractSecurityKey();
      str_sprintf(buf, "Security key = %s", (key ? key : "(NULL)"));
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
    }

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
