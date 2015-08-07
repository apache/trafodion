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
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include "msgdef.h"
#include "localio.h"
#include "clio.h"

char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
int MyRank = -1;
int MyNid = -1;
int MyPid = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
MPI_Comm Monitor;
struct message_def *msg;

FILE *shell_locio_trace_file = NULL;

void shell_locio_trace(const char *where, const char *format, va_list ap)
{
    if (shell_locio_trace_file != NULL)
    {
        int             ms;
        int             us;
        struct timeval  t;
        struct tm       tx;
        struct tm      *txp;
        char            buf[BUFSIZ];
        gettimeofday(&t, NULL);
        txp = localtime_r(&t.tv_sec, &tx);
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

void SetupFifo(int orig_fd, char *fifo_name)
{
    int fifo_fd;

    // Open the fifo for writing.
	fifo_fd = open (fifo_name, O_WRONLY);
    if (fifo_fd == -1)
    {
        printf("[%s], fifo open(%s) error, %s.\n", MyName,
               fifo_name, strerror(errno));
        return;
    }

    // Remap fifo file descriptor to desired file descriptor.
    // Close unneeded fifo file descriptor.
    if (close(orig_fd))
    {
        printf("[%s], close(%d) error, %s.\n", MyName, orig_fd,
               strerror(errno));
    }

    if ( dup2(fifo_fd, orig_fd) == -1)
    {
        printf("[%s], dup2(%d, %d) error, %s.\n", MyName,
               fifo_fd, orig_fd, strerror(errno));
    }
    else
    {   
        if (close(fifo_fd))
        {
            printf("[%s], close(%d) error, %s.\n", MyName,
                   fifo_fd, strerror(errno));
        }
    }
}

int saved_stdout_fd = -1;
int saved_stderr_fd = -1;

void SaveStdFds()
{
    saved_stdout_fd = dup(1);
    printf("SaveStdFds: saved_stdout_fd=%d\n", saved_stdout_fd);
    if (saved_stdout_fd == -1)
    {
        printf("[%s], dup(1) error, %s.\n", MyName, strerror(errno));
    }

    saved_stderr_fd = dup(2);
    printf("SaveStdFds: saved_stderr_fd=%d\n", saved_stderr_fd);
    if (saved_stderr_fd == -1)
    {
        printf("[%s], dup(1) error, %s.\n", MyName, strerror(errno));
    }
}

void RestoreStdFds()
{
    if ( saved_stdout_fd != -1)
    {
        if ( dup2(saved_stdout_fd, 1) == -1)
        {
            printf("[%s], dup2(%d, %d) error, %s.\n", MyName,
                   saved_stdout_fd, 1, strerror(errno));
        }
    }

}

void attach( int nid, char *name )
{
    int count;
    int rc;
    MPI_Status status;
    char node_name[MPI_MAX_PROCESSOR_NAME];
    char MonitorPort[MPI_MAX_PORT_NAME];
    char filename[MAX_PROCESS_PATH];
    FILE *fd;
   
    strcpy( MyName, name ); 
    printf("[%s] Attaching to the monitor.\n",MyName);
    

    gp_local_mon_io->iv_pid = getpid();
    gp_local_mon_io->init_comm();
    gp_local_mon_io->acquire_msg( &msg );
    

// DJW Remove line?
    MPI_Open_port (MPI_INFO_NULL, MyPort);

    msg->type = MsgType_Service;
    msg->noreply = false;                         // attach needs reply
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Startup;
    msg->u.request.u.startup.nid = MyNid = -1;    // -1 signals attach
    msg->u.request.u.startup.pid = MyPid = -1;    // -1 signals attach
    strcpy (msg->u.request.u.startup.process_name, name);
    strcpy (msg->u.request.u.startup.port_name, MyPort);
    msg->u.request.u.startup.os_pid = getpid ();
    msg->u.request.u.startup.event_messages = true;
    msg->u.request.u.startup.system_messages = true;
    msg->u.request.u.startup.paired = false;
    msg->u.request.u.startup.verifier = true;
    msg->u.request.u.startup.startup_size = sizeof(msg->u.request.u.startup);
    strcpy(msg->u.request.u.startup.program, "<unknown>");


    (reinterpret_cast<SharedMsgDef *>(msg))->trailer.attaching = true;
    printf("Sending ReqType_Startup via local io\n");
    gp_local_mon_io->send_recv( msg );
    count = sizeof (*msg);
    status.MPI_TAG = msg->reply_tag;
    printf("Sending ReqType_Startup via local io -- done\n");


    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_Startup))
        {
            if (msg->u.reply.u.startup_info.return_code == MPI_SUCCESS)
            {
                MyNid = msg->u.reply.u.startup_info.nid;
                MyPid = msg->u.reply.u.startup_info.pid;
                // Connect to monitor via pipes and remap stdout and stderr
                SetupFifo(2, msg->u.reply.u.startup_info.fifo_stderr);
                SetupFifo(1, msg->u.reply.u.startup_info.fifo_stdout);
                printf("[%s] process attach succeeded, MyNid=%d, MyPid=%d\n",
                       MyName, MyNid, MyPid);
            }
            else
            {
                printf ("[%s] process attach failed, rc=%d\n", MyName,
                        msg->u.reply.u.startup_info.return_code);
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] process attach reply invalid.\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
}

void exit_process (void)
{
    int count;
    MPI_Status status;

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_Exit;
    msg->u.request.u.close.nid = MyNid;
    msg->u.request.u.close.pid = MyPid;

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
                MPI_Comm_disconnect(&Monitor);
            }
            else
            {
                printf ("[%s] exit process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code);
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
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

int main (int argc, char *argv[])
{
    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);

    MyName = new char [MAX_PROCESS_PATH];
    strcpy( MyName, "$ATTACH" );
    MyNid = 0;

    // Save the standard out file descriptor for later use
    SaveStdFds();

    printf("[%s] Local IO nid = %d\n", MyName, MyNid);

    // Set the local IO nid before creating the object
    gv_ms_su_nid = MyNid;
    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
    assert (gp_local_mon_io);

    char *cmd_buffer = getenv("SQ_LOCAL_IO_SHELL_TRACE");
    if (cmd_buffer && *cmd_buffer == '1')
    {
        gp_local_mon_io->cv_trace = true;

        char tracefile[MAX_SEARCH_PATH];
        char *tmpDir;

        tmpDir = getenv( "MPI_TMPDIR" );
        if (tmpDir)
        {
            sprintf( tracefile, "%s/attach.trace.%d", tmpDir, getpid() );
        }
        else
        {
            sprintf( tracefile, "./attach.trace.%d", getpid() );
        }

        shell_locio_trace_file = fopen(tracefile, "w+");
        gp_local_mon_io->cp_trace_cb = shell_locio_trace;
    }

    attach(0,(char *) "$ATTACH");
    printf("[%s] Attach program started.\n",MyName);
    sleep(15);
    fflush (stdout);
    printf("[%s] Exiting.\n",MyName);

    // Once monitor shuts down it we can no longer route our standard
    // output through the pipe connection to the monitor.   So reset
    // stdout to its original setting.
    RestoreStdFds();
    printf("\n");

    exit_process ();

    MPI_Close_port (MyPort);
    MPI_Finalize ();

    delete gp_local_mon_io;
    exit (0);
}
