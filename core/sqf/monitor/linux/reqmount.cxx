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
#include "device.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CDeviceContainer *Devices;

CExtMountReq::CExtMountReq (reqQueueMsg_t msgType, int pid,
                            struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEH", 4);

    execTimeMax_    = 60;
}

CExtMountReq::~CExtMountReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqeh", 4);
}


void CExtMountReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (name=%s/nid=%d/pid=%d/verifier=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.mount.process_name
              , msg_->u.request.u.mount.nid
              , msg_->u.request.u.mount.pid
              , msg_->u.request.u.mount.verifier );
    requestString_.assign( strBuf );
}


void CExtMountReq::performRequest()
{
    const char method_name[] = "CExtMountReq::performRequest";
    TRACE_ENTRY;

    CProcess *requester;
    CLogicalDevice *ldevice = NULL;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_mount_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: Mount, requester %s (%d, %d:%d)\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.mount.process_name
                    , msg_->u.request.u.mount.nid
                    , msg_->u.request.u.mount.pid
                    , msg_->u.request.u.mount.verifier );
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        if (Devices->MountVolume( requester->GetName(), requester, &ldevice ))
        {
            msg_->u.reply.u.mount.primary_state = 
                ldevice->PrimaryMounted() ? State_Mounted : State_UnMounted;
            msg_->u.reply.u.mount.mirror_state = 
                ldevice->MirrorMounted() ? State_Mounted : State_UnMounted;
            msg_->u.reply.u.mount.return_code = MPI_SUCCESS;
        }
        else
        {
            msg_->u.reply.u.mount.primary_state = State_UnMounted;
            msg_->u.reply.u.mount.mirror_state = State_UnMounted;
            msg_->u.reply.u.mount.return_code = MPI_ERR_UNKNOWN;
        }
        msg_->u.reply.type = ReplyType_Mount;
 
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
