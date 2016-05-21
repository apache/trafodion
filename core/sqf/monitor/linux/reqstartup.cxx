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

CExtStartupReq::CExtStartupReq (reqQueueMsg_t msgType, int pid,
                                struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQES", 4);
}

CExtStartupReq::~CExtStartupReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqes", 4);
}

void CExtStartupReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld "
              "requester(name=%s/nid=%d/pid=%d/os_pid=%d/verifier=%d) "
              "(event_messages=%d/system_messages=%d/paired=%d)"
            , CReqQueue::svcReqType[reqType_], getId()
            , msg_->u.request.u.startup.process_name
            , msg_->u.request.u.startup.nid
            , msg_->u.request.u.startup.pid
            , msg_->u.request.u.startup.os_pid
            , msg_->u.request.u.startup.verifier
            , msg_->u.request.u.startup.event_messages
            , msg_->u.request.u.startup.system_messages
            , msg_->u.request.u.startup.paired );
    requestString_.assign( strBuf );
}

void CExtStartupReq::performRequest()
{
    const char method_name[] = "CExtStartupReq::performRequest";
    TRACE_ENTRY;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: Startup for pid=%d, name=%s, "
                     "port=%s\n",
                     method_name, __LINE__, id_,
                     msg_->u.request.u.startup.os_pid,
                     msg_->u.request.u.startup.process_name,
                     msg_->u.request.u.startup.port_name);
    }

    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_startup_Incr();

    // Process the request
    MyNode->AttachProcessCheck( msg_ );

    // note: requester does not expect an application level reply for a
    // "Startup" message but must signal client so it can dispose of
    // the local io buffer.
    lioreply(msg_, pid_);

}
