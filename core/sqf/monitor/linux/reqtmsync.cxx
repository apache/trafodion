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
extern CMonitor *Monitor;

CExtTmSyncReq::CExtTmSyncReq (reqQueueMsg_t msgType, int pid,
                              struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEV", 4);
}

CExtTmSyncReq::~CExtTmSyncReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqev", 4);
}

void CExtTmSyncReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.tm_sync.nid );
    requestString_.assign( strBuf );
}


void CExtTmSyncReq::performRequest()
{
    const char method_name[] = "CExtTmSyncReq::performRequest";
    TRACE_ENTRY;

    int         handle;
    CTmSyncReq *tmsync_req;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_tmsync_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: TmSync, nid=%d, tag=%d, length=%d\n",
                     method_name, __LINE__, id_, msg_->u.request.u.tm_sync.nid,
                     msg_->u.request.u.tm_sync.tag,
                     msg_->u.request.u.tm_sync.length);
    }

    handle = Monitor->GetHandle();
    tmsync_req = Monitor->Q_TmSync( msg_->u.request.u.tm_sync.nid,
                                    handle,
                                    msg_->u.request.u.tm_sync.data,
                                    msg_->u.request.u.tm_sync.length, 
                                    msg_->u.request.u.tm_sync.tag,
                                    false );
    msg_->u.reply.type = ReplyType_TmSync;
    msg_->u.reply.u.tm_sync.nid = -1;
    msg_->u.reply.u.tm_sync.pid = 0;
    msg_->u.reply.u.tm_sync.handle = handle;
    msg_->u.reply.u.tm_sync.return_code = MPI_SUCCESS;
    tmsync_req->Completed = true;

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
