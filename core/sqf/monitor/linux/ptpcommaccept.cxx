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

#include "ptpcommaccept.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"

#include "reqqueue.h"

extern CReqQueue ReqQueue;
extern CPtpCommAccept PtpCommAccept;
extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern int MyPNID;
extern char MyMon2MonPort[MPI_MAX_PORT_NAME];
extern char *ErrorMsg (int error_code);
extern const char *StateString( STATE state);
extern CommType_t CommType;


CPtpCommAccept::CPtpCommAccept()
           : accepting_(true)
           , shutdown_(false)
           , thread_id_(0)
{
    const char method_name[] = "CPtpCommAccept::CPtpCommAccept";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CPtpCommAccept::~CPtpCommAccept()
{
    const char method_name[] = "CPtpCommAccept::~CPtpCommAccept";
    TRACE_ENTRY;

    TRACE_EXIT;
}

// REMOVE
bool CPtpCommAccept::sendNodeInfoSock( int sockFd )
{
    const char method_name[] = "CPtpCommAccept::sendNodeInfoSock";
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

void CPtpCommAccept::processNewSock( int joinFd )
{
    const char method_name[] = "CPtpCommAccept::processNewSock";
    TRACE_ENTRY;
    
    struct internal_msg_def msg;
    nodeId_t nodeId;          
    int rc;
    
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
        return;
    }

/*
     printf( "TRK %s@%d - Accepted connection from pnid=%d\n"
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
  */                  
    mem_log_write(CMonLog::MON_CONNTONEWMON_2);
    int size;
    rc = Monitor->ReceiveSock( (char *) &size, sizeof(size), joinFd, method_name );

    if ( rc )
    {   // Handle error
        close( joinFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                     "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
        return;
    }
    // Get info about connecting monitor
    rc = Monitor->ReceiveSock( /*(char *) &nodeId*/
                              (char *) &msg
                             , size
                             , joinFd
                             , method_name );
                        
    if ( rc )
    {   // Handle error
        close( joinFd );
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], unable to obtain node id from new "
                 "monitor: %s.\n", method_name, ErrorMsg(rc));
        mon_log_write(MON_COMMACCEPT_8, SQ_LOG_ERR, buf);    
        return;
    }
    else
    {
        switch ( msg.type )
        {
            case InternalType_Process:
            {
               /*  printf("\nTRK Received ReqType_NewProcess \n");
                   ReqQueue.enqueueNewProcReq( &msg.u.process);
                   */
                 break;
            }
            case InternalType_ProcessInit:
            {
               /*  printf("\nTRK Received InternalType_ProcessInit \n");
                 ReqQueue.enqueueProcInitReq( &msg.u.processInit);
                */
                 break;
            }
            default:
            {

            }
        }
    }

    TRACE_EXIT;
}

void CPtpCommAccept::commAcceptor()
{
    const char method_name[] = "CPtpCommAccept::commAcceptor";
    TRACE_ENTRY;
    
    switch( CommType )
    {
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

// commAcceptor thread main processing loop.  Keep an accept
// request outstanding.  After accepting a connection process it.
void CPtpCommAccept::commAcceptorSock()
{
    const char method_name[] = "CPtpCommAccept::commAcceptorSock";
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
            joinFd = Monitor->AcceptMon2MonSock();
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

void CPtpCommAccept::shutdownWork(void)
{
    const char method_name[] = "CPtpCommAccept::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the PtpCommAccept thread to exit
    shutdown_ = true;   
    Monitor->ConnectToSelf();
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d waiting for PtpCommAccept thread %lx to exit.\n",
                     method_name, __LINE__, thread_id_);

    // Wait for PtpCommAcceptor thread to exit
    pthread_join(thread_id_, NULL);

    TRACE_EXIT;
}

// Initialize PtpCommAcceptor thread
static void *ptpCommAccept(void *arg)
{
    const char method_name[] = "ptpCommAccept";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CommAccept object
    CPtpCommAccept *cao = (CPtpCommAccept *) arg;

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
void CPtpCommAccept::start()
{
    const char method_name[] = "CPtpCommAccept::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, ptpCommAccept, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], thread create error=%d\n",
                 method_name, rc);
        mon_log_write(MON_COMMACCEPT_18, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CPtpCommAccept::startAccepting( void ) 
{
    const char method_name[] = "CPtpCommAccept::startAccepting";
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

void CPtpCommAccept::stopAccepting( void ) 
{
    const char method_name[] = "CPtpCommAccept::stopAccepting";
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

void CPtpCommAccept::monReqExec( void *req )
{
    const char method_name[] = "CPtpCommAcceptMon::monReqExec";
    TRACE_ENTRY;

    CExternalReq * request = (CExternalReq *)req;
    
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

//TRK-PTP-NEW
void CPtpCommAccept::monReqNewProcess( struct message_def* msg, int sockFd )
{
    const char method_name[] = "CCommAcceptMon::monReqNewProcess";
    TRACE_ENTRY;
    sockFd = sockFd; // appease compiler for the time being
    
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

    TRACE_EXIT;
}
