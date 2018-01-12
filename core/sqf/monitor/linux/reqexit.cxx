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
#include "mlio.h"

extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CNode *MyNode;

CExtExitReq::CExtExitReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQED", 4);

    if (MyNode->isInQuiesceState())
    {
        priority_ = High;
    }
    else
    {
        nid_ = msg_->u.request.u.exit.nid;
        verifier_ = msg_->u.request.u.exit.verifier;
        processName_ = msg_->u.request.u.exit.process_name;
        CProcess *process = NULL;

        if ( processName_.size() )
        { // find by name
            process = MyNode->GetProcess( processName_.c_str()
                                        , verifier_ );
        }
        else
        { // find by pid
            process = MyNode->GetProcess( pid_
                                        , verifier_ );
        }

        if (process && process->GetType() == ProcessType_SMS)
        {
            priority_ = High;
        }
    }
}

CExtExitReq::~CExtExitReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqed", 4);
}

void CExtExitReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.exit.process_name
            , msg_->u.request.u.exit.nid
            , msg_->u.request.u.exit.pid
            , pid_
            , msg_->u.request.u.exit.verifier );
    requestString_.assign( strBuf );
}


void CExtExitReq::performRequest()
{
    bool status = FAILURE;
    int target_nid = -1;
    CLNode *target_lnode = NULL;

    const char method_name[] = "CExtExitReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_exit_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Exit, requester %s (%d, %d:%d)\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.exit.process_name
                    , msg_->u.request.u.exit.nid
                    , msg_->u.request.u.exit.pid
                    , msg_->u.request.u.exit.verifier );
    }

    target_nid = msg_->u.request.u.exit.nid;
    target_lnode = Nodes->GetLNode( target_nid );
    if ( target_lnode == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[CMonitor::ExitProcess], Invalid Node ID!\n");
        mon_log_write(MON_MONITOR_EXITPROCESS, SQ_LOG_ERR, buf);
    }
    else
    {
        // Change the process state to indicate process intends to exit.
        // However, exit handling will not occur until the child death signal
        // SIGCHLD is received.
        nid_ = msg_->u.request.u.exit.nid;
        verifier_ = msg_->u.request.u.exit.verifier;
        processName_ = msg_->u.request.u.exit.process_name;
        CProcess *process = NULL;

        if ( processName_.size() )
        { // find by name
            process = MyNode->GetProcess( processName_.c_str()
                                        , verifier_ );
        }
        else
        { // find by pid
            process = MyNode->GetProcess( pid_
                                        , verifier_ );
        }

        if (process)
        {
            MyNode->SetProcessState( process, State_Down, false );

            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = process->GetNid();
            msg_->u.reply.u.generic.pid = process->GetPid();
            msg_->u.reply.u.generic.verifier = process->GetVerifier();
            strcpy (msg_->u.reply.u.generic.process_name, process->GetName());
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            msg_->noreply = false;
            // Save the pid/verifier to cleanup LIO buffers after SIGCHLD
            SQ_theLocalIOToClient->addToVerifierMap( process->GetPid()
                                                   , process->GetVerifier() );
            status = SUCCESS;
        }
    }

    if (status == FAILURE)
    {
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = -1;
        msg_->u.reply.u.generic.pid = -1;
        msg_->u.reply.u.generic.verifier = -1;
        msg_->u.reply.u.generic.process_name[0] = '\0';
        msg_->u.reply.u.generic.return_code = MPI_ERR_NAME;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d - unsuccessful\n", method_name, __LINE__);
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
