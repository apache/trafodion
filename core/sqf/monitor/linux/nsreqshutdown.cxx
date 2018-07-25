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

#include "nstype.h"

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;

CExtShutdownNsReq::CExtShutdownNsReq (reqQueueMsg_t msgType,
                                      int nid, int pid, int sockFd,
                                      struct message_def *msg )
    : CExternalReq(msgType, nid, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqER", 4);

    priority_    = High;
}

CExtShutdownNsReq::~CExtShutdownNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQer", 4);
}

void CExtShutdownNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_ );
    requestString_.assign( strBuf );
}


void CExtShutdownNsReq::performRequest()
{
    const char method_name[] = "CExtShutdownNsReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_shutdown_Incr(); // TODO

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Shutdown, requester (%d, %d), "
                     "shutdown level=%d\n", method_name, __LINE__, id_,
                     msg_->u.request.u.shutdown.nid,
                     msg_->u.request.u.shutdown.pid,
                     msg_->u.request.u.shutdown.level);
    }

    if (( MyNode->GetState() != State_Down    ) &&
        ( MyNode->GetState() != State_Stopped )   )
    {
        MyNode->SetShutdownNameServer( true );
        MyNode->SetShutdownLevel( msg_->u.request.u.shutdown.level );
        MyNode->SetState( State_Shutdown );
    }

    msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            
    if (trace_settings & TRACE_REQUEST)
        trace_printf("%s@%d" " - Shutdown Level="  "%d" "\n", method_name, __LINE__, msg_->u.request.u.shutdown.level);

    msg_->u.reply.type = ReplyType_Generic;
    msg_->u.reply.u.generic.nid = -1;
    msg_->u.reply.u.generic.pid = -1;
    msg_->u.reply.u.generic.verifier = -1;
    msg_->u.reply.u.generic.process_name[0] = '\0';

    // Send reply to requester
    monreply(msg_, sockFd_);

    TRACE_EXIT;
}
