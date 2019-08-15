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
#include "clusterconf.h"

extern CMonStats *MonStats;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CMonitor *Monitor;
extern CReplicate Replicator;

CExtNodeAddReq::CExtNodeAddReq( reqQueueMsg_t msgType
                              , int pid
                              , struct message_def *msg
                              )
               : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEX", 4);

    priority_    =  High;
}

CExtNodeAddReq::~CExtNodeAddReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqex", 4);
}

void CExtNodeAddReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf), 
              "ExtReq(%s) req #=%ld requester(pid=%d) "
              "(node_name=%s/first_core=%d/last_core=%d/processors=%d/roles=%d)"
            , CReqQueue::svcReqType[reqType_], getId(), pid_
            , msg_->u.request.u.node_add.node_name
            , msg_->u.request.u.node_add.first_core
            , msg_->u.request.u.node_add.last_core
            , msg_->u.request.u.node_add.processors
            , msg_->u.request.u.node_add.roles );
    requestString_.assign( strBuf );
}

void CExtNodeAddReq::performRequest()
{
    const char method_name[] = "CExtNodeAddReq::performRequest";
    TRACE_ENTRY;

    int             rc = MPI_SUCCESS;
    pnodeConfigInfo_t pnodeConfigInfo;
    lnodeConfigInfo_t lnodeConfigInfo;
    CClusterConfig *clusterConfig = NULL;
    CLNodeConfig   *lnodeConfig = NULL;
    CPNodeConfig   *pnodeConfig = NULL; 
    CProcess       *requester = NULL;

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_REQUEST_DETAIL))
    {
        trace_printf("%s@%d request #%ld: NodeAdd, "
                     "node_name=%s, first_core=%d, last_core=%d, "
                     "processors=%d, roles=%d\n"
                    , method_name, __LINE__, id_
                    , msg_->u.request.u.node_add.node_name
                    , msg_->u.request.u.node_add.first_core
                    , msg_->u.request.u.node_add.last_core
                    , msg_->u.request.u.node_add.processors
                    , msg_->u.request.u.node_add.roles );
    }

    requester = MyNode->GetProcess( pid_ );
    if ( requester )
    {
        clusterConfig = Nodes->GetClusterConfig();
        if (clusterConfig)
        {
            if (clusterConfig->GetPNodesCount() <
                clusterConfig->GetPNodesConfigMax())
            {
                // Check for existence of node name in the configuration
                pnodeConfig = clusterConfig->GetPNodeConfig( msg_->u.request.u.node_add.node_name );
                if (!pnodeConfig)
                {
                    pnodeConfigInfo.pnid = -1;
                    strncpy( pnodeConfigInfo.nodename
                           , msg_->u.request.u.node_add.node_name
                           , sizeof(pnodeConfigInfo.nodename) );
                    pnodeConfigInfo.excludedFirstCore = -1;
                    pnodeConfigInfo.excludedLastCore = -1;
                    clusterConfig->SetCoreMask( pnodeConfigInfo.excludedFirstCore
                                              , pnodeConfigInfo.excludedLastCore
                                              , pnodeConfigInfo.excludedCoreMask );
                    pnodeConfigInfo.spareCount = 0;
                    memset( pnodeConfigInfo.sparePNid
                          , -1
                          , sizeof(pnodeConfigInfo.sparePNid) );
                    pnodeConfig = new CPNodeConfig( NULL  // pnodesConfig
                                                  , pnodeConfigInfo );
                    if (pnodeConfig)
                    {
                        lnodeConfigInfo.nid = -1;
                        lnodeConfigInfo.pnid = -1;
                        strncpy( lnodeConfigInfo.nodename
                               , msg_->u.request.u.node_add.node_name
                               , sizeof(lnodeConfigInfo.nodename) );
                        lnodeConfigInfo.firstCore = msg_->u.request.u.node_add.first_core;
                        lnodeConfigInfo.lastCore  = msg_->u.request.u.node_add.last_core;
                        lnodeConfigInfo.processor = msg_->u.request.u.node_add.processors;
                        clusterConfig->SetCoreMask( lnodeConfigInfo.lastCore
                                                  , lnodeConfigInfo.firstCore
                                                  , lnodeConfigInfo.coreMask );
                        lnodeConfigInfo.zoneType  = static_cast<ZoneType>(msg_->u.request.u.node_add.roles);
                        lnodeConfig = new CLNodeConfig( pnodeConfig
                                                      , lnodeConfigInfo );
                        if (lnodeConfig)
                        {
                            // Tell all monitors to add this node to the configuration database
                            // Replicate request to be processed by CIntNodeAdd in all nodes
                            CReplNodeAdd *repl = new CReplNodeAdd( lnodeConfig, requester );
                            if (repl)
                            {
                                // we will not reply at this time ... but wait for 
                                // node add to be processed in CIntNodeAddReq
            
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
                                sprintf(la_buf, "[%s], Failed to allocate CReplNodeAdd, no memory!\n",
                                        method_name);
                                mon_log_write(MON_REQ_NODE_ADD_1, SQ_LOG_ERR, la_buf);
    
                                rc = MPI_ERR_NO_MEM;
                            }
                        }
                        else
                        {
                            delete pnodeConfig;
                            char la_buf[MON_STRING_BUF_SIZE];
                            sprintf(la_buf, "[%s], Failed to allocate CLNodeConfig, no memory!\n",
                                    method_name);
                            mon_log_write(MON_REQ_NODE_ADD_2, SQ_LOG_ERR, la_buf);
    
                            rc = MPI_ERR_NO_MEM;
                        }
                    }
                    else
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Failed to allocate CPNodeConfig, no memory!\n",
                                method_name);
                        mon_log_write(MON_REQ_NODE_ADD_3, SQ_LOG_ERR, la_buf);
    
                        rc = MPI_ERR_NO_MEM;
                    }
                }
                else
                {
                    // Node name already exists 
                    rc = MPI_ERR_NAME;
                }
            }
            else
            {
                // Already a nodes configuration limit
                rc = MPI_ERR_OP;
            }
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to retrieve ClusterConfig object!\n",
                    method_name);
            mon_log_write(MON_REQ_NODE_ADD_4, SQ_LOG_CRIT, la_buf);

            rc = MPI_ERR_INTERN;
        }

        if (rc != MPI_SUCCESS)
        {
            // Unable to initiate add request
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
