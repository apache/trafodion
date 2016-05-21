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
#include "mlio.h"

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern CNode *MyNode;
extern int MyPNID;

CExtKillReq::CExtKillReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEF", 4);
}

CExtKillReq::~CExtKillReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqef", 4);
}

void CExtKillReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.kill.process_name
            , msg_->u.request.u.kill.nid
            , msg_->u.request.u.kill.pid
            , pid_
            , msg_->u.request.u.kill.verifier
            , msg_->u.request.u.kill.target_process_name
            , msg_->u.request.u.kill.target_nid
            , msg_->u.request.u.kill.target_pid
            , msg_->u.request.u.kill.target_verifier );
    requestString_.assign( strBuf );
}

void CExtKillReq::Kill( CProcess *process )
{
    CNode  *node = NULL;
    CLNode *lnode = NULL;

    const char method_name[] = "CMonitor::Kill";
    TRACE_ENTRY;
    
    process->SetAbended ( true );
    process->SetState( State_Down );
    lnode = Nodes->GetLNode( process->GetNid() );
    node = lnode->GetNode();
    if ( (node->GetState() == State_Up || 
          node->GetState() == State_Shutdown) && node->GetPNid() != MyPNID )
    {
        // Replicate the kill to other nodes
        CReplKill *repl = new CReplKill( process->GetNid()
                                       , process->GetPid()
                                       , process->GetVerifier()
                                       , process->GetAbort());
        Replicator.addItem(repl);
    }
    else
    {
        // if the node is not up, then the process was or will be killed
        if ( !process->IsClone() &&
            (node->GetState() == State_Up || 
             node->GetState() == State_Shutdown) )
        {
            kill (process->GetPid(), Monitor->GetProcTermSig());
            // Save the pid/verifier to cleanup LIO buffers after SIGCHLD
            SQ_theLocalIOToClient->addToVerifierMap( process->GetPid()
                                                   , process->GetVerifier() );

            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Completed kill for process %s (%d, %d:%d)\n",
                             method_name, __LINE__, process->GetName(),
                             process->GetNid(), process->GetPid(), process->GetVerifier());

            CProcess *parent = Nodes->GetProcess( process->GetParentNid(), 
                                                  process->GetParentPid() );
            process->Switch(parent); // switch process pair roles if needed
        }
    }

    TRACE_EXIT;
}

void CExtKillReq::performRequest()
{
    bool status = FAILURE;
    CProcess *process = NULL;
    CProcess *backup = NULL;

    const char method_name[] = "CExtKillReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_kill_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Kill, requester %s (%d, %d:%d), "
                      "target %s (%d, %d:%d), persistent "
                      "abort=%d\n", method_name, __LINE__, id_,
                     msg_->u.request.u.kill.process_name,
                     msg_->u.request.u.kill.nid,
                     msg_->u.request.u.kill.pid,
                     msg_->u.request.u.kill.verifier,
                     msg_->u.request.u.kill.target_process_name,
                     msg_->u.request.u.kill.target_nid,
                     msg_->u.request.u.kill.target_pid,
                     msg_->u.request.u.kill.target_verifier,
                     msg_->u.request.u.kill.persistent_abort);
    }

    nid_ = msg_->u.request.u.kill.nid;
    verifier_ = msg_->u.request.u.kill.verifier;
    processName_ = msg_->u.request.u.kill.process_name;

    int       target_nid = -1;
    int       target_pid = -1;
    string    target_process_name;
    Verifier_t target_verifier = -1;
    CProcess *requester = NULL;
    
    target_nid = msg_->u.request.u.kill.target_nid;
    target_pid = msg_->u.request.u.kill.target_pid;
    target_process_name = (const char *) msg_->u.request.u.kill.target_process_name;
    target_verifier  = msg_->u.request.u.kill.target_verifier;

    if ( processName_.size() )
    { // find by name
        requester = MyNode->GetProcess( processName_.c_str()
                                      , verifier_ );
    }
    else
    { // find by pid
        requester = MyNode->GetProcess( pid_
                                      , verifier_ );
    }

    if ( requester )
    {
        if ( target_process_name.size() )
        { // find by name (check node state, don't check process state, not backup)
            process = Nodes->GetProcess( target_process_name.c_str()
                                       , target_verifier
                                       , true, false, false );
            if ( process &&
                (msg_->u.request.u.kill.target_nid == -1 ||
                 msg_->u.request.u.kill.target_pid == -1))
            {
                backup = process->GetBackup ();
            }
        }
        else
        { // find by nid (check node state, don't check process state, backup is Ok)
            process = Nodes->GetProcess( target_nid
                                       , target_pid
                                       , target_verifier
                                       , true, false, true );
            backup = NULL;
        }


        if (process)
        {
            process->SetAbort( msg_->u.request.u.kill.persistent_abort );
            if (backup)
            {
                // We are killing both the primary and backup processes
                Kill( backup );
            }
            Kill( process );

            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = process->GetNid();
            msg_->u.reply.u.generic.pid = process->GetPid();
            msg_->u.reply.u.generic.verifier = process->GetVerifier();
            strcpy (msg_->u.reply.u.generic.process_name, process->GetName());
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            status = SUCCESS;
        }
        else
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf( "%s@%d - Kill %s (%d, %d:%d) -- can't find target process\n"
                           , method_name, __LINE__
                           , msg_->u.request.u.kill.target_process_name
                           , msg_->u.request.u.kill.target_nid
                           , msg_->u.request.u.kill.target_pid
                           , msg_->u.request.u.kill.target_verifier);
            }
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find requester, rc=%d\n", method_name, __LINE__, MPI_ERR_NAME);
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
