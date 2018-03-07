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
#include "nameserverconfig.h"

extern CMonStats *MonStats;
extern CNodeContainer *Nodes;
extern CReplicate Replicator;

CExtNameServerInfoReq::CExtNameServerInfoReq (reqQueueMsg_t msgType, int pid,
                                  struct message_def *msg )
    : CExternalReq(msgType, pid, msg)
{
    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "RqEF", 4);
}

CExtNameServerInfoReq::~CExtNameServerInfoReq()
{
    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "rQef", 4);
}

void CExtNameServerInfoReq::populateRequestString( void )
{
    char strBuf[MON_STRING_BUF_SIZE/2] = { 0 };

    snprintf( strBuf, sizeof(strBuf),
              "ExtReq(%s) req #=%ld requester(pid=%d)"
              , CReqQueue::svcReqType[reqType_], getId(), pid_ );
    requestString_.assign( strBuf );
}


void CExtNameServerInfoReq::performRequest()
{
    const char method_name[] = "CExtNameServerInfoReq::performRequest";
    TRACE_ENTRY;

    int num_returned = 0;
    CNameServerConfigContainer *nameServerConfig;
    CNameServerConfig *config;
    char la_buf[MON_STRING_BUF_SIZE];

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->req_type_nodeinfo_Incr();

    // Trace info about request
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d request #%ld: NameServerInfo\n", method_name,
                     __LINE__, id_);
    }

#if 0 // TODO
    // We need current scheduling data from all nodes.
    Replicator.SetSyncClusterData();
#endif

    // build reply
    msg_->u.reply.type = ReplyType_NameServerInfo;
    nameServerConfig = Nodes->GetNameServerConfig();
    if (nameServerConfig)
    {
        config = nameServerConfig->GetFirstConfig();
        for ( ; config; config = config->GetNext())
        {
            msg_->u.reply.u.nameserver_info.node[num_returned].state = State_Unknown;
            CNode *node = Nodes->GetNode( (char *) config->GetName() );
            if ( node )
            {
                char nsName[10];
                sprintf( nsName, "$ZNS%d", node->GetZone() );
                CProcess *process = Nodes->GetProcessByName( nsName, false );
                if ( process )
                    msg_->u.reply.u.nameserver_info.node[num_returned].state = process->GetState();
            }
            strcpy(msg_->u.reply.u.nameserver_info.node[num_returned].node_name, config->GetName());

            num_returned++;
            msg_->u.reply.u.nameserver_info.num_returned = num_returned;
            if ( num_returned <= MAX_NODE_LIST )
            {
                msg_->u.reply.u.nameserver_info.return_code = MPI_SUCCESS;
            }
            else
            {
                msg_->u.reply.u.nameserver_info.return_code = MPI_ERR_TRUNCATE;
            }
        }
    }
    else
    {
        sprintf(la_buf, "[%s], Failed to retrive NameServerConfig object!\n",
                method_name);
        mon_log_write(MON_REQ_NAMESERVER_INFO_1, SQ_LOG_CRIT, la_buf);

        msg_->u.reply.u.nameserver_info.return_code = MPI_ERR_INTERN;
    }

    // Send reply to requester
    lioreply(msg_, pid_);

    TRACE_EXIT;
}
