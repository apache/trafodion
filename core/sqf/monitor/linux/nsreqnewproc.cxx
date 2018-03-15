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

#include "nstype.h"

#include <stdio.h>
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"
#include "device.h"
#include "replicate.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

extern const char *ProcessTypeString( PROCESSTYPE type );

CExtNewProcNsReq::CExtNewProcNsReq (reqQueueMsg_t msgType, int pid,
                                    int sockFd,
                                    struct message_def *msg )
    : CExternalReq(msgType, pid, sockFd, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEB", 4);
}

CExtNewProcNsReq::~CExtNewProcNsReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQeb", 4);
}

void CExtNewProcNsReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d/pid=%d/verifier=%d/name=%s/"
              "type=%s/priority=%d)"
              , CReqQueue::svcReqType[reqType_], getId()
              , pid_
              , msg_->u.request.u.new_process_ns.nid
              , msg_->u.request.u.new_process_ns.pid
              , msg_->u.request.u.new_process_ns.verifier
              , msg_->u.request.u.new_process_ns.process_name
              , ProcessTypeString(msg_->u.request.u.new_process_ns.type)
              , msg_->u.request.u.new_process_ns.priority );
    requestString_.assign( strBuf );
}

void CExtNewProcNsReq::performRequest()
{
    const char method_name[] = "CExtNewProcNsReq::performRequest";
    TRACE_ENTRY;

    CLNode *lnode;
    CNode *node;
    int result;
    lnode = Nodes->GetLNode(msg_->u.request.u.new_process_ns.nid);
    node = lnode->GetNode();
    strId_t pathStrId = MyNode->GetStringId ( msg_->u.request.u.new_process_ns.path );
    strId_t ldpathStrId = MyNode->GetStringId (msg_->u.request.u.new_process_ns.ldpath );
    strId_t programStrId = MyNode->GetStringId ( msg_->u.request.u.new_process_ns.program );
    CProcess *requester = MyNode->GetProcess( pid_ );
    CProcess *process = node->CreateProcess ( requester,
                                              msg_->u.request.u.new_process_ns.nid,
                                              msg_->u.request.u.new_process_ns.pid,
                                              msg_->u.request.u.new_process_ns.verifier,
                                              msg_->u.request.u.new_process_ns.type,
                                              msg_->u.request.u.new_process_ns.debug,
                                              msg_->u.request.u.new_process_ns.priority,
                                              msg_->u.request.u.new_process_ns.backup,
                                              msg_->u.request.u.new_process_ns.unhooked,
                                              msg_->u.request.u.new_process_ns.process_name,
                                              pathStrId,
                                              ldpathStrId,
                                              programStrId,
                                              msg_->u.request.u.new_process_ns.infile,
                                              msg_->u.request.u.new_process_ns.outfile,
                                              result
                                            );
    process = process; // touch

    msg_->u.reply.type = ReplyType_NewProcessNs;
    msg_->u.reply.u.new_process_ns.nid = msg_->u.request.u.new_process_ns.nid;
    msg_->u.reply.u.new_process_ns.pid = msg_->u.request.u.new_process_ns.pid;
    msg_->u.reply.u.new_process_ns.verifier = msg_->u.request.u.new_process_ns.verifier;
    strncpy(msg_->u.reply.u.new_process_ns.process_name, msg_->u.request.u.new_process_ns.process_name, MAX_PROCESS_NAME);
    msg_->u.reply.u.new_process_ns.return_code = MPI_SUCCESS;

    // Send reply to requester
    monreply(msg_, sockFd_);

    TRACE_EXIT;
}
