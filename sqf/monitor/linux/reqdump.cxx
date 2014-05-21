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
              "ExtReq(%s) req #=%ld requester(pid=%d) (name=%s/nid=%d/pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.dump.process_name
              , msg_->u.request.u.dump.target_nid
              , msg_->u.request.u.dump.target_pid );
    requestString_.assign( strBuf );
}

void CExtDumpReq::performRequest()
{
    const char method_name[] = "CExtDumpReq::performRequest";
    TRACE_ENTRY;

    CProcess *target;
    CProcess *requester;
    CLNode    *node;
    int        rc = MPI_SUCCESS;

    // Record statistics (sonar counters)
       MonStats->req_type_dump_Incr();

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        // Trace info about request
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d request #%ld: Dump, requester (%d, %d), "
                         "target %s (%d, %d), path=%s\n",
                         method_name, __LINE__, id_,
                         requester->GetNid(), requester->GetPid(),
                         msg_->u.request.u.dump.process_name,
                         msg_->u.request.u.dump.target_nid,
                         msg_->u.request.u.dump.target_pid,
                         msg_->u.request.u.dump.path);
        }

        if (msg_->u.request.u.dump.target_nid == -1)
        {
            node = Nodes->GetLNode(msg_->u.request.u.dump.process_name,
                                   &target);
            if (!target)
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Can't Find Process name=%s\n",
                                 method_name, __LINE__,
                                 msg_->u.request.u.dump.process_name);
        } else
        {
            node = Nodes->GetLNode (msg_->u.request.u.dump.target_nid);
            if (node)
            {
                target = node->GetProcessL(msg_->u.request.u.dump.target_pid);
                if (!target)
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                        trace_printf("%s@%d - Can't Find Process nid=%d, "
                                     "pid=%d\n", method_name, __LINE__,
                                     msg_->u.request.u.dump.target_nid,
                                     msg_->u.request.u.dump.target_pid);
            } else
            {
                target = NULL;
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Can't Find Process's Node nid=%d\n",
                                 method_name, __LINE__,
                                 msg_->u.request.u.dump.target_nid);
            }
        }

        if (!target)
        {
            rc = MPI_ERR_NAME;
        }

        if (rc == MPI_SUCCESS)
        {
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Dump Process name=%s, nid=%d, pid=%d\n",
                             method_name, __LINE__, target->GetName(),
                             target->GetNid(), target->GetPid());
            target->parentContext(msg_);
            if (node->Dump_Process(requester,
                                   target,
                                   msg_->u.request.u.dump.path) != SUCCESS)
                rc = MPI_ERR_SPAWN;
        }

        msg_->u.reply.u.dump.return_code = rc;
        if (rc != MPI_SUCCESS)
        {   // Unable to dump process so send an error reply.
            //
            // Otherwise the dump has been initiated and reply will be sent
            // upon completion.

            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Unsuccessful rc=%d\n", method_name, __LINE__, rc);
            // build reply
            msg_->u.reply.type = ReplyType_Dump;
            msg_->u.reply.u.dump.nid = requester->GetNid();
            msg_->u.reply.u.dump.nid = requester->GetPid();
            strcpy (msg_->u.reply.u.dump.process_name, requester->GetName());
            msg_->u.reply.u.dump.core_file[0] = 0;

            // Send reply to requester
            lioreply(msg_, pid_);
        }
    }
    else
    {   // Reply to requester so it can release the buffer.  
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
