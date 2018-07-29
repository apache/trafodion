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
extern CMonitor *Monitor;

CExtNodeDownNsReq::CExtNodeDownNsReq( reqQueueMsg_t msgType
                                    , int pid
                                    , int sockFd
                                    , struct message_def *msg )
    : CExternalReq(msgType, -1, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEJ", 4);

    priority_    =  High;
}

CExtNodeDownNsReq::~CExtNodeDownNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQej", 4);
}

void CExtNodeDownNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.down.nid );
    requestString_.assign( strBuf );
}

void CExtNodeDownNsReq::performRequest()
{
    const char method_name[] = "CExtNodeDownNsReq::performRequest";
    TRACE_ENTRY;

    CNode    *node = NULL;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodedown_Incr();
       
    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: NodeDown, nid=%d\n",  method_name,
                     __LINE__, id_, msg_->u.request.u.down.nid);
    }

    node = Nodes->GetLNode( msg_->u.request.u.down.nid )->GetNode();
    Monitor->HardNodeDownNs( node->GetPNid() );

    msg_->u.reply.type = ReplyType_Generic;
    msg_->u.reply.u.generic.nid = -1;
    msg_->u.reply.u.generic.pid = pid_;
    msg_->u.reply.u.generic.verifier = -1;
    msg_->u.reply.u.generic.process_name[0] = '\0';
    msg_->u.reply.u.generic.return_code = MPI_SUCCESS;

    // Send reply to monitor
    monreply(msg_, sockFd_);

    TRACE_EXIT;
}
