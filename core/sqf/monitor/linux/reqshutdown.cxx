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
#include "replicate.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

CExtShutdownReq::CExtShutdownReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQER", 4);

    priority_    = High;
}

CExtShutdownReq::~CExtShutdownReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqer", 4);
}

void CExtShutdownReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_ );
    requestString_.assign( strBuf );
}


void CExtShutdownReq::performRequest()
{
    const char method_name[] = "CExtShutdownReq::performRequest";
    TRACE_ENTRY;

    CProcess *requester;
    bool shutdownRejected = false;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_shutdown_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Shutdown, requester (%d, %d), "
                     "shutdown level=%d\n", method_name, __LINE__, id_,
                     msg_->u.request.u.shutdown.nid,
                     msg_->u.request.u.shutdown.pid,
                     msg_->u.request.u.shutdown.level);
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        // for non shutdown abrupt cases, check if any node is in quiescing state, then reject shutdown.
        shutdownRejected = false;
        if( msg_->u.request.u.shutdown.level != ShutdownLevel_Abrupt )
        {
            CNode *node = Nodes->GetFirstNode();
            for ( ; node ; node = node->GetNext() )
            {
                if ( node->GetState() == State_Up && node->isInQuiesceState() )
                {
                    msg_->u.reply.u.generic.return_code = MPI_ERR_OP;
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf("%s@%d" " - Shutdown rejected. Node %d is quiescing." "\n", method_name, __LINE__, node->GetPNid());

                    char buf[MON_STRING_BUF_SIZE];
                    sprintf(buf, "[CMonitor::ProcessRequest], Shutdown rejected. Node %d is quiescing.\n", node->GetPNid());
                    mon_log_write(MON_MONITOR_PROCESSREQUEST_6, SQ_LOG_ERR, buf);
                    shutdownRejected = true;
                    break;
                }
            }
        }

        if (!shutdownRejected)
        {
            if ( msg_->u.request.u.shutdown.level == ShutdownLevel_Abrupt )
            {
                // Replicate a shutdown request so that all nodes begin to shutdown locally.
                CReplShutdown *repl = new CReplShutdown(msg_->u.request.u.shutdown.level);
                Replicator.addItem(repl);
            }
            else
            {
                // normal shutdown
                // propagate the shutdown level before killing any processes.
                MyNode->SetShutdownLevel( msg_->u.request.u.shutdown.level );

                if (MyNode->GetState() == State_Up)
                {
                    MyNode->SetState( State_Shutdown );
                }
            }

            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
                
            if (trace_settings & TRACE_REQUEST)
                trace_printf("%s@%d" " - Shutdown Level="  "%d" "\n", method_name, __LINE__, msg_->u.request.u.shutdown.level);
        }
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester->GetNid();
        msg_->u.reply.u.generic.pid = requester->GetPid();
        msg_->u.reply.u.generic.verifier = requester->GetVerifier();
        msg_->u.reply.u.generic.process_name[0] = '\0';

        // Send reply to requester
        lioreply(msg_, pid_);
    }
    else
    {   // Reply to requester so it can release the buffer.  
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
