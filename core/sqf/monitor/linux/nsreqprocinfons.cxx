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
#include "replicate.h"
#include "mlio.h"

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern int MyPNID;

CExtProcInfoNsReq::CExtProcInfoNsReq( reqQueueMsg_t msgType,
                                      int nid, int pid, int sockFd,
                                      struct message_def *msg )
                 : CExternalReq(msgType, nid, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEB", 4);
}

CExtProcInfoNsReq::~CExtProcInfoNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQeb", 4);
}

// Copy information for a specific process into the reply message buffer.
void CExtProcInfoNsReq::copyInfo(CProcess *process, ProcessInfoNs_reply_def &processInfo)
{
    CProcess *parent;

    processInfo.nid = process->GetNid();
    processInfo.pid = process->GetPid();
    processInfo.verifier = process->GetVerifier();
    strncpy( processInfo.process_name, process->GetName(), MAX_PROCESS_NAME );
    processInfo.type = process->GetType();

    parent = process->GetParent();
    if (parent)
    {
        processInfo.parent_nid = parent->GetNid();
        processInfo.parent_pid = parent->GetPid();
        processInfo.parent_verifier = parent->GetVerifier();
//        strncpy(processInfo.parent_name, parent->GetName(), MAX_PROCESS_NAME );
    }
    else
    {
        processInfo.parent_nid = -1;
        processInfo.parent_pid = -1;
        processInfo.parent_verifier = -1;
//        processInfo.parent_name[0] = '\0';
    }

    processInfo.priority = process->GetPriority();
    processInfo.backup = process->IsBackup();
    processInfo.state = process->GetState();
    processInfo.unhooked = process->IsUnhooked();
    processInfo.event_messages = process->IsEventMessages();
    processInfo.system_messages = process->IsSystemMessages();
    strncpy( processInfo.program, process->program(), MAX_PROCESS_PATH );
    processInfo.pathStrId = process->pathStrId();
    processInfo.ldpathStrId = process->ldPathStrId();
    processInfo.programStrId = process->programStrId();
    strncpy( processInfo.port_name, process->GetPort(), MPI_MAX_PORT_NAME );
    processInfo.argc = process->argc();
    memcpy( processInfo.argv, process->userArgv(), process->userArgvLen() );
    strncpy( processInfo.infile, process->infile(), MAX_PROCESS_PATH );
    strncpy( processInfo.outfile, process->outfile(), MAX_PROCESS_PATH );
    processInfo.creation_time = process->GetCreationTime();
}

void CExtProcInfoNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d) pattern(name=%s)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.process_info.process_name
            , msg_->u.request.u.process_info.nid
            , msg_->u.request.u.process_info.pid
            , pid_
            , msg_->u.request.u.process_info.verifier
            , msg_->u.request.u.process_info.target_process_name
            , msg_->u.request.u.process_info.target_nid
            , msg_->u.request.u.process_info.target_pid
            , msg_->u.request.u.process_info.target_verifier
            , msg_->u.request.u.process_info.target_process_pattern );
    requestString_.assign( strBuf );
}

void CExtProcInfoNsReq::performRequest()
{
    CProcess *process = NULL;

    const char method_name[] = "CExtProcInfoNsReq::performRequest";
    TRACE_ENTRY;

#if 0 // TODO
    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_kill_Incr();
#endif

    nid_ = msg_->u.request.u.process_info.nid;
    verifier_ = msg_->u.request.u.process_info.verifier;
    processName_ = msg_->u.request.u.process_info.process_name;

    int       target_nid = -1;
    int       target_pid = -1;
    string    target_process_name;
    Verifier_t target_verifier = -1;
    
    target_nid = msg_->u.request.u.process_info.target_nid;
    target_pid = msg_->u.request.u.process_info.target_pid;
    target_process_name = (const char *) msg_->u.request.u.process_info.target_process_name;
    target_verifier  = msg_->u.request.u.process_info.target_verifier;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: ProcessInfoNs, for (%d, %d:%d), "
                      "process type=%d\n"
                    , method_name, __LINE__, id_
                    , target_nid, target_pid, target_verifier
                    , msg_->u.request.u.process_info.type);
    }

    if ( target_process_name.size() )
    { // find by name (don't check node state, don't check process state, not backup)
        process = Nodes->GetProcess( target_process_name.c_str()
                                   , target_verifier
                                   , false, false, false );
    }
    else
    { // find by nid (don't check node state, don't check process state, backup is Ok)
        process = Nodes->GetProcess( target_nid
                                   , target_pid
                                   , target_verifier
                                   , false, false, true );
    }


    if (process)
    {
        msg_->u.reply.type = ReplyType_ProcessInfoNs;
        msg_->u.reply.u.process_info_ns.return_code = MPI_SUCCESS;
        copyInfo( process, msg_->u.reply.u.process_info_ns );
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
           trace_printf( "%s@%d - Kill %s (%d, %d:%d) -- can't find target process\n"
                       , method_name, __LINE__
                       , msg_->u.request.u.process_info.target_process_name
                       , msg_->u.request.u.process_info.target_nid
                       , msg_->u.request.u.process_info.target_pid
                       , msg_->u.request.u.process_info.target_verifier);
        }
        msg_->u.reply.type = ReplyType_ProcessInfoNs;
        msg_->u.reply.u.process_info_ns.nid = target_nid;
        msg_->u.reply.u.process_info_ns.pid = target_pid;
        msg_->u.reply.u.process_info_ns.verifier = target_verifier;
        strncpy(msg_->u.reply.u.process_info_ns.process_name, target_process_name.c_str(), MAX_PROCESS_NAME);
        msg_->u.reply.u.process_info_ns.return_code = MPI_ERR_NAME;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d - unsuccessful\n", method_name, __LINE__);
    }

    // Send reply to requester
    monreply(msg_, sockFd_);

    TRACE_EXIT;
}
