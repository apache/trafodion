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

    int             rc = MPI_SUCCESS;
    CClusterConfig *clusterConfig = NULL;
    CPNodeConfig   *pnodeConfig = NULL; 
    CProcess       *requester = NULL;

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        clusterConfig = Nodes->GetClusterConfig();
        if (clusterConfig)
        {
            // Check for existence of current node name in the configuration
            pnodeConfig = clusterConfig->GetPNodeConfig( msg_->u.request.u.nodename.current_name );
            if (pnodeConfig)
            {
                if ( trace_settings & (TRACE_REQUEST | TRACE_PROCESS) )
                {
                    trace_printf( "%s@%d Requested node name change from %s to %s\n"
                                , method_name, __LINE__
                                , msg_->u.request.u.nodename.current_name
                                , msg_->u.request.u.nodename.new_name );
                }
        
                // Tell all monitors to change this node's name in the configuration database
                // Replicate request to be processed by CIntNodeAdd in all nodes
                CReplNodeName *repl = new CReplNodeName( msg_->u.request.u.nodename.current_name
                                                       , msg_->u.request.u.nodename.new_name
                                                       , requester );
                if (repl)
                {
                    // we will not reply at this time ... but wait for 
                    // node name change to be processed in CIntNodeNameReq
    
                    // Retain reference to requester's request buffer so can
                    // send completion message.
                    requester->parentContext( msg_ );
                    msg_->noreply = true;
    
                    Replicator.addItem(repl);
                }
                else
                {
                    delete pnodeConfig;
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf(la_buf, "[%s], Failed to allocate CReplNodeName, no memory!\n",
                            method_name);
                    mon_log_write(MON_REQ_NODE_NAME_1, SQ_LOG_ERR, la_buf);

                    rc = MPI_ERR_NO_MEM;
                }
            }
            else
            {
                // Node name does not exist
                rc = MPI_ERR_NAME;
            }
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to retrieve ClusterConfig object!\n",
                    method_name);
            mon_log_write(MON_REQ_NODE_NAME_2, SQ_LOG_CRIT, la_buf);

            rc = MPI_ERR_INTERN;
        }
        

        if (rc != MPI_SUCCESS)
        {
            // Unable to initiate node name change request
            msg_->u.reply.type = ReplyType_Generic;
            msg_->u.reply.u.generic.nid = requester->GetNid();
            msg_->u.reply.u.generic.pid = pid_;
            msg_->u.reply.u.generic.verifier = requester->GetVerifier() ;
            msg_->u.reply.u.generic.process_name[0] = '\0';
            msg_->u.reply.u.generic.return_code = rc;
    
            // Send reply to requester
            lioreply(msg_, pid_);
        }
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - Can't find requester, rc=%d\n", method_name, __LINE__, MPI_ERR_NAME);
        // We don't know about this process.
        errorReply( MPI_ERR_EXITED );
    }

    TRACE_EXIT;
}
