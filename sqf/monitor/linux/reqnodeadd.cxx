///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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

    int         rc = MPI_SUCCESS;
    CProcess   *requester = NULL;
    cpu_set_t   coreMask;

    // TODO:
    // Record statistics (sonar counters)
//    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
//       MonStats->req_type_nodeadd_Incr();
       
    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
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
        
        CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
        CPNodeConfig *pnodeConfig = new CPNodeConfig( NULL  // pnodesConfig
                                                    , -1    // pnid
                                                    , -1    // excludedFirstCore
                                                    , -1    // excludedLastCore
                                                    , msg_->u.request.u.node_add.node_name
                                                    );
        if (pnodeConfig)
        {
            clusterConfig->SetCoreMask( msg_->u.request.u.node_add.first_core
                                      , msg_->u.request.u.node_add.last_core
                                      , coreMask );
            CLNodeConfig *lnodeConfig = new CLNodeConfig( pnodeConfig
                                                        , -1    // nid
                                                        , coreMask
                                                        , msg_->u.request.u.node_add.first_core
                                                        , msg_->u.request.u.node_add.last_core
                                                        , msg_->u.request.u.node_add.processors
                                                        , (ZoneType)msg_->u.request.u.node_add.roles
                                                        );
            if (lnodeConfig)
            {
                // The replicate logic will delete lnodeConfig and pnodeConfig

                // Tell all monitors to add this node to the static configuration
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
                    rc = MPI_ERR_NO_MEM;
                    delete pnodeConfig;
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf(la_buf, "[%s], Failed to allocate CReplNodeAdd, no memory!\n",
                            method_name);
                    mon_log_write(MON_REQQUEUE_NODE_ADD_1, SQ_LOG_ERR, la_buf);
                }
            }
            else
            {
                rc = MPI_ERR_NO_MEM;
                delete pnodeConfig;
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Failed to allocate CLNodeConfig, no memory!\n",
                        method_name);
                mon_log_write(MON_REQQUEUE_NODE_ADD_1, SQ_LOG_ERR, la_buf);
            }
        }
        else
        {
            rc = MPI_ERR_NO_MEM;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to allocate CPNodeConfig, no memory!\n",
                    method_name);
            mon_log_write(MON_REQQUEUE_NODE_ADD_3, SQ_LOG_ERR, la_buf);
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
