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

#define CYCLES_MAX    20
#define TAKEOVER_1     5
#define TAKEOVER_2    10
#define TAKEOVER_3    15

// Server message tags
#define USER_TAG 100

// Server process commands
#define CMD_CONT 1
#define CMD_END  2
#define CMD_ABORT 3

char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
int MyRank = -1;
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
int TestNum = 1;
bool death = false;
MPI_Comm Monitor = MPI_COMM_NULL;
MPI_Comm Server = MPI_COMM_NULL;
MPI_Errhandler CommHandler;
MPI_Request Request[3] = { MPI_REQUEST_NULL, MPI_REQUEST_NULL, MPI_REQUEST_NULL };
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

//forward procedures
char *display_error (int err);
void flush_incoming_msgs( void );
int sendrecv (MPI_Comm comm, void *sbuf, int ssize, MPI_Datatype stype, 
              void *rbuf, int rsize, MPI_Datatype rtype, int stag, int rtag, MPI_Status *status=NULL);
void wait_for_death_notice (void);

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

bool check_notice (struct message_def *notice, MPI_Status * status)
{
    int count;

    MPI_Get_count (status, MPI_CHAR, &count);
    if ((status->MPI_TAG == NOTICE_TAG) &&
        (count == sizeof (struct message_def)) &&
        (notice->u.request.type == ReqType_Notice))
    {
        switch (notice->type)
        {
        case MsgType_Close:    // process close notification
        case MsgType_Open:     // process open notification
            printf ("[%s] Open/Close notices not currently supported.\n",
                    MyName);
            break;
        case MsgType_NodeDown: // node is down notification
        case MsgType_NodeUp:   // node is up notification
            printf ("[%s] Node Up/Down notices not currently supported.\n",
                    MyName);
            break;
        case MsgType_ProcessDeath: // process death notification
            printf ("[%s] ProcessDeath notice received from %d,%d.\n",
                    MyName, 
                    notice->u.request.u.death.nid,
                    notice->u.request.u.death.pid);
            death = true;        
            break;
        case MsgType_Service:  // request a service from the monitor
        default:
            printf ("[%s] Invalid notice message type received.\n",
                    MyName);
        }
    }
    else
    {
        printf ("[%s] Notice size/type mismatch.\n", MyName);
    }
    fflush (stdout);

    return death;
}

void CommRecovery (MPI_Comm * comm, int *err, ...)
{
    int rc;
    int mpi_err = *err & 0x000000FF;

    if (*comm == Server)
    {
        printf ("[%s] Server Communicator failed, err=%s\n", MyName, display_error(*err));
        fflush (stdout);
        if( Request[0] != MPI_REQUEST_NULL )
        {
            printf ("[%s] Canceling Irecv notice on monitor\n", MyName );
            rc = MPI_Cancel (&Request[0]);
            if( rc != MPI_SUCCESS )
            {
                printf ("[%s] Irecv Cancel on monitor failed, rc = %s\n", MyName, display_error(rc) );
            }
            if( Request[0] != MPI_REQUEST_NULL )
            {
                MPI_Request_free(&Request[0]);
            }
            Request[0] = MPI_REQUEST_NULL;
        }
        if( Request[1] != MPI_REQUEST_NULL )
        {
            printf ("[%s] Canceling Isend on comm\n", MyName );
            rc = MPI_Cancel (&Request[1]);
            if( rc != MPI_SUCCESS )
            {
                printf ("[%s] Isend Cancel on comm failed, rc = %s\n", MyName, display_error(rc) );
            }
            if( Request[1] != MPI_REQUEST_NULL )
            {
                MPI_Request_free(&Request[1]);
            }
            Request[1] = MPI_REQUEST_NULL;
        }
        if( Request[2] != MPI_REQUEST_NULL )
        {
            printf ("[%s] Canceling Irecv on comm\n", MyName );
            rc = MPI_Cancel (&Request[2]);
            if( rc != MPI_SUCCESS )
            {
                printf ("[%s] Irecv Cancel on comm failed, rc = %s\n", MyName, display_error(rc) );
            }
            if( Request[2] != MPI_REQUEST_NULL )
            {
                MPI_Request_free(&Request[2]);
            }
            Request[2] = MPI_REQUEST_NULL;
        }
        MPI_Comm_disconnect(&Server);
        Server = MPI_COMM_NULL;
    }
    else if (*comm == MPI_COMM_WORLD)
    {
        printf ("[%s] MPI_COMM_WORLD Communicator failed, err=%s, aborting.\n", MyName, display_error(*err));
        fflush (stdout);
//      exit (mpi_err);
    }
    else
    {
        printf ("[%s] Unknown Communicator failed, err=%s, aborting.\n", MyName, display_error(*err));
        fflush (stdout);
        MPI_Abort (MPI_COMM_WORLD,mpi_err);
    }
}

char *display_error (int err)
{
    int len = 0;
    int mpi_err;
    static char errbuf[MPI_MAX_ERROR_STRING + 6];

    mpi_err = err & 0x000000FF;
    sprintf (errbuf, "(%2.2d) ", mpi_err);
    MPI_Error_string (err, &errbuf[5], &len);
    errbuf[len + 5] = '\0';

    return errbuf;
}

void close_server (char *process_name, MPI_Comm * comm)
{
    int count;
    MPI_Status status;
    struct message_def *msg = new struct message_def;

    if (comm) {} // Avoid "unused parameter warning

    printf ("[%s] closing server %s.\n", MyName, process_name);
    fflush (stdout);

    if( *comm != MPI_COMM_NULL )
    {
#ifdef NO_OPEN_CLOSE_NOTICES
        // Send close notice
        msg->type = MsgType_Close;
        msg->noreply = false;
        msg->reply_tag = CLOSE_TAG;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.close.nid = MyNid;
        msg->u.request.u.close.pid = MyPid;
        strcpy (msg->u.request.u.close.process_name, MyName);
        msg->u.request.u.close.aborted = false;
        msg->u.request.u.close.mon = true;
        MPI_Send(msg, sizeof(struct message_def), MPI_CHAR, 0, NOTICE_TAG, *comm);
#endif

        // Close communicator
        MPI_Comm_disconnect( comm );
        *comm = MPI_COMM_NULL;
    }
    
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Close;
    msg->u.request.u.close.nid = MyNid;
    msg->u.request.u.close.pid = MyPid;
    strcpy (msg->u.request.u.close.process_name, process_name);
    sendrecv (Monitor, msg, sizeof (struct message_def), MPI_CHAR,
              msg, sizeof (struct message_def), MPI_CHAR, SERVICE_TAG, REPLY_TAG, &status);
    MPI_Get_count (&status, MPI_CHAR, &count);
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
                printf ("[%s] close process failed, rc=%s\n", MyName,
                        display_error(msg->u.reply.u.generic.return_code));
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
    delete msg;
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg = new struct message_def;

    printf ("[%s] sending exit process message.\n", MyName);
    fflush (stdout);
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.exit.nid = MyNid;
    msg->u.request.u.exit.pid = MyPid;
    sendrecv (Monitor, msg, sizeof (struct message_def), MPI_CHAR,
              msg, sizeof (struct message_def), MPI_CHAR, SERVICE_TAG, REPLY_TAG, &status);
    MPI_Get_count (&status, MPI_CHAR, &count);
    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Generic))
        {
            if (msg->u.reply.u.generic.return_code == MPI_SUCCESS)
            {
                printf ("[%s] exited process successfully.\n", MyName);
            }
            else
            {
                printf ("[%s] exit process failed, rc=%s\n", MyName,
                        display_error(msg->u.reply.u.generic.return_code));
            }
            flush_incoming_msgs ();
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
    delete msg;
}

void flush_incoming_msgs( void )
{
    int count;
    int complete = 0;
    bool done = false;
    MPI_Status status;
    struct message_def *msg = NULL;

    printf ("[%s] flush incoming event & notices.\n", MyName);
    fflush (stdout);
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
 
int open_server (char *process_name, MPI_Comm * comm)
{
    int rc;
    int count;
    MPI_Status status;
    struct message_def *msg = new struct message_def;

    printf ("[%s] opening server %s.\n", MyName, process_name);
    fflush (stdout);
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Open;
    msg->u.request.u.open.nid = MyNid;
    msg->u.request.u.open.pid = MyPid;
    strcpy (msg->u.request.u.open.target_process_name, process_name);
    msg->u.request.u.open.death_notification = 1;
    sendrecv (Monitor, msg, sizeof (struct message_def), MPI_CHAR,
              msg, sizeof (struct message_def), MPI_CHAR, SERVICE_TAG, REPLY_TAG, &status);
    MPI_Get_count (&status, MPI_CHAR, &count);
    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Open))
        {
            if (msg->u.reply.u.open.return_code == MPI_SUCCESS)
            {
                printf ("[%s] opened process successfully. Nid=%d, Pid=%d, Port=%s\n",
                     MyName, 
                     msg->u.reply.u.open.nid, 
                     msg->u.reply.u.open.pid,
                     msg->u.reply.u.open.port); 
                rc = MPI_Comm_connect (msg->u.reply.u.open.port, MPI_INFO_NULL,
                                       0, MPI_COMM_SELF, comm);
                if (rc == MPI_SUCCESS)
                {
                    MPI_Comm_set_errhandler (*comm, CommHandler);
#ifdef NO_OPEN_CLOSE_NOTICES
                    // Send open notice to server
                    msg->type = MsgType_Open;
                    msg->noreply = true;
                    msg->u.request.type = ReqType_Notice;
                    msg->u.request.u.open.nid = MyNid;
                    msg->u.request.u.open.pid = MyPid;
                    strcpy (msg->u.request.u.open.target_process_name, MyName);
                    MPI_Send(msg,sizeof(struct message_def),MPI_CHAR,0,NOTICE_TAG,*comm);
#endif
                    printf ("[%s] connected to process.\n", MyName);
                    fflush (stdout);
                }
                else
                {
                    printf ("[%s] failed to connected. rc = %s\n", 
                            MyName,
                            display_error(rc) );
                }
            }
            else
            {
                rc = msg->u.reply.u.open.return_code;
                printf ("[%s] open process failed, rc = %s\n", 
                        MyName,
                        display_error(rc));
            }
        }
        else
        {
            rc = MPI_ERR_UNKNOWN;
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for open message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        rc = MPI_ERR_UNKNOWN;
        printf ("[%s] open process reply message invalid\n", MyName);
    }
    fflush (stdout);
    delete msg;

    return( rc );
}

int open_server_retry(char *process_name, MPI_Comm * comm)
{
    int rc;
    int openTries = 0;

    do
    {
        rc = open_server( process_name, comm );
        if  ( rc == MPI_SUCCESS )
        {
            break;
        }
        sleep(1);
        openTries++;
    }
    while( rc == MPI_ERR_EXITED && openTries < 10 );
    if (rc != MPI_SUCCESS)
    {
        printf ("[%s] Open server tries exceeded, aborting\n", MyName);
        MPI_Abort(MPI_COMM_WORLD,99);
    }

    return( rc );
}

void process_startup (int argc, char *argv[])
{
    int i;
    struct message_def *msg;

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
        msg->u.request.u.startup.verifier = true;
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
                switch( i )
                {
                    case 10:
                        TestNum = atoi(argv[i]);
                        printf ("(%d)", TestNum);
                        break;
                    default: // ignore
                        break;
                }
            }
        }
        printf ("\n");
        fflush (stdout);
    }
}

void recv_localio_msg(struct message_def *recv_msg, int size)
{
    MPI_Status status;

    size = size; // Avoid "unused parameter" warning

    printf("[%s] Message received: ",MyName);
    switch ( recv_msg->type )
    {
        case MsgType_Service:
            printf("Service Reply: Type=%d, ReplyType=%d\n",recv_msg->type, recv_msg->u.reply.type);
            break;
        
        case MsgType_Event:
            printf("Event - %d\n",recv_msg->u.request.u.event_notice.event_id);
            break;

        case MsgType_UnsolicitedMessage:
            printf("Unsolicited Message Received:\n");
            break;

        default:
            printf("Notice: Type=%d, RequestType=%d\n",recv_msg->type, recv_msg->u.request.type);
            status.count = sizeof (*recv_msg);
            status.MPI_TAG = NOTICE_TAG;

            check_notice( recv_msg, &status );
    }
}

int sendrecv (MPI_Comm comm, void *sbuf, int ssize, MPI_Datatype stype, 
              void *rbuf, int rsize, MPI_Datatype rtype, int stag, int rtag, MPI_Status *status)
{
    int rc;
    int idx;
    bool done = false;
    MPI_Status local_status;
    struct message_def *msg = NULL;

    death = false;
    if (status == NULL)
    {
        status = &local_status;
    }

    if ( gp_local_mon_io && !comm )
    {
        printf ("[%s] sendrecv on monitor called.\n",MyName);
        gp_local_mon_io->acquire_msg( &msg );
        memmove( msg, sbuf, gp_local_mon_io->size_of_msg( (struct message_def *)sbuf ) );       
        gp_local_mon_io->send_recv( msg );
        if (msg)
        {
            memmove( rbuf, msg, gp_local_mon_io->size_of_msg( msg ) );
            status->count = sizeof( struct message_def );
            status->MPI_TAG = msg->reply_tag;
            rc = MPI_SUCCESS;
        }
        else
        {
            rc = MPI_ERR_REQUEST;
        }
        gp_local_mon_io->release_msg(msg);
    }
    else
    {
        if (Request[1] == MPI_REQUEST_NULL)
        {
            printf ("[%s] MPI_Isend on comm called.\n", MyName);
            rc = MPI_Isend (sbuf, ssize, stype, 0, stag, comm, &Request[1]);
            if (rc != MPI_SUCCESS)
            {
                printf ("[%s] Monitor MPI_Isend failure, rc=%s\n",
                        MyName, display_error (rc));
                done = true;
            }
        }
        else
        {
            printf ("[%s] MPI_Isend overrun.\n", MyName);
        }
            
        do
        {
            if (comm != MPI_COMM_NULL)
            {
                if (Request[2] == MPI_REQUEST_NULL)
                {
                    printf ("[%s] MPI_Irecv on comm called.\n", MyName);
                    rc = MPI_Irecv (rbuf, rsize, rtype, 0, rtag, comm, &Request[2]);
                    if (rc != MPI_SUCCESS)
                    {
                        printf ("[%s] Client MPI_Irecv failure, rc=%s\n",
                                MyName, display_error (rc));
                    }
                }
            }
            fflush(stdout);
            idx = MPI_UNDEFINED;
            rc = MPI_Waitany (3, Request, &idx, status);
            if (rc == MPI_SUCCESS)
            {
                switch(idx)
                {
                case 1:
                    printf("[%s] Isend to comm completed\n",MyName);
                    done = true;
                    break;
                case 2:
                    printf("[%s] Irecv from comm completed\n",MyName);
                    break;
                default:
                    printf("[%s] Undefined completion index\n",MyName);
                }
            }
            else
            {
                printf ("[%s] MPI_Waitany failed, rc=%s\n", MyName, display_error (rc));
                break;
            }
            fflush(stdout);
        }
        while ( !((idx == 2 && done) || death || idx == MPI_UNDEFINED) );

        if (idx == 0 || idx == MPI_UNDEFINED)
        {
            if( Request[1] != MPI_REQUEST_NULL )
            {
                printf ("[%s] Canceling Isend on comm\n", MyName );
                rc = MPI_Cancel (&Request[1]);
                if( rc != MPI_SUCCESS )
                {
                    printf ("[%s] Isend Cancel failed, rc = %s\n", MyName, display_error(rc) );
                }
                Request[1] = MPI_REQUEST_NULL;
            }
            if( Request[2] != MPI_REQUEST_NULL )
            {
                printf ("[%s] Canceling Irecv on comm\n", MyName );
                rc = MPI_Cancel (&Request[2]);
                if( rc != MPI_SUCCESS )
                {
                    printf ("[%s] Irecv Cancel failed, rc = %s\n", MyName, display_error(rc) );
                }
                Request[2] = MPI_REQUEST_NULL;
            }
        
            rc = MPI_ERR_EXITED;
        }
    }
    fflush(stdout);

    return rc;
}

void test1(void)
{
    int i;
    int rc;
    int tries = 0;
    int sendbuf[3];
    char recvbuf[100];

    for (i = 0; i < CYCLES_MAX + 1; i++)
    {
        sendbuf[0] = MyRank;
        sendbuf[1] = i;
        switch (i)
        {
        case TAKEOVER_1:  // abort the primary
        case TAKEOVER_2: // aborts the original backup after takeover due to TAKEOVER_1
        case TAKEOVER_3: // aborts the primary after takeover due to TAKEOVER_2
            sendbuf[2] = CMD_ABORT;
            break;
        case CYCLES_MAX:
            sendbuf[2] = CMD_END;
            break;
        default:
            sendbuf[2] = CMD_CONT;
        }
        rc = sendrecv (Server, sendbuf, 3, MPI_INT, recvbuf, 100, MPI_CHAR, USER_TAG, USER_TAG);
        if (rc == MPI_SUCCESS)
        {
            printf ("[%s] Cycle # %d - %s\n", MyName, i, recvbuf);
            fflush (stdout);
        }
        else
        {
            printf ("[%s] Cycle # %d - failed, rc=%s\n", MyName, i, display_error (rc));
            printf ("[%s] Re-Connecting to server\n", MyName);
            fflush (stdout);
            tries++;
            if ( tries > 3 )
            {
                printf ("[%s] Tries exceeded, aborting\n", MyName);
                MPI_Abort(MPI_COMM_WORLD,99);
            }
            if (!death)
            {      
                wait_for_death_notice();
            }
            close_server((char *) "$SERV0", &Server);
            sleep(1);
            open_server_retry((char *) "$SERV0", &Server);
            i--;
        }
    }
}

void test2(void)
{
    int i = 0;
    int rc;
    int sendbuf[3];
    char recvbuf[100];

    do
    {
        sendbuf[0] = MyRank;
        sendbuf[1] = 1;
        sendbuf[2] = CMD_CONT;
        rc = sendrecv (Server, sendbuf, 3, MPI_INT, recvbuf, 100, MPI_CHAR, USER_TAG, USER_TAG);
        if (rc == MPI_SUCCESS)
        {
            printf ("[%s] Send # 1 - %s\n", MyName, recvbuf);
            fflush (stdout);
        }
        else
        {
            i++;
            printf ("[%s] Cycle # %d - failed, rc=%s\n", MyName, i, display_error (rc));
            printf ("[%s] Re-Connecting to server\n", MyName);
            fflush (stdout);
            if (!death)
            {      
                wait_for_death_notice();
            }
            close_server((char *) "$SERV0", &Server);
            sleep(1);
            open_server_retry((char *) "$SERV0", &Server);
        }
    } while ( rc != MPI_SUCCESS && i < 30 );

    sendbuf[0] = MyRank;
    sendbuf[1] = 2;
    sendbuf[2] = CMD_CONT;
    rc = sendrecv (Server, sendbuf, 3, MPI_INT, recvbuf, 100, MPI_CHAR, USER_TAG, USER_TAG);
    if (rc == MPI_SUCCESS)
    {
        printf ("[%s] Send # 2 - %s\n", MyName, recvbuf);
        fflush (stdout);
    }
    else
    {
        printf ("[%s] FAILED - final send to server, rc=%s\n", MyName, display_error (rc));
        printf ("[%s] Aborting\n", MyName);
        fflush (stdout);
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    
}

void wait_for_death_notice (void)
{
    struct message_def *notice = new message_def;

    printf ("[%s] waiting for death Notice->\n", MyName);
    fflush (stdout);
    while (!death)
    {
        // just wait for call back to receive notice.
        usleep(1000);

    }
    fflush (stdout);
    delete notice;
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

    process_startup (argc, argv);

    MPI_Errhandler_create (CommRecovery, &CommHandler);
    MPI_Comm_set_errhandler (MPI_COMM_WORLD, CommHandler);

    open_server ((char *) "$SERV0", &Server);
    if ( Server == MPI_COMM_NULL )
    {
        printf ("[%s] Can't connect to server\n", MyName);
        exit (1);
    }

    // do the work
    printf ("[%s] Starting work with server\n", MyName);
    fflush (stdout);

    switch ( TestNum )
    {
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;
        default:
            printf ("[%s] FAILED - Invalid test number (%d)\n", MyName, TestNum);
            printf ("[%s] Aborting\n", MyName);
            fflush (stdout);
            MPI_Abort(MPI_COMM_WORLD,1);
            break;
    }
    // close the server processes
    close_server ((char *) "$SERV0", &Server);
    //wait_for_death_notice();

    // exit my process
    exit_process ();

    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port(MyPort);
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    MPI_Errhandler_free( &CommHandler );
    MPI_Finalize ();

    exit (0);
}
