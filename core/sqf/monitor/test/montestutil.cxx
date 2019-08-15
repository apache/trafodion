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

// This file contains functions that are common to monitor test suite.

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "clio.h"
#include "clusterconf.h"
#include "localio.h"
#include "sqevlog/evl_sqlog_writer.h"

// turn all printfs to trace_printf
#ifdef TRACEPRINTF
#define printf trace_printf
#endif

#include "montestutil.h"
#include "xmpi.h"

extern FILE *shell_locio_trace_file;

const char *MessageTypeString( MSGTYPE type )
{
    const char *str = NULL;

    switch( type )
    {
        case MsgType_Change:
            str = "MsgType_Change";
            break;
        case MsgType_Close:
            str = "MsgType_Close";
            break;
        case MsgType_Event:
            str = "MsgType_Event";
            break;
        case MsgType_NodeAdded:
            str = "MsgType_NodeAdded";
            break;
        case MsgType_NodeDeleted:
            str = "MsgType_NodeDeleted";
            break;
        case MsgType_NodeDown:
            str = "MsgType_NodeDown";
            break;
        case MsgType_NodeJoining:
            str = "MsgType_NodeJoining";
            break;
        case MsgType_NodeQuiesce:
            str = "MsgType_NodeQuiesce";
            break;
        case MsgType_NodeUp:
            str = "MsgType_NodeUp";
            break;
        case MsgType_Open:
            str = "MsgType_Open";
            break;
        case MsgType_ProcessCreated:
            str = "MsgType_ProcessCreated";
            break;
        case MsgType_ProcessDeath:
            str = "MsgType_ProcessDeath";
            break;
        case MsgType_ReintegrationError:
            str = "MsgType_ReintegrationError";
            break;
        case MsgType_Shutdown:
            str = "MsgType_Shutdown";
            break;
        case MsgType_SpareUp:
            str = "MsgType_SpareUp";
            break;
        default:
            str = "MsgType - Undefined";
            break;
    }
    return( str );
}


const char *StateString( int state)
{
    const char *str;
    
    switch( state )
    {
        case State_Unknown:
            str = "State_Unknown";
            break;
        case State_Up:
            str = "State_Up";
            break;
        case State_Down:
            str = "State_Down";
            break;
        case State_Stopped:
            str = "State_Stopped";
            break;
        case State_Shutdown:
            str = "State_Shutdown";
            break;
        case State_Unlinked:
            str = "State_Unlinked";
            break;
        case State_Merging:
            str = "State_Merging";
            break;
        case State_Merged:
            str = "State_Merged";
            break;
        case State_Joining:
            str = "State_Joining";
            break;
        case State_Initializing:
            str = "State_Initializing";
            break;
        default:
            str = "State - Undefined";
            break;
    }

    return( str );
}

//////////////////////////////////////////////////////////////////////////////
// Monitor test utility
//////////////////////////////////////////////////////////////////////////////
void MonTestUtil::processArgs( int argc, char *argv[] )
{
    int i;
    shutdownBeforeStartup_ = false;
    nodedownBeforeStartup_ = false;
    trace_ = false;

    // enable tracing if trace flag supplied
    for (i=0; i<argc; i++)
    {
//        if ( strcmp(argv[i], "-t") == 0 ) gv_xmpi_trace = trace_ = true;
        if ( strcmp(argv[i], "-t") == 0 ) trace_ = true;
        if ( strcmp(argv[i], "-x") == 0 ) shutdownBeforeStartup_ = true;
        if ( strcmp(argv[i], "-y") == 0 ) nodedownBeforeStartup_ = true;
    }

    if ( trace_ )
    {
        printf( "[%s] MonTestUtil::processArgs processing arguments.\n",
                argv[5] );
    }

    if (argc < 7)
    {
        printf
            ("Error: Invalid startup arguments, argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s, argv[3]=%s, argv[4]=%s, argv[5]=%s, argv[6]=%s\n",
             argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
        exit (1);
    }

    if ( trace_ )
    {
        printf("[%s] - argc=%d", argv[5], argc);
        for (i=0; i<argc; i++)
        {
            printf (", argv[%d]=%s",i,argv[i]);
        }
        printf( "\n" );
    }

    nid_ = atoi(argv[3]);
    pid_ = atoi(argv[4]);
    gv_ms_su_verif = verifier_ = atoi(argv[9]);
    strcpy( processName_, argv[5] );
    strcpy (ga_ms_su_c_port, argv[6]);

    XMPI_Open_port( MPI_INFO_NULL, port_ );

    if ( trace_ )
    {
        printf( "[%s] port=%s\n", processName_, port_);
    }

    // Look for -n argument specifying the list of tests to run.
    // If found store the list of test numbers.
    bool testList = false;
    int num;
    char * ptr;
    for (i=0; i<argc; i++)
    {
        if ( strcmp(argv[i], "-n") == 0 )
        {
            testList = true;
        }
        else if (testList)
        {
            num = static_cast<int>(strtol(argv[i], &ptr, 10));
            if (ptr == argv[i] || errno != 0)
            {   // Encountered non-numeric argument
                break;
            }

            if ( trace_ )
            {
                printf("MonTestUtil::processArgs:  test #%d specified\n", num);
            }
            testNums_.push_back(num);
        }
    }

}

void MonTestUtil::requestShutdown ( ShutdownLevel level )
{
    int count;
    struct message_def *msg;
    MPI_Status status;

    if ( trace_ )
    {
        printf ("[%s] sending shutdown message.\n", processName_);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Shutdown;
    msg->u.request.u.shutdown.nid = nid_;
    msg->u.request.u.shutdown.pid = pid_;
    msg->u.request.u.shutdown.level = level;

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
                if ( trace_ )
                {
                    printf ("[%s] shutdown request succeeded. rc=%d\n",
                            processName_, msg->u.reply.u.generic.return_code);
                }
            }
            else
            {
                printf ("[%s] shutdown process failed, rc=%d\n", processName_,
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for shutdown message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] shutdown reply invalid.\n", processName_);
    }

    gp_local_mon_io->release_msg(msg);
}

void MonTestUtil::requestStartup ( )
{
    struct message_def *msg;

    if ( trace_ )
    {
        printf ("[%s] processing startup, my_nid: %d, lio: %p\n",
                processName_, nid_, (void *)gp_local_mon_io );
    }

    gp_local_mon_io->iv_pid = pid_;
    gp_local_mon_io->init_comm();

    gp_local_mon_io->acquire_msg( &msg );
    msg->type = MsgType_Service;
    msg->noreply = true;
    msg->u.request.type = ReqType_Startup;
    msg->u.request.u.startup.nid = nid_;
    msg->u.request.u.startup.pid = pid_;
    msg->u.request.u.startup.paired = false;
    strcpy (msg->u.request.u.startup.process_name, processName_);
    strcpy (msg->u.request.u.startup.port_name, port_);
    msg->u.request.u.startup.os_pid = getpid ();
    msg->u.request.u.startup.event_messages = true;
    msg->u.request.u.startup.system_messages = true;
    msg->u.request.u.startup.verifier = verifier_;
    msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);

    if ( trace_ )
    {
        printf ("[%s] sending startup reply to monitor.\n", processName_);
    }

    gp_local_mon_io->send( msg );

    if ( trace_ )
    {
        printf ("[%s] Startup completed\n", processName_);
    }
}

void MonTestUtil::flush_incoming_msgs( bool *TestShutdown )
{
    int count;

    int complete = 0;
    bool done = false;
    MPI_Status status;
    struct message_def *msg = NULL;

    if ( trace_ )
    {
        printf( "[%s] flush incoming event & notices.\n", processName_ );
    }
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if ( msg )
        {
            printf( "[%s] Got local IO notice\n",processName_ );
            complete = true;
            count = sizeof( *msg );
            status.MPI_TAG = msg->reply_tag;
        }
        else
        {
            if ( trace_ )
            {
                printf( "[%s] No local IO notice\n",processName_ );
            }
            complete = false;
            done = true;
        }

        if ( complete )
        {
            MPI_Get_count( &status, MPI_CHAR, &count );
            if ( ((status.MPI_TAG == NOTICE_TAG) ||
                (status.MPI_TAG == EVENT_TAG )      ) &&
                (count == sizeof (struct message_def)) )
            {
                if ( msg->u.request.type == ReqType_Notice )
                {
                    switch ( msg->type )
                    {
                    case MsgType_ProcessDeath:
                        if ( msg->u.request.u.death.aborted )
                        {
                            printf( "[%s] Process %s abnormally terminated. Nid=%d, Pid=%d\n",
                                    processName_, msg->u.request.u.death.process_name,
                                    msg->u.request.u.death.nid, msg->u.request.u.death.pid );
                        }
                        else
                        {
                            printf( "[%s] Process %s terminated normally. Nid=%d, Pid=%d\n",
                                    processName_, msg->u.request.u.death.process_name,
                                    msg->u.request.u.death.nid, msg->u.request.u.death.pid );
                        }
                        break;

                    case MsgType_NodeDown:
                        printf( "[%s] Node %d (%s) is DOWN\n",
                                processName_, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        printf( "[%s] Node %d (%s) is UP\n",
                                processName_, msg->u.request.u.up.nid, msg->u.request.u.up.node_name );
                        break;

                    case MsgType_Change:
                        printf( "[%s] Configuration Change Notice for Group: %s Key: %s\n",
                                processName_,
                                msg->u.request.u.change.group,
                                msg->u.request.u.change.key );
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        printf( "[%s] Open/Close process notification\n", processName_ );
                        break;

                    case MsgType_Event:
                        printf( "[%s] Event %d received\n",
                                processName_, msg->u.request.u.event_notice.event_id );
                        break;

                    case MsgType_Shutdown:
                        if (TestShutdown != NULL)
                          *TestShutdown = true;
                        printf( "[%s] Shutdown notice, level=%d received\n",
                                processName_, msg->u.request.u.shutdown.level );
                        break;

                    default:
                        printf( "[%s] Invalid Notice Type(%d) for flush message\n",
                                processName_, msg->type );
                    }
                }
                else
                {
                    printf( "[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                            processName_, msg->type, msg->noreply, msg->u.request.type );
                }
            }
            else
            {
                printf( "[%s] Failed to flush messages, status.MPI_TAG=%d, count=%d (expecting count=%d)\n", processName_, status.MPI_TAG, count, (int) sizeof (struct message_def) );
                done = true;
            }
            fflush( stdout );
        }
        if ( msg ) delete msg;
        msg = NULL;
    }
    while ( !done );
}

void MonTestUtil::requestExit ( void )
{
    int count;
    MPI_Status status;
    struct message_def *save_msg;
    struct message_def *msg;
    bool TestShutdown = false;

    if ( trace_ )
    {
        printf ("[%s] sending exit process message.\n", processName_);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = nid_;
    msg->u.request.u.exit.pid = pid_;
    msg->u.request.u.exit.verifier = verifier_;
    strcpy (msg->u.request.u.exit.process_name, processName_);

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
                if ( trace_ )
                {
                    printf ("[%s] exited process successfully. rc=%d\n",
                            processName_, msg->u.reply.u.generic.return_code);
                }
            }
            else
            {
                printf ("[%s] exit process failed, rc=%d\n", processName_,
                        msg->u.reply.u.generic.return_code);
            }
            save_msg = msg;
            flush_incoming_msgs( &TestShutdown );
            msg = save_msg;
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] exit process reply invalid.\n", processName_);
    }

    gp_local_mon_io->release_msg(msg);
}

void shell_locio_trace(const char *where, const char *format, va_list ap)
{
    if (shell_locio_trace_file != NULL)
    {
        int             ms;
        int             us;
        struct timeval  t;
        struct tm       tx;
        char            buf[BUFSIZ];
        gettimeofday(&t, NULL);
        ms = (int) t.tv_usec / 1000;
        us = (int) t.tv_usec - ms * 1000;

        sprintf(buf, "%02d:%02d:%02d.%03d.%03d %s: (%lx)",
                tx.tm_hour, tx.tm_min, tx.tm_sec, ms, us, where,
                pthread_self());
        vsprintf(&buf[strlen(buf)], format, ap);
        fprintf(shell_locio_trace_file, buf);
        fflush(shell_locio_trace_file);
    }
}

void MonTestUtil::InitLocalIO( int MyPNid )
{
    char *cmd_buffer;

    if ( MyPNid == -1 )
    {
        CClusterConfig  ClusterConfig; // 'cluster.conf' objects
        CPNodeConfig   *pnodeConfig;
        CLNodeConfig   *lnodeConfig;

        if ( ClusterConfig.Initialize() )
        {
            if ( ! ClusterConfig.LoadConfig() )
            {
                printf("[%s], Failed to load cluster configuration.\n", (char *)processName_);
                abort();
            }
        }
        else
        {
            printf( "[%s] Warning: No cluster.conf found\n",processName_);
            if (nid_ == -1)
            {
                printf( "[%s] Warning: set default virtual node ID = 0\n",processName_);
                nid_ = 0;
            }
            abort();
        }

        lnodeConfig = ClusterConfig.GetLNodeConfig( nid_ );
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        gv_ms_su_nid = MyPNid = pnodeConfig->GetPNid();
        if ( trace_ )
        {
            printf ("[%s] Local IO pnid = %d\n", processName_, gv_ms_su_nid);
        }
    }

    gp_local_mon_io = new Local_IO_To_Monitor(-1);
    cmd_buffer = getenv("SQ_LOCAL_IO_SHELL_TRACE");
    if (cmd_buffer && *cmd_buffer == '1')
    {
        gp_local_mon_io->cv_trace = true;

        char tracefile[MAX_SEARCH_PATH];
        char *tmpDir;

        tmpDir = getenv( "TRAF_LOG" );
        if (tmpDir)
        {
            sprintf( tracefile, "%s/shell.trace.%d", tmpDir, getpid() );
        }
        else
        {
            sprintf( tracefile, "./shell.trace.%d", getpid() );
        }

        shell_locio_trace_file = fopen(tracefile, "w+");
        gp_local_mon_io->cp_trace_cb = shell_locio_trace;
    }
}


int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{

    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    printf("%s", pp_string );

    return lv_err;
}

bool MonTestUtil::requestOpen( const char *openProcessName
                             , Verifier_t openProcessVerifier
                             , int deathNotice
                             , char *port)
{
    int count;
    struct message_def *msg;
    MPI_Status status;
    bool result = false;

    if ( trace_ )
    {
        printf( "[%s] opening process %s:%d.\n"
              , processName_
              , openProcessName
              , openProcessVerifier);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Open;
    msg->u.request.u.open.nid = nid_;
    msg->u.request.u.open.pid = pid_;
    msg->u.request.u.open.verifier = verifier_;
    strcpy (msg->u.request.u.open.process_name, processName_);
    msg->u.request.u.open.target_nid = -1;
    msg->u.request.u.open.target_pid = -1;
    msg->u.request.u.open.target_verifier = openProcessVerifier;
    strcpy (msg->u.request.u.open.target_process_name, openProcessName);
    msg->u.request.u.open.death_notification = deathNotice;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;
    if ( trace_ )
    {
        printf( "[%s] opening request completed %s:%d.\n"
              , processName_
              , openProcessName
              , openProcessVerifier);
    }

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Open))
        {
            if (msg->u.reply.u.open.return_code == MPI_SUCCESS)
            {
                if ( trace_ )
                {
                    printf("[%s] opened process %s successfully. Nid=%d, Pid=%d, "
                           "Verifier=%d, "
                           "Port=%s, rtn=%d\n",
                           processName_, openProcessName,
                           msg->u.reply.u.open.nid,
                           msg->u.reply.u.open.pid,
                           msg->u.reply.u.open.verifier,
                           msg->u.reply.u.open.port,
                           msg->u.reply.u.open.return_code);
                }
                strcpy (port, msg->u.reply.u.open.port);
                result = true;
            }
            else
            {
                printf ("[%s] open process %s:%d failed, rc=%d\n",
                        processName_, openProcessName, openProcessVerifier,
                        msg->u.reply.u.open.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for open message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] open process reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}

bool MonTestUtil:: requestClose( const char *closeProcessName
                               , Verifier_t closeProcessVerifier )
{
    int count;
    struct message_def *msg;
    MPI_Status status;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] closing server %s.\n", processName_, closeProcessName);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Close;
    msg->u.request.u.close.nid = nid_;
    msg->u.request.u.close.pid = pid_;
    msg->u.request.u.close.verifier = closeProcessVerifier;
    strcpy (msg->u.request.u.close.process_name, closeProcessName);

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
   {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS
             || msg->u.reply.u.generic.return_code == MPI_ERR_NAME)
            {
                if ( trace_ )
                {
                    printf("[%s] closed process %s successfully. Nid=%d, "
                           "Pid=%d, rtn=%d\n",
                           processName_, closeProcessName,
                           msg->u.reply.u.generic.nid,
                           msg->u.reply.u.generic.pid,
                           msg->u.reply.u.generic.return_code);
                }
                result = true;
            }
            else
            {
                printf ("[%s] close process %s failed, rc=%d\n",
                        processName_, closeProcessName,
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for close message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] close process %s reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", processName_, closeProcessName,
                msg->reply_tag, count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}

void get_server_death (char *my_name)
{
    int count;
    MPI_Status status;

    printf ("[%s] waiting for death message.\n", my_name);
    fflush (stdout);

    struct message_def *msg = NULL;
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if (msg)
        {
            count = sizeof (struct message_def);
            status.MPI_TAG = NOTICE_TAG;
        }
        usleep(1000);
    }
    while (!msg);

    if ((status.MPI_TAG == NOTICE_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_ProcessDeath) &&
            (msg->u.request.type == ReqType_Notice))
        {
            printf ("[%s] process death successfully. "
                    "Name=%s, Nid=%d, Pid=%d, Verifier=%d\n",
                    my_name,
                    msg->u.request.u.death.process_name,
                    msg->u.request.u.death.nid,
                    msg->u.request.u.death.pid,
                    msg->u.request.u.death.verifier);
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessDeath message\n",
                 my_name, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Invalid process death message\n", my_name);
    }
    fflush (stdout);
    delete msg;
}

void get_shutdown (char *my_name)
{
    int count;
    MPI_Status status;

    printf ("[%s] Waiting for shutdown message.\n", my_name);
    fflush (stdout);

    struct message_def *msg = NULL;
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if (msg)
        {
            count = sizeof (struct message_def);
            status.MPI_TAG = NOTICE_TAG;
        }
        sleep(1);
    }
    while (!msg);

    if ((status.MPI_TAG == NOTICE_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Shutdown) &&
            (msg->u.request.type == ReqType_Notice))
        {
            printf ("[%s] Shutdown received, level=%d\n",
                    my_name,
                    msg->u.request.u.shutdown.level);
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Shutdown message\n",
                 my_name, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Invalid shutdown message\n", my_name);
    }
    fflush (stdout);
    delete msg;
}

bool MonTestUtil::requestProcInfo( const char *processName
                                 , int &nid
                                 , int &pid
                                 , Verifier_t &verifier )
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = nid_;
    msg->u.request.u.process_info.pid = pid_;
    msg->u.request.u.process_info.verifier = verifier_;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    strcpy(msg->u.request.u.process_info.target_process_name, processName);
    msg->u.request.u.process_info.type = ProcessType_Undefined;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ( (status.MPI_TAG == REPLY_TAG) &&
         (count == sizeof (struct message_def)) )
    {
        if ( (msg->type == MsgType_Service) &&
             (msg->u.reply.type == ReplyType_ProcessInfo) )
        {
            if ( msg->u.reply.u.process_info.return_code == MPI_SUCCESS )
            {
                if ( msg->u.reply.u.process_info.num_processes == 1 )
                {
                    if ( trace_ )
                    {
                        printf ( "[%s] Got process status for %s (%d, %d:%d), state=%s\n",
                             processName_,
                             msg->u.reply.u.process_info.process[0].process_name,
                             msg->u.reply.u.process_info.process[0].nid,
                             msg->u.reply.u.process_info.process[0].pid,
                             msg->u.reply.u.process_info.process[0].verifier,
                             (msg->u.reply.u.process_info.process[0].state == State_Up) ? "Up" : "not Up");
                    }
                    nid = msg->u.reply.u.process_info.process[0].nid;
                    pid = msg->u.reply.u.process_info.process[0].pid;
                    verifier = msg->u.reply.u.process_info.process[0].verifier;
                    result = true;
                }
                else
                {
                    if ( trace_ )
                    {
                        printf( "[%s] - process info for %s returned data for %d processes\n"
                                , processName_
                                , processName
                                , msg->u.reply.u.process_info.num_processes );
                    }
                }
            }
            else
            {
                printf( "[%s] ProcessInfo failed, error=%s\n", processName_,
                        XMPIErrMsg(msg->u.reply.u.process_info.return_code) );
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                    processName_, msg->type, msg->u.reply.type );
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return result;
}

// requestNodeInfo parameters:
//
// targetNid: -1 for all nodes or integer for specific node
// resumeFlag:  false = initial request
//              true = continuation (when previous request could not
//                     return all data), set last_nid and last_pnid
// lastNid:  when resumeFlag = true, last nid returned on previous request
// lastPNid: when resumeFlag = true, last pnid returned on previous request
//
// results:
//    return value = true if node info obtained successfully,
//         In this case nodeData points to the NodeInfo data.  Caller
//         must "free" this to avoid memory leaks.
//    return value = false if could not obtain node data.

bool MonTestUtil::requestNodeInfo ( int targetNid,
                                    bool resumeFlag,
                                    int lastNid,
                                    int lastPNid,
                                    struct NodeInfo_reply_def *& nodeData)
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeInfo;
    msg->u.request.u.node_info.nid = nid_;
    msg->u.request.u.node_info.pid = pid_;
    msg->u.request.u.node_info.target_nid = targetNid;
    msg->u.request.u.node_info.last_nid
        = (resumeFlag == false) ? -1 : lastNid;
    msg->u.request.u.node_info.last_pnid
        = (resumeFlag == false) ? -1 : lastPNid;
    msg->u.request.u.node_info.continuation = resumeFlag;


    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ( (status.MPI_TAG == REPLY_TAG) &&
         (count == sizeof (struct message_def)) )
    {
        if ( (msg->type == MsgType_Service) &&
             (msg->u.reply.type == ReplyType_NodeInfo) )
        {
            if ( (msg->u.reply.u.node_info.return_code == MPI_SUCCESS     ) ||
                (msg->u.reply.u.node_info.return_code == MPI_ERR_TRUNCATE) )
            {
                nodeData = (NodeInfo_reply_def *)
                             malloc(sizeof(NodeInfo_reply_def));
                *nodeData = msg->u.reply.u.node_info;
                result = true;
            }
            else
            {
                printf( "[%s] NodeInfo failed, error=%s\n", processName_,
                        XMPIErrMsg( msg->u.reply.u.node_info.return_code ) );
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for NodeInfo message\n",
                    processName_, msg->type, msg->u.reply.type );
        }
    }
    else
    {
        printf ("[%s] NodeInfo reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg( msg );

    return result;
}

bool MonTestUtil::requestNotice( int nid
                               , int pid
                               , Verifier_t verifier
                               , const char *name
                               , bool cancelFlag
                               , _TM_Txid_External &transid )
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] Request notification on process death for %s (%d, %d:%d), "
                "trans_id=%lld.%lld.%lld.%lld.\n",
                processName_, name, nid, pid, verifier, transid.txid[0],
                transid.txid[1], transid.txid[2], transid.txid[3] );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Notify;
    msg->u.request.u.notify.nid = nid_;
    msg->u.request.u.notify.pid = pid_;
    msg->u.request.u.notify.cancel = cancelFlag ? 1: 0;
    msg->u.request.u.notify.target_nid = nid;
    msg->u.request.u.notify.target_pid = pid;
    msg->u.request.u.notify.verifier = verifier_;
    strcpy(msg->u.request.u.notify.process_name, processName_);
    msg->u.request.u.notify.target_verifier = verifier;
    strcpy(msg->u.request.u.notify.target_process_name, name);
    msg->u.request.u.notify.trans_id = transid;

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
                if ( trace_ )
                {
                    printf ("[%s] Notify request succeeded\n",
                            processName_);
                }
                result = true;
            }
            else
            {
                printf ("[%s] Notify request failed for (%d, %d), rc=%d\n",
                        processName_,
                        msg->u.reply.u.generic.return_code,
                        msg->u.reply.u.generic.nid,
                        msg->u.reply.u.generic.pid);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Notify message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Notify process reply message invalid.  Reply tag=%d,"
                " count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}


void MonTestUtil::requestKill( const char *name, Verifier_t verifier )
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    if ( trace_ )
    {
        printf ("[%s] sending kill request for process %s:%d.\n",
                processName_, name, verifier );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Kill;
    msg->u.request.u.kill.nid = nid_;
    msg->u.request.u.kill.pid = pid_;
    msg->u.request.u.kill.target_nid = -1;
    msg->u.request.u.kill.target_pid = -1;
    msg->u.request.u.kill.verifier = verifier_;
    strcpy(msg->u.request.u.kill.process_name, processName_);
    msg->u.request.u.kill.target_verifier = verifier;
    strcpy (msg->u.request.u.kill.target_process_name, name);

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code != MPI_SUCCESS)
            {
                printf ("[%s] Kill %s:%d failed, rc=%d\n", processName_, name,
                        verifier, msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Kill message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Kill reply message invalid.  Reply tag=%d, count=%d "
                "(expected %d)\n", processName_, msg->reply_tag, count,
                (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

}

bool MonTestUtil::requestNewProcess (int nid, PROCESSTYPE type, bool nowait,
                                     const char *processName,
                                     const char *progName, const char *inFile,
                                     const char *outFile,
                                     int progArgC, char *progArgV[],
                                     int &newNid, int &newPid,
                                     Verifier_t &newVerifier,
                                     char *newProcName,
                                     bool unhooked)
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] starting process %s on node=%d.\n", processName_,
                processName, nid);
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = nid;
    msg->u.request.u.new_process.type = type;
    msg->u.request.u.new_process.debug = 0;
    msg->u.request.u.new_process.priority = 0;
    msg->u.request.u.new_process.backup = 0;
    msg->u.request.u.new_process.unhooked = unhooked;
    msg->u.request.u.new_process.nowait = nowait;
    msg->u.request.u.new_process.tag = 0;
    strcpy (msg->u.request.u.new_process.process_name, processName);
    strcpy (msg->u.request.u.new_process.path, getenv ("PATH"));
    strcpy (msg->u.request.u.new_process.ldpath, getenv ("LD_LIBRARY_PATH"));
    strcpy (msg->u.request.u.new_process.program, progName);
    STRCPY (msg->u.request.u.new_process.infile, inFile);
    STRCPY (msg->u.request.u.new_process.outfile, outFile);
    msg->u.request.u.new_process.argc = progArgC;
    for (int i=0; i<progArgC; i++)
    {
        strcpy (msg->u.request.u.new_process.argv[i], progArgV[i]);
    }

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_NewProcess))
        {
            if (msg->u.reply.u.new_process.return_code == MPI_SUCCESS)
            {
                if ( trace_ )
                {
                    printf
                        ( "[%s] started process successfully. Nid=%d, Pid=%d:%d, Process_name=%s, rtn=%d\n"
                        , processName_
                        , msg->u.reply.u.new_process.nid
                        , msg->u.reply.u.new_process.pid
                        , msg->u.reply.u.new_process.verifier
                        , msg->u.reply.u.new_process.process_name
                        , msg->u.reply.u.new_process.return_code);
                }
                result = true;
                newNid = msg->u.reply.u.new_process.nid;
                newPid = msg->u.reply.u.new_process.pid;
                newVerifier = msg->u.reply.u.new_process.verifier;
                strcpy(newProcName, msg->u.reply.u.new_process.process_name);
            }
            else
            {
                printf ("[%s] new process failed to spawn, rc=%d (%s)\n",
                        processName_,
                        msg->u.reply.u.new_process.return_code,
                        XMPIErrMsg(msg->u.reply.u.new_process.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] new process reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;

}

bool MonTestUtil::requestGet ( ConfigType type,
                               const char *group,
                               const char *key,
                               bool resumeFlag,
                               struct Get_reply_def *& regData
                               )
{
    /*  ReqType_Get arguments:
      type: ConfigType_Cluster, ConfigType_Node, or ConfigType_Process
      next: false if start from beginning, true if start from key
      group: name of group, if NULL and type=ConfigNode assume local node
      key: name of the item to be returned, empty string for all in group
    */

    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Get;
    msg->u.request.u.get.nid = nid_;
    msg->u.request.u.get.pid = pid_;
    msg->u.request.u.get.verifier = verifier_;
    strcpy(msg->u.request.u.get.process_name, processName_);
    msg->u.request.u.get.type = type;
    msg->u.request.u.get.next = resumeFlag;
    STRCPY(msg->u.request.u.get.group, group);
    STRCPY(msg->u.request.u.get.key, key);

    gp_local_mon_io->send_recv( msg );
    count = sizeof( *msg );
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Get))
        {
            regData = (Get_reply_def*) malloc(sizeof(Get_reply_def));
            regData->type = msg->u.reply.u.get.type;
            strcpy(regData->group, msg->u.reply.u.get.group);
            regData->num_keys = msg->u.reply.u.get.num_keys;
            regData->num_returned = msg->u.reply.u.get.num_returned;
            for (int i=0; i<msg->u.reply.u.get.num_returned; i++)
            {
                strcpy(regData->list[i].key, msg->u.reply.u.get.list[i].key);
                strcpy(regData->list[i].value, msg->u.reply.u.get.list[i].value);
            }
            result = true;
        }
    }
    else
    {
        printf ("[%s] Get reply message invalid.  Reply tag=%d, count=%d "
                "(expected %d)\n", processName_, msg->reply_tag, count,
                (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}


bool MonTestUtil::requestSet ( ConfigType type,
                               const char *group,
                               const char *key,
                               const char *value )
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] set request: type=%d, group=%s, key=%s, value=%s.\n",
                processName_, type, group, key, value );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Set;
    msg->u.request.u.set.nid = nid_;
    msg->u.request.u.set.pid = pid_;
    msg->u.request.u.set.verifier = verifier_;
    strcpy(msg->u.request.u.set.process_name, processName_);
    msg->u.request.u.set.type = type;
    STRCPY(msg->u.request.u.set.group, group);
    STRCPY(msg->u.request.u.set.key, key);
    STRCPY(msg->u.request.u.set.value, value);

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
                if ( trace_ )
                {
                    printf("[%s] Set completed successfully.\n", processName_);
                }
                result = true;
            }
            else
            {
                printf ("[%s] Set failed, error=%s (%d)\n", processName_,
                        XMPIErrMsg(msg->u.reply.u.generic.return_code),
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Set message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Set reply message invalid.  Reply tag=%d, count=%d "
                "(expected %d)\n", processName_, msg->reply_tag, count,
                (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}

bool MonTestUtil::requestNodeDown( int nid )
{
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] node down request: nid=%d\n", processName_, nid );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = true;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeDown;
    msg->u.request.u.down.nid = nid;
    msg->u.request.u.down.node_name[0] = '\0';

    gp_local_mon_io->send( msg );

    return result;
}

bool MonTestUtil::requestSendEvent( int targetNid
                                  , int targetPid
                                  , Verifier_t targetVerifier
                                  , const char * targetProcessName
                                  , PROCESSTYPE type
                                  , int eventId
                                  , const char *eventData)
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] send event request: \n",
                processName_ );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Event;
    msg->u.request.u.event.nid = nid_;
    msg->u.request.u.event.pid = pid_;
    msg->u.request.u.event.target_nid = targetNid;
    msg->u.request.u.event.target_pid = targetPid;
    msg->u.request.u.event.verifier = verifier_;
    strcpy(msg->u.request.u.event.process_name, processName_);
    msg->u.request.u.event.target_verifier = targetVerifier;
    strcpy(msg->u.request.u.event.target_process_name, targetProcessName);
    msg->u.request.u.event.type = type;
    msg->u.request.u.event.event_id = eventId;
    msg->u.request.u.event.length = static_cast<int>(strlen(eventData));
    STRCPY(msg->u.request.u.event.data, eventData);

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
                if ( trace_ )
                {
                    printf("[%s] Send event completed successfully.\n",
                           processName_);
                }
                result = true;
            }
            else
            {
                printf ("[%s] Send event failed, error=%s (%d)\n", processName_,
                        XMPIErrMsg(msg->u.reply.u.generic.return_code),
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Send Event "
                 "message\n", processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Send event reply message invalid.  Reply tag=%d, count=%d"
                " (expected %d)\n", processName_, msg->reply_tag, count,
                (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}

bool MonTestUtil::requestTmReady( void )
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] Request TmReady by %s (%d, %d:%d)\n"
               , processName_, processName_, nid_, pid_, verifier_ );
    }

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", processName_);
        return result;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_TmReady;
    msg->u.request.u.tm_ready.nid = nid_;
    msg->u.request.u.tm_ready.pid = pid_;

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
                if ( trace_ )
                {
                    printf ("[%s] TmReady request succeeded\n",
                            processName_);
                }
                result = true;
            }
            else
            {
                printf ("[%s] TmReady request failed for (%d, %d), rc=%d\n",
                        processName_,
                        msg->u.reply.u.generic.return_code,
                        msg->u.reply.u.generic.nid,
                        msg->u.reply.u.generic.pid);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for TmReady message\n",
                 processName_, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] TmReady process reply message invalid.  Reply tag=%d,"
                " count=%d (expected %d)\n", processName_, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);

    return result;
}


char * MonTestUtil::MPIErrMsg ( int code )
{
    int length;
    static char buffer[MPI_MAX_ERROR_STRING];

    if (MPI_Error_string (code, buffer, &length) != MPI_SUCCESS)
    {
        sprintf(buffer,"MPI_Error_string: Invalid error code (%d)\n", code);
        length = static_cast<int>(strlen(buffer));
    }
    buffer[length] = '\0';

    return buffer;
}

bool MonTestUtil::validateNodeCount(int minNodesExpected)
{
    struct NodeInfo_reply_def * nodeData;
    bool result = false;

    if ( trace_ )
        printf ("[%s] Checking configuration for correct number of nodes.\n",
                processName_);

    result = requestNodeInfo ( -1, false, -1, -1, nodeData );
    if (result)
    {
        if ( nodeData->num_nodes >= minNodesExpected )
        {
            result = true;
        }
        else
        {
            printf("[%s] *** ERROR *** This test requires at least %d logical"
                   " nodes but only %d nodes are configured.\n", processName_,
                   minNodesExpected, nodeData->num_nodes);
            result = false;
        }
        free( nodeData );
    }

    return result;
}

int MonTestUtil::getNodeCount ( void )
{
    struct NodeInfo_reply_def * nodeData;
    bool result = false;
    int nodeCount = 0;

    if ( trace_ )
        printf ("[%s] Getting node count.\n", processName_);

    result = requestNodeInfo ( -1, false, -1, -1, nodeData );
    if (result)
    {
        nodeCount = nodeData->num_nodes;
        free( nodeData );
    }

    return nodeCount;
}

bool MonTestUtil::openProcess( const char *procName
                             , Verifier_t procVerifier
                             , int deathNotice
                             , MPI_Comm &comm)
{
    char procPort[MPI_MAX_PORT_NAME];
    bool result = false;

    if ( trace_ )
    {
        printf ("[%s] opening process %s\n",
                processName_, procName );
    }

    if ( requestOpen( procName, procVerifier, deathNotice, procPort ) )
    {
        if ( trace_ ) printf ("[%s] XMPI_Comm_connect(port=%s)\n", processName_, procPort);

        int rc = XMPI_Comm_connect (procPort, MPI_INFO_NULL,
                                   0, MPI_COMM_SELF, &comm);
        if (rc == MPI_SUCCESS)
        {
            XMPI_Comm_set_errhandler (comm, MPI_ERRORS_RETURN);
            result = true;
        }
        else
        {
            printf ("[%s] failed to connect. rc = %d (%s)\n", processName_, rc,
                    XMPIErrMsg(rc));
        }
    }
    else
    {
        printf ("[%s] failed to open %s\n", processName_, procName);
    }

    return result;
}

bool MonTestUtil::closeProcess ( MPI_Comm &comm )
{
    bool result = false;

    int rc = XMPI_Comm_disconnect ( &comm );
    if (rc == MPI_SUCCESS)
    {
        result = true;
        if ( trace_ )
        {
            printf ("[%s] disconnected from process.\n", processName_);
        }
    }
    else
    {
        printf ("[%s] failed to disconnect. rc=%d (%s)\n",
                processName_, rc, XMPIErrMsg(rc));
    }

    return result;
}

// Notes:
// todo:
//    1.  Undo #ifdef in InitLocalIO (who uses this code?)
//    2.  Use seabed tracing (replace shell_locio_trace?), integrate clio tracing
//        perhaps use command line option to control tracing
//    3.  Better to have a localio wrapper object instead of invocing InitLocalIO?
//    4.  Better not to rely on external variable my_name
//    5.  For bug, need to test with and without concurrent requests; also
//        both waited and no-waited new process (need to test for both user
//        assigned and monitor assigned process names)
//    6.  currently clio tracing is enabled by setting environment variable
//        SQ_LOCAL_IO_SHELL_TRACE to 1.  Trace output goes to shell.trace.<pid>.
//
//
// InitLocalIO:
//    same implementation in:
//       client.cxx
//       getseq.cxx
//       notify.cxx
//       nsclient.cxx
//       nsserver.cxx
//       pingpong2.cxx
//       server.cxx
//       testspx.cxx
//    variation in:
//       testtm.cxx (calls trace_printf)
// process_startup
//    => I used testspx.cxx variation, added parameters, removed #ifdef
//       client.cxx
//       getseq.cxx
//       notify.cxx
//       pingpong2.cxx
//       server.cxx
//    variation in:
//       nsclient.cxx
//    variation in:
//       nsserver.cxx
//    variation in:
//       testspx.cxx (mostly formatting changes)
//    variation in:
//       testtm.cxx (calls trace_printf)
// exit_process
//    same implementation in:
//       client.cxx
//       getseq.cxx
//       notify.cxx
//       pingpong2.cxx
//       server.cxx
//    variation in:
//       nsclient.cxx
//    variation in:
//       nsserver.cxx
//    variation in:
//       testspx.cxx (mostly formatting differences)
//    variation in:
//       testtm.cxx
// shell_locio_trace
//    same implementation in:
//       client.cxx
//       getseq.cxx
//       notify.cxx
//       nsclient.cxx
//       nsserver.cxx
//       pingpong2.cxx
//       server.cxx
//       testspx.cxx
//       testtm.cxx
// flush_incoming_msgs
//    => I used testspx.cxx variation, added parameter
//    same implementation in:
//       client.cxx
//       getseq.cxx
//       notify.cxx
//       pingpong2.cxx
//       server.cxx
//    variation in:
//       nsclient.cxx (uses local variable for "msg")
//    variation in:
//       nsserver.cxx (mostly formatting differences)
//    variation in:
//       testspx.cxx  (mostly formatting differences (standard)) + global var
//    variation in:
//       testtm.cxx (uses trace_printf)

