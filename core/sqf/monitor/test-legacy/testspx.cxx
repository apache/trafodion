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
#include <string.h>
#include <mpi.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "msgdef.h"
#include "props.h"
#include "localio.h"
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"

long trace_settings = 0;

#define MAX_SPX_PROCESSES 6
#define MAX_TEST_NODES 6

char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
int MyRank = -1;
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
int VirtualNodes = 0;
int NumNodes = 0;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
MPI_Comm Monitor;

int event_id = -1;
bool node_down = false;
int spx_count = 0;
int dead_spx_count = 0;
int test_spx_count = 0;
int down_node_count = 0;
struct spx_process_def {
    bool dead;
    int  nid;
    int  pid;
    char process_name[MAX_PROCESS_NAME];    // SPX process Name
    char program[MAX_PROCESS_PATH];
} spxProcess[MAX_SPX_PROCESSES];
int NumPNodes = 0;
bool node_up[MAX_TEST_NODES] = {true,true,true,true,true,true};
bool EventMsgRecv = false;
bool UnsolMsgRecv = false;
bool NoticeMsgRecv = false;
bool Test_Initialized = false;
bool TestShutdown = false;

void flush_incoming_msgs( void );

FILE *shell_locio_trace_file = NULL;

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    printf("%s", pp_string );

    return lv_err;
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
        localtime_r(&t.tv_sec, &tx);
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

char *ErrorMsg (int error_code)
{
    int rc;
    int length;
    static char buffer[MPI_MAX_ERROR_STRING];

    rc = MPI_Error_string (error_code, buffer, &length);
    if (rc != MPI_SUCCESS)
    {
        sprintf(buffer,"MPI_Error_string: Invalid error code (%d)\n", error_code);
        length = strlen(buffer);
    }
    buffer[length] = '\0';

    return buffer;
}

bool completed_test( void )
{
    bool done = dead_spx_count ? true : false;

    Test_Initialized = done ? false : true;

    return( done );
}

int GetNumOfNode( int *numNodes, int *numPNodes )
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NodeInfo;
    msg->u.request.u.node_info.nid = MyNid;
    msg->u.request.u.node_info.pid = MyPid;
    msg->u.request.u.node_info.target_nid = -1;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;


    if ( (status.MPI_TAG == REPLY_TAG) && (count == sizeof (struct message_def)) )
    {
        if ( (msg->type == MsgType_Service) && (msg->u.reply.type == ReplyType_NodeInfo) )
        {
            if ( (msg->u.reply.u.node_info.return_code == MPI_SUCCESS     ) ||
                (msg->u.reply.u.node_info.return_code == MPI_ERR_TRUNCATE) )
            {
                if ( msg->u.reply.u.node_info.num_returned )
                {
                    //printf ("[%s] NodeInfo, num_nodes=%d\n", MyName,
                    //    msg->u.reply.u.node_info.num_nodes);
                    *numNodes = msg->u.reply.u.node_info.num_nodes;
                    *numPNodes = msg->u.reply.u.node_info.num_pnodes;
                    gp_local_mon_io->release_msg( msg );
                    return 0;
                }
            }
            else
            {
                printf( "[%s] NodeInfo failed, error=%s\n", MyName,
                        ErrorMsg( msg->u.reply.u.node_info.return_code ) );
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for NodeInfo message\n",
                    MyName, msg->type, msg->u.reply.type );
        }
    }
    else
    {
        printf( "[%s] NodeInfo reply message invalid\n", MyName );
    }

    gp_local_mon_io->release_msg( msg );
    msg = NULL;

    return -1;
}

int get_spx_processes( void )
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = MyNid;
    msg->u.request.u.process_info.pid = MyPid;
    msg->u.request.u.process_info.verifier = -1;
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    msg->u.request.u.process_info.type = ProcessType_SPX;

        msg->u.request.u.process_info.target_process_name[0] = 0;
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
                if ( msg->u.reply.u.process_info.num_processes == test_spx_count )
                {
                    for ( int i = 0; i < msg->u.reply.u.process_info.num_processes; i++ )
                    {
                        printf( "[%s] SPX ProcessInfo: nid=%d, pid=%d, Name=%s, program=%s\n", MyName
                              , msg->u.reply.u.process_info.process[i].nid
                              , msg->u.reply.u.process_info.process[i].pid
                              , msg->u.reply.u.process_info.process[i].process_name
                              , msg->u.reply.u.process_info.process[i].program );
                        spxProcess[i].dead = msg->u.reply.u.process_info.process[i].state == State_Up ? false : true;
                        spxProcess[i].nid = msg->u.reply.u.process_info.process[i].nid;
                        spxProcess[i].pid = msg->u.reply.u.process_info.process[i].pid;
                        strcpy (spxProcess[i].process_name, msg->u.reply.u.process_info.process[i].process_name );
                        strcpy (spxProcess[i].program, msg->u.reply.u.process_info.process[i].program );
                    }
                }
                else
                {
                    printf( "[%s] - Test1 - Invalid number of SPX processes, count=%d, expected=%d\n"
                          , MyName
                          , msg->u.reply.u.process_info.num_processes
                          , MAX_SPX_PROCESSES);
                    return 1;
                }
            }
            else
            {
                printf( "[%s] SPX ProcessInfo failed, error=%s\n", MyName,
                        ErrorMsg(msg->u.reply.u.process_info.return_code) );
                return 1;
            }
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for SPX ProcessInfo message\n",
                    MyName, msg->type, msg->u.reply.type );
            return 1;
        }
    }
    else
    {
        printf( "[%s] SPX ProcessInfo reply message invalid\n", MyName );
        return 1;
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return 0;
}

void initialize_test( void )
{
    printf( "[%s] Initializing for test\n",MyName );
    memset( spxProcess, 0, sizeof(spxProcess) );
    char *cmd_buffer = getenv("SQ_VIRTUAL_NODES");
    if (cmd_buffer && isdigit(cmd_buffer[0]))
    {
        VirtualNodes = atoi(cmd_buffer);
    }
    else
    {
        VirtualNodes = 0;
    }
    if (VirtualNodes)
    {
        test_spx_count = MAX_SPX_PROCESSES;
    }
    else
    {
        test_spx_count = MAX_SPX_PROCESSES/2;
    }
    Test_Initialized = true;
}

void process_startup( int argc, char *argv[] )
{
    int i;
    
    struct message_def *msg;

    printf( "[%s] processing startup.\n", argv[5] );
    fflush( stdout );

    printf("[%s] - argc=%d", argv[5], argc);
    for (i=0; i<argc; i++)
    {
        printf (", argv[%d]=%s",i,argv[i]);
    }
    printf( "\n" );
    fflush( stdout );

    strcpy( MyName, argv[5] );
    MPI_Open_port( MPI_INFO_NULL, MyPort );

#ifdef OFED_MUTEX
    // free monitor.sem semaphore
    printf( "[%s] Opening mutex\n",MyName );
    fflush( stdout );
    char sem_name[MAX_PROCESS_PATH];
    sprintf( sem_name,"/monitor.sem2.%s",getenv("USER") );
    sem_t *mutex = sem_open(sem_name,0,0644,0);
    if ( mutex == SEM_FAILED )
    {
        printf ("[%s] Can't access %s semaphore\n", MyName, sem_name);
        sem_close(mutex);
        abort();
    }
    printf ("[%s] Putting mutex\n",MyName);
    fflush(stdout);
    sem_post(mutex);
    sem_close(mutex);
#endif
    MyNid = atoi(argv[3]);
    MyPid = atoi(argv[4]);
    gv_ms_su_verif  = atoi(argv[9]);
    printf ("[%s] process_startup, MyNid: %d, lio: %p\n", 
             MyName, MyNid, (void *)gp_local_mon_io );

    gp_local_mon_io->iv_pid = MyPid;
    gp_local_mon_io->init_comm();

    
    if (argc < 10)
    {
        printf
            ("Error: Invalid startup arguments, argc=%d, argv[0]=%s, argv[1]=%s, argv[2]=%s, argv[3]=%s, argv[4]=%s, argv[5]=%s, argv[6]=%s, argv[7]=%s, argv[8]=%s, argv[9]=%s\n",
             argc, argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);
        exit (1);
    }
    else
    {
        gp_local_mon_io->acquire_msg( &msg );

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
        msg->u.request.u.startup.verifier = gv_ms_su_verif;
        msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);
        printf ("[%s] sending startup reply to monitor.\n", argv[5]);
        fflush (stdout);

        gp_local_mon_io->send( msg );

        printf ("[%s] Startup completed", argv[5]);
        if (argc > 9)
        {
            for (i = 10; i < argc; i++)
            {
                printf (", argv[%d]=%s", i, argv[i]);
            }
        }
        printf ("\n");
        fflush (stdout);

        msg = NULL;
    }
}

void flush_incoming_msgs( void )
{
    int count;

    int complete = 0;
    bool done = false;
    MPI_Status status;
    struct message_def *msg = NULL;

    printf( "[%s] flush incoming event & notices.\n", MyName );
    fflush( stdout );
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if ( msg )
        {
            printf( "[%s] Got local IO notice\n",MyName );
            complete = true;
            count = sizeof( *msg );
            status.MPI_TAG = msg->reply_tag;
        }
        else
        {
            printf( "[%s] No local IO notice\n",MyName );
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
                                    MyName, msg->u.request.u.death.process_name, 
                                    msg->u.request.u.death.nid, msg->u.request.u.death.pid );
                        }
                        else
                        {
                            printf( "[%s] Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                    MyName, msg->u.request.u.death.process_name, 
                                    msg->u.request.u.death.nid, msg->u.request.u.death.pid );
                        }
                        break;

                    case MsgType_NodeDown:
                        printf( "[%s] Node %d (%s) is DOWN\n", 
                                MyName, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        printf( "[%s] Node %d (%s) is UP\n", 
                                MyName, msg->u.request.u.up.nid, msg->u.request.u.up.node_name );
                        break;

                    case MsgType_Change:
                        printf( "[%s] Configuration Change Notice for Group: %s Key: %s\n", 
                                MyName, 
                                msg->u.request.u.change.group,
                                msg->u.request.u.change.key );
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        printf( "[%s] Open/Close process notification\n", MyName );
                        break;

                    case MsgType_Event:
                        printf( "[%s] Event %d received\n",
                                MyName, msg->u.request.u.event_notice.event_id );
                        break;

                    case MsgType_Shutdown:
                        TestShutdown = true;
                        printf( "[%s] Shutdown notice, level=%d received\n",
                                MyName, msg->u.request.u.shutdown.level );
                        break;

                    default:
                        printf( "[%s] Invalid Notice Type(%d) for flush message\n",
                                MyName, msg->type );
                    }
                }
                else
                {
                    printf( "[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                            MyName, msg->type, msg->noreply, msg->u.request.type );
                }
            }
            else
            {
                printf( "[%s] Failed to flush messages\n", MyName );
                done = true;
            }
            fflush( stdout );
        }
        if ( msg ) delete msg;
        msg = NULL;
    }
    while ( !done );
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    printf( "[%s] sending exit process message.\n", MyName );
    fflush( stdout );

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = MyNid;
    msg->u.request.u.exit.pid = MyPid;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ( (status.MPI_TAG == REPLY_TAG) &&
         (count == sizeof (struct message_def)) )
    {
        if ( (msg->type == MsgType_Service) &&
             (msg->u.reply.type == ReplyType_Generic) )
        {
            if ( msg->u.reply.u.generic.return_code == MPI_SUCCESS )
            {
                printf( "[%s] exited process successfully. rc=%d\n",
                        MyName, msg->u.reply.u.generic.return_code );
            }
            else
            {
                printf( "[%s] exit process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code );
            }
            flush_incoming_msgs( );
            MPI_Comm_disconnect( &Monitor );
        }
        else
        {
            printf( "[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                    MyName, msg->type, msg->u.reply.type );
        }
    }
    else
    {
        printf( "[%s] exit process reply invalid.\n", MyName );
    }
    fflush( stdout );

    gp_local_mon_io->release_msg( msg );
    msg = NULL;
}

void process_event( struct message_def *msg )
{
    printf( "[%s] Processing event message.\n", MyName );
    event_id = -1;
    if ( msg->u.request.type == ReqType_Notice )
    {
        if ( msg->type == MsgType_Event )
        {
            printf( "[%s] Event %d (%s) received\n", 
                    MyName, 
                    msg->u.request.u.event_notice.event_id,
                    msg->u.request.u.event_notice.data );
            event_id = msg->u.request.u.event_notice.event_id;
        }
        else
        {
            printf( "[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                    MyName, msg->type, msg->noreply, msg->u.request.type );
        }
    }
    else
    {
        printf( "[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type );
    }
}

bool process_notice( struct message_def *msg )
{
    int i;
    bool completed = false;

    printf ("[%s] Processing notice.\n", MyName);
    if( msg->u.request.type == ReqType_Notice )
    {
        if( msg->type == MsgType_ProcessDeath )
        {
            if (( msg->u.request.u.death.trans_id.txid[0] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[1] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[2] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[3] == 0 )   )
            {
                for ( i = 0; i < MAX_SPX_PROCESSES; i++ )
                {
                    if ( spxProcess[i].nid == msg->u.request.u.death.nid &&
                         spxProcess[i].pid == msg->u.request.u.death.pid )
                    {
                        spxProcess[msg->u.request.u.death.nid].dead = true;
                        dead_spx_count++;
                        break;
                    }
                }
                printf("[%s] Process Death Notification for Nid=%d, Pid=%d\n",
                    MyName, msg->u.request.u.death.nid, msg->u.request.u.death.pid);
            }
            else
            {
                printf("[%s] Transaction Process Death Notification for Nid=%d, Pid=%d, Trans_id=%lld.%lld.%lld.%lld\n",
                    MyName, 
                    msg->u.request.u.death.nid, 
                    msg->u.request.u.death.pid, 
                    msg->u.request.u.death.trans_id.txid[0],
                    msg->u.request.u.death.trans_id.txid[1],
                    msg->u.request.u.death.trans_id.txid[2],
                    msg->u.request.u.death.trans_id.txid[3]);
            }
        }
        else if ( msg->type == MsgType_NodeDown )
        {
            node_down=true;
            printf("[%s] Node %d (%s) is DOWN, Transactions aborted\n",
                MyName, 
                msg->u.request.u.down.nid,
                msg->u.request.u.down.node_name);
            node_up[msg->u.request.u.down.nid]=false;
            down_node_count++;
            NumNodes--;
        }
        else if ( msg->type == MsgType_NodeUp )
        {
            printf("[%s] Node %d (%s) is UP\n",
                MyName, 
                msg->u.request.u.up.nid,
                msg->u.request.u.up.node_name);
        }
        else
        {
            printf("[%s] Invalid Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }
    }
    else
    {
        printf("[%s] Invalid Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }    
    return completed;
}

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
        case MsgType_NodeDown:
            str = "MsgType_NodeDown";
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
        case MsgType_Service:
            str = "MsgType_Service";
            break;
        case MsgType_Shutdown:
            str = "MsgType_Shutdown";
            break;
        case MsgType_TmSyncAbort:
            str = "MsgType_TmSyncAbort";
            break;
        case MsgType_TmSyncCommit:
            str = "MsgType_TmSyncCommit";
            break;
        case MsgType_UnsolicitedMessage:
            str = "MsgType_UnsolicitedMessage";
            break;
        default:
            str = "MsgType - Undefined";
            break;
    }
    return( str );
}

const char *RequestTypeString( REQTYPE type )
{
    const char *str = NULL;

    switch( type )
    {
        case ReqType_Close:
            str = "ReqType_Close";
            break;
        case ReqType_Exit:
            str = "ReqType_Exit";
            break;
        case ReqType_Event:
            str = "ReqType_Event";
            break;
        case ReqType_Get:
            str = "ReqType_Get";
            break;
        case ReqType_Kill:
            str = "ReqType_Kill";
            break;
        case ReqType_Mount:
            str = "ReqType_Mount";
            break;
        case ReqType_NewProcess:
            str = "ReqType_NewProcess";
            break;
        case ReqType_NodeDown:
            str = "ReqType_NodeDown";
            break;
        case ReqType_NodeInfo:
            str = "ReqType_NodeInfo";
            break;
        case ReqType_NodeUp:
            str = "ReqType_NodeUp";
            break;
        case ReqType_Notice:
            str = "ReqType_Notice";
            break;
        case ReqType_Notify:
            str = "ReqType_Notify";
            break;
        case ReqType_Open:
            str = "ReqType_Open";
            break;
        case ReqType_OpenInfo:
            str = "ReqType_OpenInfo";
            break;
        case ReqType_ProcessInfo:
            str = "ReqType_ProcessInfo";
            break;
        case ReqType_Set:
            str = "ReqType_Set";
            break;
        case ReqType_Shutdown:
            str = "ReqType_Shutdown";
            break;
        case ReqType_Startup:
            str = "ReqType_Startup";
            break;
        case ReqType_TmLeader:
            str = "ReqType_TmLeader";
            break;
        case ReqType_TmSync:
            str = "ReqType_TmSync";
            break;
        case ReqType_TransInfo:
            str = "ReqType_TransInfo";
            break;
        case ReqType_Stfsd:
            str = "ReqType_Stfsd";
            break;
        case ReqType_ProcessInfoCont:
            str = "ReqType_ProcessInfoCont";
            break;
        default:
            str = "ReqType - Undefined";
            break;
    }
    return( str );
}

void recv_localio_msg(struct message_def *recv_msg, int size)
{
    size = size; // Avoid "unused parameter" warning

    // CHECK TO SEE THAT WE HAVE STARTED THE TEST BEFORE WE CONTINUE
    while ( !Test_Initialized ) usleep(1000);

    printf("[%s] Message received: ",MyName);
    switch ( recv_msg->type )
    {
        case MsgType_Service:
            printf("Service Reply: Type=%d, ReplyType=%d\n",recv_msg->type, recv_msg->u.reply.type);
            break;
        
        case MsgType_Event:
            printf("Event - %d\n",recv_msg->u.request.u.event_notice.event_id);
            if ( EventMsgRecv )
            {
                printf("[%s] - Event Message overrun!\n", MyName);
                abort();
            }
            EventMsgRecv = true;
            process_event( recv_msg );
            break;

        case MsgType_UnsolicitedMessage:
            printf("Unsolicited Message:\n");
            if ( UnsolMsgRecv )
            {
                printf("[%s] - Unsolicitied Message overrun!\n", MyName);
//                abort();
            }
            UnsolMsgRecv = true;
            break;

         default:
            MessageTypeString( recv_msg->type );
            RequestTypeString( recv_msg->u.request.type );
            printf( "Notice: Type=%d(%s), RequestType=%d(%s)\n"
                   , recv_msg->type, MessageTypeString( recv_msg->type )
                   , recv_msg->u.request.type, RequestTypeString( recv_msg->u.request.type ));
            if ( NoticeMsgRecv )
            {
//                printf("[%s] - Notice Message overrun!\n", MyName);
//                abort();
            }
            NoticeMsgRecv = true;
            process_notice( recv_msg );
    }
}

MPI_Status recv (MPI_Comm comm, struct message_def *msg, int tag)
{
    MPI_Status status;
    msg = msg;
    tag = tag;
    if (comm) {} // Avoid "unused parameter" warning
    
    printf("[%s] Waiting for message from monitor\n",MyName);
    if ( gp_local_mon_io )
    {
        if (tag == MPI_ANY_TAG)
        {
            // spin until message has been received
            while (!NoticeMsgRecv) {usleep(1000);}
            NoticeMsgRecv = false;
            status.MPI_TAG = NOTICE_TAG;
        }
        else if (tag == UNSOLICITED_TAG)
        {
            // spin until message has been received
            while (!UnsolMsgRecv) {usleep(1000);}
            UnsolMsgRecv = false;
            status.MPI_TAG = UNSOLICITED_TAG;
        }
        else
        {
            printf("[%s] Can't call recv with localIO\n", MyName);
            abort();
        }
        status.MPI_ERROR = MPI_SUCCESS;
        return status;
    }
    else
    {
        printf("[%s] LocalIO not in use\n", MyName);
        abort();
    }
}

bool wait_for_event()
{
    MPI_Status status;
    struct message_def *msg;

    printf ("[%s] Waiting for event message.\n", MyName);

    status.MPI_ERROR = gp_local_mon_io->wait_for_event( &msg );

    process_event(msg);
    if (status.MPI_ERROR != MPI_SUCCESS)
    {
        printf ("[%s] Recv failed, rc = %d\n", MyName, status.MPI_ERROR);
    }
    delete msg;
    return (status.MPI_ERROR == MPI_SUCCESS);
}

bool wait_for_notice()
{
    MPI_Status status;
    struct message_def *msg = NULL;

    printf ("[%s] Waiting for notice.\n", MyName);
    status = recv (Monitor, msg, MPI_ANY_TAG);
    if (status.MPI_ERROR != MPI_SUCCESS)
    {
        printf ("[%s] Recv failed, rc = %d\n", MyName, status.MPI_ERROR);
    }
    return (status.MPI_ERROR == MPI_SUCCESS && status.MPI_TAG == NOTICE_TAG);
}

bool wait_for_unsolicited_msg()
{
    MPI_Status status;
    struct message_def *msg = NULL;

    printf ("[%s] Waiting for unsolicited message.\n", MyName);
    status = recv (Monitor, msg, UNSOLICITED_TAG);
    if (status.MPI_ERROR != MPI_SUCCESS)
    {
        printf ("[%s] Recv failed, rc = %d\n", MyName, status.MPI_ERROR);
    }
    return (status.MPI_ERROR == MPI_SUCCESS);
}

void InitLocalIO( void )
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
                printf("[%s], Failed to load cluster configuration.\n", MyName);
                abort();
            }
        }
        else
        {
            printf( "[%s] Warning: No cluster.conf found\n",MyName);
            if (MyNid == -1)
            {
                printf( "[%s] Warning: set default virtual node ID = 0\n",MyName);
                MyNid = 0;
            }
            abort();
        }

        lnodeConfig = ClusterConfig.GetLNodeConfig( MyNid );
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        gv_ms_su_nid = MyPNid = pnodeConfig->GetPNid();
        printf ("[%s] Local IO pnid = %d\n", MyName, MyPNid);
    }

    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
    cmd_buffer = getenv("SQ_LOCAL_IO_SHELL_TRACE");
    if (cmd_buffer && *cmd_buffer == '1')
    {
        gp_local_mon_io->cv_trace = true;
        
        char tracefile[MAX_SEARCH_PATH];
        char *tmpDir;
    
        tmpDir = getenv( "MPI_TMPDIR" );
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

int main (int argc, char *argv[])
{
    bool done = false;
    bool end_of_test = false;
    
    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);

    MyName = new char [MAX_PROCESS_PATH];
    strcpy( MyName, argv[5] );
    MyNid = atoi(argv[3]);
    
    if ( ! gp_local_mon_io )
    {
        InitLocalIO();
        assert (gp_local_mon_io);
    }
 
    gp_local_mon_io->set_cb(recv_localio_msg, "notice");
    gp_local_mon_io->set_cb(recv_localio_msg, "recv");
    gp_local_mon_io->set_cb(recv_localio_msg, "unsol");

    process_startup (argc, argv);
    int rc = GetNumOfNode( &NumNodes, &NumPNodes );
    if ( rc == 0 )
    {
        do
        {
            initialize_test();
            // wait for event signal to start test across all TMs
            wait_for_event();
            switch ( event_id )
            {
            case 1:
                 // *** test 1 -- failed SPX process test
                 printf("[%s] Test1 - SPX process death test - Waiting to start test.\n",MyName);
                 if ( get_spx_processes() )
                 {
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                 }
                 if ( MyNid == 5 )
                 {
                    sleep(2);
                    printf("[%s] - Test1 - Stopping to generate process death message to peer SPX processes\n",MyName);
                    fflush (stdout);
                    MPI_Abort (MPI_COMM_WORLD, 99);
                 }
                 while ( ! end_of_test )
                 {
                    printf("[%s] Test1 - Wait for SPX process death.\n",MyName);
                    fflush (stdout);
                    if( !wait_for_notice() )
                    {
                        if ( ! dead_spx_count )
                        {
                            printf("[%s] Test1 - Failed to receive notice! Aborting\n",MyName);
                            fflush (stdout);
                            MPI_Abort( MPI_COMM_WORLD, 99 );
                        }
                    }
                    printf( "[%s] Test1 - Dead SPX count =%d\n", MyName, dead_spx_count );
                    if ( dead_spx_count )
                    {
                        //TODO: printf("[%s] Test1 - Waiting for restart of SPX process.\n",MyName);
                        //TODO: wait_for_event(); // event 2
                        end_of_test = true;
                    }
                    if ( TestShutdown == true )
                    {
                        end_of_test = true;
                    }
                 }
                 printf("[%s] Test1 - SPX process death test w/restart of SPX processes - Completed test.\n",MyName);
                 break;

            case 2:
                 //TODO:  *** test 1 -- failed SPX process test w/ restart of SPX process 
                 //TODO:  Restarted TMs execute this path (event 2), surviving TMs are in case 1: above
                 printf("[%s] Test1 - SPX process death test w/restart of SPX processes - Completed test.\n",MyName);
                 break;
                 
            default:
                printf("[%s] Event = %d, No test defined, stopping\n",MyName, event_id);
                done = true;
            }

            if (!done)
            {
                fflush (stdout);
            }
        }
        while (!done);    
    }
                
    // exit my process
    exit_process ();

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port( MyPort );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
