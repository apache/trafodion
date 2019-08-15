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
#include "msgdef.h"
#include "props.h"
#include "localio.h"
#include "clio.h"

#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"
long trace_settings = 0;

#define MAX_WORKERS 4
#define MAX_CYCLES  60

// Server message tags
#define USER_TAG 100

// Server process commands
#define CMD_CONT 1
#define CMD_END  2
#define CMD_ABORT 3

char WorkerPort[MAX_WORKERS][MPI_MAX_PORT_NAME];
char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
int MyRank = -1;
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
MPI_Comm Monitor;
struct message_def *msg;

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

void process_startup (int argc, char *argv[])
{
    int i;

    printf ("[%s] processing startup.\n", argv[5]);
    fflush (stdout);

    printf ("[%s] - argc=%d", argv[5], argc);
    for(i=0; i<argc; i++)
    {
        printf (", argv[%d]=%s",i,argv[i]);
    }
    printf ("\n");
    fflush(stdout);

    strcpy (MyName, argv[5]);
    MPI_Open_port (MPI_INFO_NULL, MyPort);

#ifdef OFED_MUTEX
    // free monitor.sem semaphore
    printf ("[%s] Opening mutex\n",MyName);
    fflush(stdout);
    char sem_name[MAX_PROCESS_PATH];
    sprintf(sem_name,"/monitor.sem2.%s",getenv("USER"));
    sem_t *mutex = sem_open(sem_name,0,0644,0);
    if(mutex == SEM_FAILED)
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

    }
}

void start_process (int nid, char *name)
{
    int count;
    MPI_Status status;

    printf ("[%s] starting process %s.\n", MyName, name);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = nid;
    msg->u.request.u.new_process.type = ProcessType_Generic;
    msg->u.request.u.new_process.debug = 0;
    msg->u.request.u.new_process.priority = 0;
    msg->u.request.u.new_process.backup = 0;
    msg->u.request.u.new_process.unhooked = true;
    msg->u.request.u.new_process.nowait = false;
    msg->u.request.u.new_process.tag = 0;
    strcpy (msg->u.request.u.new_process.process_name, name);
    strcpy (msg->u.request.u.new_process.path, getenv ("PATH"));
    strcpy (msg->u.request.u.new_process.ldpath, getenv ("LD_LIBRARY_PATH"));
    strcpy (msg->u.request.u.new_process.program, getenv ("PWD"));
    strcat (msg->u.request.u.new_process.program, "/server");
    msg->u.request.u.new_process.infile[0] = '\0';
    msg->u.request.u.new_process.outfile[0] = '\0';
    msg->u.request.u.new_process.argc = 2;
    strcpy (msg->u.request.u.new_process.argv[0], "CLIENT_ARG_0");
    strcpy (msg->u.request.u.new_process.argv[1], "CLIENT_ARG_1");

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
                printf
                    ("[%s] started process successfully. Nid=%d, Pid=%d, Process_name=%s, rtn=%d\n",
                     MyName, msg->u.reply.u.new_process.nid,
                     msg->u.reply.u.new_process.pid,
                     msg->u.reply.u.new_process.process_name,
                     msg->u.reply.u.new_process.return_code);
            }
            else
            {
                printf ("[%s] new process failed to spawn, rc=%d\n", MyName,
                        msg->u.reply.u.new_process.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] new process reply message invalid\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

}

void open_server (char *process_name, char *port, MPI_Comm * comm)
{
    int rc;
    int count;
    MPI_Status status;

    printf ("[%s] opening server %s.\n", MyName, process_name);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Open;
    msg->u.request.u.open.nid = MyNid;
    msg->u.request.u.open.pid = MyPid;
    strcpy (msg->u.request.u.open.target_process_name, process_name);
    msg->u.request.u.open.death_notification = 0;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Open))
        {
            if (msg->u.reply.u.open.return_code == MPI_SUCCESS)
            {
                printf
                    ("[%s] opened process successfully. Nid=%d, Pid=%d, Port=%s, rtn=%d\n",
                     MyName, msg->u.reply.u.open.nid, msg->u.reply.u.open.pid,
                     msg->u.reply.u.open.port, msg->u.reply.u.open.return_code);
                strcpy (port, msg->u.reply.u.open.port);
                rc = MPI_Comm_connect (msg->u.reply.u.open.port, MPI_INFO_NULL,
                                       0, MPI_COMM_SELF, comm);
                if (rc == MPI_SUCCESS)
                {
                    MPI_Comm_set_errhandler (*comm, MPI_ERRORS_RETURN);
                    printf ("[%s] connected to process.\n", MyName);
                    fflush (stdout);
                }
                else
                {
                    printf ("[%s] failed to connected. rc = (%d)\n", MyName,
                            rc);
                    fflush (stdout);
                    abort();
                }
            }
            else
            {
                printf ("[%s] open process failed, rc=%d\n", MyName,
                        msg->u.reply.u.open.return_code);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for open message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] open process reply message invalid\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

}

void close_server (char *process_name, MPI_Comm * comm)
{
    int rc;
    int count;
    MPI_Status status;

    printf ("[%s] closing server %s.\n", MyName, process_name);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Close;
    msg->u.request.u.close.nid = MyNid;
    msg->u.request.u.close.pid = MyPid;
    strcpy (msg->u.request.u.close.process_name, process_name);

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
                printf
                    ("[%s] closed process successfully. Nid=%d, Pid=%d, rtn=%d\n",
                     MyName, msg->u.reply.u.generic.nid,
                     msg->u.reply.u.generic.pid,
                     msg->u.reply.u.generic.return_code);
            }
            else
            {
                printf ("[%s] close process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code);
            }
            rc = MPI_Comm_disconnect (comm);
            if (rc == MPI_SUCCESS)
            {
                printf ("[%s] disconnected from process.\n", MyName);
                fflush (stdout);
            }
            else
            {
                printf ("[%s] failed to disconnected. rc = (%d)\n", MyName, rc);
                fflush (stdout);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for close message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] close process reply message invalid\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
 
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    struct message_def *save_msg;

    printf ("[%s] sending exit process message.\n", MyName);
    fflush (stdout);

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

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                printf ("[%s] exited process successfully. rc=%d\n",
                        MyName, msg->u.reply.u.generic.return_code);
            }
            else
            {
                printf ("[%s] exit process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code);
            }
            save_msg = msg;
            flush_incoming_msgs();
            msg = save_msg;
            MPI_Comm_disconnect( &Monitor );
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] exit process reply invalid.\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

void flush_incoming_msgs( void )
{
    int count;
    int complete = 0;
    bool done = false;
    MPI_Status status;

    printf ("[%s] flush incoming event & notices.\n", MyName);
    fflush (stdout);
    msg = NULL;
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if (msg) 
        {
            printf("[%s] Got local IO notice\n",MyName);
            complete = true;
            count = sizeof( *msg );
            status.MPI_TAG = msg->reply_tag;
        }
        else
        {
            printf("[%s] No local IO notice\n",MyName);
            complete = false;
            done = true;
        }

        if (complete)
        {
            MPI_Get_count (&status, MPI_CHAR, &count);
            if (((status.MPI_TAG == NOTICE_TAG) ||
                 (status.MPI_TAG == EVENT_TAG )      ) &&
                (count == sizeof (struct message_def))   )
            {
                if (msg->u.request.type == ReqType_Notice)
                {
                    switch (msg->type)
                    {
                    case MsgType_ProcessDeath:
                        if ( msg->u.request.u.death.aborted )
                        {
                            printf ("[%s] Process %s abnormally terminated. Nid=%d, Pid=%d\n",
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        else
                        {
                            printf ("[%s] Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        break;

                    case MsgType_NodeDown:
                        printf ("[%s] Node %d (%s) is DOWN\n", 
                            MyName, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        printf ("[%s] Node %d (%s) is UP\n", 
                            MyName, msg->u.request.u.up.nid, msg->u.request.u.up.node_name);
                        break;    

                    case MsgType_Change:
                        printf ("[%s] Configuration Change Notice for Group: %s Key: %s\n", 
                            MyName, 
                            msg->u.request.u.change.group,
                            msg->u.request.u.change.key);
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        printf ("[%s] Open/Close process notification\n", MyName);
                        break;
                    
                    case MsgType_Event:
                        printf("[%s] Event %d received\n",
                            MyName, msg->u.request.u.event_notice.event_id);
                        break;

                    case MsgType_Shutdown:
                        printf("[%s] Shutdown notice, level=%d received\n",
                            MyName, msg->u.request.u.shutdown.level);
                        break;
                        
                    default:
                        printf("[%s] Invalid Notice Type(%d) for flush message\n",
                            MyName, msg->type);
                    }
                }    
                else
                {
                    printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                        MyName, msg->type, msg->noreply, msg->u.request.type);
                }
            }
            else
            {
                printf ("[%s] Failed to flush messages\n", MyName);
                done = true;
            }
            fflush (stdout);
        }
        if (msg) delete msg;    
    }
    while (!done);
}

void get_server_death (void)
{
    int count;
    MPI_Status status;

    printf ("[%s] waiting for death message.\n", MyName);
    fflush (stdout);

    msg = NULL;
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
            printf ("[%s] process death successfully. Nid=%d, Pid=%d\n",
                    MyName,
                    msg->u.request.u.death.nid, msg->u.request.u.death.pid);
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessDeath message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] Invalid process death message\n", MyName);
    }
    fflush (stdout);
    delete msg;
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
    int i;
    int j;
    int rc;
    int len;
    bool worker_up[MAX_WORKERS];
    int sendbuf[3];
    char errbuf[MPI_MAX_ERROR_STRING + 1];
    char recvbuf[100];
    MPI_Comm manager_comm[MAX_WORKERS];
    MPI_Status status;

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

    process_startup (argc, argv);

    sleep (2);
    for (i = 0; i < MAX_WORKERS; i++)
    {
        switch (i)
        {
        case 0:
//              start_process(-1,"$SERV0");
            open_server ((char *) "$SERV0", WorkerPort[i], &manager_comm[i]);
            break;
        case 1:
//              start_process(-1,"$SERV1");
            open_server ((char *) "$SERV1", WorkerPort[i], &manager_comm[i]);
            break;
        case 2:
            start_process (1, (char *) "$SERV2");
            open_server ((char *) "$SERV2", WorkerPort[i], &manager_comm[i]);
            break;
        case 3:
            start_process (1, (char *) "$SERV3");
            open_server ((char *) "$SERV3", WorkerPort[i], &manager_comm[i]);
            break;
        default:
            printf ("[%s] TOO MANY WORKERS.\n", MyName);
            fflush (stdout);
            exit (1);
        }
        if (manager_comm[i] != MPI_COMM_NULL)
        {
            worker_up[i] = true;
            printf ("[%s] worker %d connected.\n", MyName, i);
            fflush (stdout);
        }
        else
        {
            worker_up[i] = false;
            printf ("[%s] worker %d failed to connected.\n", MyName, i);
            fflush (stdout);
        }
    }

    // do the work
    printf ("[%s] Starting work with %d workers\n", MyName, MAX_WORKERS);
    fflush (stdout);
    for (j = 0; j < MAX_CYCLES + 1; j++)
    {
        for (i = 0; i < MAX_WORKERS; i++)
        {
//          printf("[%s] Sending from process %d to worker %d, try %d\n", MyName, MyRank, i, j); fflush(stdout);
            if (worker_up[i])
            {
                sendbuf[0] = MyRank;
                sendbuf[1] = i;
                switch (j)
                {
                case 1:
                    sendbuf[2] = CMD_CONT;
//                  sendbuf[2] = ( i == 1 ? CMD_ABORT : CMD_CONT );
                    break;
                case MAX_CYCLES:
                    sendbuf[2] = CMD_END;
                    break;
                default:
                    sendbuf[2] = CMD_CONT;
                }
                rc = MPI_Sendrecv (sendbuf, 3, MPI_INT, 0, USER_TAG,
                                   recvbuf, 100, MPI_CHAR, MPI_ANY_SOURCE,
                                   USER_TAG, manager_comm[i], &status);
                if (rc != MPI_SUCCESS)
                {
                    MPI_Error_string (rc, errbuf, &len);
                    errbuf[len] = '\0';
                    printf
                        ("[%s] SendRecv failed to worker %d, rc = (%d)%s\n",
                         MyName, i, rc, errbuf);
                    fflush (stdout);
                    MPI_Comm_disconnect (&manager_comm[i]);
                    sleep (5);
                    switch (i)
                    {
                    case 0:
                        open_server ((char *) "$SERV0", WorkerPort[i],
                                     &manager_comm[i]);
                        break;
                    case 1:
                        open_server ((char *) "$SERV1", WorkerPort[i],
                                     &manager_comm[i]);
                        break;
                    case 2:
                        open_server ((char *) "$SERV2", WorkerPort[i],
                                     &manager_comm[i]);
                        break;
                    case 3:
                        open_server ((char *) "$SERV3", WorkerPort[i],
                                     &manager_comm[i]);
                        break;
                    default:
                        printf ("[%s] TOO MANY WORKERS.\n", MyName);
                        fflush (stdout);
                        exit (1);
                    }
                }
            }
            else
            {
                printf ("[%s] Not sending to worker %d\n", MyName, i);
                fflush (stdout);
            }
        }
    }

    // close the server processes
    for (i = 0; i < MAX_WORKERS; i++)
    {
        if (worker_up[i])
        {
            switch (i)
            {
            case 0:
                close_server ((char *) "$SERV0", &manager_comm[i]);
                break;
            case 1:
                close_server ((char *) "$SERV1", &manager_comm[i]);
                break;
            case 2:
                close_server ((char *) "$SERV2", &manager_comm[i]);
                break;
            case 3:
                close_server ((char *) "$SERV3", &manager_comm[i]);
                break;
            }
        }
    }

    // get the server death messages
    for (i = 0; i < MAX_WORKERS; i++)
    {
        switch (i)
        {
        case 0:
//          get_server_death();
            break;
        case 1:
//          get_server_death();
            break;
        case 2:
            get_server_death ();
            break;
        case 3:
            get_server_death ();
            break;
        default:
            ;
        }
    }

    // exit my process
    exit_process ();

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port (MyPort);
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
