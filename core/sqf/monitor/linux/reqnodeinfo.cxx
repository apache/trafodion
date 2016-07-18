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

extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

CExtNodeInfoReq::CExtNodeInfoReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEK", 4);
}

CExtNodeInfoReq::~CExtNodeInfoReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqek", 4);
}

void CExtNodeInfoReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d))"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.node_info.target_nid );
    requestString_.assign( strBuf );
}


void CExtNodeInfoReq::performRequest()
{
    const char method_name[] = "CExtNodeInfoReq::performRequest";
    TRACE_ENTRY;

    int nid;
    int end_nid;
    int num_returned = 0;
    bool continuation = msg_->u.request.u.node_info.continuation;
    int pnid;
    int last_nid = msg_->u.request.u.node_info.last_nid;
    int last_pnid = msg_->u.request.u.node_info.last_pnid;
    int end_pnid = -1;
    int target_nid = msg_->u.request.u.node_info.target_nid;
    int lnodesCount = 0;
    int pnodesCount = 0;
    CLNode *lnode = NULL;
    CNode *pnode = NULL;
    char la_buf[MON_STRING_BUF_SIZE];

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodeinfo_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        if (continuation)
        {
            trace_printf("%s@%d request #%ld: NodeInfo, nid=%d, continue "
                         "from nid=%d, pnid=%d\n", method_name, __LINE__, id_,
                         target_nid, last_nid, last_pnid);
        }
        else
        {
            trace_printf("%s@%d request #%ld: NodeInfo, nid=%d\n", method_name,
                         __LINE__, id_, target_nid);
        }
    }

    // We need current scheduling data from all nodes.
    Replicator.SetSyncClusterData();

    if ( target_nid == -1 )
    {
        nid = 0;
        end_nid = Nodes->GetLNodesConfigMax()-1;
        pnid = 0;
        end_pnid = Nodes->GetPNodesConfigMax()-1;
    }
    else if ((target_nid >= 0                         ) ||
             (target_nid < Nodes->GetLNodesConfigMax()) )
    {
        nid = end_nid = target_nid;
    }
    else
    {
        nid = -1;
    }
    
    // build reply
    msg_->u.reply.type = ReplyType_NodeInfo;
    if ( nid != -1 )
    {
        if ( continuation )
        {   // This is the continuation of a previous request.  Find
            // the place in the list where we left off.
            if ( last_nid > -1 )
            {
                nid = (last_nid + 1);
                lnodesCount = nid;
            }
            else
            {
                nid = -1;
                pnid = (last_pnid + 1);
                pnodesCount = pnid;
            }
        }

        // Load the logical nodes first
        msg_->u.reply.u.node_info.num_nodes = Nodes->GetLNodesCount();
        msg_->u.reply.u.node_info.num_pnodes = Nodes->GetPNodesCount();
        msg_->u.reply.u.node_info.num_spares = Nodes->GetSNodesCount();
        msg_->u.reply.u.node_info.num_available_spares = Nodes->GetAvailableSNodesCount();
        if ( nid != -1 )
        {
            for (num_returned=0;
                 (nid < Nodes->GetLNodesConfigMax() && nid <= end_nid) && lnodesCount < Nodes->GetLNodesCount() ;
                 nid++)
            {
                lnode  = Nodes->GetLNode(nid);
                if ( lnode )
                {
                    lnodesCount++;
                    pnode = lnode->GetNode();
                    if ( pnode )
                    {
                        pnodesCount++;
                        msg_->u.reply.u.node_info.node[num_returned].nid = lnode->GetNid();
                        msg_->u.reply.u.node_info.node[num_returned].state = lnode->GetState();
                        msg_->u.reply.u.node_info.node[num_returned].type = lnode->GetZoneType();
                        msg_->u.reply.u.node_info.node[num_returned].processors = lnode->GetProcessors();
                        msg_->u.reply.u.node_info.node[num_returned].process_count = lnode->GetNumProcs();

                        msg_->u.reply.u.node_info.node[num_returned].pnid = pnid = pnode->GetPNid();
                        msg_->u.reply.u.node_info.node[num_returned].pstate = pnode->GetState();
                        msg_->u.reply.u.node_info.node[num_returned].spare_node = pnode->IsSpareNode();
                        if (( pnode->GetState() == State_Up       ) ||
                            ( pnode->GetState() == State_Shutdown )  )
                        {
                            msg_->u.reply.u.node_info.node[num_returned].memory_free = pnode->GetFreeMemory();
                            msg_->u.reply.u.node_info.node[num_returned].swap_free = pnode->GetFreeSwap();
                            msg_->u.reply.u.node_info.node[num_returned].cache_free = pnode->GetFreeCache();
                            msg_->u.reply.u.node_info.node[num_returned].cores = pnode->GetNumCores();
                            msg_->u.reply.u.node_info.node[num_returned].memory_total = pnode->GetMemTotal();
                            msg_->u.reply.u.node_info.node[num_returned].memory_active = pnode->GetMemActive();
                            msg_->u.reply.u.node_info.node[num_returned].memory_inactive = pnode->GetMemInactive();
                            msg_->u.reply.u.node_info.node[num_returned].memory_dirty = pnode->GetMemDirty();
                            msg_->u.reply.u.node_info.node[num_returned].memory_writeback = pnode->GetMemWriteback();
                            msg_->u.reply.u.node_info.node[num_returned].memory_VMallocUsed = pnode->GetMemVMallocUsed();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_user = lnode->GetCpuUser();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_nice = lnode->GetCpuNice();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_system = lnode->GetCpuSystem();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_idle = lnode->GetCpuIdle();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_iowait = lnode->GetCpuIowait();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_irq = lnode->GetCpuIrq();
                            msg_->u.reply.u.node_info.node[num_returned].cpu_soft_irq = lnode->GetCpuSoftIrq();
                            msg_->u.reply.u.node_info.node[num_returned].btime = pnode->GetBTime();
                        }
                        else
                        {
                            msg_->u.reply.u.node_info.node[num_returned].memory_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].swap_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cache_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cores = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_total = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_active = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_inactive = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_dirty = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_writeback = 0;
                            msg_->u.reply.u.node_info.node[num_returned].memory_VMallocUsed = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_user = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_nice = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_system = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_idle = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_iowait = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_irq = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cpu_soft_irq = 0;
                            msg_->u.reply.u.node_info.node[num_returned].btime = 0;
                        }
                        if( getenv("SQ_VIRTUAL_NODES") )
                        {
                            sprintf(msg_->u.reply.u.node_info.node[num_returned].node_name,"%s:%d", pnode->GetName(), pnid);
                        }
                        else
                        {
                            strcpy(msg_->u.reply.u.node_info.node[num_returned].node_name, pnode->GetName());
                        }

                        num_returned++;
                        if ( num_returned == MAX_NODE_LIST )
                        {
                            msg_->u.reply.u.node_info.last_nid = lnode->GetNid();
                            msg_->u.reply.u.node_info.last_pnid = -1;
                            break;
                        }
                    }
                    else
                    {
                        sprintf(la_buf, "[%s], Can't find node for Nid=%d\n", method_name, nid);
                        mon_log_write(MON_MONITOR_NODEINFO_1, SQ_LOG_ERR, la_buf);
                    }
                }
                else
                {
                    if (trace_settings & TRACE_REQUEST)
                    {
                        trace_printf( "%s@%d Can't find lnode, nid=%d\n"
                                    , method_name, __LINE__, nid);
                    }
                }
            }
        }
        if ( target_nid == -1 && (num_returned != MAX_NODE_LIST) )
        {
            if ( continuation )
            {   // This is the continuation of a previous request.  Find
                // the place in the list where we left off.
                if ( last_nid == -1 )
                {
                    pnid = (last_pnid + 1);
                    pnodesCount = pnid;
                }
                else
                {
                    pnid = 0;
                }
            }
    
            // Now the remainder of physical nodes
            for (; 
                 (pnid < Nodes->GetPNodesConfigMax() && pnid <= end_pnid) && pnodesCount < Nodes->GetPNodesCount();
                 pnid++)
            {
                pnode = Nodes->GetNode( pnid );
                if ( pnode )
                {
                    pnodesCount++;
                    if ( pnode->GetLNodesCount() == 0 )
                    {
                        msg_->u.reply.u.node_info.node[num_returned].nid = -1;
                        msg_->u.reply.u.node_info.node[num_returned].state = State_Unknown;
                        msg_->u.reply.u.node_info.node[num_returned].type = ZoneType_Undefined;
                        msg_->u.reply.u.node_info.node[num_returned].processors = 0;
                        msg_->u.reply.u.node_info.node[num_returned].process_count = 0;

                        msg_->u.reply.u.node_info.node[num_returned].pnid = pnode->GetPNid();
                        msg_->u.reply.u.node_info.node[num_returned].pstate = pnode->GetState();
                        msg_->u.reply.u.node_info.node[num_returned].spare_node = pnode->IsSpareNode();
                        if (( pnode->GetState() == State_Up       ) ||
                            ( pnode->GetState() == State_Shutdown )  )
                        {
                            msg_->u.reply.u.node_info.node[num_returned].memory_free = pnode->GetFreeMemory();
                            msg_->u.reply.u.node_info.node[num_returned].swap_free = pnode->GetFreeSwap();
                            msg_->u.reply.u.node_info.node[num_returned].cache_free = pnode->GetFreeCache();
                            msg_->u.reply.u.node_info.node[num_returned].cores = pnode->GetNumCores();
                        }
                        else
                        {
                            msg_->u.reply.u.node_info.node[num_returned].memory_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].swap_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cache_free = 0;
                            msg_->u.reply.u.node_info.node[num_returned].cores = 0;
                        }
                        if( getenv("SQ_VIRTUAL_NODES") )
                        {
                            sprintf(msg_->u.reply.u.node_info.node[num_returned].node_name,"%s:%d", pnode->GetName(), pnid);
                        }
                        else
                        {
                            strcpy(msg_->u.reply.u.node_info.node[num_returned].node_name, pnode->GetName());
                        }

                        num_returned++;
                        if ( num_returned == MAX_NODE_LIST )
                        {
                            msg_->u.reply.u.node_info.last_nid = -1;
                            msg_->u.reply.u.node_info.last_pnid = pnode->GetPNid();
                            break;
                        }
                    }
                }
                else
                {
                    if (trace_settings & TRACE_REQUEST)
                    {
                        trace_printf( "%s@%d Can't find node, pnid=%d\n"
                                    , method_name, __LINE__, pnid);
                    }
                }
            }
        }
        msg_->u.reply.u.node_info.num_returned = num_returned;
        if ( num_returned <= MAX_NODE_LIST )
        {
            msg_->u.reply.u.node_info.return_code = MPI_SUCCESS;
        }
        else
        {
            msg_->u.reply.u.node_info.return_code = MPI_ERR_TRUNCATE;
        }
    }
    else         
    {
        sprintf(la_buf, "[%s], Invalid Node ID!\n", method_name);
        mon_log_write(MON_MONITOR_NODEINFO_4, SQ_LOG_ERR, la_buf);
       
        msg_->u.reply.u.node_info.num_returned = 0;
        msg_->u.reply.u.node_info.return_code = MPI_ERR_NAME;
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
