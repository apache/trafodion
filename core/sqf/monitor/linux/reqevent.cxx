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
extern CNodeContainer *Nodes;

CExtEventReq::CExtEventReq (reqQueueMsg_t msgType, int pid,
                            struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEC", 4);
}

CExtEventReq::~CExtEventReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqec", 4);
}

void CExtEventReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d)"
              "type=%d, event_id=%d, event bytes=%d\n"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.event.process_name
            , msg_->u.request.u.event.nid
            , msg_->u.request.u.event.pid
            , pid_
            , msg_->u.request.u.event.verifier
            , msg_->u.request.u.event.target_process_name
            , msg_->u.request.u.event.target_nid
            , msg_->u.request.u.event.target_pid
            , msg_->u.request.u.event.target_verifier
            , msg_->u.request.u.event.type
            , msg_->u.request.u.event.event_id
            , msg_->u.request.u.event.length );
    requestString_.assign( strBuf );
}

void CExtEventReq::performRequest()
{
    const char method_name[] = "CExtEventReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_ERR_NAME;
    Verifier_t target_verifier = -1;
    string target_process_name;
    int nid;
    int target_nid;
    int pid;
    int num_procs;
    PROCESSTYPE type;
    CProcess *process = NULL;
    CProcess *requester;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_event_Incr();

    nid_ = msg_->u.request.u.event.nid;
    verifier_ = msg_->u.request.u.event.verifier;
    processName_ = msg_->u.request.u.event.process_name;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Event, requester %s (%d, %d:%d), "
                      "target %s (%d, %d:%d), "
                      "type=%d, event_id=%d, event bytes=%d\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.event.process_name
                    , msg_->u.request.u.event.nid
                    , msg_->u.request.u.event.pid
                    , msg_->u.request.u.event.verifier
                    , msg_->u.request.u.event.target_process_name
                    , msg_->u.request.u.event.target_nid
                    , msg_->u.request.u.event.target_pid
                    , msg_->u.request.u.event.target_verifier
                    , msg_->u.request.u.event.type
                    , msg_->u.request.u.event.event_id
                    , msg_->u.request.u.event.length);
    }

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
        // setup for type of status request
        type = msg_->u.request.u.event.type;

        // Only monitor can send events to SQWatchdog process
        if ( type != ProcessType_Watchdog )
        { // Only monitor can send events to SQWatchdog process
            if (msg_->u.request.u.event.target_nid == -1)
            {
                if (trace_settings & TRACE_REQUEST)
                    trace_printf("%s@%d" " - Event for Process set=<-1,-1>" "\n", method_name, __LINE__);
                // get info for all processes in all nodes
                nid = 0;
                target_nid = -1;
                pid = -1;
            }
            else
            {
                target_nid = nid = msg_->u.request.u.event.target_nid;
                pid = msg_->u.request.u.event.target_pid;
                target_process_name = (const char *) msg_->u.request.u.event.target_process_name;
                target_verifier  = msg_->u.request.u.event.target_verifier;

                if (trace_settings & TRACE_REQUEST)
                    trace_printf( "%s@%d - Event for %s (%d, %d:%d)\n"
                                , method_name, __LINE__
                                , msg_->u.request.u.event.target_process_name
                                , msg_->u.request.u.event.target_nid
                                , msg_->u.request.u.event.target_pid
                                , msg_->u.request.u.event.target_verifier);

                if ( target_process_name.size() )
                { // find by name
                    process = Nodes->GetProcess( target_process_name.c_str()
                                               , target_verifier );
                    if ( process && trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Found target by name %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier());
                    }
                    pid = process ? process->GetPid() : -1;
                }
                else if (pid == -1)
                { // get info for all processes in node
                    process = Nodes->GetLNode(nid)->GetFirstProcess();
                    if ( process && trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Found target first process %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier());
                    }
                }
                else
                { // get info for single process in node by pid
                    process = Nodes->GetProcess( target_nid
                                               , pid
                                               , target_verifier );
                    if ( process && trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Found target by nid,pid %s (%d, %d:%d)\n"
                                    , method_name, __LINE__
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier());
                    }
                }
            }
            
            if ( !process && target_nid != -1 && 
                 (trace_settings & (TRACE_REQUEST | TRACE_PROCESS)))
                trace_printf("%s@%d" " - Target process not found! %s (%d, %d:%d)\n"
                            , method_name, __LINE__
                            , msg_->u.request.u.event.target_process_name
                            , msg_->u.request.u.event.target_nid
                            , msg_->u.request.u.event.target_pid
                            , msg_->u.request.u.event.target_verifier);

            // process events
            num_procs = 0;
            CLNode *lnode = Nodes->GetFirstLNode();
            for ( ; lnode; lnode = lnode->GetNext() )
            {
                if (target_nid == -1)
                {
                    process = lnode->GetFirstProcess();
                }
                while (process && num_procs < MAX_PROC_LIST)
                {
                    if (pid == -1 || process->GetPid() == pid)
                    {
                        if ( process->GetType() != ProcessType_Watchdog )
                        {
                            if (type == ProcessType_Undefined || type == process->GetType())
                            {
                                process->GenerateEvent( msg_->u.request.u.event.event_id,
                                                        msg_->u.request.u.event.length,
                                                        msg_->u.request.u.event.data );
                                if (trace_settings & TRACE_REQUEST)
                                    trace_printf( "%s@%d - Event %d sent to %s (%d, %d:%d)\n"
                                                , method_name, __LINE__
                                                , msg_->u.request.u.event.event_id
                                                , process->GetName()
                                                , process->GetNid()
                                                , process->GetPid()
                                                , process->GetVerifier());
                                rc = MPI_SUCCESS;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                    process = process->GetNextL();
                }
            }

            // build reply
            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = requester->GetNid();
            msg_->u.reply.u.generic.pid = requester->GetPid();
            msg_->u.reply.u.generic.verifier = requester->GetVerifier();
            strcpy (msg_->u.reply.u.generic.process_name, requester->GetName());
            msg_->u.reply.u.generic.return_code = rc;

            // Send reply to requester
            lioreply(msg_, pid_);
        }
        else
        {
            // Invalid when SQWatchdog process
            errorReply( MPI_ERR_OP );
        }
    }
    else
    {   // Reply to requester so it can release the buffer.  
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
