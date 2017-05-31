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

extern CMonitor *Monitor;
extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

CExtPNodeInfoReq::CExtPNodeInfoReq (reqQueueMsg_t msgType, int pid,
                                    struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEX", 4);
}

CExtPNodeInfoReq::~CExtPNodeInfoReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqex", 4);
}

void CExtPNodeInfoReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d) (pnid=%d) (name=%s))"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.pnode_info.target_pnid
              , msg_->u.request.u.pnode_info.target_name );
    requestString_.assign( strBuf );
}


void CExtPNodeInfoReq::performRequest()
{
    const char method_name[] = "CExtPNodeInfoReq::performRequest";
    TRACE_ENTRY;

    int num_returned = 0;
    bool continuation = msg_->u.request.u.pnode_info.continuation;
    bool byNodeName = strlen(msg_->u.request.u.pnode_info.target_name) ? true : false;
    int pnid;
    int last_pnid = msg_->u.request.u.pnode_info.last_pnid;
    int end_pnid = -1;
    int target_pnid = msg_->u.request.u.pnode_info.target_pnid;
//    CLNode *lnode = NULL;
    CNode  *pnode = NULL;
    char    node_name[MPI_MAX_PROCESSOR_NAME] = { 0 }; // Node's name
    char    la_buf[MON_STRING_BUF_SIZE] = { 0 };

    if ( byNodeName )
    {
        strcpy( node_name, msg_->u.request.u.pnode_info.target_name );
    }
    
    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodeinfo_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        if (continuation)
        {
            trace_printf("%s@%d request #%ld: PNodeInfo, pnid=%d, name=%s, continue "
                         "from pnid=%d\n", method_name, __LINE__, id_,
                         target_pnid, node_name, last_pnid);
        }
        else
        {
            trace_printf("%s@%d request #%ld: PNodeInfo, pnid=%d, name=%s\n", method_name,
                         __LINE__, id_, target_pnid, node_name);
        }
    }

    // We need current scheduling data from all nodes.
    Replicator.SetSyncClusterData();

    if ( target_pnid == -1 && byNodeName )
    { // Get node by name
        pnode = Nodes->GetNode( node_name );
        if ( pnode )
        {
            target_pnid = pnode->GetPNid();
        }
    }

    if ( target_pnid == -1 )
    {
        pnid = 0;
        end_pnid = Nodes->GetPNodesConfigMax()-1;
    }
    else if ((target_pnid >= 0                     ) ||
             (target_pnid < Nodes->GetPNodesConfigMax())   )
    {
        pnid = end_pnid = target_pnid;
    }
    else
    {
        pnid = -1;
    }
    
    // build reply
    msg_->u.reply.type = ReplyType_PNodeInfo;
    if ( pnid != -1 )
    {
        if ( continuation )
        {   // This is the continuation of a previous request.  Find
            // the place in the list where we left off.
            pnid = (last_pnid + 1);
        }

        // Load the node counts first
        msg_->u.reply.u.pnode_info.num_nodes = Nodes->GetLNodesCount();
        msg_->u.reply.u.pnode_info.num_pnodes = Nodes->GetPNodesCount();
        msg_->u.reply.u.pnode_info.num_spares = Nodes->GetSNodesCount();
        msg_->u.reply.u.pnode_info.num_available_spares = Nodes->GetAvailableSNodesCount();
    
        for ( ; pnid < Nodes->GetPNodesConfigMax() && pnid <= end_pnid; pnid++)
        {
            pnode = Nodes->GetNode( pnid );
            if ( pnode )
            {
                msg_->u.reply.u.pnode_info.node[num_returned].pnid = pnode->GetPNid();
                strcpy(msg_->u.reply.u.pnode_info.node[num_returned].node_name, pnode->GetName());
                msg_->u.reply.u.pnode_info.node[num_returned].pstate = pnode->GetState();
                msg_->u.reply.u.pnode_info.node[num_returned].lnode_count = pnode->GetLNodesCount();
                msg_->u.reply.u.pnode_info.node[num_returned].process_count = pnode->GetNumProcs();

                msg_->u.reply.u.pnode_info.node[num_returned].spare_node = pnode->IsSpareNode();
                if (( pnode->GetState() == State_Up       ) ||
                    ( pnode->GetState() == State_Shutdown )  )
                {
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_total = pnode->GetMemTotal();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_free = pnode->GetFreeMemory();
                    msg_->u.reply.u.pnode_info.node[num_returned].swap_free = pnode->GetFreeSwap();
                    msg_->u.reply.u.pnode_info.node[num_returned].cache_free = pnode->GetFreeCache();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_active = pnode->GetMemActive();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_inactive = pnode->GetMemInactive();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_dirty = pnode->GetMemDirty();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_writeback = pnode->GetMemWriteback();
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_VMallocUsed = pnode->GetMemVMallocUsed();
                    msg_->u.reply.u.pnode_info.node[num_returned].btime = pnode->GetBTime();
                    msg_->u.reply.u.pnode_info.node[num_returned].cores = pnode->GetNumCores();
                }
                else
                {
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_total = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_free = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].swap_free = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].cache_free = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_active = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_inactive = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_dirty = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_writeback = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].memory_VMallocUsed = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].btime = 0;
                    msg_->u.reply.u.pnode_info.node[num_returned].cores = 0;
                }

                num_returned++;
                if ( num_returned == MAX_NODE_LIST )
                {
                    msg_->u.reply.u.pnode_info.last_pnid = pnode->GetPNid();
                    break;
                }
            }
            else
            {
                sprintf(la_buf, "[%s], Can't find PNid=%d\n", method_name, pnid);
                mon_log_write(MON_MONITOR_PNODEINFO_1, SQ_LOG_ERR, la_buf);
                msg_->u.reply.u.pnode_info.num_returned = 0;
                msg_->u.reply.u.pnode_info.return_code = MPI_ERR_NAME;
            }
        }

        msg_->u.reply.u.pnode_info.integrating = Monitor->IsIntegrating();

        msg_->u.reply.u.pnode_info.num_returned = num_returned;
        if ( num_returned <= MAX_NODE_LIST )
        {
            msg_->u.reply.u.pnode_info.return_code = MPI_SUCCESS;
        }
        else
        {
            msg_->u.reply.u.pnode_info.return_code = MPI_ERR_TRUNCATE;
        }
    }
    else         
    {
        sprintf(la_buf, "[%s], Invalid Node ID!\n", method_name);
        mon_log_write(MON_MONITOR_PNODEINFO_2, SQ_LOG_ERR, la_buf);
        msg_->u.reply.u.pnode_info.num_returned = 0;
        msg_->u.reply.u.pnode_info.return_code = MPI_ERR_NAME;
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
