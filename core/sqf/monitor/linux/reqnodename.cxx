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
#include "replicate.h"
#include "mlio.h"

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;
extern CNode *MyNode;


CExtNodeNameReq::CExtNodeNameReq (reqQueueMsg_t msgType, int pid,
                          struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEZ", 4);
}

CExtNodeNameReq::~CExtNodeNameReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqez", 4);
}

void CExtNodeNameReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_);
    requestString_.assign( strBuf );
}

void CExtNodeNameReq::performRequest()
{
   const char method_name[] = "CExtNodeNameReq::performRequest";
    TRACE_ENTRY;

    CNode    *node = NULL;

    // Trace info about request
     node = Nodes->GetNode(msg_->u.request.u.nodename.current_name); 
     if (node)
     {
           if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           {
                 trace_printf("[%s], Requested node name change from %s to %s\n" , method_name, msg_->u.request.u.nodename.current_name, msg_->u.request.u.nodename.new_name);
           }

           node->SetName(msg_->u.request.u.nodename.new_name);

           CReplNodeName *repl = new CReplNodeName(msg_->u.request.u.nodename.current_name, 
                                msg_->u.request.u.nodename.new_name);
           Replicator.addItem(repl);
           if (!msg_->noreply)  // client needs a reply 
           {
                msg_->u.reply.type = ReplyType_NodeInfo;
                msg_->u.reply.u.generic.return_code = MPI_SUCCESS;
   
                // Send reply to requester
                lioreply(msg_, pid_);
           }
        } else 
        {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           {
                trace_printf("[%s], Internal Error - Requested node name change from %s to %s\n" , method_name, msg_->u.request.u.nodename.current_name, msg_->u.request.u.nodename.new_name);
           }

            if (!msg_->noreply)  // client needs a reply 
            {
                msg_->u.reply.type = ReplyType_NodeInfo;
                msg_->u.reply.u.generic.return_code = MPI_ERR_UNKNOWN;
   
                // Send reply to requester
                 lioreply(msg_, pid_);
            }
	  
	}

    TRACE_EXIT;

}
