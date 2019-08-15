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

extern CNode* MyNode;
extern int    ClusterId ;
extern int    InstanceId;

CExtInstanceIdReq::CExtInstanceIdReq( reqQueueMsg_t msgType
                                    , int pid
                                    , struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEV", 4);
}

CExtInstanceIdReq::~CExtInstanceIdReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqev", 4);
}

void CExtInstanceIdReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(nid=%d/pid=%d/verifier=%d) "
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.instance_id.nid
            , msg_->u.request.u.instance_id.pid
            , msg_->u.request.u.instance_id.verifier );
    requestString_.assign( strBuf );
}

void CExtInstanceIdReq::performRequest()
{
    const char method_name[] = "CExtInstanceIdReq::performRequest";
    TRACE_ENTRY;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Get, requester (%d,%d:%d)\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.instance_id.nid
                    , msg_->u.request.u.instance_id.pid
                    , msg_->u.request.u.instance_id.verifier );
    }

    CProcess *requester;

    nid_ = msg_->u.request.u.instance_id.nid;
    verifier_ = msg_->u.request.u.instance_id.verifier;

    requester = MyNode->GetProcess( pid_
                                  , verifier_ );
    if ( requester )
    {
        // Process the request
        msg_->u.reply.type = ReplyType_InstanceId;
        msg_->u.reply.u.instance_id.cluster_id  = ClusterId;
        msg_->u.reply.u.instance_id.instance_id = InstanceId;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - cluster_id=%d, instance_id=%d\n"
                        , method_name, __LINE__
                        , msg_->u.reply.u.instance_id.cluster_id
                        , msg_->u.reply.u.instance_id.instance_id );
        }
    }
    else
    {
        // the requester already exited and the LIO buffer will be cleaned up
        msg_->u.reply.type = ReplyType_Generic;
        msg_->u.reply.u.generic.nid = -1;
        msg_->u.reply.u.generic.pid = -1;
        msg_->u.reply.u.generic.verifier = -1;
        msg_->u.reply.u.generic.process_name[0] = '\0';
        msg_->u.reply.u.generic.return_code = MPI_ERR_NAME;
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - requester process not found!\n"
                        , method_name, __LINE__);
        }
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
