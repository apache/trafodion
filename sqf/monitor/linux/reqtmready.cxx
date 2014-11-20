///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;

CExtTmReadyReq::CExtTmReadyReq( reqQueueMsg_t msgType, int pid,
                                struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
    , nid_(msg_->u.request.u.tm_ready.nid)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEY", 4);
}

CExtTmReadyReq::~CExtTmReadyReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqey", 4);
}

void CExtTmReadyReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.tm_ready.nid, msg_->u.request.u.tm_ready.pid);
    requestString_.assign( strBuf );
}


void CExtTmReadyReq::performRequest()
{
    const char method_name[] = "CExtTmReadyReq::performRequest";
    TRACE_ENTRY;

    //TODO: Record statistics (sonar counters)
    //MonStats->req_type_tmready_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: TmReady, requester %d\n",
                     method_name, __LINE__, id_, pid_);
    }

    Monitor->NodeTmReady( nid_ );

    msg_->u.reply.type = ReplyType_Generic;
    msg_->u.reply.u.generic.nid = nid_;
    msg_->u.reply.u.generic.pid = pid_;
    msg_->u.reply.u.generic.verifier = verifier_;
    msg_->u.reply.u.generic.process_name[0] = 0;
    msg_->u.reply.u.generic.return_code = MPI_SUCCESS;

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
