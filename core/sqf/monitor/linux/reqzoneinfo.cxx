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

CExtZoneInfoReq::CExtZoneInfoReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RQEW", 4);
}

CExtZoneInfoReq::~CExtZoneInfoReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rqew", 4);
}

void CExtZoneInfoReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d) (nid=%d))"
              , CReqQueue::svcReqType[reqType_], getId(), pid_
              , msg_->u.request.u.node_info.target_nid );
    requestString_.assign( strBuf );
}


void CExtZoneInfoReq::performRequest()
{
    const char method_name[] = "CExtZoneInfoReq::performRequest";
    TRACE_ENTRY;

    int nid;
    int end_nid;
    int num_returned = 0;
    bool continuation = msg_->u.request.u.zone_info.continuation;
    int zid;
    int target_nid = msg_->u.request.u.zone_info.target_nid;
    int target_zid = msg_->u.request.u.zone_info.target_zid;
    int lnodesCount = 0;
    CLNode *lnode = NULL;
    CNode *pnode = NULL;
    char la_buf[MON_STRING_BUF_SIZE];
    
    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_zoneinfo_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        if (continuation)
        {
            trace_printf("%s@%d request #%ld: ZoneInfo, nid=%d, zid=%d, "
                         "continue from nid=%d, pnid=%d\n",  method_name,
                          __LINE__, id_, target_nid, target_zid, 
                         msg_->u.request.u.zone_info.last_nid,
                         msg_->u.request.u.zone_info.last_pnid);
        }
        else
        {
            trace_printf("%s@%d request #%ld: ZoneInfo, nid=%d, zid=%d\n",
                         method_name, __LINE__, id_, target_nid, target_zid);
        }
    }

    // We need current scheduling data from all nodes.
    Replicator.SetSyncClusterData();

    // Check for nid or zid
    if ( msg_->u.request.u.zone_info.target_nid == -1 &&
         msg_->u.request.u.zone_info.target_zid == -1 )
    {
        nid = 0;
        end_nid = Nodes->GetLNodesConfigMax()-1;
    }
    else if ((msg_->u.request.u.zone_info.target_zid == -1 &&
              (msg_->u.request.u.zone_info.target_nid >= 0)) ||
             (msg_->u.request.u.zone_info.target_nid < 
              (Nodes->GetPNodesConfigMax() - Nodes->GetSNodesCount())) )
    {
        nid = 0;
        end_nid = Nodes->GetLNodesConfigMax()-1;
    }
    else if ((msg_->u.request.u.zone_info.target_nid == -1 &&
              (msg_->u.request.u.zone_info.target_zid >= 0)) ||
             (msg_->u.request.u.zone_info.target_zid < 
              (Nodes->GetPNodesConfigMax() - Nodes->GetSNodesCount())) )
    {
        nid = 0;
        end_nid = Nodes->GetLNodesConfigMax()-1;
    }
    else
    {
        nid = -1;
    }

    // build reply
    msg_->u.reply.type = ReplyType_ZoneInfo;
    if ( nid != -1 )
    {
        if ( continuation )
        {   // This is the continuation of a previous request.  Find
            // the place in the list where we left off.
            if ( msg_->u.request.u.zone_info.last_nid > -1 )
            {
                nid = (msg_->u.request.u.zone_info.last_nid + 1);
                lnodesCount = nid;
            }
            else
            {
                nid = -1;
            }
        }

        msg_->u.reply.u.zone_info.num_nodes = Nodes->GetLNodesCount();
        if ( nid != -1 )
        {
            for (num_returned=0; 
                 (nid < Nodes->GetLNodesConfigMax() && nid <= end_nid) && lnodesCount < Nodes->GetLNodesCount();
                 nid++)
            {
                lnode  = Nodes->GetLNode(nid);
                if ( lnode )
                {
                    lnodesCount++;
                    pnode = lnode->GetNode();
                    if ( pnode )
                    {
                        zid = pnode->GetZone();
                        if (((target_nid == -1 && target_zid == -1)  ||
                             (target_nid != -1 && nid == target_nid) ||
                             (target_zid != -1 && zid == target_zid)))
                        {
                            msg_->u.reply.u.zone_info.node[num_returned].nid = nid;
                            msg_->u.reply.u.zone_info.node[num_returned].zid = zid;
                            msg_->u.reply.u.zone_info.node[num_returned].pnid = pnode->GetPNid();
                            msg_->u.reply.u.zone_info.node[num_returned].pstate = pnode->GetState();
                            strcpy(msg_->u.reply.u.zone_info.node[num_returned].node_name, pnode->GetName());

                            num_returned++;
                            if ( target_nid != -1 && nid == target_nid )
                            {
                                break;
                            }
                            if ( target_zid != -1 && zid == target_zid )
                            {
                                break;
                            }
                        }
                        
                        if ( num_returned == MAX_NODE_LIST )
                        {
                            msg_->u.reply.u.zone_info.last_nid = lnode->GetNid();
                            msg_->u.reply.u.zone_info.last_pnid = pnode->GetPNid();
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
        msg_->u.reply.u.zone_info.num_returned = num_returned;
        if ( num_returned <= MAX_NODE_LIST )
        {
            msg_->u.reply.u.zone_info.return_code = MPI_SUCCESS;
        }
        else
        {
            msg_->u.reply.u.zone_info.return_code = MPI_ERR_TRUNCATE;
        }
    }
    else         
    {
        sprintf(la_buf, "[%s], Invalid Node ID!\n", method_name);
        mon_log_write(MON_MONITOR_NODEINFO_4, SQ_LOG_ERR, la_buf);
       
        msg_->u.reply.u.zone_info.num_returned = 0;
        msg_->u.reply.u.zone_info.return_code = MPI_ERR_NAME;
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
