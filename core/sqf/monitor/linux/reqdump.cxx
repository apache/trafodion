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
extern bool NameServerEnabled;

CExtDumpReq::CExtDumpReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEB", 4);
}

CExtDumpReq::~CExtDumpReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqeb", 4);
}

void CExtDumpReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "target(name=%s/nid=%d/pid=%d/verifier=%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.dump.process_name
            , msg_->u.request.u.dump.nid
            , msg_->u.request.u.dump.pid
            , pid_
            , msg_->u.request.u.dump.verifier
            , msg_->u.request.u.dump.target_process_name
            , msg_->u.request.u.dump.target_nid
            , msg_->u.request.u.dump.target_pid
            , msg_->u.request.u.dump.target_verifier );
    requestString_.assign( strBuf );
}

void CExtDumpReq::performRequest()
{
    const char method_name[] = "CExtDumpReq::performRequest";
    TRACE_ENTRY;

    CProcess *cloneProcess = NULL;
    CProcess *targetProcess = NULL;
    CProcess *requester = NULL;
    CLNode   *lnode;
    string    target_process_name;
    int       target_nid = -1;
    int       target_pid = -1;
    Verifier_t target_verifier = -1;
    int       rc = MPI_SUCCESS;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_dump_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Dump, requester %s (%d, %d:%d), "
                      "target %s (%d, %d:%d), path=%s\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.dump.process_name
                    , msg_->u.request.u.dump.nid
                    , msg_->u.request.u.dump.pid
                    , msg_->u.request.u.dump.verifier
                    , msg_->u.request.u.dump.target_process_name
                    , msg_->u.request.u.dump.target_nid
                    , msg_->u.request.u.dump.target_pid
                    , msg_->u.request.u.dump.target_verifier
                    , msg_->u.request.u.dump.path);
    }


    target_nid = msg_->u.request.u.dump.target_nid;
    target_pid = msg_->u.request.u.dump.target_pid;
    target_process_name = (const char *) msg_->u.request.u.dump.target_process_name;
    nid_ = msg_->u.request.u.dump.nid;
    verifier_ = msg_->u.request.u.dump.verifier;
    processName_ = msg_->u.request.u.dump.process_name;
    target_verifier  = msg_->u.request.u.dump.target_verifier;

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
        { // find by name
            if (msg_->u.request.u.dump.target_process_name[0] == '$' )
            {
                targetProcess = Nodes->GetProcess( target_process_name.c_str()
                                                 , target_verifier );
            }
        }
        else
        { // find by nid, pid
            targetProcess = Nodes->GetProcess( target_nid
                                             , target_pid
                                             , target_verifier );
        }

        if ( !targetProcess )
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
                    if (msg_->u.request.u.dump.target_process_name[0] == '$' )
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

        if ( targetProcess )
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Dump Process name=%s, nid=%d, pid=%d\n",
                             method_name, __LINE__, targetProcess->GetName(),
                             targetProcess->GetNid(), targetProcess->GetPid());
            targetProcess->parentContext(msg_);
            lnode = Nodes->GetLNode(target_nid);
            if (lnode->Dump_Process(requester,
                                    targetProcess,
                                    msg_->u.request.u.dump.path) != SUCCESS)
            {
                rc = MPI_ERR_SPAWN;
            }
        }
        else
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Can't find target, rc=%d\n", method_name, __LINE__, rc);
            rc = MPI_ERR_NAME;
        }

        if (rc != MPI_SUCCESS)
        {   // Unable to dump process so send an error reply.
            //
            // Otherwise the dump has been initiated and reply will be sent
            // upon completion.

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

            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Unsuccessful rc=%d\n", method_name, __LINE__, rc);
            // build reply
            msg_->u.reply.type = ReplyType_Dump;
            msg_->u.reply.u.dump.nid = target_nid;
            msg_->u.reply.u.dump.pid = target_pid;
            msg_->u.reply.u.dump.verifier = target_verifier;
            strcpy (msg_->u.reply.u.dump.process_name, target_process_name.c_str());
            msg_->u.reply.u.dump.core_file[0] = 0;
            msg_->u.reply.u.dump.return_code = rc;

            // Send reply to requester
            lioreply(msg_, pid_);
        }
    }
    else
    {   // Reply to requester so it can release the buffer.  
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find requester, rc=%d\n", method_name, __LINE__, MPI_ERR_EXITED);
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
