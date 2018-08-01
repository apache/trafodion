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

extern const char *ProcessTypeString( PROCESSTYPE type );

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
void CExtProcInfoNsReq::copyInfo(CProcess *process, ProcessInfoNs_reply_def &process_info_ns)
{
    const char method_name[] = "CExtProcInfoNsReq::copyInfo";
    TRACE_ENTRY;

    CProcess *parent;

    process_info_ns.nid = process->GetNid();
    process_info_ns.pid = process->GetPid();
    process_info_ns.verifier = process->GetVerifier();
    strncpy( process_info_ns.process_name, process->GetName(), MAX_PROCESS_NAME );
    process_info_ns.type = process->GetType();
    parent = (process->GetParentNid() == -1 ? 
              NULL : 
              Nodes->GetLNode(process->GetParentNid())
                 ->GetProcessL(process->GetParentPid()));
    if (parent)
    {
        process_info_ns.parent_nid = parent->GetNid();
        process_info_ns.parent_pid = parent->GetPid();
        process_info_ns.parent_verifier = parent->GetVerifier();
    }
    else
    {
        process_info_ns.parent_nid = -1;
        process_info_ns.parent_pid = -1;
        process_info_ns.parent_verifier = -1;
    }

    process_info_ns.priority = process->GetPriority();
    process_info_ns.backup = process->IsBackup();
    process_info_ns.state = process->GetState();
    process_info_ns.unhooked = process->IsUnhooked();
    process_info_ns.event_messages = process->IsEventMessages();
    process_info_ns.system_messages = process->IsSystemMessages();
    strncpy( process_info_ns.path, process->path(), MAX_SEARCH_PATH );
    strncpy( process_info_ns.ldpath, process->ldpath(), MAX_SEARCH_PATH );
    strncpy( process_info_ns.program, process->program(), MAX_PROCESS_PATH );
//    process_info_ns.pathStrId = process->pathStrId();
//    process_info_ns.ldpathStrId = process->ldPathStrId();
//    process_info_ns.programStrId = process->programStrId();
    strncpy( process_info_ns.port_name, process->GetPort(), MPI_MAX_PORT_NAME );
    process_info_ns.argc = process->argc();
    memcpy( process_info_ns.argv, process->userArgv(), process->userArgvLen() );
    strncpy( process_info_ns.infile, process->infile(), MAX_PROCESS_PATH );
    strncpy( process_info_ns.outfile, process->outfile(), MAX_PROCESS_PATH );
    process_info_ns.creation_time = process->GetCreationTime();
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        char desc[2048];
        char* descp = desc;
        sprintf( desc, 
                 "process-info-ns reply:\n"
                 "        process_info_ns.nid=%d\n"
                 "        process_info_ns.pid=%d\n"
                 "        process_info_ns.verifier=%d\n"
                 "        process_info_ns.process_name=%s\n"
                 "        process_info_ns.type=%d\n"
                 "        process_info_ns.parent_nid=%d\n"
                 "        process_info_ns.parent_pid=%d\n"
                 "        process_info_ns.parent_verifier=%d\n"
                 "        process_info_ns.priority=%d\n"
                 "        process_info_ns.backup=%d\n"
                 "        process_info_ns.state=%d\n"
                 "        process_info_ns.unhooked=%d\n"
                 "        process_info_ns.event_messages=%d\n"
                 "        process_info_ns.system_messages=%d\n"
                 "        process_info_ns.path=%s\n"
                 "        process_info_ns.ldpath=%s\n"
                 "        process_info_ns.program=%s\n"
//                 "        process_info_ns.pathStrId=%d:%d\n"
//                 "        process_info_ns.ldpathStrId=%d:%d\n"
//                 "        process_info_ns.programStrId=%d:%d\n"
                 "        process_info_ns.port_name=%s\n"
                 "        process_info_ns.argc=%d\n"
                 "        process_info_ns.infile=%s\n"
                 "        process_info_ns.outfile=%s\n"
                 "        process_info_ns.return_code=%d"
                 , process_info_ns.nid
                 , process_info_ns.pid
                 , process_info_ns.verifier
                 , process_info_ns.process_name
                 , process_info_ns.type
                 , process_info_ns.parent_nid
                 , process_info_ns.parent_pid
                 , process_info_ns.parent_verifier
                 , process_info_ns.priority
                 , process_info_ns.backup
                 , process_info_ns.state
                 , process_info_ns.unhooked
                 , process_info_ns.event_messages
                 , process_info_ns.system_messages
                 , process_info_ns.path
                 , process_info_ns.ldpath
                 , process_info_ns.program
//                 , process_info_ns.pathStrId.nid
//                 , process_info_ns.pathStrId.id
//                 , process_info_ns.ldpathStrId.nid
//                 , process_info_ns.ldpathStrId.id
//                 , process_info_ns.programStrId.nid
//                 , process_info_ns.programStrId.id
                 , process_info_ns.port_name
                 , process_info_ns.argc
                 , process_info_ns.infile
                 , process_info_ns.outfile
                 , process_info_ns.return_code );
        trace_printf( "%s@%d - %s\n"
                    , method_name, __LINE__, descp );
    }
    TRACE_EXIT;
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

    PROCESSTYPE target_type  = msg_->u.request.u.process_info.type;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: ProcessInfoNs, for %s (%d, %d:%d), "
                      "process type=%s\n"
                    , method_name, __LINE__, id_
                    , target_process_name.c_str(), target_nid, target_pid, target_verifier
                    , ProcessTypeString(target_type));
    }

    if (target_process_name.size())
    { // find by name (don't check node state, don't check process state, not backup)
        if (msg_->u.request.u.process_info.target_process_name[0] == '$' )
        {
            process = Nodes->GetProcess( target_process_name.c_str()
                                       , target_verifier
                                       , false, false, false );
        }
    }
    else
    {
        if (target_pid != -1)
        { // find by nid,pid (don't check node state, don't check process state, backup is Ok)
            process = Nodes->GetProcess( target_nid
                                       , target_pid
                                       , target_verifier
                                       , false, false, true );
        }
        else
        {
            CLNode *lnode = Nodes->GetLNode( target_nid );
            if (lnode)
            {
                process = lnode->GetProcessLByType( target_type );
            }
        }
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
           trace_printf( "%s@%d - ProcessInfoNs %s (%d, %d:%d) -- can't find target process\n"
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
