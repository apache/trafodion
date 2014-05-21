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
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              ,  msg_->u.request.u.exit.nid, msg_->u.request.u.exit.pid );
    requestString_.assign( strBuf );
}


void CExtExitReq::performRequest()
{
    bool status = FAILURE;

    const char method_name[] = "CExtExitReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
       MonStats->req_type_exit_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld, %d: exit (%d, %d)\n", method_name,
                     __LINE__, id_, pid_, msg_->u.request.u.exit.nid,
                     msg_->u.request.u.exit.pid);
    }

    if ((msg_->u.request.u.exit.nid < 0) ||
        (msg_->u.request.u.exit.nid >= Nodes->NumberLNodes))
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[CMonitor::ExitProcess], Invalid Node ID!\n");
        mon_log_write(MON_MONITOR_EXITPROCESS, SQ_LOG_ERR, buf);
    }
    else
    {
        CProcess *process;
        CNode *node;

        node = Nodes->GetLNode(msg_->u.request.u.exit.nid)->GetNode();
        // Change the process state to indicate process intends to exit.
        // However, exit handling will not occur until the child death signal
        // SIGCHLD is received.
        process = node->GetProcess( msg_->u.request.u.exit.pid );

        if (process)
        {
            node->SetProcessState( process, State_Down, false );

            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = process->GetNid();
            msg_->u.reply.u.generic.pid = process->GetPid();
            strcpy (msg_->u.reply.u.generic.process_name, process->GetName());
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            msg_->noreply = false;
            status = SUCCESS;
        }
    }

    if (status == FAILURE)
    {
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = -1;
        msg_->u.reply.u.generic.pid = -1;
        msg_->u.reply.u.generic.process_name[0] = '\0';
        msg_->u.reply.u.generic.return_code = MPI_ERR_NAME;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d - unsuccessful\n", method_name, __LINE__);
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
