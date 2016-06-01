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

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern const char *StateString( STATE state);
extern char *ErrorMsg (int error_code);

CExtNodeUpReq::CExtNodeUpReq (reqQueueMsg_t msgType, int pid,
                              struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEL", 4);
}

CExtNodeUpReq::~CExtNodeUpReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqel", 4);
}


void CExtNodeUpReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.up.nid );
    requestString_.assign( strBuf );
}


void CExtNodeUpReq::performRequest()
{
    const char method_name[] = "CExtNodeUpReq::performRequest";
    TRACE_ENTRY;

    CNode  *node;
    int     rc = MPI_SUCCESS;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodeup_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: NodeUp, name=%s, nid=%d\n",
                     method_name, __LINE__, id_, msg_->u.request.u.up.node_name,
                     msg_->u.request.u.up.nid);
    }

    if ( msg_->u.request.u.up.nid == -1 )
    {
        node = Nodes->GetNode( msg_->u.request.u.up.node_name );
    }
    else
    {
        node = Nodes->GetNode( msg_->u.request.u.up.nid );
    }

    if ( node != NULL )
    {
        if (  Emulate_Down )
        {   // Virtual node. Set indication that ImAlive cycle will
            // propagate node state change.
            node->SetChangeState( true );
        }
        // note: see CommAcceptor thread for node up handling in
        // a real cluster.
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Invalid node, pnid=%d, name=%s\n",
                          method_name, __LINE__, msg_->u.request.u.up.nid,
                          msg_->u.request.u.up.node_name );

        rc = MPI_ERR_NAME;
    }

    if (!msg_->noreply)  // client needs a reply 
    {
        CProcess *requester;
        requester = MyNode->GetProcess( pid_ );

        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester ? requester->GetNid() : 0;
        msg_->u.reply.u.generic.pid = pid_;
        msg_->u.reply.u.generic.verifier = verifier_;
        msg_->u.reply.u.generic.process_name[0] = '\0';
        msg_->u.reply.u.generic.return_code = rc;

        if ( rc != MPI_SUCCESS )
        {
            MyNode->SetCreator( false, -1, -1 );
        }
        // Send reply to requester
        lioreply(msg_, pid_);
    }

    TRACE_EXIT;
}
