///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett Packard Enterprise Development LP
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "replicate.h"
#include "reqqueue.h"
#include "montrace.h"
#include "monsonar.h"
#include "monlogging.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;
extern CReplicate Replicator;

CExtNodeDeleteReq::CExtNodeDeleteReq( reqQueueMsg_t msgType
                                    , int pid
                                    , struct message_def *msg
                                    )
                  : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEY", 4);

    priority_    =  High;
}

CExtNodeDeleteReq::~CExtNodeDeleteReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqey", 4);
}

void CExtNodeDeleteReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) "
              "(target_pnid=%d/target_node_name=%s)"
            , CReqQueue::svcReqType[reqType_], getId(), pid_
            , msg_->u.request.u.node_delete.target_pnid
            , msg_->u.request.u.node_delete.target_node_name );
    requestString_.assign( strBuf );
}

void CExtNodeDeleteReq::performRequest()
{
    const char method_name[] = "CExtNodeDeleteReq::performRequest";
    TRACE_ENTRY;

    int pnid = msg_->u.request.u.node_delete.target_pnid;
    int rc = MPI_SUCCESS;
    CClusterConfig *clusterConfig = NULL;
    CPNodeConfig   *pnodeConfig = NULL; 
    CProcess       *requester = NULL;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf( "%s@%d request #%ld: NodeDelete, nid=%d, node_name=%s\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.node_delete.target_pnid
                    , msg_->u.request.u.node_delete.target_node_name );
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        clusterConfig = Nodes->GetClusterConfig();
        if (clusterConfig)
        {
            if (clusterConfig->IsConfigReady())
            {
                if ( pnid == -1 )
                {
                    pnid = clusterConfig->GetPNid( msg_->u.request.u.node_delete.target_node_name );
                }
                pnodeConfig = clusterConfig->GetPNodeConfig( pnid );
                if (pnodeConfig)
                {
                    // Tell all monitors to delete this node from the configuration database
                    // Replicate request to be processed by CIntNodeDelete in all nodes
                    CReplNodeDelete *repl = new CReplNodeDelete( pnodeConfig, requester );
                    if (repl)
                    {
                        // we will not reply at this time ... but wait for 
                        // node delete to be processed in CIntNodeDeleteReq
    
                        // Retain reference to requester's request buffer so can
                        // send completion message.
                        requester->parentContext( msg_ );
                        msg_->noreply = true;
    
                        Replicator.addItem(repl);
                    }
                    else
                    {
                        rc = MPI_ERR_NO_MEM;
                        delete pnodeConfig;
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Failed to allocate CReplNodeDelete, no memory!\n",
                                method_name);
                        mon_log_write(MON_REQ_NODE_DELETE_1, SQ_LOG_ERR, la_buf);
                    }
                }
                else
                {
                    rc = MPI_ERR_NAME;
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf( la_buf
                           , "[%s], Physical node %d (%s) does not exist in configuration!\n"
                           , method_name
                           , msg_->u.request.u.node_delete.target_pnid
                           , msg_->u.request.u.node_delete.target_node_name );
                    mon_log_write(MON_REQ_NODE_DELETE_2, SQ_LOG_ERR, la_buf);
                }
            }
            else
            {
                rc = MPI_ERR_NO_MEM;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Configuration is not available!\n",
                        method_name);
                mon_log_write(MON_REQ_NODE_DELETE_3, SQ_LOG_ERR, la_buf);
            }
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to retrieve ClusterConfig object!\n",
                    method_name);
            mon_log_write(MON_REQ_NODE_DELETE_4, SQ_LOG_CRIT, la_buf);

            rc = MPI_ERR_INTERN;
        }

        if (!msg_->noreply)  // client needs a reply 
        {
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
