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
#include "msgdef.h"
#include <sys/time.h>
#include "props.h"
#include "localio.h"
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"

long trace_settings = 0;

// Server message tags
#define USER_TAG 100

// Server process commands
#define CMD_CONT  1
#define CMD_END   2
#define CMD_ABORT 3

// global variables
int MyRank = -1;
char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
char DisplayName[MAX_PROCESS_NAME+MPI_MAX_PORT_NAME];
char DisplayPid[MPI_MAX_PORT_NAME];
char MonitorPort[MPI_MAX_PORT_NAME];
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
PROCESSTYPE process_type = ProcessType_Generic;
int PeerNid = -1;
int PeerPid = -1;
int OpenCount = 0;
int TakeoverCount = 0;
int TakeoverCountMax = 0;
bool ImBackup = false;
bool Takeover = false;
bool ClientOpenNotice = false;
bool PeerOpenNotice = false;
MPI_Comm Comm = MPI_COMM_NULL;
MPI_Comm Client = MPI_COMM_NULL;
MPI_Comm Monitor = MPI_COMM_NULL;
MPI_Comm Peer = MPI_COMM_NULL;
MPI_Errhandler PeerHandler;
MPI_Request Request[2] = { MPI_REQUEST_NULL, MPI_REQUEST_NULL };
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

void kill_peer (void);
char *display_error (int err);
void flush_incoming_msgs( void );
int recv (MPI_Comm * comm, void *buf, int size, MPI_Datatype type, int tag, MPI_Status *status);
void start_backup (int nid);
bool takeover (void);
void wait_for_open_notice (MPI_Comm *comm);
void wait_for_client_open(void);
void wait_for_peer_open(void);

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

bool check_notice (struct message_def *notice, MPI_Status *status)
{
    int count;
    bool done = false;

    MPI_Get_count (status, MPI_CHAR, &count);
    if ((status->MPI_TAG == NOTICE_TAG) &&
        (count == sizeof (struct message_def)) &&
        (notice->u.request.type == ReqType_Notice))
    {
        switch (notice->type)
        {
        case MsgType_Close:    // process close notification
            printf ("[%s] Close notice received from %s.\n", DisplayName,
                    notice->u.request.u.close.process_name);
            if (strcmp (MyName, notice->u.request.u.close.process_name) == 0)
            {
                printf("[%s] Disconnecting Communicator to Peer\n", DisplayName );
                MPI_Comm_disconnect( &Peer );
                Peer = MPI_COMM_NULL;
            }
            else
            {
                OpenCount--;
                if (OpenCount <= 0)
                {
                    done = true;
                }
                printf("[%s] Disconnecting from client\n", DisplayName );
                MPI_Comm_disconnect( &Client );
                Client = MPI_COMM_NULL;
            }
            break;
        case MsgType_NodeDown: // node is down notification
        case MsgType_NodeUp:   // node is up notification
            printf ("[%s] Node Up/Down notices not currently supported.\n",
                    DisplayName);
            break;
        case MsgType_Open:     // process open notification
            printf ("[%s] Open notice received from %s.\n", DisplayName,
                    notice->u.request.u.open.target_process_name);
            if (strcmp (MyName, notice->u.request.u.open.target_process_name) == 0)
            {
                PeerOpenNotice = true;
#ifndef NO_OPEN_CLOSE_NOTICES
                printf ("[%s] Waiting for backup to connect.\n", DisplayName);
                fflush (stdout);
                MPI_Comm_accept (MyPort, MPI_INFO_NULL, 0, MPI_COMM_SELF, &Peer);
                MPI_Comm_set_errhandler (Peer, PeerHandler);
                printf ("[%s] check_notice() - Backup Connected.\n", DisplayName);
                fflush (stdout);
#endif
            }
            else
            {
                ClientOpenNotice = true;
                OpenCount++;
#ifndef NO_OPEN_CLOSE_NOTICES
                printf ("[%s] Waiting for Client to connect.\n", DisplayName);
                fflush (stdout);
                MPI_Comm_accept (MyPort, MPI_INFO_NULL, 0, MPI_COMM_SELF, &Client);
                MPI_Comm_set_errhandler (Client, PeerHandler);
                printf ("[%s] check_notice() - Client connected.\n", DisplayName);
                fflush (stdout);
#endif
            }
            break;
        case MsgType_ProcessDeath: // process death notification
            printf ("[%s] ProcessDeath notice received from %d,%d.\n",
                    DisplayName, notice->u.request.u.death.nid,
                    notice->u.request.u.death.pid);
            if ((PeerNid == notice->u.request.u.death.nid)
                && (PeerPid == notice->u.request.u.death.pid))
            {
                Takeover = takeover ();
            }
            break;
        case MsgType_Service:  // request a service from the monitor
        default:
            printf ("[%s] Invalid notice message type received.\n",
                    DisplayName);
        }
    }
    else
    {
        printf ("[%s] Notice size/type mismatch.\n", DisplayName);
    }
    fflush (stdout);

    return done;
}

int checkpoint (void *buf, int *size, MPI_Datatype type)
{
    MPI_Status status;
    int rc = 0;

    if ( Peer != MPI_COMM_NULL )
    {
        if (ImBackup)
        {
            printf ("[%s] Waiting for checkpoint.\n", DisplayName);
            rc = recv (&Peer, buf, *size, type, CHKPNT_TAG, &status);
            if (rc != MPI_SUCCESS)
            {
                printf ("[%s] checkpoint receive failed, rc = %s\n", DisplayName, display_error (rc));
                sleep(1);
            }
        }
        else
        {
            printf ("[%s] Sending checkpoint.\n", DisplayName);
            rc = MPI_Send (buf, *size, type, 0, CHKPNT_TAG, Peer);
            if (rc != MPI_SUCCESS)
            {
                printf ("[%s] checkpoint send failed, rc = %s\n", DisplayName, display_error (rc));
            }
        }
    }
    
    return rc;
}

void close_primary ( void )
{
    int rc;
    int count;
    MPI_Status status;
    struct message_def *msg;

    printf ("[%s] closing primary server.\n", DisplayName);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

#ifdef NO_OPEN_CLOSE_NOTICES
    if( Peer != MPI_COMM_NULL )
    {
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
        MPI_Send(msg, sizeof(struct message_def), MPI_CHAR, 0, NOTICE_TAG, Peer);
    }
#endif

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Close;
    msg->u.request.u.close.nid = MyNid;
    msg->u.request.u.close.pid = MyPid;
    strcpy (msg->u.request.u.close.process_name, MyName);
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
                    ("[%s] closed primary successfully. Nid=%d, Pid=%d, rtn=%d\n",
                     DisplayName, msg->u.reply.u.generic.nid,
                     msg->u.reply.u.generic.pid,
                     msg->u.reply.u.generic.return_code);
                rc = MPI_Comm_disconnect (&Peer);
                if (rc == MPI_SUCCESS)
                {
                    printf ("[%s] Disconnected Communicator from peer.\n", DisplayName);
                    fflush (stdout);
                }
                else
                {
                    printf ("[%s] backup failed to disconnected. rc = (%d)\n",
                            DisplayName, rc);
                    fflush (stdout);
                }
            }
            else
            {
                printf ("[%s] close primary failed, rc=%d\n", DisplayName,
                        msg->u.reply.u.generic.return_code);
                MPI_Comm_disconnect (&Peer);
            }
        }
        else
        {
            printf ("[%s] Invalid MsgType(%d)/ReplyType(%d) for close message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] close process reply message invalid\n", DisplayName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

char *display_error (int err)
{
    int len;
    int mpi_err;
    static char errbuf[MPI_MAX_ERROR_STRING + 6];

    mpi_err = err & 0x000000FF;
    sprintf (errbuf, "(%2.2d) ", mpi_err);
    MPI_Error_string (err, &errbuf[5], &len);
    errbuf[len + 5] = '\0';

    return errbuf;
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    if (PeerPid != -1)
    {
        printf ("[%s] stopping backup.\n", DisplayName);
        kill_peer ();
    }
    printf ("[%s] sending exit process message.\n", DisplayName);
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
                printf ("[%s] exited process successfully.\n", DisplayName);
            }
            else
            {
                printf ("[%s] exit process failed, rc=%s\n", DisplayName,
                        display_error (msg->u.reply.u.generic.return_code));
            }
            flush_incoming_msgs ();
            MPI_Comm_disconnect(&Monitor);
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] exit process reply invalid.\n", DisplayName);
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
    struct message_def *msg = NULL;

    printf ("[%s] flush incoming event & notices.\n", DisplayName);
    fflush (stdout);
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if (msg) 
        {
            printf("[%s] Got local IO notice\n",DisplayName);
            complete = true;
            count = sizeof( *msg );
            status.MPI_TAG = msg->reply_tag;
        }
        else
        {
            printf("[%s] No local IO notice\n",DisplayName);
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
                                DisplayName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        else
                        {
                            printf ("[%s] Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                DisplayName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        break;

                    case MsgType_NodeDown:
                        printf ("[%s] Node %d (%s) is DOWN\n", 
                            DisplayName, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        printf ("[%s] Node %d (%s) is UP\n", 
                            DisplayName, msg->u.request.u.up.nid, msg->u.request.u.up.node_name);
                        break;    

                    case MsgType_Change:
                        printf ("[%s] Configuration Change Notice for Group: %s Key: %s\n", 
                            DisplayName, 
                            msg->u.request.u.change.group,
                            msg->u.request.u.change.key);
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        printf ("[%s] Open/Close process notification\n", DisplayName);
                        break;
                    
                    case MsgType_Event:
                        printf("[%s] Event %d received\n",
                            DisplayName, msg->u.request.u.event_notice.event_id);
                        break;

                    case MsgType_Shutdown:
                        printf("[%s] Shutdown notice, level=%d received\n",
                            DisplayName, msg->u.request.u.shutdown.level);
                        break;
                        
                    default:
                        printf("[%s] Invalid Notice Type(%d) for flush message\n",
                            DisplayName, msg->type);
                    }
                }    
                else
                {
                    printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                        DisplayName, msg->type, msg->noreply, msg->u.request.type);
                }
            }
            else
            {
                printf ("[%s] Failed to flush messages\n", DisplayName);
                done = true;
            }
            fflush (stdout);
        }
        if (msg) delete msg;    
    }
    while (!done);
}
 
void get_open_notice( void )
{
    bool done = false;
    int rc;
    MPI_Status status;
    struct message_def *notice = new struct message_def;

    printf ("[%s] Waiting for connection.\n", DisplayName);
    fflush (stdout);
    Comm = MPI_COMM_NULL;
    MPI_Comm_accept (MyPort, MPI_INFO_NULL, 0, MPI_COMM_SELF, &Comm);
    MPI_Comm_set_errhandler (Comm, PeerHandler);
    printf ("[%s] get_connection() - Got connection.\n", DisplayName);
    fflush (stdout);

    if (Comm != MPI_COMM_NULL)
    {
        do
        {
            printf ("[%s] MPI_Recv on Comm called.\n", DisplayName);
            fflush (stdout);
            rc = MPI_Recv (notice, sizeof(struct message_def), MPI_CHAR, 0, MPI_ANY_TAG, Comm, &status);
            if (rc != MPI_SUCCESS)
            {
                printf ("[%s] MPI_Recv failure on Comm, rc=%s\n", DisplayName, display_error (rc));
                fflush (stdout);
                MPI_Abort(MPI_COMM_WORLD,1);
            }
            else
            {
                if (status.MPI_TAG == NOTICE_TAG)
                {
                    check_notice (notice, &status);
                    done = true;
                }
                else
                {
                    printf ("[%s] ERROR - MPI_Recv got an unknown tag.\n", DisplayName);
                    fflush (stdout);
                    MPI_Abort(MPI_COMM_WORLD,1);
                }
            }
        }
        while ( !done );
    }
    else
    {
        printf ("[%s] ERROR - Comm handle is null\n", DisplayName);
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    
}

void get_client_open( void )
{
    printf ("[%s] get_client_open() - Waiting for Client to connect.\n", DisplayName);
    fflush (stdout);
    if ( Client == MPI_COMM_NULL )
    {
        wait_for_client_open();
    }

    printf ("[%s] get_client_open() - Client connected.\n", DisplayName);
    fflush (stdout);
}

void get_peer_open( void )
{
    printf ("[%s] get_peer_open() - Waiting for Peer to connect.\n", DisplayName);
    fflush (stdout);
    if ( Peer == MPI_COMM_NULL )
    {
        
        wait_for_peer_open();
    }

    printf ("[%s] get_peer_open() - Peer Connected.\n", DisplayName);
    fflush (stdout);
}

bool isBackup (void)
{
    int count;
    bool backup = false;
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
    msg->u.request.u.process_info.target_nid = MyNid;
    msg->u.request.u.process_info.target_pid = MyPid;
    msg->u.request.u.process_info.target_verifier = -1;
    strcpy (msg->u.request.u.process_info.target_process_name, MyName);
    msg->u.request.u.process_info.type = ProcessType_Undefined;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_ProcessInfo))
        {
            if (msg->u.reply.u.process_info.return_code == MPI_SUCCESS)
            {
                if (msg->u.reply.u.process_info.num_processes == 0)
                {
                    printf ("[%s] Error: ProcessInfo returned no processes\n", DisplayName);
                }
                if (msg->u.reply.u.process_info.num_processes > 1)
                {
                    printf
                        ("[%s] Error: ProcessInfo returned more than 1 process\n",
                         DisplayName);
                }
                process_type = msg->u.reply.u.process_info.process[0].type;
                backup = msg->u.reply.u.process_info.process[0].backup;
            }
            else
            {
                printf ("[%s] ProcessInfo failed, rc=%s\n",
                        DisplayName,
                        display_error (msg->u.reply.u.process_info.
                                       return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for ProcessInfo message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid\n", DisplayName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

    return backup;
}

void kill_peer (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

 //DJW I'm deciding dup.   gp_local_mon_io->acquire_msg( &msg );

    printf ("[%s] sending kill process message.\n", DisplayName);
    fflush (stdout);
//    MPI_Comm_disconnect( &Peer );
//DJW Is this an error of duplicated code.  2 acquire msgs back to back?

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Kill;
    msg->u.request.u.kill.nid = MyNid;
    msg->u.request.u.kill.pid = MyPid;
    msg->u.request.u.kill.target_nid = PeerNid;
    msg->u.request.u.kill.target_pid = PeerPid;
    strcpy (msg->u.request.u.kill.target_process_name, MyName);

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
                printf ("[%s] Kill failed, rc=%s\n",
                        DisplayName,
                        display_error (msg->u.reply.u.generic.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Kill message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] kill reply message invalid\n", DisplayName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

}

void mount_device (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;
  
    printf ("[%s] sending mount device message.\n", DisplayName);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Mount;
    msg->u.request.u.mount.nid = MyNid;
    msg->u.request.u.mount.pid = MyPid;

    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;

    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Mount))
        {
            if (msg->u.reply.u.mount.return_code == MPI_SUCCESS)
            {
                printf ("[%s] mount device successfully.\n", DisplayName);
            }
            else
            {
                printf ("[%s] mount device failed, rc=%s\n", DisplayName,
                        display_error(msg->u.reply.u.mount.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Mount message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] mount device reply invalid.\n", DisplayName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

}

void open_primary (void)
{
    int rc;
    int count;
    MPI_Status status;
    struct message_def *msg;
  
    printf ("[%s] opening primary server.\n", DisplayName);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Open;
    msg->u.request.u.open.nid = MyNid;
    msg->u.request.u.open.pid = MyPid;
    strcpy (msg->u.request.u.open.target_process_name, MyName);
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
                    ("[%s] opened primary process successfully. Nid=%d, Pid=%d, Port=%s, rtn=%d\n",
                     DisplayName, msg->u.reply.u.open.nid,
                     msg->u.reply.u.open.pid, msg->u.reply.u.open.port,
                     msg->u.reply.u.open.return_code);
                PeerNid = msg->u.reply.u.open.nid;
                PeerPid = msg->u.reply.u.open.pid;
                rc = MPI_Comm_connect (msg->u.reply.u.open.port, MPI_INFO_NULL,
                                       0, MPI_COMM_SELF, &Peer);
                if (rc == MPI_SUCCESS)
                {
                    MPI_Comm_set_errhandler (Peer, PeerHandler);
#ifdef NO_OPEN_CLOSE_NOTICES
                    msg->type = MsgType_Open;
                    msg->noreply = true;
                    msg->u.request.type = ReqType_Notice;
                    msg->u.request.u.open.nid = MyNid;
                    msg->u.request.u.open.pid = MyPid;
                    strcpy (msg->u.request.u.open.target_process_name, MyName);
                    MPI_Send(msg,sizeof(struct message_def),MPI_CHAR,0,NOTICE_TAG,Peer);
#endif
                    printf ("[%s] Primary Connected.\n", DisplayName);
                    fflush (stdout);
                }
                else
                {
                    printf ("[%s] failed to connected. rc=%s\n", DisplayName,
                            display_error (rc));
                    fflush (stdout);
                }
            }
            else
            {
                printf ("[%s] open primary process failed, rc=%s\n",
                        DisplayName,
                        display_error (msg->u.reply.u.open.return_code));
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for open message\n",
                 DisplayName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] open primary process reply message invalid\n",
                DisplayName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

void PeerRecovery (MPI_Comm * comm, int *err, ...)
{
    int len;
    int mpi_err;
    char errbuf[MPI_MAX_ERROR_STRING + 1];

    mpi_err = *err & 0x000000FF;
    MPI_Error_string (*err, errbuf, &len);
    errbuf[len] = '\0';
    if (*comm == Client)
    {
        printf ("[%s] Client Communicator failed, err=(%d) %s\n", DisplayName, mpi_err, errbuf);
        fflush (stdout);
    }
    else if (*comm == Peer)
    {
        printf ("[%s] Peer Communicator failed, err=(%d) %s\n", DisplayName, mpi_err, errbuf);
        fflush (stdout);
    }
    else if (*comm == Monitor)
    {
        printf ("[%s] Monitor Communicator failed, err=(%d) %s, aborting.\n", DisplayName, mpi_err, errbuf);
        fflush (stdout);
        MPI_Abort (Monitor, mpi_err);
    }
    else if (*comm == MPI_COMM_WORLD)
    {
        printf ("[%s] MPI_COMM_WORLD Communicator failed, err=(%d) %s, aborting.\n", DisplayName, mpi_err, errbuf);
//      exit (mpi_err);
    }
    else
    {
        printf ("[%s] Unknown Communicator failed, err=(%d) %s, aborting.\n", DisplayName, mpi_err, errbuf);
        MPI_Abort(MPI_COMM_WORLD,mpi_err);
    }
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
                printf(", argv[%d]=%s", i, argv[i]);
                switch( i )
                {
                    case 10:
                        TakeoverCountMax = atoi(argv[i]);
                        printf ("(%d)", TakeoverCountMax);
                        break;
                    case 11:
                        TakeoverCount = atoi(argv[i]);
                        printf ("(%d)", TakeoverCount);
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

    printf("[%s] recv_localio_msg() - Message received: ",DisplayName);
    switch ( recv_msg->type )
    {
        case MsgType_Service:
            printf("Service Reply: Type=%d, ReplyType=%d\n",recv_msg->type, recv_msg->u.reply.type);
            fflush (stdout);
            break;
        
        case MsgType_Event:
            printf("Event - %d\n",recv_msg->u.request.u.event_notice.event_id);
            fflush (stdout);
            break;

        case MsgType_UnsolicitedMessage:
            printf("Unsolicited Message Received:\n");
            fflush (stdout);
            break;

        default:
            printf("Notice: Type=%d, RequestType=%d\n",recv_msg->type, recv_msg->u.request.type);
            fflush (stdout);
            status.count = sizeof (*recv_msg);
            status.MPI_TAG = NOTICE_TAG;

            check_notice( recv_msg, &status );
    }
}

int recv (MPI_Comm * comm, void *buf, int size, MPI_Datatype type, int tag, MPI_Status *status)
{
    int rc;
    int idx;
    int len;
    bool done;
    struct message_def *notice = new struct message_def;
    //DJW Not sure how to clean this up right now.
    if ( gp_local_mon_io )
    {
        if (*comm != MPI_COMM_NULL)
        {
            do
            {
                printf ("[%s] MPI_Recv on comm called.\n", DisplayName);
                rc = MPI_Recv (notice, sizeof(struct message_def), MPI_CHAR, 0, MPI_ANY_TAG, *comm, status);
                if (rc != MPI_SUCCESS)
                {
                    printf ("[%s] MPI_Recv failure on comm, rc=%s\n", DisplayName, display_error (rc));
                    done = true;
                }
#ifdef NO_OPEN_CLOSE_NOTICES
                else
                {
                    if (status->MPI_TAG == USER_TAG)
                    {
                        MPI_Get_count (status, MPI_CHAR, &len);
                        memcpy(buf, notice, len);
                        done = true;
                    }
                    else if (status->MPI_TAG == NOTICE_TAG)
                    {
                        check_notice (notice, status);
                        done = false;
                    }
                    else if (status->MPI_TAG == CHKPNT_TAG)
                    {
                        MPI_Get_count (status, MPI_CHAR, &len);
                        memcpy(buf, notice, len);
                        printf ("[%s] MPI_Recv got a CHKPNT_TAG tag.\n", DisplayName);
                        done = true;
                        //done = false;
                    }
                    else
                    {
                        printf ("[%s] ERROR - MPI_Recv got an unknown tag.\n", DisplayName);
                        done = false;
                    }
                }
#endif
            }
            while ( !done );
        }
        else
        {
            rc = MPI_ERR_REQUEST;
        }
    }
    else
    {
        do
        {
            if (Request[0] == MPI_REQUEST_NULL)
            {
                printf ("[%s] MPI_Irecv on Monitor called.\n", DisplayName);
                rc = MPI_Irecv (notice, sizeof (struct message_def), MPI_CHAR, 0,
                                NOTICE_TAG, Monitor, &Request[0]);
                if (rc != MPI_SUCCESS)
                {
                    printf ("[%s] MPI_Irecv failure on Monitor, rc=%s\n",
                            DisplayName, display_error (rc));
                }
            }
            if (*comm != MPI_COMM_NULL)
            {
                if (Request[1] == MPI_REQUEST_NULL)
                {
                    printf ("[%s] MPI_Irecv on comm called.\n", DisplayName);
                    rc = MPI_Irecv (buf, size, type, 0, tag, *comm, &Request[1]);
                    if (rc != MPI_SUCCESS)
                    {
                        printf ("[%s] MPI_Irecv failure on comm, rc=%s\n",
                                DisplayName, display_error (rc));
                    }
                }
            }
            idx = MPI_UNDEFINED;
            rc = MPI_Waitany (2, Request, &idx, status);
            if (rc == MPI_SUCCESS)
            {
                if (idx == 0)
                {
                    check_notice (notice,status);
                }
#ifdef NO_OPEN_CLOSE_NOTICES
                else
                {
                    if (status->MPI_TAG == NOTICE_TAG)
                    {
                        check_notice (notice, status);
                    }
                }
#endif
            }
            else
            {
                printf ("[%s] MPI_Waitany failed, rc=%s\n", DisplayName, display_error (rc));
                break;
            }
        }
        while (idx == 0 && !Takeover);

        if (idx == 0 || idx == MPI_UNDEFINED)
        {
            if (Request[1] != MPI_REQUEST_NULL)
            {
                printf ("[%s] Cancelled comm request.\n", DisplayName);
                rc = MPI_Cancel (&Request[1]);
                if ( Request[1] != MPI_REQUEST_NULL )
                {
                    MPI_Request_free(&Request[1]);
                }
                Request[1] = MPI_REQUEST_NULL;
            }
            if (*comm == Client)
            {
                rc = MPI_ERR_REQUEST;
            }
            else if (*comm == Peer)
            {
                rc = MPI_ERR_EXITED;
            }
        }
    }
    delete notice;
    return rc;
}

void start_backup (int nid)
{
    char outfile[MAX_PROCESS_PATH];
    char takeoverCountMaxString[10];
    char takeoverCountString[10];
    int count;
    MPI_Status status;
    struct message_def *msg;

    printf ("[%s] starting backup process.\n", DisplayName);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = nid;
    msg->u.request.u.new_process.debug = 0;
    msg->u.request.u.new_process.type = process_type;
    msg->u.request.u.new_process.priority = 0;
    msg->u.request.u.new_process.backup = 1;
    msg->u.request.u.new_process.unhooked = true;
    msg->u.request.u.new_process.nowait = false;
    msg->u.request.u.new_process.tag = 0;
    strcpy (msg->u.request.u.new_process.process_name, MyName);
    strcpy (msg->u.request.u.new_process.path, getenv ("PATH"));
    strcpy (msg->u.request.u.new_process.ldpath, getenv ("LD_LIBRARY_PATH"));
    strcpy (msg->u.request.u.new_process.program, getenv ("PWD"));
    strcat (msg->u.request.u.new_process.program, "/nsserver");
    msg->u.request.u.new_process.infile[0] = '\0';
    sprintf( outfile, "SERV0.T%d.log", ((TakeoverCountMax > 0) ? 2 : 1) );
    strcpy( msg->u.request.u.new_process.outfile, outfile );
    if ( TakeoverCountMax > 0 )
    {
        msg->u.request.u.new_process.argc = 2;
        sprintf( takeoverCountMaxString, "%d", TakeoverCountMax );
        sprintf( takeoverCountString, "%d", TakeoverCount );
        strcpy (msg->u.request.u.new_process.argv[0], takeoverCountMaxString);
        strcpy (msg->u.request.u.new_process.argv[1], takeoverCountString);
    }
    else
    {
        msg->u.request.u.new_process.argc = 0;
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
                printf
                    ("[%s] started process successfully. Nid=%d, Pid=%d, Process_name=%s, rtn=%d\n",
                     DisplayName, msg->u.reply.u.new_process.nid,
                     msg->u.reply.u.new_process.pid,
                     msg->u.reply.u.new_process.process_name,
                     msg->u.reply.u.new_process.return_code);
                fflush (stdout);
                PeerNid = msg->u.reply.u.new_process.nid;
                PeerPid = msg->u.reply.u.new_process.pid;
#ifdef NO_OPEN_CLOSE_NOTICES
                get_peer_open();
#else
                wait_for_open_notice (&Peer);
#endif
            }
            else
            {
                printf ("[%s] new process failed to spawn, rc=%s\n",
                        DisplayName,
                        display_error (msg->u.reply.u.new_process.
                                       return_code));
                printf ("[%s] Exiting(ABORT)\n", DisplayName);
                fflush (stdout);
                MPI_Abort(MPI_COMM_WORLD,1);
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                 DisplayName, msg->type, msg->u.reply.type);
            printf ("[%s] Exiting(ABORT)\n", DisplayName);
            fflush (stdout);
            MPI_Abort(MPI_COMM_WORLD,1);
        }
    }
    else
    {
        printf ("[%s] new process reply message invalid\n", DisplayName);
        printf ("[%s] Exiting(ABORT)\n", DisplayName);
        fflush (stdout);
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

}

bool takeover (void)
{
    bool activate = false;

    if (ImBackup)
    {
        if ( TakeoverCountMax > 0 )
        {
            ++TakeoverCount;
        }
        printf( "[%s] mark primary down, takeover (%d) and start backup.\n"
               , DisplayName, TakeoverCount);
        PeerNid = -1;
        PeerPid = -1;
        ImBackup = false;
        strcpy (DisplayName, DisplayPid);
        strcat (DisplayName, ":");
        strcat (DisplayName, MyName);
        strcat (DisplayName, "-P(B)");
        activate = true;
    }
    else
    {
        printf ("[%s] mark backup down and restart backup.\n", DisplayName);
        start_backup (-1);
    }
    fflush (stdout);

    return activate;
}

void wait_for_client_close (void)
{
    MPI_Status status;
    struct message_def *notice = new message_def;

    printf ("[%s] waiting for client close.\n", DisplayName);
    fflush (stdout);
#ifdef NO_OPEN_CLOSE_NOTICES
    MPI_Recv( notice, sizeof(struct message_def), MPI_CHAR, 0, NOTICE_TAG, Client, &status);
    check_notice( notice, &status );
#endif    
    while (OpenCount > 0)
    {
        // just wait for call back to receive notice.
        usleep(1000);
    }
    fflush (stdout);
    delete notice;
}

void wait_for_open_notice (MPI_Comm *comm)
{
    struct message_def *notice = new message_def;

    printf ("[%s] Waiting for open.\n", DisplayName);
    fflush (stdout);
    while (*comm == MPI_COMM_NULL)
    {
        // just wait for call back to receive notice.
        usleep(1000);
 
    }
    fflush (stdout);
    delete notice;
}

void wait_for_client_open(void)
{
    do
    {
        printf ("[%s] wait_for_client_open - Waiting for open.\n", DisplayName);
        fflush (stdout);
        get_open_notice();
        if ( ClientOpenNotice  )
        {
            printf ("[%s] wait_for_client_open - Got Client open.\n", DisplayName);
            fflush (stdout);
            Client = Comm;
        }
        if ( PeerOpenNotice )
        {
            printf ("[%s] wait_for_client_open - Got Peer open.\n", DisplayName);
            fflush (stdout);
            PeerOpenNotice = false;
            Peer = Comm;
        }
    }
    while ( !ClientOpenNotice );

    ClientOpenNotice = false;
}

void wait_for_peer_open(void)
{
    do
    {
        printf ("[%s] wait_for_peer_open - Waiting for open.\n", DisplayName);
        fflush (stdout);
        get_open_notice();
        if ( ClientOpenNotice  )
        {
            printf ("[%s] wait_for_peer_open - Got Client open.\n", DisplayName);
            fflush (stdout);
            ClientOpenNotice = false;
            Client = Comm;
        }
        if ( PeerOpenNotice )
        {
            printf ("[%s] wait_for_peer_open - Got Peer open.\n", DisplayName);
            fflush (stdout);
            Peer = Comm;
        }
    }
    while ( !PeerOpenNotice );

    PeerOpenNotice = false;
}

bool process_request (int *recvbuf)
{
    int rc;
    char sendbuf[100];
    char wkbuf[100];
    bool done = false;
    bool abort = false;
    
    switch (recvbuf[2])
    {
    case CMD_CONT:
        sprintf (wkbuf, "Received (%d:%d) CMD_CONT", recvbuf[0], recvbuf[1]);
        if ( TakeoverCountMax > 0 && TakeoverCount <= TakeoverCountMax )
        {
            printf ("[%s] Exiting(ABORT)\n", DisplayName);
            fflush (stdout);
            MPI_Abort(MPI_COMM_WORLD,1);
        }
        break;
    case CMD_END:
        sprintf (wkbuf, "Received (%d:%d) CMD_END", recvbuf[0], recvbuf[1]);
        done = true;
        break;
    case CMD_ABORT:
        sprintf (wkbuf, "Received (%d:%d) CMD_ABORT", recvbuf[0], recvbuf[1]);
        abort = true;
        break;
    default:
        sprintf (wkbuf, "Received (%d:%d) UNKNOWN", recvbuf[0], recvbuf[1]);
    }
    if (Client == MPI_COMM_NULL)
    {
        printf ("[%s] processed checkpointed data (%s)\n", DisplayName, wkbuf);
    }
    else
    {
        printf ("[%s] processed client request\n", DisplayName);
        sprintf(sendbuf,"[%s] %s", DisplayName, wkbuf);
        rc = MPI_Send (sendbuf, (int) strlen (sendbuf) + 1, MPI_CHAR, 0,
                       USER_TAG, Client);
        if (rc != MPI_SUCCESS)
        {
            printf ("[%s] Send to client failed, rc=%s\n", DisplayName,
                    display_error (rc));
        }
    }

    if( abort )
    {
        printf ("[%s] Aborting\n", DisplayName);
        fflush (stdout);
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    return done;
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
    int j;
    int rc;
    int recvbuf[3];
    int len;
    bool done = false;
    MPI_Status status;

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);
 
    MyName = new char [MAX_PROCESS_PATH];
    strcpy( MyName, argv[5] );
    MyNid = atoi(argv[3]);
    strcpy( DisplayPid, argv[4] );
    
    if ( ! gp_local_mon_io )
    {
        InitLocalIO();
        assert (gp_local_mon_io);
    }
    
    gp_local_mon_io->set_cb(recv_localio_msg, "notice");
    gp_local_mon_io->set_cb(recv_localio_msg, "recv");

    process_startup (argc, argv);

    MPI_Errhandler_create (PeerRecovery, &PeerHandler);
    MPI_Comm_set_errhandler (MPI_COMM_WORLD, PeerHandler);
    strcpy (DisplayName, DisplayPid);
    strcat (DisplayName, ":");
    strcat (DisplayName, MyName);
    if (isBackup ())
    {
        strcat (DisplayName, "-B");
        printf ("[%s] We are the backup process.\n", DisplayName);
        fflush (stdout);
        open_primary ();
        ImBackup = true;
        while (!Takeover)
        {
            len = 3;
            rc = checkpoint (recvbuf, &len, MPI_INT);
            if (rc == MPI_SUCCESS)
            {
                process_request (recvbuf); 
            }
            else
            {
                // just wait for a bit
                usleep(5000);
            }
        }
        printf ("[%s] The backup is now the primary process.\n", DisplayName);
        fflush (stdout);
        close_primary ();
        strcpy (DisplayName, DisplayPid);
        strcat (DisplayName, ":");
        strcat (DisplayName, MyName);
        strcat (DisplayName, "-P");
    }
    else
    {
        strcpy (DisplayName, DisplayPid);
        strcat (DisplayName, ":");
        strcat (DisplayName, MyName);
        strcat (DisplayName, "-P");
        printf ("[%s] We are the primary process.\n", DisplayName);
        fflush (stdout);
    }
    start_backup (-1);
    mount_device();

#ifdef NO_OPEN_CLOSE_NOTICES
    get_client_open();
#else
    wait_for_open_notice (&Client);
#endif

    printf ("[%s] Starting work.\n", DisplayName);
    fflush (stdout);
    j = 0;
    do
    {
        j++;
        recvbuf[0] = -1;
        recvbuf[1] = -1;
        rc = recv (&Client, recvbuf, 3, MPI_INT, USER_TAG, &status);
        if (rc == MPI_SUCCESS)
        {
            MPI_Get_count (&status, MPI_INT, &len);
            if( len != 3 )
            {
                printf("[%s] recv length invalid, (%d) should be 3.\n", DisplayName, len);
            }
            // primary will abort in process_request() when 
            // CMD_ABORT is sent by client
            done = process_request (recvbuf);
            checkpoint (recvbuf, &len, MPI_INT);
        }
        else
        {
            if (rc == MPI_ERR_REQUEST)
            {
                printf ("[%s] retry recv i/o with client\n", DisplayName);
            }
            else
            {
                printf ("[%s] try %d failed with %s\n", DisplayName, j, display_error (rc));
                fflush (stdout);
                exit (1);
            }
        }
    }
    while (!done);

    wait_for_client_close();

    exit_process ();

    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }

    sleep(1);
    printf ("[%s] calling Finalize!\n", DisplayName); 
    fflush (stdout);
    MPI_Close_port(MyPort);
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
    MPI_Errhandler_free( &PeerHandler );
    MPI_Finalize ();

    printf ("[%s] calling exit(0)\n", DisplayName);
    fflush (stdout);
    exit (0);
}
