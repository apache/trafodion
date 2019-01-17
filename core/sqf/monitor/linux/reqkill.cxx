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
#include "ptpclient.h"

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern CNode *MyNode;
extern int MyPNID;
extern CPtpClient *PtpClient;
extern bool NameServerEnabled;

CExtKillReq::CExtKillReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg )
    : CExternalReq(msgType, -1, pid, -1, msg)
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

    const char method_name[] = "CExtKillReq::Kill";
    TRACE_ENTRY;
    
    process->SetAbended ( true );
    process->SetState( State_Down );
    lnode = Nodes->GetLNode( process->GetNid() );
    node = lnode->GetNode();
    if ( (node->GetState() == State_Up || 
          node->GetState() == State_Shutdown) && node->GetPNid() != MyPNID )
    {
        if (NameServerEnabled)
        {
            // Forward the process create to the target node
            int rc = PtpClient->ProcessKill( process
                                           , process->GetAbort()
                                           , lnode->GetNid()
                                           , node->GetName());
            if (rc)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Can't send process kill "
                          "request for child process %s (%d, %d) "
                          "to child node %s, nid=%d\n"
                        , method_name
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , node->GetName()
                        , lnode->GetNid() );
                mon_log_write(MON_REQ_KILL_1, SQ_LOG_ERR, la_buf);
            }
        }
        else
        {
            // Replicate the kill to other nodes
            CReplKill *repl = new CReplKill( process->GetNid()
                                           , process->GetPid()
                                           , process->GetVerifier()
                                           , process->GetAbort());
            Replicator.addItem(repl);
        }
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
    CProcess *cloneProcess = NULL;
    CProcess *targetProcess = NULL;
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
            if (msg_->u.request.u.kill.target_process_name[0] == '$' )
            {
                targetProcess = Nodes->GetProcess( target_process_name.c_str()
                                                  , target_verifier
                                                  , true, false, false );
                if ( targetProcess &&
                    (msg_->u.request.u.kill.target_nid == -1 ||
                     msg_->u.request.u.kill.target_pid == -1))
                {
                    backup = targetProcess->GetBackup ();
                }
            }
        }
        else
        { // find by nid (check node state, don't check process state, backup is Ok)
            targetProcess = Nodes->GetProcess( target_nid
                                             , target_pid
                                             , target_verifier
                                             , true, false, true );
            backup = NULL;
        }

        if ( targetProcess )
        {
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                            , method_name, __LINE__
                            , targetProcess->GetName()
                            , targetProcess->GetNid()
                            , targetProcess->GetPid()
                            , targetProcess->GetVerifier()
                            , targetProcess->IsClone() );
            }
        }
        else
        {
            if (NameServerEnabled)
            {
                if ( target_process_name.size() )
                { // Name Server find by name:verifier
                    if (trace_settings & TRACE_REQUEST)
                    {
                        trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%s:%d)" "\n"
                                    , method_name, __LINE__
                                    , target_process_name.c_str()
                                    , target_verifier );
                    }
                    if (msg_->u.request.u.kill.target_process_name[0] == '$' )
                    {
                        cloneProcess = Nodes->CloneProcessNs( target_process_name.c_str()
                                                            , target_verifier );
                        targetProcess = cloneProcess;
                    }
                }     
                else
                { // Name Server find by nid,pid:verifier
                    if (trace_settings & TRACE_REQUEST)
                    {
                        trace_printf( "%s@%d" " - Getting targetProcess from Name Server (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , target_nid
                                    , target_pid
                                    , target_verifier );
                    }
                    cloneProcess = Nodes->CloneProcessNs( target_nid
                                                        , target_pid
                                                        , target_verifier );
                    targetProcess = cloneProcess;
                }
                if (targetProcess)
                {
                    if (trace_settings & TRACE_REQUEST)
                        trace_printf( "%s@%d - Found targetProcess %s (%d,%d:%d), clone=%d\n"
                                    , method_name, __LINE__
                                    , targetProcess->GetName()
                                    , targetProcess->GetNid()
                                    , targetProcess->GetPid()
                                    , targetProcess->GetVerifier()
                                    , targetProcess->IsClone() );
                }
            }
        }

        if (targetProcess)
        {
            targetProcess->SetAbort( msg_->u.request.u.kill.persistent_abort );
            if (backup)
            {
                // We are killing both the primary and backup processes
                Kill( backup );
            }
            Kill( targetProcess );

            if (NameServerEnabled && cloneProcess)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                {
                    trace_printf( "%s@%d - Deleting clone process %s, (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , cloneProcess->GetName()
                                , cloneProcess->GetNid()
                                , cloneProcess->GetPid()
                                , cloneProcess->GetVerifier() );
                }
                Nodes->DeleteCloneProcess( cloneProcess );
            }

            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = targetProcess->GetNid();
            msg_->u.reply.u.generic.pid = targetProcess->GetPid();
            msg_->u.reply.u.generic.verifier = targetProcess->GetVerifier();
            strcpy (msg_->u.reply.u.generic.process_name, targetProcess->GetName());
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
