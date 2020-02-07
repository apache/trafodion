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

#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>

#include "redirector.h"
#include "ptpcommaccept.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"

#include "reqqueue.h"

extern CRedirector Redirector;
extern CReqQueue ReqQueue;
extern CPtpCommAccept *PtpCommAccept;
extern CMonitor *Monitor;
extern CNode *MyNode;
extern CNodeContainer *Nodes;
extern CRedirector Redirector;
extern bool IsRealCluster;
extern bool NameServerEnabled;
extern int MyPNID;
extern char MyPtPPort[MPI_MAX_PORT_NAME];
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern CommType_t CommType;

extern char *ErrorMsg (int error_code);
extern const char *StateString( STATE state);

static void *ptpProcess( void *arg );

CPtpCommAccept::CPtpCommAccept()
              : accepting_(true)
              , shutdown_(false)
              , ioWaitTimeout_(EPOLL_IO_WAIT_TIMEOUT_MSEC)
              , ioRetryCount_(EPOLL_IO_RETRY_COUNT)
              , ptpSock_(-1)
              , ptpSocketPort_(-1)
              , ptpPort_("")
              , thread_id_(0)
{
    const char method_name[] = "CPtpCommAccept::CPtpCommAccept";
    TRACE_ENTRY;

    // Use the EPOLL timeout and retry values
    char *ioWaitTimeoutEnv = getenv( "SQ_MON_EPOLL_WAIT_TIMEOUT" );
    if ( ioWaitTimeoutEnv )
    {
        // Timeout in seconds
        ioWaitTimeout_ = atoi( ioWaitTimeoutEnv );
        char *ioRetryCountEnv = getenv( "SQ_MON_EPOLL_RETRY_COUNT" );
        if ( ioRetryCountEnv )
        {
            ioRetryCount_ = atoi( ioRetryCountEnv );
        }
        if ( ioRetryCount_ > EPOLL_IO_RETRY_COUNT_MAX )
        {
            ioRetryCount_ = EPOLL_IO_RETRY_COUNT_MAX;
        }
    }

    int ptpPort = 0;
    int val = 0;
    unsigned char addr[4] = {0,0,0,0};
    struct hostent *he;

    he = gethostbyname( Node_name );
    if ( !he )
    {
        char ebuff[256];
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] gethostbyname(%s) error: %s\n"
                , method_name, __LINE__
                , Node_name, strerror_r( h_errno, ebuff, 256 ) );
        mon_log_write( PTP_COMMACCEPT_COMMACCEPT_1, SQ_LOG_CRIT, buf );

        mon_failure_exit();
    }
    memcpy( addr, he->h_addr, 4 );

    if (NameServerEnabled)
    {
        char *env = getenv("MON_P2P_COMM_PORT");
        if ( env )
        {
            val = atoi(env);
            if ( val > 0)
            {
                ptpPort = val;
            }
        }
        else
        {
           char buf[MON_STRING_BUF_SIZE];
           snprintf( buf, sizeof(buf)
                   , "[%s@%d] MON_P2P_COMM_PORT environment variable is not set!\n"
                   , method_name, __LINE__ );
           mon_log_write( PTP_COMMACCEPT_COMMACCEPT_2, SQ_LOG_CRIT, buf );

           mon_failure_exit();
        }
    
        // For virtual env, add PNid to the port so we can still test without collisions of port numbers
        if (!IsRealCluster)
        {
            ptpPort += MyNode->GetPNid();
        }
    
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d MON_P2P_COMM_PORT Node_name=%s, env=%s, ptpPort=%d, val=%d\n"
                        , method_name, __LINE__
                        , Node_name, env, ptpPort, val );
        }
    
        ptpSock_ = CComm::Listen( &ptpPort );
        if ( ptpSock_ < 0 )
        {
            char ebuff[MON_STRING_BUF_SIZE];
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Listen(MON_P2P_COMM_PORT=%d) error: %s\n"
                    , method_name, __LINE__, ptpPort
                    , strerror_r( errno, ebuff, MON_STRING_BUF_SIZE ) );
            mon_log_write( PTP_COMMACCEPT_COMMACCEPT_3, SQ_LOG_CRIT, buf );

            mon_failure_exit();
        }
        else
        {
            snprintf( MyPtPPort, sizeof(MyPtPPort)
                    , "%d.%d.%d.%d:%d"
                    , (int)((unsigned char *)addr)[0]
                    , (int)((unsigned char *)addr)[1]
                    , (int)((unsigned char *)addr)[2]
                    , (int)((unsigned char *)addr)[3]
                    , ptpPort );
            setPtPPort( MyPtPPort );
            setPtPSocketPort( ptpPort );
    
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d Initialized my ptp socket port, "
                              "pnid=%d (%s:%s) (ptpPort=%s)\n"
                            , method_name, __LINE__
                            , MyPNID
                            , MyNode->GetName()
                            , MyPtPPort
                            , getPtPPort() );
    
        }
    }

    TRACE_EXIT;
}

CPtpCommAccept::~CPtpCommAccept()
{
    const char method_name[] = "CPtpCommAccept::~CPtpCommAccept";
    TRACE_ENTRY;

    TRACE_EXIT;
}

void CPtpCommAccept::processNewSock( int sockFd )
{
    const char method_name[] = "CPtpCommAccept::processNewSock";
    TRACE_ENTRY;

    int rc;

    mem_log_write(CMonLog::MON_CONNTONEWMON_1);

    // need to create context in case back-to-back accept is too fast
    Context *ctx = new Context();
    ctx->this_ = this;
    ctx->pendingFd_ = sockFd;
    rc = pthread_create(&process_thread_id_, NULL, ptpProcess, ctx);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], ptpProcess thread create error=%d\n",
                 method_name, rc);
        mon_log_write(PTP_COMMACCEPT_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}

void CPtpCommAccept::processMonReqs( int sockFd )
{
    const char method_name[] = "CPtpCommAccept::processMonReqs";
    TRACE_ENTRY;

    int rc;
    struct internal_msg_def msg;

    while ( true )
    {
        mem_log_write(CMonLog::MON_CONNTONEWMON_2);
        ptpMsgInfo_t remoteInfo;
    
        // Get info about connecting monitor
        rc = CComm::Receive( sockFd
                           , (char *) &remoteInfo
                           , sizeof(ptpMsgInfo_t)
                           , (char *) "Remote monitor"
                           , method_name );
        if ( rc )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unable to obtain message size and pnid "
                         "from remote monitor: %s.\n", method_name, ErrorMsg(rc));
            mon_log_write(PTP_COMMACCEPT_2, SQ_LOG_ERR, buf);    
            return;
        }
    
        // Get info about connecting monitor
        rc = CComm::Receive( sockFd
                           , (char *) &msg
                           , remoteInfo.size
                           , (char *) "Remote monitor"
                           , method_name );
        if ( rc )
        {   // Handle error
            char buf[MON_STRING_BUF_SIZE];
            CNode *node = Nodes->GetNode(remoteInfo.pnid);
            snprintf( buf, sizeof(buf)
                    , "[%s], unable to obtain message size (%d) from remote "
                      "monitor %d(%s), error: %s.\n"
                    , method_name
                    , remoteInfo.size
                    , remoteInfo.pnid
                    , node ? node->GetName() : ""
                    , ErrorMsg(rc));
            mon_log_write(PTP_COMMACCEPT_3, SQ_LOG_ERR, buf);    
            return;
        }
        else
        {
            switch ( msg.type )
            {
                case InternalType_Clone:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Clone\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueCloneReq( &msg.u.clone );
                    break;
                }
                case InternalType_Exit:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Exit\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueExitReq( &msg.u.exit );
                    break;
                }
                case InternalType_IoData:
                {
                    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_IoData\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueIoDataReq( &msg.u.iodata );
                    break;
                }
                case InternalType_Kill:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Kill\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueKillReq( &msg.u.kill );
                    break;
                }
                case InternalType_Notify:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Notify\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueNotifyReq( &msg.u.notify );
                    break;
                }
                case InternalType_Open:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Open\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueOpenReq( &msg.u.open );
                    break;
                }
                case InternalType_Process:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_Process\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueNewProcReq( &msg.u.process);
                    break;
                }
                case InternalType_ProcessInit:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_ProcessInit\n"
                                    , method_name, __LINE__ );
                    }
                    if ( MyNode->IsMyNode(msg.u.processInit.origNid) )
                    {  // New process request originated on this node
                        ReqQueue.enqueueProcInitReq( &msg.u.processInit);
                    }
                    else
                    {
                        abort();
                    }
                    break;
                }
                case InternalType_StdinReq:
                {
                    if (trace_settings & (TRACE_REDIRECTION | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_StdinReq\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueStdInReq( &msg.u.stdin_req );
                    break;
                }
                case InternalType_UniqStr:
                {
                    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d" " - Received InternalType_UniqStr\n"
                                    , method_name, __LINE__ );
                    }
                    ReqQueue.enqueueUniqStrReq( &msg.u.uniqstr);
                    break;
                }
                default:
                {
                    char buf[MON_STRING_BUF_SIZE];
                    CNode *node = Nodes->GetNode(remoteInfo.pnid);
                    snprintf( buf, sizeof(buf)
                            , "[%s], Invalid msg.type: %d, msg size=%d, "
                              "remote monitor %d(%s)\n"
                            , method_name
                            , msg.type
                            , remoteInfo.size
                            , remoteInfo.pnid
                            , node ? node->GetName() : "" );
                    mon_log_write(PTP_COMMACCEPT_4, SQ_LOG_ERR, buf);    
                    abort();
                }
            }
        }
    }

    close( sockFd );

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
    
    int sockFd = -1;

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
            sockFd = CComm::Accept( ptpSock_ );
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

        if ( sockFd < 0 )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], cannot accept new monitor: %s.\n",
                     method_name, strerror(errno));
            mon_log_write(PTP_COMMACCEPT_5, SQ_LOG_ERR, buf);
        }
        else
        {
            processNewSock( sockFd );
            //close( sockFd );
        }
    }

    if ( !(sockFd < 0) ) close( sockFd );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        trace_printf("%s@%d thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    TRACE_EXIT;
}

void CPtpCommAccept::connectToCommSelf( void )
{
    const char method_name[] = "CPtpCommAccept::connectToCommSelf";
    TRACE_ENTRY;

    CComm::ConnectLocal( getPtPSocketPort() );

    TRACE_EXIT;
}

void CPtpCommAccept::shutdownWork(void)
{
    const char method_name[] = "CPtpCommAccept::shutdownWork";
    TRACE_ENTRY;

    // Set flag that tells the PtpCommAccept thread to exit
    shutdown_ = true;   
    connectToCommSelf();
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d waiting for PtpCommAccept thread %lx to exit.\n",
                     method_name, __LINE__, thread_id_);

    // Wait for PtpCommAcceptor thread to exit
    pthread_join(thread_id_, NULL);

    TRACE_EXIT;
}

// Initialize ptpCommAcceptor thread
static void *ptpCommAccept(void *arg)
{
    const char method_name[] = "ptpCommAccept";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CPtpCommAccept object
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
        mon_log_write(PTP_COMMACCEPT_6, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    cao->commAcceptor();

    TRACE_EXIT;
    return NULL;
}


// Initialize ptpProcess thread
static void *ptpProcess(void *arg)
{
    const char method_name[] = "ptpProcess";
    TRACE_ENTRY;

    // Parameter passed to the thread is an context
    CPtpCommAccept::Context *ctx = (CPtpCommAccept::Context *) arg;
    CPtpCommAccept *cao = ctx->this_;

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
        mon_log_write(PTP_COMMACCEPT_7, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    cao->processMonReqs(ctx->pendingFd_);
    delete ctx;

    TRACE_EXIT;
    return NULL;
}

// Create a ptpCommAccept thread
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
        mon_log_write(PTP_COMMACCEPT_8, SQ_LOG_ERR, buf);
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
