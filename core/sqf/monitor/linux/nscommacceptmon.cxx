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

using namespace std;

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "nscommacceptmon.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"

extern CCommAcceptMon CommAcceptMon;
extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern int MyPNID;
extern char MyMon2NsPort[MPI_MAX_PORT_NAME];
extern char *ErrorMsg (int error_code);
extern const char *StateString( STATE state);
extern CommType_t CommType;

static void *mon2nsProcess(void *arg);

CCommAcceptMon::CCommAcceptMon()
           : accepting_(true)
           , shutdown_(false)
           , thread_id_(0)
{
    const char method_name[] = "CCommAcceptMon::CCommAcceptMon";
    TRACE_ENTRY;


    TRACE_EXIT;
}

CCommAcceptMon::~CCommAcceptMon()
{
    const char method_name[] = "CCommAcceptMon::~CCommAcceptMon";
    TRACE_ENTRY;

    TRACE_EXIT;
}


struct message_def *CCommAcceptMon::Notice( const char *msgText )
{
    struct message_def *msg;

    const char method_name[] = "CCluster::Notice";
    TRACE_ENTRY;
    
    msg = new struct message_def;
    msg->type = MsgType_ReintegrationError;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    strncpy( msg->u.request.u.reintegrate.msg, msgText,
             sizeof(msg->u.request.u.reintegrate.msg) );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d - Reintegrate notice %s\n",
                     method_name, __LINE__, msgText );

    TRACE_EXIT;

    return msg;
}

#if 0
// Send node names and port numbers for all existing monitors
// to the new monitor.
bool CCommAcceptMon::sendNodeInfoMPI( MPI_Comm interComm )
{
    const char method_name[] = "CCommAcceptMon::sendNodeInfoMPI";
    TRACE_ENTRY;
    bool sentData = true;

    int pnodeCount = Nodes->GetPNodesCount();

    nodeId_t *nodeInfo;
    nodeInfo = new nodeId_t[pnodeCount];
    int rc;

    CNode *node;

    for (int i=0; i<pnodeCount; ++i)
    {
        node = Nodes->GetNode( i );
        if ( node->GetState() == State_Up)
        {
            strncpy(nodeInfo[i].nodeName, node->GetName(),
                    sizeof(nodeInfo[i].nodeName));
            strncpy(nodeInfo[i].commPort, node->GetCommPort(),
                    sizeof(nodeInfo[i].commPort));

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Port for node %d (%s)\n"
                              "CommPort=%s\n"
                              "SyncPort=%s\n"
                            , method_name, __LINE__
                            , i, node->GetName()
                            , node->GetCommPort()
                            , node->GetSyncPort());
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d - No port for node %d (node not up)\n",
                             method_name, __LINE__, i);
            }

            nodeInfo[i].nodeName[0] = '\0';
            nodeInfo[i].commPort[0] = '\0';
            nodeInfo[i].syncPort[0] = '\0';
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
       trace_printf("%s@%d - Sending port info to new monitor\n", method_name,
                    __LINE__);
    }

    rc = Monitor->SendMPI((char *) nodeInfo, sizeof(nodeId_t)*pnodeCount, 0,
                       MON_XCHNG_DATA, interComm);
    if ( rc != MPI_SUCCESS )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], cannot send node/port info to "
                 " new monitor process: %s.\n"
                 , method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_1, SQ_LOG_ERR, buf); 

        sentData = false;
    }

    delete [] nodeInfo;

    TRACE_EXIT;

    return sentData;
}
#endif

// Send node names and port numbers for all existing monitors
// to the new monitor.
bool CCommAcceptMon::sendNodeInfoSock( int sockFd )
{
    const char method_name[] = "CCommAcceptMon::sendNodeInfoSock";
    TRACE_ENTRY;
    bool sentData = true;

    int pnodeCount = Nodes->GetPNodesCount();

    nodeId_t *nodeInfo;
    size_t nodeInfoSize = (sizeof(nodeId_t) * pnodeCount);
    nodeInfo = (nodeId_t *) new char[nodeInfoSize];
    int rc;

    CNode *node;

    for (int i=0; i<pnodeCount; ++i)
    {
        node = Nodes->GetNodeByMap( i );
        if ( node->GetState() == State_Up)
        {
            strncpy(nodeInfo[i].nodeName, node->GetName(),
                    sizeof(nodeInfo[i].nodeName));
            strncpy(nodeInfo[i].commPort, node->GetCommPort(),
                    sizeof(nodeInfo[i].commPort));
            strncpy(nodeInfo[i].syncPort, node->GetSyncPort(),
                    sizeof(nodeInfo[i].syncPort));
            nodeInfo[i].pnid = node->GetPNid();
            nodeInfo[i].creatorPNid = (nodeInfo[i].pnid == MyPNID) ? MyPNID : -1;

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Node info for pnid=%d (%s)\n"
                              "        CommPort=%s\n"
                              "        SyncPort=%s\n"
                              "        creatorPNid=%d\n"
                            , method_name, __LINE__
                            , nodeInfo[i].pnid
                            , nodeInfo[i].nodeName
                            , nodeInfo[i].commPort
                            , nodeInfo[i].syncPort
                            , nodeInfo[i].creatorPNid );
            }
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - No nodeInfo[%d] for pnid=%d (%s) node not up!\n"
                            , method_name, __LINE__
                            , i, node->GetPNid(), node->GetName());
            }

            nodeInfo[i].pnid = -1;
            nodeInfo[i].nodeName[0] = '\0';
            nodeInfo[i].commPort[0] = '\0';
            nodeInfo[i].syncPort[0] = '\0';
            nodeInfo[i].creatorPNid = -1;
        }
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Sending port info to new monitor\n"
                    , method_name, __LINE__);
        for (int i=0; i<pnodeCount; i++)
        {
            trace_printf( "Port info for pnid=%d\n"
                          "        nodeInfo[%d].nodeName=%s\n"
                          "        nodeInfo[%d].commPort=%s\n"
                          "        nodeInfo[%d].syncPort=%s\n"
                          "        nodeInfo[%d].creatorPNid=%d\n"
                        , nodeInfo[i].pnid
                        , i, nodeInfo[i].nodeName
                        , i, nodeInfo[i].commPort
                        , i, nodeInfo[i].syncPort
                        , i, nodeInfo[i].creatorPNid );
        }
    }

    rc = Monitor->SendSock( (char *) nodeInfo
                          , nodeInfoSize
                          , sockFd);
    if ( rc )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], cannot send node/port info to "
                 " new monitor process: %s.\n"
                 , method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_2, SQ_LOG_ERR, buf); 

        sentData = false;
    }

    delete [] nodeInfo;

    TRACE_EXIT;

    return sentData;
}

#if 1
void CCommAcceptMon::monReqDeleteProcess( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqDeleteProcess";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Received monitor request delete-process data.\n"
                      "        msg.del_process_ns.nid=%d\n"
                      "        msg.del_process_ns.pid=%d\n"
                      "        msg.del_process_ns.verifier=%d\n"
                      "        msg.del_process_ns.process_name=%s\n"
                      "        msg.del_process_ns.target_nid=%d\n"
                      "        msg.del_process_ns.target_pid=%d\n"
                      "        msg.del_process_ns.target_verifier=%d\n"
                      "        msg.del_process_ns.target_process_name=%s\n"
                    , method_name, __LINE__
                    , msg->u.request.u.del_process_ns.nid
                    , msg->u.request.u.del_process_ns.pid
                    , msg->u.request.u.del_process_ns.verifier
                    , msg->u.request.u.del_process_ns.process_name
                    , msg->u.request.u.del_process_ns.target_nid
                    , msg->u.request.u.del_process_ns.target_pid
                    , msg->u.request.u.del_process_ns.target_verifier
                    , msg->u.request.u.del_process_ns.target_process_name
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    int pid;
    CExternalReq * request;
    msgType = CExternalReq::NonStartupMsg;
    pid = msg->u.request.u.del_process_ns.pid;
    request = new CExtDelProcessNsReq(msgType, pid, sockFd, msg);
    monReqExec(request);

    TRACE_EXIT;
}
#endif

void CCommAcceptMon::monReqExec( CExternalReq * request )
{
    const char method_name[] = "CCommAcceptMon::monReqExec";
    TRACE_ENTRY;

    if ( trace_settings & TRACE_REQUEST_DETAIL )
    {
        request->populateRequestString();
        trace_printf("%s@%d request = %s\n", method_name, __LINE__, request->requestString());
    }
    request->validateObj();
    request->performRequest();
    delete request;

    TRACE_EXIT;
}

#if 1
void CCommAcceptMon::monReqProcessInfo( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqProcessInfo";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Received monitor request process-info data.\n"
                      "        msg.info.nid=%d\n"
                      "        msg.info.pid=%d\n"
                      "        msg.info.verifier=%d\n"
                      "        msg.info.target_nid=%d\n"
                      "        msg.info.target_pid=%d\n"
                      "        msg.info.target_verifier=%d\n"
                      "        msg.info.target_process_name=%s\n"
                      "        msg.info.target_process_pattern=%s\n"
                      "        msg.info.type=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.process_info.nid
                    , msg->u.request.u.process_info.pid
                    , msg->u.request.u.process_info.verifier
                    , msg->u.request.u.process_info.target_nid
                    , msg->u.request.u.process_info.target_pid
                    , msg->u.request.u.process_info.target_verifier
                    , msg->u.request.u.process_info.target_process_name
                    , msg->u.request.u.process_info.target_process_pattern
                    , msg->u.request.u.process_info.type
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    int pid;
    CExternalReq * request;
    msgType = CExternalReq::NonStartupMsg;
    pid = msg->u.request.u.process_info.pid;
    request = new CExtProcInfoReq(msgType, pid, sockFd, msg);
    monReqExec(request);

    TRACE_EXIT;
}
#endif

#if 1
void CCommAcceptMon::monReqProcessInfoCont( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqProcessInfoCont";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Received monitor request process-info-cont data.\n"
                      "        msg.info_cont.nid=%d\n"
                      "        msg.info_cont.pid=%d\n"
                      "        msg.info_cont.context[0].nid=%d\n"
                      "        msg.info_cont.context[0].pid=%d\n"
                      "        msg.info_cont.context[1].nid=%d\n"
                      "        msg.info_cont.context[1].pid=%d\n"
                      "        msg.info_cont.context[2].nid=%d\n"
                      "        msg.info_cont.context[2].pid=%d\n"
                      "        msg.info_cont.context[3].nid=%d\n"
                      "        msg.info_cont.context[3].pid=%d\n"
                      "        msg.info_cont.context[4].nid=%d\n"
                      "        msg.info_cont.context[5].pid=%d\n"
                      "        msg.info_cont.type=%d\n"
                      "        msg.info_cont.allNodes=%d\n"
                    , method_name, __LINE__
                    , msg->u.request.u.process_info_cont.nid
                    , msg->u.request.u.process_info_cont.pid
                    , msg->u.request.u.process_info_cont.context[0].nid
                    , msg->u.request.u.process_info_cont.context[0].pid
                    , msg->u.request.u.process_info_cont.context[1].nid
                    , msg->u.request.u.process_info_cont.context[1].pid
                    , msg->u.request.u.process_info_cont.context[2].nid
                    , msg->u.request.u.process_info_cont.context[2].pid
                    , msg->u.request.u.process_info_cont.context[3].nid
                    , msg->u.request.u.process_info_cont.context[3].pid
                    , msg->u.request.u.process_info_cont.context[4].nid
                    , msg->u.request.u.process_info_cont.context[4].pid
                    , msg->u.request.u.process_info_cont.type
                    , msg->u.request.u.process_info_cont.allNodes
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    int pid;
    CExternalReq * request;
    msgType = CExternalReq::NonStartupMsg;
    pid = msg->u.request.u.process_info_cont.pid;
    request = new CExtProcInfoContReq(msgType, pid, sockFd, msg);
    monReqExec(request);

    TRACE_EXIT;
}
#endif

#if 1
void CCommAcceptMon::monReqNewProcess( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqNewProcess";
    TRACE_ENTRY;
    if (trace_settings & (TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Received monitor request new-process data.\n"
                      "        msg.new_process_ns.parent_nid=%d\n"
                      "        msg.new_process_ns.parent_pid=%d\n"
                      "        msg.new_process_ns.parent_verifier=%d\n"
                      "        msg.new_process_ns.nid=%d\n"
                      "        msg._nsnew_process.pid=%d\n"
                      "        msg._nsnew_process.verifier=%d\n"
                      "        msg.new_process_ns.type=%d\n"
                      "        msg.new_process_ns.priority=%d\n"
                      "        msg.new_process_ns.process_name=%s\n"
                    , method_name, __LINE__
                    , msg->u.request.u.new_process_ns.parent_nid
                    , msg->u.request.u.new_process_ns.parent_pid
                    , msg->u.request.u.new_process_ns.parent_verifier
                    , msg->u.request.u.new_process_ns.nid
                    , msg->u.request.u.new_process_ns.pid
                    , msg->u.request.u.new_process_ns.verifier
                    , msg->u.request.u.new_process_ns.type
                    , msg->u.request.u.new_process_ns.priority
                    , msg->u.request.u.new_process_ns.process_name
                    );
    }

    CExternalReq::reqQueueMsg_t msgType;
    int pid = -1;
    CExternalReq * request;
    msgType = CExternalReq::NonStartupMsg;
    request = new CExtNewProcNsReq(msgType, pid, sockFd, msg);
    monReqExec(request);

    TRACE_EXIT;
}
#endif

#if 0
void CCommAcceptMon::processNewComm(MPI_Comm interComm)
{
    const char method_name[] = "CCommAcceptMon::processNewComm";
    TRACE_ENTRY;

    int rc;
    MPI_Comm intraComm;
    nodeId_t nodeId;

    mem_log_write(CMonLog::MON_CONNTONEWMON_2);

    MPI_Comm_set_errhandler( interComm, MPI_ERRORS_RETURN );

    // Get info about connecting monitor
    rc = Monitor->ReceiveMPI((char *) &nodeId, sizeof(nodeId_t),
                          MPI_ANY_SOURCE, MON_XCHNG_DATA, interComm);
    if ( rc != MPI_SUCCESS )
    {   // Handle error
        MPI_Comm_free( &interComm );

        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_3, SQ_LOG_ERR, buf);    
        return;
    }

    if ( nodeId.creator )
    {
        // Indicate that this node is the creator monitor for the node up
        // operation.
        MyNode->SetCreator( true
                          , nodeId.creatorShellPid
                          , nodeId.creatorShellVerifier );
    }
    
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Accepted connection from node %s, port=%s, "
                      "creator=%d, creatorShellPid=%d:%d\n"
                    , method_name, __LINE__
                    , nodeId.nodeName
                    , nodeId.commPort
                    , nodeId.creator
                    , nodeId.creatorShellPid
                    , nodeId.creatorShellVerifier );
    }

    CNode * node= Nodes->GetNode( nodeId.nodeName );
    int pnid = -1;
    if ( node != NULL )
    {   // Store port number for the node
        pnid = node->GetPNid();
        node->SetCommPort( nodeId.commPort );
        node->SetSyncPort( nodeId.syncPort );
    }
    else
    {
        MPI_Comm_free( &interComm );

        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], got connection from unknown "
                 "node %s. Ignoring it.\n", method_name, nodeId.nodeName);
        mon_log_write(MON_COMMACCEPT_4, SQ_LOG_ERR, buf);    

        return;
    }

    // Merge the inter-communicators obtained from the connect/accept
    // between this monitor and the connecting monitor.
    rc = MPI_Intercomm_merge( interComm, 0, &intraComm );
    if ( rc )
    {
        MPI_Comm_free( &interComm );

        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], Cannot merge intercomm: %s.\n",
                 method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_5, SQ_LOG_ERR, buf);

#if 0
        if ( MyNode->IsCreator() )
        {
            snprintf(buf, sizeof(buf), "Cannot merge intercomm for node %s: %s.\n",
                     nodeId.nodeName, ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
        }
#endif
        return;
    }

    MPI_Comm_set_errhandler( intraComm, MPI_ERRORS_RETURN );

    mem_log_write(CMonLog::MON_CONNTONEWMON_4, pnid);

    if ( MyNode->IsCreator() )
    {  // Send port and node info for existing nodes
        mem_log_write(CMonLog::MON_CONNTONEWMON_3, pnid);

        if ( !sendNodeInfoMPI( interComm ) )
        {   // Had problem communicating with new monitor
            MPI_Comm_free( &intraComm );
            MPI_Comm_free( &interComm );

#if 0
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "Cannot send node/port info to "
                     " node %s monitor: %s.\n", nodeId.nodeName, ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif
            return;
        }

        Monitor->SetJoinComm( interComm );

        Monitor->SetIntegratingPNid( pnid );

        Monitor->addNewComm( pnid, 1, intraComm );

        node->SetState( State_Merging ); 
    }
    else
    {   // No longer need inter-comm from "MPI_Comm_accept"

        Monitor->addNewComm( pnid, 1, intraComm );

        node->SetState( State_Merging ); 

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Sending ready flag to new monitor\n",
                          method_name, __LINE__);
        }

        // Tell connecting monitor that we are ready to integrate it.
        int readyFlag = 1;
        rc = Monitor->SendMPI((char *) &readyFlag, sizeof(readyFlag), 0,
                           MON_XCHNG_DATA, interComm);
        if ( rc != MPI_SUCCESS )
        {
            MPI_Comm_free( &interComm );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send connect "
                     "acknowledgement to new monitor: %s.\n", method_name,
                     ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_6, SQ_LOG_ERR, buf);    

#if 0
            if ( MyNode->IsCreator() )
            {
                snprintf(buf, sizeof(buf), "Cannot send connect acknowledgment "
                         "to new monitor: %s.\n", ErrorMsg(rc));
                SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                       , MyNode->GetCreatorVerifier()
                                                       , Notice( buf )
                                                       , NULL );
            }
#endif

            return;
        }

        MPI_Comm_free( &interComm );
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Connected to new monitor for node %d\n",
                      method_name, __LINE__, pnid );
    }

    // Ideally the following logic should be done in another thread
    // so this thread can post another accept without delay.  For
    // initial implementation simplicity this work is being done 
    // here for now
    if ( MyNode->IsCreator() )
    {
        mem_log_write(CMonLog::MON_CONNTONEWMON_5, pnid);

        // Get status from new monitor indicating whether
        // it is fully connected to other monitors.
        nodeStatus_t nodeStatus;
        rc = Monitor->ReceiveMPI((char *) &nodeStatus,
                              sizeof(nodeStatus_t),
                              MPI_ANY_SOURCE, MON_XCHNG_DATA,
                              interComm);
        if ( rc != MPI_SUCCESS )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain "
                     "node status from new monitor: %s.\n",
                     method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_7, SQ_LOG_ERR, buf);

#if 0
            snprintf(buf, sizeof(buf), "Unable to obtain node status from "
                     "node %s monitor: %s.\n", nodeId.nodeName, ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif

            node->SetState( State_Down );

            MPI_Comm_free ( &interComm );
            Monitor->ResetIntegratingPNid();
        }
        else
        {
            mem_log_write(CMonLog::MON_CONNTONEWMON_6, node->GetPNid(),
                          nodeStatus.state);

            if (nodeStatus.state == State_Up)
            {
                // communicate the change and handle it after sync
                // in ImAlive
                node->SetChangeState( true );
            }
            else
            {
#if 0
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "Node %s monitor failed to complete "
                         "initialization\n", nodeId.nodeName);
                SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                       , MyNode->GetCreatorVerifier()
                                                       , Notice( buf )
                                                       , NULL );
#endif
                node->SetState( State_Down ); 

                MPI_Comm_free ( &interComm );
                Monitor->ResetIntegratingPNid();
            }
        }
    }

    TRACE_EXIT;
}
#endif

#if 1
void CCommAcceptMon::processMonReqs( int sockFd )
{
    const char method_name[] = "CCommAcceptMon::processMonReqs";
    int rc;
    nodeId_t nodeId;
    struct message_def msg;

    TRACE_ENTRY;

    // Get info about connecting monitor
    rc = Monitor->ReceiveSock( (char *) &nodeId
                             , sizeof(nodeId_t)
                             , sockFd );
    if ( rc )
    {   // Handle error
        close( sockFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Accepted connection from pnid=%d\n"
                      "        nodeId.nodeName=%s\n"
                      "        nodeId.commPort=%s\n"
                      "        nodeId.syncPort=%s\n"
                      "        nodeId.creatorPNid=%d\n"
                      "        nodeId.creator=%d\n"
                      "        nodeId.creatorShellPid=%d\n"
                      "        nodeId.creatorShellVerifier=%d\n"
                      "        nodeId.ping=%d\n"
                    , method_name, __LINE__
                    , nodeId.pnid
                    , nodeId.nodeName
                    , nodeId.commPort
                    , nodeId.syncPort
                    , nodeId.creatorPNid
                    , nodeId.creator
                    , nodeId.creatorShellPid
                    , nodeId.creatorShellVerifier
                    , nodeId.ping );
    }

    while (true)
    {
        // Get monitor request (hdr)
        int size;
        rc = Monitor->ReceiveSock( (char *) &size, sizeof(size), sockFd );
        if ( rc )
        {   // Handle error
            close( sockFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                     "monitor: %s.\n", method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
            return;
        }

        rc = Monitor->ReceiveSock( (char *) &msg, size, sockFd );
        if ( rc )
        {   // Handle error
            close( sockFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                     "monitor: %s.\n", method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
            return;
        }
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Received monitor request hdr.\n"
                          "        msg.type=%d\n"
                          "        msg.noreply=%d\n"
                          "        msg.reply_tag=%d\n"
                          "        msg.u.request.type=%d\n"
                        , method_name, __LINE__
                        , msg.type
                        , msg.noreply
                        , msg.reply_tag
                        , msg.u.request.type
                        );
            switch (msg.u.request.type)
            {
            case ReqType_DelProcessNs:
                monReqDeleteProcess(&msg, sockFd);
                break;

            case ReqType_ProcessInfo:
                monReqProcessInfo(&msg, sockFd);
                break;

            case ReqType_ProcessInfoCont:
                monReqProcessInfoCont(&msg, sockFd);
                break;

            case ReqType_NewProcessNs:
                monReqNewProcess(&msg, sockFd);
                break;

            default:
                trace_printf( "%s@%d - Received monitor request UNKNOWN data.\n"
                            , method_name, __LINE__
                            );
            }
        }
    }


    TRACE_EXIT;
}
#endif

void CCommAcceptMon::processNewSock( int joinFd )
{
    const char method_name[] = "CCommAcceptMon::processNewSock";
    TRACE_ENTRY;

    int rc;
#if 0
    int integratingFd; 
    nodeId_t nodeId;
    CNode *node;
#endif

    mem_log_write(CMonLog::MON_CONNTONEWMON_2);

    pendingFd_ = joinFd;
    rc = pthread_create(&thread_id_, NULL, mon2nsProcess, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], thread create error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_18, SQ_LOG_ERR, buf);
    }

#if 0

    // Get info about connecting monitor
    rc = Monitor->ReceiveSock( (char *) &nodeId
                             , sizeof(nodeId_t)
                             , joinFd );
    if ( rc )
    {   // Handle error
        close( joinFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
        return;
    }

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Accepted connection from pnid=%d\n"
                      "        nodeId.nodeName=%s\n"
                      "        nodeId.commPort=%s\n"
                      "        nodeId.syncPort=%s\n"
                      "        nodeId.creatorPNid=%d\n"
                      "        nodeId.creator=%d\n"
                      "        nodeId.creatorShellPid=%d\n"
                      "        nodeId.creatorShellVerifier=%d\n"
                      "        nodeId.ping=%d\n"
                    , method_name, __LINE__
                    , nodeId.pnid
                    , nodeId.nodeName
                    , nodeId.commPort
                    , nodeId.syncPort
                    , nodeId.creatorPNid
                    , nodeId.creator
                    , nodeId.creatorShellPid
                    , nodeId.creatorShellVerifier
                    , nodeId.ping );
    }

    node= Nodes->GetNode( nodeId.nodeName );

    if ( nodeId.ping )
    {
        // Reply with my node info
        nodeId.pnid = MyPNID;
        strcpy(nodeId.nodeName, MyNode->GetName());
        strcpy(nodeId.commPort, MyNode->GetCommPort());
        strcpy(nodeId.syncPort, MyNode->GetSyncPort());
        nodeId.ping = true;
        nodeId.creatorPNid = -1;
        nodeId.creator = false;
        nodeId.creatorShellPid = -1;
        nodeId.creatorShellVerifier = -1;

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "Sending my nodeInfo.pnid=%d\n"
                          "        nodeInfo.nodeName=%s\n"
                          "        nodeInfo.commPort=%s\n"
                          "        nodeInfo.syncPort=%s\n"
                          "        nodeInfo.ping=%d\n"
                        , nodeId.pnid
                        , nodeId.nodeName
                        , nodeId.commPort
                        , nodeId.syncPort
                        , nodeId.ping );
        }
    
        rc = Monitor->SendSock( (char *) &nodeId
                              , sizeof(nodeId_t)
                              , joinFd );
        if ( rc )
        {
            close( joinFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Cannot send ping node info to node %s: (%s)\n"
                    , method_name, node?node->GetName():"", ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_19, SQ_LOG_ERR, buf);    
        }
        return;
    }
    
    if ( nodeId.creator )
    {
        // Indicate that this node is the creator monitor for the node up
        // operation.
        MyNode->SetCreator( true
                          , nodeId.creatorShellPid
                          , nodeId.creatorShellVerifier );
    }
    
    int pnid = -1;
    if ( node != NULL )
    {   // Store port numbers for the node
        char commPort[MPI_MAX_PORT_NAME];
        char syncPort[MPI_MAX_PORT_NAME];
        strncpy(commPort, nodeId.commPort, MPI_MAX_PORT_NAME);
        strncpy(syncPort, nodeId.syncPort, MPI_MAX_PORT_NAME);
        char *pch1;
        char *pch2;
        pnid = nodeId.pnid;

        node->SetCommPort( commPort );
        pch1 = strtok (commPort,":");
        pch1 = strtok (NULL,":");
        node->SetCommSocketPort( atoi(pch1) );

        node->SetSyncPort( syncPort );
        pch2 = strtok (syncPort,":");
        pch2 = strtok (NULL,":");
        node->SetSyncSocketPort( atoi(pch2) );

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Setting node %d (%s), commPort=%s(%d), syncPort=%s(%d)\n"
                        , method_name, __LINE__
                        , node->GetPNid()
                        , node->GetName()
                        , pch1, atoi(pch1)
                        , pch2, atoi(pch2) );
        }
    }
    else
    {
        close( joinFd );

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], got connection from unknown "
                  "node %d (%s). Ignoring it.\n"
                , method_name
                , nodeId.pnid
                , nodeId.nodeName);
        mon_log_write(MON_COMMACCEPT_9, SQ_LOG_ERR, buf);

        return;
    }

    // Sanity check, re-integrating node must be down
    if ( node->GetState() != State_Down )
    {
        int intdata = -1;
        rc = Monitor->SendSock( (char *) &intdata
                              , 0
                              , joinFd );

        close( joinFd );

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], got connection from node %s. "
                  "Node not down, node state=%s\n"
                , method_name, nodeId.nodeName
                , StateString(node->GetState()));
        mon_log_write(MON_COMMACCEPT_10, SQ_LOG_ERR, buf);

        return;
    }

    mem_log_write(CMonLog::MON_CONNTONEWMON_4, pnid);

    if ( MyNode->IsCreator() )
    {  // Send port and node info for existing nodes
        mem_log_write(CMonLog::MON_CONNTONEWMON_3, pnid);

        if ( !sendNodeInfoSock( joinFd ) )
        {   // Had problem communicating with new monitor
            close( joinFd );

#if 0
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "Cannot send node/port info to "
                     " node %s monitor: %s.\n", nodeId.nodeName, ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif
            return;
        }
    }
    else
    {   // No longer need joinFd

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Sending ready indication to new monitor\n",
                          method_name, __LINE__);
        }

        // Tell connecting monitor that we are ready to integrate it.
        int mypnid = MyPNID;
        rc = Monitor->SendSock( (char *) &mypnid
                              , sizeof(mypnid)
                              , joinFd );
        if ( rc )
        {
            close( joinFd );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send connect "
                     "acknowledgement to new monitor: %s.\n", method_name,
                     ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_11, SQ_LOG_ERR, buf);
            return;
        }

        // Connect to new monitor
        integratingFd = Monitor->MkCltSock( node->GetSyncPort() );
        Monitor->addNewSock( pnid, 1, integratingFd );

        node->SetState( State_Merging ); 

        close( joinFd );
    }

    if ( MyNode->IsCreator() )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Sending my pnid to new monitor\n",
                          method_name, __LINE__);
        }

        // Sanity check, tell integrating monitor my creator pnid
        int mypnid = MyPNID;
        rc = Monitor->SendSock( (char *) &mypnid
                              , sizeof(mypnid)
                              , joinFd );
        if ( rc )
        {
            close( joinFd );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send pnid "
                     "acknowledgement to new monitor: %s.\n", method_name,
                     ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_12, SQ_LOG_ERR, buf);

#if 0
            snprintf(buf, sizeof(buf), "Cannot send pnid acknowledgment "
                     "to new monitor: %s.\n", ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Wait for ok to connect to new monitor in node %d\n",
                          method_name, __LINE__, pnid );
        }

        // Get new monitor acknowledgement that creator can connect
        int newpnid = -1;
        rc = Monitor->ReceiveSock( (char *) &newpnid
                                 , sizeof(newpnid)
                                 , joinFd );
        if ( rc || newpnid != pnid )
        {
            close( joinFd );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send connect "
                  "acknowledgement to new monitor: %s.\n", method_name,
                  ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_13, SQ_LOG_ERR, buf);

#if 0
            snprintf(buf, sizeof(buf), "Cannot receive connect acknowledgment "
                  "to new monitor: %s.\n", ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                , MyNode->GetCreatorVerifier()
                                                , Notice( buf )
                                                , NULL );
#endif
            return;
        }

        Monitor->SetJoinSock( joinFd );

        Monitor->SetIntegratingPNid( pnid );

        // Connect to new monitor
        integratingFd = Monitor->MkCltSock( node->GetSyncPort() );
        Monitor->addNewSock( pnid, 1, integratingFd );

        node->SetState( State_Merging ); 

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Connected to new monitor in node %d\n",
                          method_name, __LINE__, pnid );
        }

        mem_log_write(CMonLog::MON_CONNTONEWMON_5, pnid);

        // Get status from new monitor indicating whether
        // it is fully connected to other monitors.
        nodeStatus_t nodeStatus;
        rc = Monitor->ReceiveSock( (char *) &nodeStatus
                                 , sizeof(nodeStatus_t)
                                 , joinFd);
        if ( rc != MPI_SUCCESS )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain "
                     "node status from new monitor: %s.\n",
                     method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_14, SQ_LOG_ERR, buf);

#if 0
            snprintf(buf, sizeof(buf), "Unable to obtain node status from "
                     "node %s monitor: %s.\n", nodeId.nodeName, ErrorMsg(rc));
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif

            node->SetState( State_Down );
            close( joinFd );
            Monitor->ResetIntegratingPNid();
            return;
        }

        mem_log_write(CMonLog::MON_CONNTONEWMON_6, node->GetPNid(),
                      nodeStatus.state);

        if (nodeStatus.state == State_Up)
        {
            // communicate the change and handle it after sync
            // in ImAlive
            node->SetChangeState( true );
        }
        else
        {
#if 0
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "Node %s monitor failed to complete "
                     "initialization\n", nodeId.nodeName);
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
            node->SetState( State_Down );
#endif
            close( joinFd );
            Monitor->ResetIntegratingPNid();
        }
    }
#endif

    TRACE_EXIT;
}

void CCommAcceptMon::commAcceptor()
{
    const char method_name[] = "CCommAcceptMon::commAcceptor";
    TRACE_ENTRY;

    switch( CommType )
    {
#if 0
        case CommType_InfiniBand:
            commAcceptorIB();
            break;
#endif
        case CommType_Sockets:
            commAcceptorSock();
            break;
        default:
            // Programmer bonehead!
            abort();
    }

    TRACE_EXIT;
    pthread_exit(0);
}

#if 0
// commAcceptor thread main processing loop.  Keep an MPI_Comm_accept
// request outstanding.  After accepting a connection process it.
void CCommAcceptMon::commAcceptorIB()
{
    const char method_name[] = "CCommAcceptMon::commAcceptorIB";
    TRACE_ENTRY;

    int rc;
    int errClass;
    MPI_Comm interComm;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d thread %lx starting\n", method_name,
                     __LINE__, thread_id_);
    }

    while (true)
    {
        if (isAccepting())
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d - Posting socket accept\n", method_name, __LINE__);
            }
    
            mem_log_write(CMonLog::MON_CONNTONEWMON_1);
            interComm = MPI_COMM_NULL;
            rc = MPI_Comm_accept( MyCommPort, MPI_INFO_NULL, 0, MPI_COMM_SELF,
                                  &interComm );
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d - Waiting to post accept\n", method_name, __LINE__);
            }
    
            CLock::lock();
            CLock::wait();
            CLock::unlock();
            if (!shutdown_)
            {
                continue; // Ok to accept another connection
            }
        }

        if (shutdown_)
        {   // We are being notified to exit.
            break;
        }

        if ( rc )
        {
            char buf[MON_STRING_BUF_SIZE];
            MPI_Error_class( rc, &errClass );
            snprintf(buf, sizeof(buf), "[%s], cannot accept remote monitor: %s.\n",
                     method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_15, SQ_LOG_ERR, buf);

        }
        else
        {
            processNewComm( interComm );
        }
    }

    if ( interComm != MPI_COMM_NULL ) MPI_Comm_free ( &interComm );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    TRACE_EXIT;
}
#endif

// commAcceptor thread main processing loop.  Keep an accept
// request outstanding.  After accepting a connection process it.
void CCommAcceptMon::commAcceptorSock()
{
    const char method_name[] = "CCommAcceptMon::commAcceptorSock";
    TRACE_ENTRY;

    int joinFd = -1;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d thread %lx starting\n", method_name,
                     __LINE__, thread_id_);
    }

    while (true)
    {
        if (isAccepting())
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d - Posting accept\n", method_name, __LINE__);
            }
    
            mem_log_write(CMonLog::MON_CONNTONEWMON_1);
            joinFd = Monitor->AcceptMon2NsSock();
        }
        else
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d - Waiting to post accept\n", method_name, __LINE__);
            }
    
            CLock::lock();
            CLock::wait();
            CLock::unlock();
            if (!shutdown_)
            {
                continue; // Ok to accept another connection
            }
        }
        
        if (shutdown_)
        {   // We are being notified to exit.
            break;
        }

        if ( joinFd < 0 )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], cannot accept new monitor: %s.\n",
                     method_name, strerror(errno));
            mon_log_write(MON_COMMACCEPT_16, SQ_LOG_ERR, buf);

        }
        else
        {
            processNewSock( joinFd );
        }
    }

    if ( !(joinFd < 0) ) close( joinFd );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    TRACE_EXIT;
}

void CCommAcceptMon::shutdownWork(void)
{
    const char method_name[] = "CCommAcceptMon::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the commAcceptor thread to exit
    shutdown_ = true;   
    Monitor->ConnectToSelf();
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d waiting for mon2nsAcceptMon thread %lx to exit.\n",
                     method_name, __LINE__, thread_id_);

    // Wait for commAcceptor thread to exit
    pthread_join(thread_id_, NULL);

    TRACE_EXIT;
}

// Initialize mon2nsAcceptor thread
static void *mon2nsAcceptMon(void *arg)
{
    const char method_name[] = "mon2nsAcceptMon";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAcceptMon object
    CCommAcceptMon *cao = (CCommAcceptMon *) arg;

    // Mask all allowed signals 
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_17, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    cao->commAcceptor();

    TRACE_EXIT;
    return NULL;
}

// Initialize mon2nsProcess thread
static void *mon2nsProcess(void *arg)
{
    const char method_name[] = "mon2nsProcess";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAcceptMon object
    CCommAcceptMon *cao = (CCommAcceptMon *) arg;

    // Mask all allowed signals 
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_17, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    cao->processMonReqs(cao->pendingFd_);

    TRACE_EXIT;
    return NULL;
}


// Create a commAcceptorMon thread
void CCommAcceptMon::start()
{
    const char method_name[] = "CCommAcceptMon::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, mon2nsAcceptMon, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], thread create error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_18, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CCommAcceptMon::startAccepting( void ) 
{
    const char method_name[] = "CCommAcceptMon::startAccepting";
    TRACE_ENTRY;

    CAutoLock lock( getLocker( ) );
    
    if ( !accepting_ )
    {
        accepting_ = true;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Enabling accepting_=%d\n"
                        , method_name, __LINE__, accepting_ );
        }
        CLock::wakeOne();
    }

    TRACE_EXIT;
}

void CCommAcceptMon::stopAccepting( void ) 
{
    const char method_name[] = "CCommAcceptMon::stopAccepting";
    TRACE_ENTRY;

    CAutoLock lock( getLocker( ) );
    
    if ( accepting_ )
    {
        accepting_ = false;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d - Disabling accepting_=%d\n"
                        , method_name, __LINE__, accepting_ );
        }
        CLock::wakeOne();
    }

    TRACE_EXIT;
}
