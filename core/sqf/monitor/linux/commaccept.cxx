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

using namespace std;

#include "commaccept.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"

#include <signal.h>
#include <unistd.h>

extern CCommAccept CommAccept;
extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern int MyPNID;
extern char MyCommPort[MPI_MAX_PORT_NAME];
extern char *ErrorMsg (int error_code);
extern const char *StateString( STATE state);
extern CommType_t CommType;
extern bool IsRealCluster;

CCommAccept::CCommAccept()
           : accepting_(true)
           , shutdown_(false)
           , thread_id_(0)
{
    const char method_name[] = "CCommAccept::CCommAccept";
    TRACE_ENTRY;


    TRACE_EXIT;
}

CCommAccept::~CCommAccept()
{
    const char method_name[] = "CCommAccept::~CCommAccept";
    TRACE_ENTRY;


    TRACE_EXIT;
}


struct message_def *CCommAccept::Notice( const char *msgText )
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

// Send node names and port numbers for all existing monitors
// to the new monitor.
bool CCommAccept::sendNodeInfoMPI( MPI_Comm interComm )
{
    const char method_name[] = "CCommAccept::sendNodeInfoMPI";
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

// Send node names and port numbers for all existing monitors
// to the new monitor.
bool CCommAccept::sendNodeInfoSock( int sockFd )
{
    const char method_name[] = "CCommAccept::sendNodeInfoSock";
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

            nodeInfo[i].nodeName[0] = '\0';
            nodeInfo[i].commPort[0] = '\0';
            nodeInfo[i].syncPort[0] = '\0';
            nodeInfo[i].pnid = -1;
            nodeInfo[i].creatorPNid = -1;
        }
        nodeInfo[i].creatorShellPid = -1;
        nodeInfo[i].creatorShellVerifier = -1;
        nodeInfo[i].creator = false;
        nodeInfo[i].ping = false;
        nodeInfo[i].nsPid = -1;
        nodeInfo[i].nsPNid = -1;
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
                          , sockFd
                          , method_name );
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

void CCommAccept::processNewComm(MPI_Comm interComm)
{
    const char method_name[] = "CCommAccept::processNewComm";
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

    CNode * node = Nodes->GetNode( nodeId.nodeName );
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

#ifndef NAMESERVER_PROCESS
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

#ifndef NAMESERVER_PROCESS
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

#ifndef NAMESERVER_PROCESS
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

#ifndef NAMESERVER_PROCESS
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
#ifndef NAMESERVER_PROCESS
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

void CCommAccept::processNewSock( int joinFd )
{
    const char method_name[] = "CCommAccept::processNewSock";
    TRACE_ENTRY;

    int rc;
    int integratingFd; 
    nodeId_t nodeId;
    CNode *node;

    mem_log_write(CMonLog::MON_CONNTONEWMON_2);

    // Get info about connecting monitor
    rc = Monitor->ReceiveSock( (char *) &nodeId
                             , sizeof(nodeId_t)
                             , joinFd
                             , method_name );
    if ( rc )
    {   // Handle error
        close( joinFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
        CommAccept.startAccepting();
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

#ifdef NAMESERVER_PROCESS
    if ( IsRealCluster )
        node = Nodes->GetNode( nodeId.nodeName );
    else
        node = Nodes->GetNode( nodeId.pnid );
#else
    node = Nodes->GetNode( nodeId.nodeName );
#endif

    if ( node == NULL )
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

        // Requests is complete, begin accepting connections again
        CommAccept.startAccepting();

        return;
    }

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
                              , joinFd
                              , method_name );
        if ( rc )
        {
            close( joinFd );
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Cannot send ping node info to node %s: (%s)\n"
                    , method_name, node?node->GetName():"", ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_19, SQ_LOG_ERR, buf);    
        }

        // Requests is complete, begin accepting connections again
        CommAccept.startAccepting();

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
    
    // Sanity check, re-integrating node must be down
    if ( node->GetState() != State_Down )
    {
        int intdata = -1;
        rc = Monitor->SendSock( (char *) &intdata
                              , 0
                              , joinFd
                              , method_name );

        close( joinFd );

        // This reply will terminate the other monitor
        node->SetState(State_Down);

        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], got connection from node %s (pnid=%d). "
                  "Node not down, node state=%s\n"
                , method_name, nodeId.nodeName, nodeId.pnid
                , StateString(node->GetState()));
        mon_log_write(MON_COMMACCEPT_10, SQ_LOG_ERR, buf);

        // Requests is complete, begin accepting connections again
        CommAccept.startAccepting();

        return;
    }

    int pnid = -1;

    // Store port numbers for the node
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

    mem_log_write(CMonLog::MON_CONNTONEWMON_4, pnid);

    if ( MyNode->IsCreator() )
    {  // Send port and node info for existing nodes
        mem_log_write(CMonLog::MON_CONNTONEWMON_3, pnid);

        if ( !sendNodeInfoSock( joinFd ) )
        {   // Had problem communicating with new monitor
            close( joinFd );

#ifndef NAMESERVER_PROCESS
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
                              , joinFd
                              , method_name );
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
                              , joinFd
                              , method_name );
        if ( rc )
        {
            close( joinFd );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send pnid "
                     "acknowledgement to new monitor: %s.\n", method_name,
                     ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_12, SQ_LOG_ERR, buf);

#ifndef NAMESERVER_PROCESS
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
                                 , joinFd
                                 , method_name );
        if ( rc || newpnid != pnid )
        {
            close( joinFd );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to send connect "
                  "acknowledgement to new monitor: %s.\n", method_name,
                  ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_13, SQ_LOG_ERR, buf);

#ifndef NAMESERVER_PROCESS
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
                                 , joinFd
                                 , method_name );
        if ( rc != MPI_SUCCESS )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain "
                     "node status from new monitor: %s.\n",
                     method_name, ErrorMsg(rc));
            mon_log_write(MON_COMMACCEPT_14, SQ_LOG_ERR, buf);

#ifndef NAMESERVER_PROCESS
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
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d - Received reintegrate status: state=%s, error=%d\n"
                            , method_name, __LINE__
                            , StateString(nodeStatus.state)
                            , nodeStatus.status );
            }
            // communicate the change and handle it after sync
            // in ImAlive
            node->SetChangeState( true );
        }
        else
        {
#ifndef NAMESERVER_PROCESS
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "Node %s monitor failed to complete "
                     "initialization\n", nodeId.nodeName);
            SQ_theLocalIOToClient->putOnNoticeQueue( MyNode->GetCreatorPid()
                                                   , MyNode->GetCreatorVerifier()
                                                   , Notice( buf )
                                                   , NULL );
#endif
            node->SetState( State_Down );
            close( joinFd );
            Monitor->ResetIntegratingPNid();
        }
    }

    TRACE_EXIT;
}

void CCommAccept::commAcceptor()
{
    const char method_name[] = "CCommAccept::commAcceptor";
    TRACE_ENTRY;

    switch( CommType )
    {
        case CommType_InfiniBand:
            commAcceptorIB();
            break;
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

// commAcceptor thread main processing loop.  Keep an MPI_Comm_accept
// request outstanding.  After accepting a connection process it.
void CCommAccept::commAcceptorIB()
{
    const char method_name[] = "CCommAccept::commAcceptorIB";
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
            // Stop accepting connections until this request completes
            CommAccept.stopAccepting();
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

// commAcceptor thread main processing loop.  Keep an accept
// request outstanding.  After accepting a connection process it.
void CCommAccept::commAcceptorSock()
{
    const char method_name[] = "CCommAccept::commAcceptorSock";
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
            joinFd = Monitor->AcceptCommSock();
            // Stop accepting connections until this request completes
            CommAccept.stopAccepting();
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

void CCommAccept::shutdownWork(void)
{
    const char method_name[] = "CCommAccept::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the commAcceptor thread to exit
    shutdown_ = true;   
    Monitor->ConnectToSelf();
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d waiting for commAccept thread %lx to exit.\n",
                     method_name, __LINE__, thread_id_);

    // Wait for commAcceptor thread to exit
    pthread_join(thread_id_, NULL);

    TRACE_EXIT;
}

// Initialize commAcceptor thread
static void *commAccept(void *arg)
{
    const char method_name[] = "commAccept";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAccept object
    CCommAccept *cao = (CCommAccept *) arg;

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


// Create a commAcceptor thread
void CCommAccept::start()
{
    const char method_name[] = "CCommAccept::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, commAccept, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], thread create error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_18, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CCommAccept::startAccepting( void ) 
{
    const char method_name[] = "CCommAccept::startAccepting";
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

void CCommAccept::stopAccepting( void ) 
{
    const char method_name[] = "CCommAccept::stopAccepting";
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
