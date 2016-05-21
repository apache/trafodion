///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

extern CMonitor *Monitor;
extern void monMallocStats();

CExtMonStatsReq::CExtMonStatsReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEG", 4);
}

CExtMonStatsReq::~CExtMonStatsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqeg", 4);
}

void CExtMonStatsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d)"
             , CReqQueue::svcReqType[reqType_], getId(), pid_ );
    requestString_.assign( strBuf );
}


void CExtMonStatsReq::performRequest()
{
    const char method_name[] = "CExtMonStatsReq::performRequest";
    TRACE_ENTRY;

    // Trace info about request
    if (trace_settings & TRACE_REQUEST )
        trace_printf("%s@%d msgType=%d, pid=%d, msg=%p\n", method_name,
                     __LINE__, msgType_, pid_, msg_);

    msg_->u.reply.type = ReplyType_MonStats;
    msg_->u.reply.u.mon_info.acquiredMax
        = SQ_theLocalIOToClient->getAcquiredBufferCount();
    msg_->u.reply.u.mon_info.availMin
        = SQ_theLocalIOToClient->getAvailableBufferCount();
    msg_->u.reply.u.mon_info.bufMisses
        = SQ_theLocalIOToClient->getMissedBufferCount();

    // Write malloc statistics info to stderr
    monMallocStats();

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
