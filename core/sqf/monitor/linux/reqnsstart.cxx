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
#include "nameserver.h"

extern CNameServer *NameServer;

CExtNameServerStartReq::CExtNameServerStartReq (reqQueueMsg_t msgType, int pid,
                                                struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEE", 4);
}

CExtNameServerStartReq::~CExtNameServerStartReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQee", 4);
}


void CExtNameServerStartReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf)
            , "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d)"
            , CReqQueue::svcReqType[reqType_], getId(), pid_
            , msg_->u.request.u.nameserver_start.nid );
    requestString_.assign( strBuf );
}


void CExtNameServerStartReq::performRequest()
{
    const char method_name[] = "CExtNameServerStartReq::performRequest";
    TRACE_ENTRY;

    NameServer->NameServerStop(msg_); // in reqQueue thread (CExternalReq)

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
