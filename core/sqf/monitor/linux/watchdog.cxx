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
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <mpi.h>
#include <unistd.h>
#include <sys/time.h>
#include "msgdef.h"
#include "props.h"
#include "localio.h"
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"
#include "lock.h"
#include "monlogging.h"
#include "montrace.h"
#include "sdtimer.h"
#include "procmon.h"
#include "watchdog.h"

#include "SCMVersHelp.h"

long trace_settings = 0;
int  traceFileFb = 0;
bool traceOpen = false;
char traceFileName[MAX_PROCESS_PATH];
FILE *locio_trace_file = NULL;

bool IsRealCluster = true;
char MyPort[MPI_MAX_PORT_NAME] = {0};;
char *MyName;
char *dbgLine;
int MyRank = -1;
int MyPNID = -1;
int MyNid = -1;
int MyPid = -1;
int Event_id = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
int gv_ms_su_pid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connection port - not used
SB_Verif_Type  gv_ms_su_verif = -1;
Verifier_t  MyVerifier = -1;
int Timeout = 0;
bool genSnmpTrapEnabled = false;

class CWatchdog;

CMonLog *MonLog = NULL;
CWatchdog *Watchdog = NULL;
CProcessMonitor *ProcessMonitor = NULL;

DEFINE_EXTERN_COMP_DOVERS(sqwatchdog)
DEFINE_EXTERN_COMP_PRINTVERS(sqwatchdog)

CWatchdog::CWatchdog( void )
          :CLock()
          ,event_(Watchdog_Stop)
          ,nodeDown_(false)
{
    const char method_name[] = "CWatchdog::CWatchdog";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "WTCH", 4);

    TRACE_EXIT;
}

CWatchdog::~CWatchdog( void )
{
    const char method_name[] = "CWatchdog::~CWatchdog";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "wtch", 4);

    TRACE_EXIT;
}

void CWatchdog::ExpireSoftdogTimer( bool dontSleep )
{
    const char method_name[] = "CWatchdog::ExpireSoftdogTimer";
    TRACE_ENTRY;
    sdTimer_.SetState( CSdTimer::SDT_EXPIRE );
    sdTimer_.CLock::wakeOne();
    while ( ! dontSleep && sdTimer_.IsSoftdogEnabled() )
    {
        // wait until node down completes
        sleep(1);
    }
    
    TRACE_EXIT;
}

void CWatchdog::ShutdownSoftdogTimer( bool dontSleep )
{
    const char method_name[] = "CWatchdog::ShutdownSoftdogTimer";
    TRACE_ENTRY;
    sdTimer_.SetState( CSdTimer::SDT_SHUTDOWN );
    sdTimer_.CLock::wakeOne();
    while ( ! dontSleep && sdTimer_.IsSoftdogEnabled() )
    {
        // wait until node down completes
        sleep(1);
    }
    
    TRACE_EXIT;
}

WatchdogEvent_t CWatchdog::GetEvent( void ) 
{ 
    const char method_name[] = "CWatchdog::GetEvent";
    TRACE_ENTRY;
    CAutoLock autoLock(getLocker());
    WatchdogEvent_t event = event_;
    // Reset event to default value
    event_ = Watchdog_Refresh;
    TRACE_EXIT;
    return( event ); 
}

bool CWatchdog::IsNodeDown( void ) 
{ 
    const char method_name[] = "CWatchdog::IsNodeDown";
    TRACE_ENTRY;
    CAutoLock autoLock(getLocker());
    TRACE_EXIT;
    return( nodeDown_ );
}

void CWatchdog::ResetSoftdogTimer( void )
{
    const char method_name[] = "CWatchdog::ResetSoftdogTimer";
    TRACE_ENTRY;
    sdTimer_.SetState( CSdTimer::SDT_RESET );
    sdTimer_.CLock::wakeOne();
    TRACE_EXIT;
}

void CWatchdog::SetEvent( WatchdogEvent_t event ) 
{ 
    const char method_name[] = "CWatchdog::SetEvent";
    TRACE_ENTRY;
    CAutoLock autoLock(getLocker());
    event_ = event; 
    TRACE_EXIT;
}

void CWatchdog::SetNodeDown( void ) 
{ 
    const char method_name[] = "CWatchdog::SetNodeDown";
    TRACE_ENTRY;
    CAutoLock autoLock(getLocker());
    nodeDown_ = true; 
    TRACE_EXIT;
}

void CWatchdog::StartSoftdogTimer( void )
{
    const char method_name[] = "CWatchdog::StartSoftdogTimer";
    TRACE_ENTRY;
    ProcessMonitor->SetState( CProcessMonitor::PM_START );
    ProcessMonitor->CLock::wakeOne();
    sdTimer_.SetState( CSdTimer::SDT_START );
    sdTimer_.CLock::wakeOne();
    TRACE_EXIT;
}

void CWatchdog::StopSoftdogTimer( void )
{
    const char method_name[] = "CWatchdog::StopSoftdogTimer";
    TRACE_ENTRY;
    sdTimer_.SetState( CSdTimer::SDT_STOP );
    sdTimer_.CLock::wakeOne();
    ProcessMonitor->SetState( CProcessMonitor::PM_STOP );
    ProcessMonitor->CLock::wakeOne();
    TRACE_EXIT;
}

int CWatchdog::StartWorkerThreads( void )
{ 
    const char method_name[] = "CWatchdog::StartWorkerThreads";
    TRACE_ENTRY;

    int rc;

    // Create the Process Monitor thread
    rc = ProcessMonitor->StartWorker();
    if (rc != 0)
    {
        TRACE_EXIT;
        return( rc );
    }

    // Create the Softdog thread
    rc = sdTimer_.StartWorker();
    if (rc != 0)
    {
        TRACE_EXIT;
        return( rc );
    }

    TRACE_EXIT;
    return( rc );
}

int CWatchdog::StopWorkerThreads( void ) 
{ 
    const char method_name[] = "CWatchdog::StopWorkerThreads";
    TRACE_ENTRY;

    int rc;

    // Stop the Softdog thread
    rc = sdTimer_.ShutdownWork();
    if (rc != 0)
    {
        TRACE_EXIT;
        return( rc );
    }

    // Stop the Process Monitor thread
    rc = ProcessMonitor->ShutdownWork();
    if (rc != 0)
    {
        TRACE_EXIT;
        return( rc );
    }

    TRACE_EXIT;
    return( rc );
}

void CWatchdog::WaitForEvent( void ) 
{ 
    const char method_name[] = "CWatchdog::WaitForEvent";
    TRACE_ENTRY;
    CAutoLock autoLock(getLocker());
    wait();
    TRACE_EXIT;
}


void exit_process(void)
{
    const char method_name[] = "exit_process";
    TRACE_ENTRY;
    
    int count;
    MPI_Status status;
    struct message_def *msg;

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d Sending exit process message\n",
                      method_name, __LINE__ );
    }

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = MyNid;
    msg->u.request.u.exit.pid = MyPid;
    msg->u.request.u.exit.verifier = MyVerifier;
    msg->u.request.u.exit.process_name[0] = 0;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;


    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d exited process successfully. rc=%d\n",
                                  method_name, __LINE__, msg->u.reply.u.generic.return_code );
                }
            }
            else
            {
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d exit process failed, rc=%d\n",
                                  method_name, __LINE__, msg->u.reply.u.generic.return_code );
                }
            }
        }
        else
        {
            if (trace_settings & TRACE_INIT)
            {
                trace_printf( "%s@%d Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                              method_name, __LINE__, msg->type, msg->u.reply.type );
            }
        }
    }
    else
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d Invalid exit process reply\n",
                          method_name, __LINE__ );
        }
    }
    fflush (stdout);

    if (gp_local_mon_io)
    {
        gp_local_mon_io->release_msg(msg);
    }
    else
    {
        delete msg;
    }
    TRACE_EXIT;
}

void process_startup(int argc, char *argv[])
{
    const char method_name[] = "process_startup";
    TRACE_ENTRY;

    int i;
    struct message_def *msg;

    strcpy (MyName, argv[5]);
    MyNid = atoi(argv[3]);
    MyPid = atoi(argv[4]);
    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d %s process_startup, MyNid: %d.\n",
                      method_name, __LINE__, MyName, MyNid );
    }

    gp_local_mon_io->iv_pid = MyPid;
    gp_local_mon_io->init_comm();

    if (argc < 10)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s - process_startup], Error: Invalid startup arguments, argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s, argv[3]=%s, argv[4]=%s, argv[5]=%s, argv[6]=%s, argv[7]=%s, argv[8]=%s, argv[9]=%s\n"
                   , MyName, argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
        monproc_log_write(MON_WATCHDOG_PROCSTARTUP_1, SQ_LOG_ERR, buf);
        exit (1);
    }
    else
    {
        bool lv_done = false;
        while ( ! lv_done )
        {
            gp_local_mon_io->acquire_msg( &msg );
            if ( msg )
            {
                lv_done = true;
            }
            else
            {
                sleep( 5 );
            }
        }

        msg->type = MsgType_Service;
        msg->noreply = true;
        msg->u.request.type = ReqType_Startup;
        msg->u.request.u.startup.nid = MyNid;
        msg->u.request.u.startup.pid = MyPid;
        msg->u.request.u.startup.paired = false;
        strcpy (msg->u.request.u.startup.process_name, argv[5]);
        strcpy (msg->u.request.u.startup.port_name, MyPort);
        msg->u.request.u.startup.os_pid = getpid ();
        msg->u.request.u.startup.event_messages = true;
        msg->u.request.u.startup.system_messages = true;
        msg->u.request.u.startup.verifier = MyVerifier;
        msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);
        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d %s sending startup reply to monitor.\n",
                          method_name, __LINE__, MyName );
        }
        fflush (stdout);

        gp_local_mon_io->send( msg );


        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d %s Startup completed\n",
                          method_name, __LINE__, MyName );
            if (argc > 9)
            {
                for (i = 10; i < argc; i++)
                {
                    trace_printf( "%s@%d %s, argv[%d]=%s\n",
                                  method_name, __LINE__, MyName, i, argv[i] );
                }
            }
        }

        if (!gp_local_mon_io)
        {
            delete msg;
        }
    }
    TRACE_EXIT;
}

void localIORecvCallback(struct message_def *recv_msg, int size)
{
    const char method_name[] = "localIORecvCallback";
    TRACE_ENTRY;

    size = size; // Avoid "unused parameter" warning
    CAutoLock autoLock(Watchdog->getLocker());

    switch ( recv_msg->type )
    {
        case MsgType_Service:
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d CB Service Reply: Type=%d, ReplyType=%d\n",
                              method_name, __LINE__, recv_msg->type, recv_msg->u.reply.type );
            }
            break;
        
        case MsgType_Change:
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d CB Registry Change: Type=%d, ReplyType=%d\n",
                              method_name, __LINE__, recv_msg->type, recv_msg->u.reply.type );
            }
            break;

        case MsgType_Event:
            Event_id = recv_msg->u.request.u.event_notice.event_id;
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d CB Event - %d\n",
                              method_name, __LINE__, Event_id );
            }
            switch( Event_id )
            {
            case Watchdog_Start:
                Watchdog->SetEvent( Watchdog_Start );
                break;
            case Watchdog_Refresh:
                Watchdog->SetEvent( Watchdog_Refresh );
                break;
            case Watchdog_Expire:
                Watchdog->SetEvent( Watchdog_Expire );
                break;
            case Watchdog_Shutdown:
                Watchdog->SetEvent( Watchdog_Shutdown );
                break;
            case Watchdog_Stop:
                Watchdog->SetEvent( Watchdog_Stop );
                break;
            case Watchdog_Exit:
                Watchdog->SetEvent( Watchdog_Exit );
                break;
            default:
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf( "%s@%d CB Unknown event received: %d\n",
                                  method_name, __LINE__, Event_id );
                }
            }
            // Wake up waiting main()
            Watchdog->CLock::wakeOne();
            break;

        default:
            if (trace_settings & TRACE_REQUEST)
            {
                trace_printf( "%s@%d CB Notice: Type=%d, RequestType=%d\n",
                              method_name, __LINE__, recv_msg->type, recv_msg->u.reply.type );
            }
    }
    TRACE_EXIT;
}

void InitLocalIO( void )
{
    const char method_name[] = "InitLocalIO";
    TRACE_ENTRY;

    if ( MyPNID == -1 )
    {
        CClusterConfig  ClusterConfig; // Configuration objects
        CPNodeConfig   *pnodeConfig;
        CLNodeConfig   *lnodeConfig;

        if ( ClusterConfig.Initialize() )
        {
            if ( ! ClusterConfig.LoadConfig() )
            {
                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s - InitLocalIO], Error= Failed to load cluster configuration!\n", MyName);
                monproc_log_write(MON_WATCHDOG_INITLOCALIO_1, SQ_LOG_ERR, buf);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s - InitLocalIO], Warning: No cluster.conf found!\n", MyName);
            monproc_log_write(MON_WATCHDOG_INITLOCALIO_2, SQ_LOG_ERR, buf);
            if (MyNid == -1)
            {
                sprintf(buf, "[%s - InitLocalIO], Warning: set default virtual node ID = 0!\n", MyName);
                monproc_log_write(MON_WATCHDOG_INITLOCALIO_3, SQ_LOG_ERR, buf);
                MyNid = 0;
            }
            exit(EXIT_FAILURE);
        }

        lnodeConfig = ClusterConfig.GetLNodeConfig( MyNid );
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        MyPNID = pnodeConfig->GetPNid();
    }

    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
    TRACE_EXIT;
}

void TraceOpen ( void )
{
    // Initialize tracing
    trace_init(traceFileName,
               false,  // don't append pid to file name
               NULL,  // prefix
               false);
    if (traceFileFb > 0)
    {
        trace_set_mem(traceFileFb);
    }
    traceOpen = true;
}

void TraceUpdate ( int flags )
{
    if ( flags & 1 )
    {
        trace_settings |= TRACE_REQUEST;
    }
    if ( flags & 2 )
    {
        trace_settings |= TRACE_INIT;
    }
    if ( flags & 4 )
    {
        trace_settings |= TRACE_MLIO;
        Local_IO_To_Monitor::cv_trace = true;
        Local_IO_To_Monitor::cp_trace_cb = trace_where_vprintf;
    }
    if ( flags & 8 )
    {
        trace_settings |= TRACE_ENTRY_EXIT;
    }
}

void TraceInit( int & argc, char **&argv )
{
    // Determine trace file name
    const char *tmpDir;
    tmpDir = getenv( "TRAF_LOG" );
        
    const char *envVar;
    envVar = getenv("WDT_TRACE_FILE");
    if (envVar != NULL)
    {
        if ((strcmp(envVar, "STDOUT") == 0)
          || strcmp(envVar, "STDERR") == 0)
        {
            strcpy( traceFileName, envVar);
        }
        else if (tmpDir)
        {
            sprintf( traceFileName, "%s/%s.%d", tmpDir, envVar, getpid() );
        }
        else
        {
            sprintf( traceFileName, "./%s.%d", envVar, getpid() );
        }

    }
    else
    {   // No trace file name specified, use default name
        if (tmpDir)
        {
            sprintf( traceFileName, "%s/watchdog.trace.%d", tmpDir, getpid() );
        }
        else
        {
            sprintf( traceFileName, "./watchdog.trace.%d", getpid() );
        }
    }

    // Get trace settings from environment variables
    trace_settings = 0;
    envVar = getenv("WDT_TRACE_CMD");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_EVLOG_MSG;
        trace_settings |= TRACE_REQUEST;
    }

    envVar = getenv("WDT_TRACE_INIT");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_INIT;
        trace_settings |= TRACE_EVLOG_MSG;
    }

    envVar = getenv("WDT_TRACE_LIO");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_MLIO;
        Local_IO_To_Monitor::cv_trace = true;
        Local_IO_To_Monitor::cp_trace_cb = trace_where_vprintf;
    }

    envVar = getenv("WDT_TRACE_ENTRY_EXIT");
    if (envVar && atoi (envVar) != 0 )
    {
        trace_settings |= TRACE_ENTRY_EXIT;
    }

    // Get environment variable value for trace buffer size if specified
    envVar = getenv("WDT_TRACE_FILE_FB");
    if (envVar)
    {
        traceFileFb = atoi ( envVar );
    }

    // Check for trace flags specified on the command line.
    int flags;
    for (int i = 0; i < argc; i++)
    {
        if ( strcmp ( argv[i], "-t" ) == 0 && (i != argc-1) )
        {   // Trace flag setting specified on command line.
            flags = strtol(argv[i+1], NULL, 0);
            TraceUpdate ( flags );

            // Remove the trace flag arguments from the list of command
            // line arguments.
            for (int j=i, k=i+2; k < argc; j++, k++)
            {
                if ( trace_settings & TRACE_INIT)
                {
                    trace_printf( "TraceInit@%d setting argv[%d] = argv[%d]\n",
                                  __LINE__, j, k );
                }
                argv[j] = argv[k];
            }
            argc -= 2;
        }
    }

    if ( trace_settings & TRACE_INIT)
    {
        trace_printf( "TraceInit@%d traceFileName=%s, trace_settings=%lX, traceFileFb=%d\n",
                      __LINE__, traceFileName, trace_settings, traceFileFb );
    }

    // Initialize tracing if any trace flags set
    if ( trace_settings )
    {
        TraceOpen();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: main()
//
// Description:     Software watchdog process
//
///////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
    bool done = false;
    char la_buf[MON_STRING_BUF_SIZE];
    dbgLine = new char [1000];

    CALL_COMP_DOVERS(sqwatchdog, argc, argv);
    CALL_COMP_PRINTVERS(sqwatchdog)

    TraceInit ( argc, argv );

    char *cmd_buffer = getenv("MPI_TIME_DELAY");
    if ( cmd_buffer )
    {
        Timeout = atoi(cmd_buffer) * 1000;
    }
    if (Timeout <= 0)    
    {
        Timeout = 25000;
    }

    cmd_buffer = getenv("MON_SNMP_ENABLE");
    if ( cmd_buffer)
    {
        genSnmpTrapEnabled = true;
    }
    
    // Mask all allowed signals except SIGPROF
    sigset_t    mask;
    sigfillset( &mask);
    sigdelset( &mask, SIGPROF ); // allows profiling such as google profiler

    int rc = pthread_sigmask( SIG_SETMASK, &mask, NULL );
    if ( rc != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf( buf, "[%s - main], pthread_sigmask error=%d\n", MyName, rc );
        monproc_log_write( MON_WATCHDOG_MAIN_7, SQ_LOG_ERR, buf );
    }

    MyName = new char [MAX_PROCESS_PATH];
    strcpy( MyName, argv[5] );
    gv_ms_su_nid = MyPNID = atoi(argv[2]);
    MyNid = atoi(argv[3]);
    MyPid = atoi (argv[4]);
    gv_ms_su_verif  = MyVerifier = atoi(argv[9]);

    MonLog = new CMonLog( "log4cxx.monitor.wdg.config", "WDG", "alt.wdg", MyPNID, MyNid, MyPid, MyName  );

    Watchdog = new CWatchdog();
    ProcessMonitor = new CProcessMonitor();

    if ( ! gp_local_mon_io )
    {
        InitLocalIO();
        assert (gp_local_mon_io);
    }
    
    gv_ms_su_pid = MyPid;
    gv_ms_su_verif = MyVerifier;
    gp_local_mon_io->iv_pid = MyPid;
    gp_local_mon_io->iv_verifier = MyVerifier;

    gp_local_mon_io->set_cb(localIORecvCallback, "notice");
    gp_local_mon_io->set_cb(localIORecvCallback, "event");
    gp_local_mon_io->set_cb(localIORecvCallback, "recv");

    process_startup (argc, argv);
    
    if ( !Watchdog->StartWorkerThreads() )
    {
        do
        {
            Watchdog->WaitForEvent();
            switch ( Watchdog->GetEvent() )
            {
            case Watchdog_Start:
                Watchdog->StartSoftdogTimer();
                sprintf(la_buf, "[%s - main], Watchdog process timer started!\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_1, SQ_LOG_INFO, la_buf);
                break;
            case Watchdog_Refresh:
                Watchdog->ResetSoftdogTimer();
                break;
            case Watchdog_Expire:
                sprintf(la_buf, "[%s - main], Watchdog process timer expired!\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_2, SQ_LOG_INFO, la_buf);
                Watchdog->ExpireSoftdogTimer();
                done = true;
                break;
            case Watchdog_Shutdown:
                sprintf(la_buf, "[%s - main], Watchdog process timer shutdown!\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_3, SQ_LOG_INFO, la_buf);
                Watchdog->ShutdownSoftdogTimer();
                done = true;
                break;
            case Watchdog_Stop:
                sprintf(la_buf, "[%s - main], Watchdog process timer stopped!\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_4, SQ_LOG_INFO, la_buf);
                Watchdog->StopSoftdogTimer();
                break;
            case Watchdog_Exit:
                sprintf(la_buf, "[%s - main], Watchdog process is exiting!\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_5, SQ_LOG_INFO, la_buf);
                Watchdog->StopSoftdogTimer();
                done = true;
                break;
            default:
                sprintf(la_buf, "[%s - main], Received invalid watchdog event_id.\n"
                              , MyName);
                monproc_log_write(MON_WATCHDOG_MAIN_6, SQ_LOG_INFO, la_buf);
                abort();
            }
        }
        while (!done && !Watchdog->IsNodeDown() );    
    }
                
    fflush (stdout);

    if ( !Watchdog->IsNodeDown() )
    {
        exit_process ();
    }

    Watchdog->StopWorkerThreads();

    delete MonLog;
    delete ProcessMonitor;
    delete Watchdog;
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
