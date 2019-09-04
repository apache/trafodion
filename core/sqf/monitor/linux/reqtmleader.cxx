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
#include "nameserver.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;
extern bool NameServerEnabled;

CExtTmLeaderReq::CExtTmLeaderReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQET", 4);
}

CExtTmLeaderReq::~CExtTmLeaderReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqet", 4);
}

void CExtTmLeaderReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.leader.nid, msg_->u.request.u.leader.pid);
    requestString_.assign( strBuf );
}


void CExtTmLeaderReq::performRequest()
{
    const char method_name[] = "CExtTmLeaderReq::performRequest";
    TRACE_ENTRY;

    CProcess *requester;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_tmleader_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: TmLeader, requester %d\n",
                     method_name, __LINE__, id_, pid_);
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        if ( MyNode->GetShutdownLevel() != ShutdownLevel_Undefined )
        {   // If system is shutting down cannot reliably get tm leader
            // info since some TM processes may have exited.  Just return
            // an error.
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf("%s@%d - System shutting down (level=%d) so not "
                             "getting TmLeader\n", method_name, __LINE__,
                             MyNode->GetShutdownLevel());
            }

            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = -1;
            msg_->u.reply.u.generic.pid = -1;
            msg_->u.reply.u.generic.verifier = -1;
            msg_->u.reply.u.generic.process_name[0] = '\0';
            msg_->u.reply.u.generic.return_code = MPI_ERR_UNKNOWN;
 
            // Send reply to requester
            lioreply(msg_, pid_);

           return;
        }

        // If there is a Regroup in progress in the sync thread, 
        // wait until it is completed. 
        if ( !Emulate_Down )
        {
            Monitor->EnterSyncCycle();
        }
        int tmLeaderNid = Monitor->GetTmLeader();
        if ( !Emulate_Down )
        {
            Monitor->ExitSyncCycle();
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
        {
            trace_printf( "%s@%d - tmLeaderNid=%d\n"
                        , method_name, __LINE__, tmLeaderNid );
        }

        if ( MyNode->GetShutdownLevel() == ShutdownLevel_Undefined )
        {
            CProcess *process;

            process = Nodes->GetLNode(tmLeaderNid)->GetProcessLByType( ProcessType_DTM );
            if (!process && NameServerEnabled)
            {
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                {
                    trace_printf( "%s@%d - Getting process from Name Server, nid=%d, type=ProcessType_DTM\n"
                                , method_name, __LINE__, tmLeaderNid );
                }
            
                process = Nodes->GetProcessLByTypeNs( tmLeaderNid, ProcessType_DTM );
            }

            if (!process)
            {
                int pnid = Nodes->GetLNode(tmLeaderNid)->GetNode()->GetPNid();

                // If there is a Regroup in progress in the sync thread, 
                // wait until it is completed. 
                if ( !Emulate_Down )
                {
                    Monitor->EnterSyncCycle();
                }
                Monitor->AssignTmLeader( pnid, true );
                tmLeaderNid = Monitor->GetTmLeader();
                process = Nodes->GetLNode(tmLeaderNid)->GetProcessLByType( ProcessType_DTM );
                if ( !Emulate_Down )
                {
                    Monitor->ExitSyncCycle();
                }
            }

            if (process)
            {
                // populate the TM leader process info
                msg_->u.reply.type = ReplyType_Generic;
                msg_->u.reply.u.generic.nid = process->GetNid();
                msg_->u.reply.u.generic.pid = process->GetPid();
                msg_->u.reply.u.generic.verifier = process->GetVerifier();
                strcpy (msg_->u.reply.u.generic.process_name, process->GetName());
            }
            else
            {
                tmLeaderNid = -1;
                msg_->u.reply.type = ReplyType_Generic;
                msg_->u.reply.u.generic.nid = -1;
                msg_->u.reply.u.generic.pid = -1;
                msg_->u.reply.u.generic.verifier = -1;
                msg_->u.reply.u.generic.process_name[0] = 0;
            }

            if (process && NameServerEnabled)
            {
                if (!MyNode->IsMyNode( process->GetNid() ))
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                    {
                        trace_printf( "%s@%d - Deleting clone process %s, (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier() );
                    }
                    Nodes->DeleteCloneProcess( process );
                }
            
            }
        }
        else
        {
            tmLeaderNid = -1;
            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = -1;
            msg_->u.reply.u.generic.pid = -1;
            msg_->u.reply.u.generic.verifier = -1;
            msg_->u.reply.u.generic.process_name[0] = 0;
        }

        if (requester->GetNid() == tmLeaderNid)
        {
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            if (trace_settings & TRACE_REQUEST)
                trace_printf("%s@%d" " - I'm the NEW TmLeader" "\n", method_name, __LINE__);
        }
        else
        {
            msg_->u.reply.u.generic.return_code = MPI_ERR_UNKNOWN;
            if (trace_settings & TRACE_REQUEST)
                trace_printf("%s@%d" " - I'm NOT the TmLeader" "\n", method_name, __LINE__);
        }

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
