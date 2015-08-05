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
#include "gentrap.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;

CExtNodeDownReq::CExtNodeDownReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEJ", 4);

    priority_    =  High;
}

CExtNodeDownReq::~CExtNodeDownReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqej", 4);
}

void CExtNodeDownReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.down.nid );
    requestString_.assign( strBuf );
}

void CExtNodeDownReq::performRequest()
{
    const char method_name[] = "CExtNodeDownReq::performRequest";
    TRACE_ENTRY;

    CNode    *node = NULL;
    CProcess *requester = NULL;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodedown_Incr();
       
    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: NodeDown, nid=%d\n",  method_name,
                     __LINE__, id_, msg_->u.request.u.down.nid);
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        node = Nodes->GetLNode( msg_->u.request.u.down.nid )->GetNode();
        Monitor->HardNodeDown( node->GetPNid(), true );

        char la_buf[MON_STRING_BUF_SIZE*2];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s], Process %s (%d,%d:%d) requested node down %d on Node %s (%s) \n"
                , method_name
                , requester->GetName()
                , requester->GetNid()
                , requester->GetPid()
                , requester->GetVerifier()
                , msg_->u.request.u.down.nid
                , node->GetName()
                , msg_->u.request.u.down.reason);
        mon_log_write(MON_EXT_NODEDOWN_REQ, SQ_LOG_CRIT, la_buf); 
        snprintf( la_buf, sizeof(la_buf)
                , "Process %s (%d,%d:%d) requested node down %d on Node %s (%s) \n"
                , requester->GetName()
                , requester->GetNid()
                , requester->GetPid()
                , requester->GetVerifier()
                , msg_->u.request.u.down.nid
                , node->GetName()
                , msg_->u.request.u.down.reason);
        genSnmpTrap( la_buf );

        if (!msg_->noreply)  // client needs a reply 
        {
            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = requester->GetNid();
            msg_->u.reply.u.generic.pid = pid_;
            msg_->u.reply.u.generic.verifier = requester->GetVerifier() ;
            msg_->u.reply.u.generic.process_name[0] = '\0';
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
    
            // Send reply to requester
            lioreply(msg_, pid_);
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find requester, rc=%d\n", method_name, __LINE__, MPI_ERR_NAME);
    }

    TRACE_EXIT;
}
