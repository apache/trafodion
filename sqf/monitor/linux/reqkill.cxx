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
#include "replicate.h"

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
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
              "ExtReq(%s) req #=%ld requester(pid=%d) (name=%s/nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_,
              msg_->u.request.u.kill.process_name,
              msg_->u.request.u.kill.target_nid,
              msg_->u.request.u.kill.target_pid );
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
    if ( node->GetState() == State_Up && node->GetPNid() != MyPNID )
    {
        // Replicate the kill to other nodes
        CReplKill *repl = new CReplKill( process->GetNid()
                                       , process->GetPid()
                                       , process->GetAbort());
        Replicator.addItem(repl);
    }
    else
    {
        // if the node is not up, then the process was or will be killed
        if ( node->GetState() == State_Up && !process->IsClone() )
        {
            kill (process->GetPid(), Monitor->GetProcTermSig());
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Completed kill for process %s (%d, %d)\n",
                             method_name, __LINE__, process->GetName(),
                             process->GetNid(), process->GetPid());

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
    CLNode *lnode = NULL;

    const char method_name[] = "CExtKillReq::performRequest";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
       MonStats->req_type_kill_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Kill, %s (%d, %d), persistent "
                     "abort=%d\n", method_name, __LINE__, id_,
                     msg_->u.request.u.kill.process_name,
                     msg_->u.request.u.kill.target_nid,
                     msg_->u.request.u.kill.target_pid,
                     msg_->u.request.u.kill.persistent_abort);
    }

    if ((msg_->u.request.u.kill.target_nid == -1) ||
        (msg_->u.request.u.kill.target_pid == -1))
    {
        lnode = Nodes->GetLNode (msg_->u.request.u.kill.process_name, &process);
        if ( process )
        {
            backup = process->GetBackup ();
        }
    }
    else
    {
        lnode = Nodes->GetLNode (msg_->u.request.u.kill.target_nid);
        if ( lnode  && lnode->GetState() == State_Up )
        {
            process = lnode->GetProcessL(msg_->u.request.u.kill.target_pid);
        }
        backup = NULL;
    }
    if (lnode)
    {
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
            strcpy (msg_->u.reply.u.generic.process_name, process->GetName());
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            status = SUCCESS;
        }
        else
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
               trace_printf("%s@%d - Kill %s (%d, %d) -- can't find process\n",
                            method_name, __LINE__,
                        msg_->u.request.u.kill.process_name,
                        msg_->u.request.u.kill.target_nid,
                        msg_->u.request.u.kill.target_pid);
            }
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
           trace_printf("%s@%d - Kill %s (%d, %d) -- can't find process's "
                        "node\n", method_name, __LINE__,
                        msg_->u.request.u.kill.process_name,
                        msg_->u.request.u.kill.target_nid,
                        msg_->u.request.u.kill.target_pid);
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
