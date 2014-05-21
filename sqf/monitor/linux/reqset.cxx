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
extern CConfigContainer *Config;

CExtSetReq::CExtSetReq (reqQueueMsg_t msgType, int pid,
                        struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEQ", 4);
}

CExtSetReq::~CExtSetReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqeq", 4);
}

void CExtSetReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), "ExtReq(%s) req #=%ld requester(pid=%d) (group=%s/key=%s)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_,
              msg_->u.request.u.set.group, msg_->u.request.u.set.key );
    requestString_.assign( strBuf );
}


void CExtSetReq::performRequest()
{
    const char method_name[] = "CExtSetReq::performRequest";
    TRACE_ENTRY;

    CConfigGroup *group;
    CProcess *requester;

    // Record statistics (sonar counters)
       MonStats->req_type_set_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Set, type=%d, group=%s, key=%s, "
                     "value=%s\n", method_name, __LINE__, id_,
                     msg_->u.request.u.set.type, msg_->u.request.u.set.group,
                     msg_->u.request.u.set.key,  msg_->u.request.u.set.value);
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        switch (msg_->u.request.u.set.type)
        {
        case ConfigType_Cluster:
            group = Config->GetClusterGroup();
            break;
        case ConfigType_Node:
            if (*msg_->u.request.u.set.group == '\0')
            {
                sprintf(msg_->u.request.u.set.group,"NODE%d",msg_->u.request.u.set.nid);
            }
        case ConfigType_Process:
            group = Config->GetGroup(msg_->u.request.u.set.group);
            if (group==NULL)
            {
                group = Config->AddGroup(msg_->u.request.u.set.group,msg_->u.request.u.set.type);
            }
            break;
        default:
            group = NULL;
        }

        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = requester->GetNid();
        msg_->u.reply.u.generic.nid = requester->GetPid();
        strcpy (msg_->u.reply.u.generic.process_name, requester->GetName());
        if (group)
        {
            group->Set(msg_->u.request.u.set.key,msg_->u.request.u.set.value);
            msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
        }
        else
        {
            msg_->u.reply.u.generic.return_code = MPI_ERR_UNKNOWN;
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
