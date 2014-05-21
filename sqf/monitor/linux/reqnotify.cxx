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

CExtNotifyReq::CExtNotifyReq (reqQueueMsg_t msgType, int pid,
                              struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEM", 4);
}

CExtNotifyReq::~CExtNotifyReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqem", 4);
}

void CExtNotifyReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_,
              msg_->u.request.u.notify.target_nid, 
              msg_->u.request.u.notify.target_pid );
    requestString_.assign( strBuf );
}

void CExtNotifyReq::performRequest()
{
    const char method_name[] = "CExtNotifyReq::performRequest";
    TRACE_ENTRY;

    bool status = FAILURE;
    CProcess *sourceProcess = NULL;
    CProcess *requester;
    CLNode *node;

    // Record statistics (sonar counters)
       MonStats->req_type_notify_Incr();

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Notify, requester (%d, %d), "
                     "target (%d, %d), cancel=%d\n", method_name, __LINE__,
                     id_, msg_->u.request.u.notify.nid,
                     msg_->u.request.u.notify.pid,
                     msg_->u.request.u.notify.target_nid,
                     msg_->u.request.u.notify.target_pid,
                     msg_->u.request.u.notify.cancel );
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {

        node = Nodes->GetLNode (msg_->u.request.u.notify.nid);
        if ( node )
        {
            sourceProcess = node->GetProcessL(msg_->u.request.u.notify.pid);
        }

        if ( msg_->u.request.u.notify.target_nid == -1 )
        {
            if ( sourceProcess
                 && msg_->u.request.u.notify.target_pid == -1
                 && msg_->u.request.u.notify.cancel)
            {   // Cancel all death notification requests for the source
                // process (considering transaction id)
                sourceProcess->procExitUnregAll(msg_->u.request.u.notify.trans_id);
                status = SUCCESS;
            }
        }
        else
        {
            CProcess *targetProcess = NULL;

            node = Nodes->GetLNode (msg_->u.request.u.notify.target_nid);
            if  (node)
            {
                targetProcess = node->GetProcessL(msg_->u.request.u.notify.target_pid);
            }

            if ( targetProcess )
            { 
                if ( msg_->u.request.u.notify.cancel )
                {   // Unregister interest in death of target process 
                    status = targetProcess->CancelDeathNotification (
                                                                     msg_->u.request.u.notify.nid,
                                                                     msg_->u.request.u.notify.pid,
                                                                     msg_->u.request.u.notify.trans_id);

                }
                else if ( sourceProcess)
                {   // Register interest in death of target process 
                    sourceProcess->procExitReg( targetProcess,
                                                msg_->u.request.u.notify.trans_id);
                    status = SUCCESS;
                }
            }
        }

        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester->GetNid();
        msg_->u.reply.u.generic.pid = requester->GetPid();
        strcpy (msg_->u.reply.u.generic.process_name, requester->GetName());
        if (status == SUCCESS)
        {
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
            if (trace_settings & TRACE_REQUEST)
                trace_printf("%s@%d" " - Successful" "\n", method_name, __LINE__);
        }
        else
        {
            msg_->u.reply.u.generic.return_code = MPI_ERR_NAME;
            if (trace_settings & TRACE_REQUEST)
                trace_printf("%s@%d" " - Unsuccessful" "\n", method_name, __LINE__);
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
