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
#include "replicate.h"
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"

extern int MyPNID;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

extern const char *ProcessTypeString( PROCESSTYPE type );

CExtNewProcNsReq::CExtNewProcNsReq (reqQueueMsg_t msgType,
                                    int nid, int pid, int sockFd,
                                    struct message_def *msg )
    : CExternalReq(msgType, nid, pid, sockFd, msg)
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
    CLNode *parent_lnode;
    CNode *parent_node;
    CProcess *parent;
    int result;
    lnode = Nodes->GetLNode( nid_ );
    node = lnode->GetNode();
    parent_lnode = Nodes->GetLNode( msg_->u.request.u.new_process_ns.parent_nid );
    if ( parent_lnode )
        parent_node = parent_lnode->GetNode();
    else
        parent_node = NULL;
    if ( parent_node )
        parent = parent_node->GetProcess( msg_->u.request.u.new_process_ns.parent_pid );
    else
        parent = NULL;
    if ( parent )
    {
        int parentVerifier = msg_->u.request.u.new_process_ns.parent_verifier;

        if ( (parentVerifier == -1) || 
             (parentVerifier == parent->GetVerifier()) )
        {
            if ( msg_->u.request.u.new_process_ns.backup && 
                (parent->GetPairParentNid() == -1 && 
                 parent->GetPairParentPid() == -1))
            {
                parent->SetPairParentNid( msg_->u.request.u.new_process_ns.pair_parent_nid );
                parent->SetPairParentPid( msg_->u.request.u.new_process_ns.pair_parent_pid );
                parent->SetPairParentVerifier( msg_->u.request.u.new_process_ns.pair_parent_verifier );
            }
        }
    }
    CProcess *process = node->CreateProcess ( parent,
                                              msg_->u.request.u.new_process_ns.nid,
                                              msg_->u.request.u.new_process_ns.pid,
                                              msg_->u.request.u.new_process_ns.verifier,
                                              msg_->u.request.u.new_process_ns.event_messages,
                                              msg_->u.request.u.new_process_ns.system_messages,
                                              msg_->u.request.u.new_process_ns.type,
                                              msg_->u.request.u.new_process_ns.debug,
                                              msg_->u.request.u.new_process_ns.priority,
                                              msg_->u.request.u.new_process_ns.backup,
                                              msg_->u.request.u.new_process_ns.unhooked,
                                              msg_->u.request.u.new_process_ns.process_name,
                                              msg_->u.request.u.new_process_ns.path,
                                              msg_->u.request.u.new_process_ns.ldpath,
                                              msg_->u.request.u.new_process_ns.program,
//                                              msg_->u.request.u.new_process_ns.pathStrId,
//                                              msg_->u.request.u.new_process_ns.ldpathStrId,
//                                              msg_->u.request.u.new_process_ns.programStrId,
                                              msg_->u.request.u.new_process_ns.infile,
                                              msg_->u.request.u.new_process_ns.outfile,
                                              0, // tag
                                              result
                                            );
    if (process)
    {
        if ( MyPNID == process->GetOrigPNidNs() )
        {
            process->userArgs (  msg_->u.request.u.new_process_ns.argc,
                                 msg_->u.request.u.new_process_ns.argv );

            process->CompleteProcessStartup( msg_->u.request.u.new_process_ns.port_name
                                           , msg_->u.request.u.new_process_ns.pid
                                           , msg_->u.request.u.new_process_ns.event_messages
                                           , msg_->u.request.u.new_process_ns.system_messages
                                           , false
                                           , &msg_->u.request.u.new_process_ns.creation_time
                                           , process->GetOrigPNidNs() );

            // Replicate to other nodes
            CReplClone *repl = new CReplClone(process);
            if (repl)
            {
                // we will not reply at this time ... but wait for 
                // new process request to be processed in CIntCloneProcNsReq
    
                // Retain reference to requester's request buffer so can
                // send completion message.
                process->SetMonContext( msg_ );
                process->SetMonSockFd( sockFd_ );
                msg_->noreply = true;
    
                Replicator.addItem(repl);
            }
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        msg_->u.reply.type = ReplyType_NewProcessNs;
        msg_->u.reply.u.new_process_ns.return_code = MPI_ERR_SPAWN;
        sprintf(la_buf, "[%s], Unsuccessful!\n", method_name);
        mon_log_write(NS_EXTNEWPROCNSREQ_1, SQ_LOG_CRIT, la_buf);

        // Send reply to requester
        monreply(msg_, sockFd_);
    }

    TRACE_EXIT;
}
