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

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "(type%d/group=%s/key=%s)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.set.process_name
            , msg_->u.request.u.set.nid
            , msg_->u.request.u.set.pid
            , pid_
            , msg_->u.request.u.set.verifier
            , msg_->u.request.u.set.type
            , msg_->u.request.u.set.group
            , msg_->u.request.u.set.key );
    requestString_.assign( strBuf );
}


void CExtSetReq::performRequest()
{
    const char method_name[] = "CExtSetReq::performRequest";
    TRACE_ENTRY;

    CConfigGroup *group;
    CProcess *requester;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_set_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Set, requester %s (%d, %d:%d), "
                      "type=%d, group=%s, key=%s, value=%s\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.set.process_name
                    , msg_->u.request.u.set.nid
                    , msg_->u.request.u.set.pid
                    , msg_->u.request.u.set.verifier
                    , msg_->u.request.u.set.type
                    , msg_->u.request.u.set.group
                    , msg_->u.request.u.set.key
                    , msg_->u.request.u.set.value);
    }
    
    nid_ = msg_->u.request.u.set.nid;
    verifier_ = msg_->u.request.u.set.verifier;
    processName_ = msg_->u.request.u.set.process_name;

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
        msg_->u.reply.u.generic.pid = requester->GetPid();
        msg_->u.reply.u.generic.verifier = requester->GetVerifier();
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
    {
        // the requester already exited and the LIO buffer will be cleaned up
    }

    TRACE_EXIT;
}
