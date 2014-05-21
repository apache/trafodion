///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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
              "ExtReq(%s) req #=%ld requester(pid=%d) (group=%s/key=%s)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_,
              msg_->u.request.u.set.group,
              msg_->u.request.u.set.key );
    requestString_.assign( strBuf );
}

void CExtEventReq::performRequest()
{
    const char method_name[] = "CExtEventReq::performRequest";
    TRACE_ENTRY;

    int rc = MPI_ERR_NAME;
    int nid;
    int target_nid;
    int pid;
    int num_procs;
    PROCESSTYPE type;
    CProcess *process = NULL;
    CProcess *requester;

    // Record statistics (sonar counters)
       MonStats->req_type_event_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Event, nid=%d, pid=%d, type=%d, "
                     "event_id=%d, event bytes=%d\n", method_name, __LINE__,
                     id_, msg_->u.request.u.event.target_nid,
                     msg_->u.request.u.event.target_pid,
                     msg_->u.request.u.event.type,
                     msg_->u.request.u.event.event_id,
                     msg_->u.request.u.event.length);
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        // setup for type of status request
        type = msg_->u.request.u.event.type;
        if ( type != ProcessType_Watchdog )
        {
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
                if (trace_settings & TRACE_REQUEST)
                    trace_printf("%s@%d - Event for (%d, %d)\n", method_name, __LINE__, msg_->u.request.u.event.target_nid, msg_->u.request.u.event.target_pid);
                nid = msg_->u.request.u.event.target_nid;
                target_nid = nid;
                pid = msg_->u.request.u.event.target_pid;
                if (pid == -1)
                {
                    // get info for all processes in node
                    process = Nodes->GetLNode(nid)->GetFirstProcess();
                }
                else
                {
                    // get info for single process in node
                    process = Nodes->GetLNode(nid)->GetProcessL(pid);
                }
            }

            // process events
            num_procs = 0;
            for (; nid < Nodes->NumberLNodes; nid++)
            {
                if (target_nid == -1)
                {
                    process = Nodes->GetLNode(nid)->GetFirstProcess();
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
                                    trace_printf("%s@%d - Event %d sent to (%d, %d)\n", method_name, __LINE__, msg_->u.request.u.event.event_id, process->GetNid(), process->GetPid());
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
        }

        // build reply
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester->GetNid();
        msg_->u.reply.u.generic.nid = requester->GetPid();
        strcpy (msg_->u.reply.u.generic.process_name, requester->GetName());
        msg_->u.reply.u.generic.return_code = rc;

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
