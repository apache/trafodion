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
#include "gentrap.h"

extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;
extern bool IsRealCluster;

CExtNameServerStopNsReq::CExtNameServerStopNsReq (reqQueueMsg_t msgType,
                                                  int nid, int pid, int sockFd,
                                                  struct message_def *msg )
    : CExternalReq(msgType, nid, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEH", 4);

    priority_    =  High;
}

CExtNameServerStopNsReq::~CExtNameServerStopNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQeh", 4);
}

void CExtNameServerStopNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.nameserver_stop.nid );
    requestString_.assign( strBuf );
}

void CExtNameServerStopNsReq::performRequest()
{
    const char method_name[] = "CExtNameServerStopNsReq::performRequest";
    TRACE_ENTRY;

    CNode    *node = NULL;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nameserverstop_Incr();
       
    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: NameServerStop, nid=%d\n",  method_name,
                     __LINE__, id_, msg_->u.request.u.nameserver_stop.nid);
    }

    if ( IsRealCluster )
        node = Nodes->GetNode( msg_->u.request.u.nameserver_stop.node_name );
    else
    {
        int nid = atoi( msg_->u.request.u.nameserver_stop.node_name );
        node = Nodes->GetLNode( nid )->GetNode();
    }
    Monitor->HardNodeDownNs( node->GetPNid() );

    char la_buf[MON_STRING_BUF_SIZE*2];
    snprintf( la_buf, sizeof(la_buf)
            , "[%s], Process (%d,%d) requested nameserver stop on Node %s\n"
            , method_name
            , msg_->u.request.u.nameserver_stop.nid
            , msg_->u.request.u.nameserver_stop.pid
            , msg_->u.request.u.nameserver_stop.node_name );
    mon_log_write(MON_EXT_NAMESERVERDOWN_REQ, SQ_LOG_CRIT, la_buf);
    snprintf( la_buf, sizeof(la_buf)
            , "Process (%d,%d) requested nameserver stop on Node %s\n"
            , msg_->u.request.u.nameserver_stop.nid
            , msg_->u.request.u.nameserver_stop.pid
            , msg_->u.request.u.nameserver_stop.node_name );
    genSnmpTrap( la_buf );

    if (!msg_->noreply)  // client needs a reply 
    {
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = msg_->u.request.u.nameserver_stop.nid;
        msg_->u.reply.u.generic.pid = pid_;
        msg_->u.reply.u.generic.verifier = verifier_;
        msg_->u.reply.u.generic.process_name[0] = '\0';
        msg_->u.reply.u.generic.return_code = MPI_SUCCESS;

        // Send reply to requester
        monreply(msg_, sockFd_);
    }

    TRACE_EXIT;
}
