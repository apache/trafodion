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

// constructing queues
#include <deque>
#include <list>
#include <queue>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "msgdef.h"
#include "props.h"
#include "localio.h"
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "clusterconf.h"
#include "seabed/trace.h"

long trace_settings = 0;

#define MAX_SYNCS 10
#define MAX_TEST_NODES 6
#define SYNC_DELAY 150000 // delay (150 ms)

char MyPort[MPI_MAX_PORT_NAME];
char *MyName;
int MyRank = -1;
int MyPNid = -1;
int MyNid = -1;
int MyPid = -1;
int NumNodes = 0;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
MPI_Comm Monitor;

int trans_count = 0;
int trans_starts = 0;
int trans_abort = 0;
int trans_commit = 0;
int event_id = -1;
int trans_active = 0;
bool node_down = false;
int dead_tm_count = 0;
int down_node_count = 0;
int up_node_count = 0;
bool node_up[MAX_TEST_NODES] = {true,true,true,true,true,true};
bool IamTmLeader = false;
bool TmLeaderDied = false;
struct tm_process_def {
    bool dead;
    bool leader;
    int  pid;
    char process_name[MAX_PROCESS_NAME];    // TM process's Name
    char program[MAX_PROCESS_PATH];
} tmProcess[MAX_TEST_NODES];
bool Abort_transaction = false;
bool EventMsgRecv = false;
bool UnsolMsgRecv = false;
bool NoticeMsgRecv = false;
bool Test_Initialized = false;
bool TestWait = false;
bool TestShutdown = false;
int seq_number = 0;
int VirtualNodes = 0;

struct req_def {
    int handle;
    bool completed;
    bool aborted;
} request[MAX_TEST_NODES][MAX_SYNCS];

pthread_mutex_t     notice_mutex;
pthread_cond_t      notice_cv;
bool                notice_signaled = false;

pthread_mutex_t     test_mutex;
pthread_cond_t      test_cv;
bool                test_signaled = false;

pthread_mutex_t     unsolicited_mutex;
pthread_cond_t      unsolicited_cv;
bool                unsolicited_signaled = false;

struct sync_buf_def
{
    int seq_number;
    int length;
    char string[132];
} __attribute__((__may_alias__));

queue<struct message_def> unsolicited_queue; // unsolicited msg queue

void flush_incoming_msgs( void );

FILE *shell_locio_trace_file = NULL;

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    trace_printf("%s", pp_string );

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

void lock_notice()
{
    int rc = pthread_mutex_lock(&notice_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to lock notice mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_notice()
{
    int rc = pthread_mutex_unlock(&notice_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to unlock notice mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}

int signal_notice() 
{
    int rc = 0;

    notice_signaled = true;
    rc = pthread_cond_broadcast(&notice_cv);
    if ( rc != 0) 
    {
        errno = rc;
        trace_printf("[%s] - Unable to signal notice: %s (%d)\n",
                     MyName, strerror(errno), errno);
        rc = -1;
    }

    return( rc );
}

int wait_on_notice( void ) 
{
    int rc = 0;

    if ( ! notice_signaled ) 
    {
        rc = pthread_cond_wait(&notice_cv, &notice_mutex);
        if ( rc != 0) 
        {
            errno = rc;
            trace_printf("[%s] - Unable to signal notice: %s (%d)\n",
                         MyName, strerror(errno), errno);
            rc = -1;
        }
    }
    notice_signaled = false;

    return( rc );
}

void lock_test()
{
    int rc = pthread_mutex_lock(&test_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to lock test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_test()
{
    int rc = pthread_mutex_unlock(&test_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to unlock test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}

int signal_test() 
{
    int rc = 0;

    test_signaled = true;
    rc = pthread_cond_broadcast(&test_cv);
    if ( rc != 0) 
    {
        errno = rc;
        trace_printf("[%s] - Unable to signal test: %s (%d)\n",
                     MyName, strerror(errno), errno);
        rc = -1;
    }

    return( rc );
}

int wait_on_test( void ) 
{
    int rc = 0;

    if ( ! test_signaled ) 
    {
        rc = pthread_cond_wait(&test_cv, &test_mutex);
        if ( rc != 0) 
        {
            errno = rc;
            trace_printf("[%s] - Unable to signal test: %s (%d)\n",
                         MyName, strerror(errno), errno);
            rc = -1;
        }
    }
    test_signaled = false;

    return( rc );
}

void lock_unsolicited()
{
    int rc = pthread_mutex_lock(&unsolicited_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to lock unsolicited mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_unsolicited()
{
    int rc = pthread_mutex_unlock(&unsolicited_mutex);

    if (rc != 0)
    {
        trace_printf("[%s] - Unable to unlock unsolicited mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}

int signal_unsolicited() 
{
    int rc = 0;

    unsolicited_signaled = true;
    rc = pthread_cond_broadcast(&unsolicited_cv);
    if ( rc != 0) 
    {
        errno = rc;
        trace_printf("[%s] - Unable to signal unsolicited: %s (%d)\n",
                     MyName, strerror(errno), errno);
        rc = -1;
    }

    return( rc );
}

int wait_on_unsolicited( void ) 
{
    int rc = 0;

    if ( ! unsolicited_signaled ) 
    {
        rc = pthread_cond_wait(&unsolicited_cv, &unsolicited_mutex);
        if ( rc != 0) 
        {
            errno = rc;
            trace_printf("[%s] - Unable to signal unsolicited: %s (%d)\n",
                         MyName, strerror(errno), errno);
            rc = -1;
        }
    }
    unsolicited_signaled = false;

    return( rc );
}

int completed_request(int nid, int handle, bool abort)
{
    int j;
    trace_printf("[%s] - Completing request: node=%d, handle=%d, abort=%d\n", MyName, nid, handle, abort);

    for(j=0; j<MAX_SYNCS; j++)
    {
        //printf("[%s] - Complete request: node=%d, index=%d, handle=%d, completed=%d\n", MyName, nid, j, request[nid][j].handle, request[nid][j].completed);
        if ( request[nid][j].handle == handle )
        {
            request[nid][j].completed = true;
            request[nid][j].aborted = abort;
            //printf("[%s] - Completed request: node=%d, index=%d, handle=%d, completed=%d\n", MyName, nid, j, request[nid][j].handle, request[nid][j].completed);
            return j;
        }
    }

    trace_printf("[%s] - Can't find TmSync handle=%d\n", MyName, handle);
    return -1;
}

bool completed_test( void )
{
    int nid;
    int j;
    bool done = true;

    lock_test();
    for( nid=0; nid<MAX_TEST_NODES; nid++)
    {
        for(j=0; j<MAX_SYNCS; j++)
        {
            if ( node_up[nid] )
            {
                //printf("[%s] - Complete test: node=%d, handle=%d, completed=%d\n", MyName, nid, request[nid][j].handle, request[nid][j].completed);
                if ((  request[nid][j].handle == -1 ) ||
                    ( !request[nid][j].completed    )   )
                {
                    trace_printf("[%s] - Test not completed @ node=%d,  handle index=%d\n",MyName,nid,j);
                    done = false;
                }
            }
        }
    }
    unlock_test();

    Test_Initialized = done ? false : true;
    return( done );
}

void end_requests( int nid )
{
    int j;
    trace_printf( "[%s] - Ending requests for: node=%d\n", MyName, nid );

    for(j=0; j<MAX_SYNCS; j++)
    {
        request[nid][j].completed = true;
        request[nid][j].aborted = true;
    }
}

int GetNumOfNode( void )
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


    if ((status.MPI_TAG == REPLY_TAG) &&
        (count == sizeof (struct message_def)))
    {
        if ((msg->type == MsgType_Service) &&
            (msg->u.reply.type == ReplyType_NodeInfo))
        {
            if ((msg->u.reply.u.node_info.return_code == MPI_SUCCESS     ) ||
                (msg->u.reply.u.node_info.return_code == MPI_ERR_TRUNCATE)   )
            {
                if (msg->u.reply.u.node_info.num_returned)
                {
                    printf ("[%s] NodeInfo, num_nodes=%d\n", MyName,
                        msg->u.reply.u.node_info.num_nodes);
                    int nodeCount = msg->u.reply.u.node_info.num_nodes;
                    gp_local_mon_io->release_msg(msg);
                    return nodeCount;
                }
            }
            else
            {
                trace_printf ("[%s] NodeInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.node_info.return_code));
            }
        }
        else
        {
            trace_printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NodeInfo message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        trace_printf ("[%s] NodeInfo reply message invalid\n", MyName);
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return 0;
}

int get_tm_processes( int tmCount )
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
    msg->u.request.u.process_info.type = ProcessType_DTM;

    msg->u.request.u.process_info.target_process_name[0] = 0;
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
                if ( msg->u.reply.u.process_info.num_processes == tmCount )
                {
                    for ( int i = 0; i < msg->u.reply.u.process_info.num_processes; i++ )
                    {
                        int nid = msg->u.reply.u.process_info.process[i].nid;
                        trace_printf ( "[%s] TM ProcessInfo: nid=%d, pid=%d, Name=%s, program=%s\n", MyName
                                , msg->u.reply.u.process_info.process[i].nid
                                , msg->u.reply.u.process_info.process[i].pid
                                , msg->u.reply.u.process_info.process[i].process_name
                                , msg->u.reply.u.process_info.process[i].program );
                        tmProcess[nid].dead = msg->u.reply.u.process_info.process[i].state == State_Up ? false : true;
                        tmProcess[nid].pid = msg->u.reply.u.process_info.process[i].pid;
                        strcpy (tmProcess[nid].process_name, msg->u.reply.u.process_info.process[i].process_name );
                        strcpy (tmProcess[nid].program, msg->u.reply.u.process_info.process[i].program );
                    }
                }
                else
                {
                    trace_printf( "[%s] TM ProcessInfo failed, invalid number of TM processes, count=%d\n", MyName
                          , msg->u.reply.u.process_info.num_processes);
                    return 1;
                }
            }
            else
            {
                trace_printf ("[%s] TM ProcessInfo failed, error=%s\n", MyName,
                    ErrorMsg(msg->u.reply.u.process_info.return_code));
                return 1;
            }
        }
        else
        {
            trace_printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for TM ProcessInfo message\n",
                    MyName, msg->type, msg->u.reply.type);
            return 1;
        }
    }
    else
    {
        trace_printf ("[%s] TM ProcessInfo reply message invalid\n", MyName);
        return 1;
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return 0;
}

int start_tm_process( int nid )
{
    int count;
    int rc = -1;
    char outfile[MAX_PROCESS_PATH];
    MPI_Status status;
    struct message_def *msg;

    trace_printf ("[%s] starting process %s.\n", MyName, tmProcess[nid].process_name);
    fflush (stdout);

    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_NewProcess;
    msg->u.request.u.new_process.nid = nid;
    msg->u.request.u.new_process.type = ProcessType_DTM;
    msg->u.request.u.new_process.debug = 0;
    msg->u.request.u.new_process.priority = 0;
    msg->u.request.u.new_process.backup = 0;
    msg->u.request.u.new_process.unhooked = true;
    msg->u.request.u.new_process.nowait = false;
    msg->u.request.u.new_process.tag = 0;
    strcpy( msg->u.request.u.new_process.process_name, tmProcess[nid].process_name );
    strcpy( msg->u.request.u.new_process.path, getenv ("PATH") );
    strcpy( msg->u.request.u.new_process.ldpath, getenv ("LD_LIBRARY_PATH") );
    strcpy( msg->u.request.u.new_process.program, tmProcess[nid].program );
    msg->u.request.u.new_process.infile[0] = '\0';
    sprintf( outfile, "TM0%d.lst", nid );
    strcpy( msg->u.request.u.new_process.outfile, outfile );
    msg->u.request.u.new_process.argc = 0;
    msg->u.request.u.new_process.argv[0][0] = '\0';

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
                tmProcess[msg->u.reply.u.new_process.nid].dead = false;
                dead_tm_count--;
                trace_printf
                    ("[%s] started process successfully. Dead TM count =%d, Nid=%d, Pid=%d, Process_name=%s, rtn=%d\n",
                     MyName, dead_tm_count, msg->u.reply.u.new_process.nid,
                     msg->u.reply.u.new_process.pid,
                     msg->u.reply.u.new_process.process_name,
                     msg->u.reply.u.new_process.return_code);
                rc = 0;
            }
            else
            {
                trace_printf ("[%s] new process failed to spawn, rc=%d\n", MyName,
                        msg->u.reply.u.new_process.return_code);
            }
        }
        else
        {
            trace_printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for NewProcess message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        trace_printf ("[%s] new process reply message invalid\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
    
    return( rc );
}

int restart_all_dead_tm( void )
{
    for ( int nid = 0; nid < MAX_TEST_NODES; nid++ )
    {
        if ( tmProcess[nid].dead )
        {
            if ( start_tm_process( nid ) )
            {
                return( -1 );
            }
        }
    }

    return( 0 );
}


void initialize_test( void )
{
    int nid;
    int j;

    trace_printf ("[%s] Initializing for test\n",MyName);
    memset( tmProcess, 0, sizeof(tmProcess) );
    for( nid=0; nid<MAX_TEST_NODES; nid++)
    {
        for(j=0; j<MAX_SYNCS; j++)
        {
            request[nid][j].handle = -1;
            request[nid][j].completed = false;
            request[nid][j].aborted = false;
       }
    }
    trans_count = 0;
    trans_starts = 0;
    trans_abort = 0;
    trans_commit = 0;
    trans_active=0;
    dead_tm_count = 0;
    Test_Initialized = true;
}

void process_startup (int argc, char *argv[])
{
    int i;
    
    struct message_def *msg;

    trace_printf ("[%s] processing startup.\n", argv[5]);
    fflush (stdout);

    trace_printf ("[%s] - argc=%d", argv[5], argc);
    for(i=0; i<argc; i++)
    {
        trace_printf (", argv[%d]=%s",i,argv[i]);
    }
    trace_printf ("\n");
    fflush(stdout);

    strcpy (MyName, argv[5]);
    MPI_Open_port (MPI_INFO_NULL, MyPort);

#ifdef OFED_MUTEX
    // free monitor.sem semaphore
    trace_printf ("[%s] Opening mutex\n",MyName);
    fflush(stdout);
    char sem_name[MAX_PROCESS_PATH];
    sprintf(sem_name,"/monitor.sem2.%s",getenv("USER"));
    sem_t *mutex = sem_open(sem_name,0,0644,0);
    if(mutex == SEM_FAILED)
    {
        trace_printf ("[%s] Can't access %s semaphore\n", MyName, sem_name);
        sem_close(mutex);
        abort();
    }
    trace_printf ("[%s] Putting mutex\n",MyName);
    fflush(stdout);
    sem_post(mutex);
    sem_close(mutex);
#endif
    MyNid = atoi(argv[3]);
    MyPid = atoi(argv[4]);
    gv_ms_su_verif  = atoi(argv[9]);
    trace_printf ("[%s] process_startup, MyNid: %d, lio: %p\n", 
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
        trace_printf ("[%s] sending startup reply to monitor.\n", argv[5]);
        fflush (stdout);

        gp_local_mon_io->send( msg );

        trace_printf ("[%s] Startup completed", argv[5]);
        if (argc > 9)
        {
            for (i = 10; i < argc; i++)
            {
                trace_printf (", argv[%d]=%s", i, argv[i]);
            }
        }
        trace_printf ("\n");
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

    trace_printf ("[%s] flush incoming event & notices.\n", MyName);
    fflush (stdout);
    do
    {
        gp_local_mon_io->get_notice( &msg );
        if (msg) 
        {
            trace_printf("[%s] Got local IO notice\n",MyName);
            complete = true;
            count = sizeof( *msg );
            status.MPI_TAG = msg->reply_tag;
        }
        else
        {
            trace_printf("[%s] No local IO notice\n",MyName);
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
                            trace_printf ("[%s] Process %s abnormally terminated. Nid=%d, Pid=%d\n",
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        else
                        {
                            trace_printf ("[%s] Process %s terminated normally. Nid=%d, Pid=%d\n", 
                                MyName, msg->u.request.u.death.process_name, 
                                msg->u.request.u.death.nid, msg->u.request.u.death.pid);
                        }
                        break;

                    case MsgType_NodeDown:
                        trace_printf ("[%s] Node %d (%s) is DOWN\n", 
                            MyName, msg->u.request.u.down.nid, msg->u.request.u.down.node_name );
                        break;

                    case MsgType_NodeUp:
                        trace_printf ("[%s] Node %d (%s) is UP\n", 
                            MyName, msg->u.request.u.up.nid, msg->u.request.u.up.node_name);
                        break;    

                    case MsgType_Change:
                        trace_printf ("[%s] Configuration Change Notice for Group: %s Key: %s\n", 
                            MyName, 
                            msg->u.request.u.change.group,
                            msg->u.request.u.change.key);
                        break;
                    case MsgType_Open:
                    case MsgType_Close:
                        trace_printf ("[%s] Open/Close process notification\n", MyName);
                        break;
                    
                    case MsgType_Event:
                        trace_printf("[%s] Event %d received\n",
                            MyName, msg->u.request.u.event_notice.event_id);
                        break;

                    case MsgType_Shutdown:
                        TestShutdown = true;
                        trace_printf("[%s] Shutdown notice, level=%d received\n",
                            MyName, msg->u.request.u.shutdown.level);
                        break;
                        
                    default:
                        trace_printf("[%s] Invalid Notice Type(%d) for flush message\n",
                            MyName, msg->type);
                    }
                }    
                else
                {
                    trace_printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                        MyName, msg->type, msg->noreply, msg->u.request.type);
                }
            }
            else
            {
                trace_printf ("[%s] Failed to flush messages\n", MyName);
                done = true;
            }
            fflush (stdout);
        }
        if (msg) delete msg;    
        msg = NULL;
    }
    while (!done);
}

void exit_process (void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    trace_printf ("[%s] sending exit process message.\n", MyName);
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
                trace_printf ("[%s] exited process successfully. rc=%d\n",
                        MyName, msg->u.reply.u.generic.return_code);
            }
            else
            {
                trace_printf ("[%s] exit process failed, rc=%d\n", MyName,
                        msg->u.reply.u.generic.return_code);
            }
            flush_incoming_msgs();
            MPI_Comm_disconnect( &Monitor );
        }
        else
        {
            trace_printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for Exit message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        trace_printf ("[%s] exit process reply invalid.\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
    msg = NULL;
}

void process_unsolicited_msg( struct message_def *msg )
{
    int rc;
    int handle;
    struct message_def *reply;
    struct sync_buf_def *buf;

    gp_local_mon_io->acquire_msg( &reply );

    trace_printf ("[%s] Processing unsolicited tm_sync message.\n", MyName);
    if( msg->u.request.type == ReqType_TmSync )
    {
        if( msg->type == MsgType_UnsolicitedMessage )
        {
            handle = msg->u.request.u.unsolicited_tm_sync.handle;

            buf = (struct sync_buf_def*)msg->u.request.u.unsolicited_tm_sync.data;
            request[msg->u.request.u.unsolicited_tm_sync.nid][buf->seq_number].handle = handle;
            trace_printf("[%s] TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                msg->u.request.u.unsolicited_tm_sync.nid,
                msg->u.request.u.unsolicited_tm_sync.pid,
                handle,
                buf->seq_number,
                buf->length,
                buf->string);
            seq_number = buf->seq_number;    
            trans_count++;
            trans_active++;
            // sending reply    
            reply->type = MsgType_UnsolicitedMessage;
            reply->noreply = true;
            reply->u.reply.type = ReplyType_TmSync;
            reply->u.reply.u.unsolicited_tm_sync.nid = MyNid;
            reply->u.reply.u.unsolicited_tm_sync.pid = MyPid;
            reply->u.reply.u.unsolicited_tm_sync.handle = handle;
            if (Abort_transaction)
            {
                trace_printf("[%s] Sending Abort to monitor.\n",MyName);
                reply->u.reply.u.unsolicited_tm_sync.return_code = 99;
            }
            else
            {
                trace_printf("[%s] Sending Commit to monitor.\n",MyName);
                reply->u.reply.u.unsolicited_tm_sync.return_code = MPI_SUCCESS;
            }
            trace_printf ("[%s] sending TmSync Unsolicited Message reply to monitor.\n", MyName);
            fflush (stdout);
            if (!gp_local_mon_io)
            {
                rc = MPI_Send (&reply, sizeof (struct message_def), MPI_CHAR, 0, 
                               SERVICE_TAG, Monitor);
            }
            else
            {
                rc = gp_local_mon_io->send( reply );
            }

            if( rc != MPI_SUCCESS )
            {
                trace_printf ("[%s] Send failed, rc = %d\n", MyName, rc);
            }
        }
        else
        {
            trace_printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }     
    }
    else
    {
        trace_printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }    
}

void process_event( struct message_def *msg )
{
    trace_printf ("[%s] Processing event message.\n", MyName);
    event_id = -1;
    if( msg->u.request.type == ReqType_Notice )
    {
        if( msg->type == MsgType_Event )
        {
            trace_printf("[%s] Event %d (%s) received\n", 
                   MyName, 
                   msg->u.request.u.event_notice.event_id,
                   msg->u.request.u.event_notice.data);
            event_id = msg->u.request.u.event_notice.event_id;
        }
        else
        {
            trace_printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }     
    }
    else
    {
        trace_printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }    
}

void request_to_become_TmLeader(void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    trace_printf ("[%s] sending TmLeader request.\n", MyName);
    fflush (stdout);


    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_TmLeader;
    msg->u.request.u.leader.nid = MyNid;
    msg->u.request.u.leader.pid = MyPid;

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
                trace_printf ("[%s] I'm the new TmLeader.\n", MyName);
                IamTmLeader = true;
            }
            else
            {
                trace_printf ("[%s] I'm not the TmLeader.\n", MyName);
                IamTmLeader = false;
            }
            if (msg->u.reply.u.generic.return_code == -1)
            {
                trace_printf ("[%s] Failed to get the TmLeader\n", MyName);
            }
            else
            {
                trace_printf ("[%s] The new TmLeader is %s(%d,%d)\n", MyName,
                    msg->u.reply.u.generic.process_name,
                    msg->u.reply.u.generic.nid,
                    msg->u.reply.u.generic.pid);
            }
        }
        else
        {
            trace_printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for TmLeader message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        trace_printf ("[%s] TmLeader process reply invalid.\n", MyName);
    }
    fflush (stdout);

    gp_local_mon_io->release_msg(msg);
    msg = NULL;
}

bool process_notice( struct message_def *msg )
{
    int j;
    int seq;
    bool completed = false;

    trace_printf ("[%s] Processing tm_sync completion notice.\n", MyName);
    if( msg->u.request.type == ReqType_Notice )
    {
        if( msg->type == MsgType_TmSyncAbort )
        {
            lock_test();
            trace_printf ("[%s] Processing %d Abort notice(s).\n", MyName, msg->u.request.u.tm_sync_notice.count);
            for(j=0; j < msg->u.request.u.tm_sync_notice.count; j++)
            {
                seq=completed_request(msg->u.request.u.tm_sync_notice.nid[j],
                                      msg->u.request.u.tm_sync_notice.handle[j],
                                      true);
                trace_printf("[%s] TmSyncAbort seq#=%d, handle=%d\n", 
                    MyName,
                    seq,
                    msg->u.request.u.tm_sync_notice.handle[j]);
                trans_abort++;
                if ( trans_active && seq > -1 )
                {
                    trans_active--;
                    assert( trans_active >= 0 );
                    completed = true;
                }
            }
            unlock_test();
        }
        else if( msg->type == MsgType_TmSyncCommit )
        {
            lock_test();
            trace_printf ("[%s] Processing %d Commit notice(s).\n", MyName, msg->u.request.u.tm_sync_notice.count);
            for(j=0; j < msg->u.request.u.tm_sync_notice.count; j++)
            {
                seq=completed_request(msg->u.request.u.tm_sync_notice.nid[j],
                                      msg->u.request.u.tm_sync_notice.handle[j],
                                      false);
                if ( seq > -1 )
                {
                    trans_active--;
                    assert( trans_active >= 0 );
                    trace_printf("[%s] TmSyncCommit seq#=%d, handle=%d\n", 
                        MyName,
                        seq,
                        msg->u.request.u.tm_sync_notice.handle[j]);
                    trans_commit++;
                    completed = true;
                }
            }
            unlock_test();
        }
        else if( msg->type == MsgType_ProcessDeath )
        {
            if (( msg->u.request.u.death.trans_id.txid[0] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[1] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[2] == 0 ) &&
                ( msg->u.request.u.death.trans_id.txid[3] == 0 )   )
            {
                if ( tmProcess[msg->u.request.u.death.nid].pid == 
                     msg->u.request.u.death.pid                   )
                {
                    tmProcess[msg->u.request.u.death.nid].dead = true;
                    //tmProcess[msg->u.request.u.death.nid].pid = -1;
                    dead_tm_count++;
                    if ( tmProcess[msg->u.request.u.death.nid].leader )
                    {
                        TmLeaderDied = true;
                    }
                }
                trace_printf("[%s] TM Process Death Notification for Nid=%d, Pid=%d\n",
                    MyName, msg->u.request.u.death.nid, msg->u.request.u.death.pid);
            }
            else
            {
                trace_printf("[%s] Transaction Process Death Notification for Nid=%d, Pid=%d, Trans_id=%lld.%lld.%lld.%lld\n",
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
            trace_printf("[%s] Node %d (%s) is DOWN, Transactions aborted\n",
                MyName, 
                msg->u.request.u.down.nid,
                msg->u.request.u.down.node_name);
            node_up[msg->u.request.u.down.nid]=false;
            down_node_count++;
            NumNodes--;
        }
        else if ( msg->type == MsgType_NodeUp )
        {
            trace_printf("[%s] Node %d (%s) is UP\n",
                MyName, 
                msg->u.request.u.up.nid,
                msg->u.request.u.up.node_name);
            up_node_count++;
        }
        else
        {
            trace_printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }
    }
    else
    {
        trace_printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }    
    return completed;
}

void MessageTypeString( char *str, MSGTYPE type )
{
    switch( type )
    {
        case MsgType_Change:
            sprintf(str, "%s", "MsgType_Change" );
            break;
        case MsgType_Close:
            sprintf(str, "%s", "MsgType_Close" );
            break;
        case MsgType_Event:
            sprintf(str, "%s", "MsgType_Event" );
            break;
        case MsgType_NodeDown:
            sprintf(str, "%s", "MsgType_NodeDown" );
            break;
        case MsgType_Open:
            sprintf(str, "%s", "MsgType_Open" );
            break;
        case MsgType_ProcessCreated:
            sprintf(str, "%s", "MsgType_ProcessCreated" );
            break;
        case MsgType_ProcessDeath:
            sprintf(str, "%s", "MsgType_ProcessDeath" );
            break;
        case MsgType_Service:
            sprintf(str, "%s", "MsgType_Service" );
            break;
        case MsgType_Shutdown:
            sprintf(str, "%s", "MsgType_Shutdown" );
            break;
        case MsgType_TmSyncAbort:
            sprintf(str, "%s", "MsgType_TmSyncAbort" );
            break;
        case MsgType_TmSyncCommit:
            sprintf(str, "%s", "MsgType_TmSyncCommit" );
            break;
        case MsgType_UnsolicitedMessage:
            sprintf(str, "%s", "MsgType_UnsolicitedMessage" );
            break;
        default:
            sprintf(str, "%s", "MsgType - Undefined" );
            break;
    }
}

void RequestTypeString( char *str, REQTYPE type )
{
    switch( type )
    {
        case ReqType_Close:
            sprintf(str, "%s", "ReqType_Close" );
            break;
        case ReqType_Exit:
            sprintf(str, "%s", "ReqType_Exit" );
            break;
        case ReqType_Event:
            sprintf(str, "%s", "ReqType_Event" );
            break;
        case ReqType_Get:
            sprintf(str, "%s", "ReqType_Get" );
            break;
        case ReqType_Kill:
            sprintf(str, "%s", "ReqType_Kill" );
            break;
        case ReqType_Mount:
            sprintf(str, "%s", "ReqType_Mount" );
            break;
        case ReqType_NewProcess:
            sprintf(str, "%s", "ReqType_NewProcess" );
            break;
        case ReqType_NodeDown:
            sprintf(str, "%s", "ReqType_NodeDown" );
            break;
        case ReqType_NodeInfo:
            sprintf(str, "%s", "ReqType_NodeInfo" );
            break;
        case ReqType_NodeUp:
            sprintf(str, "%s", "ReqType_NodeUp" );
            break;
        case ReqType_Notice:
            sprintf(str, "%s", "ReqType_Notice" );
            break;
        case ReqType_Notify:
            sprintf(str, "%s", "ReqType_Notify" );
            break;
        case ReqType_Open:
            sprintf(str, "%s", "ReqType_Open" );
            break;
        case ReqType_OpenInfo:
            sprintf(str, "%s", "ReqType_OpenInfo" );
            break;
        case ReqType_ProcessInfo:
            sprintf(str, "%s", "ReqType_ProcessInfo" );
            break;
        case ReqType_Set:
            sprintf(str, "%s", "ReqType_Set" );
            break;
        case ReqType_Shutdown:
            sprintf(str, "%s", "ReqType_Shutdown" );
            break;
        case ReqType_Startup:
            sprintf(str, "%s", "ReqType_Startup" );
            break;
        case ReqType_TmLeader:
            sprintf(str, "%s", "ReqType_TmLeader" );
            break;
        case ReqType_TmSync:
            sprintf(str, "%s", "ReqType_TmSync" );
            break;
        case ReqType_TransInfo:
            sprintf(str, "%s", "ReqType_TransInfo" );
            break;
        case ReqType_Stfsd:
            sprintf(str, "%s", "ReqType_Stfsd" );
            break;
        case ReqType_ProcessInfoCont:
            sprintf(str, "%s", "ReqType_ProcessInfoCont" );
            break;
        default:
            sprintf(str, "%s", "ReqType - Undefined" );
            break;
    }
}

void recv_localio_msg(struct message_def *recv_msg, int size)
{
    int  rc = -1;
    char msgTypeStr[30] = {'\0'};
    char reqTypeStr[30] = {'\0'};
    size = size; // Avoid "unused parameter" warning

    // CHECK TO SEE THAT WE HAVE STARTED THE TEST BEFORE WE CONTINUE
    while ( !Test_Initialized ) usleep(1000);

    trace_printf("[%s] Message received: ",MyName);
    switch ( recv_msg->type )
    {
        case MsgType_Service:
            trace_printf("Service Reply: Type=%d, ReplyType=%d\n",recv_msg->type, recv_msg->u.reply.type);
            break;
        
        case MsgType_Event:
            trace_printf("Event - %d\n",recv_msg->u.request.u.event_notice.event_id);
            if ( EventMsgRecv )
            {
                trace_printf("[%s] - Event Message overrun!\n", MyName);
                abort();
            }
            EventMsgRecv = true;
            process_event( recv_msg );
            break;

        case MsgType_UnsolicitedMessage:
            trace_printf("Unsolicited Message:\n");
            if ( UnsolMsgRecv )
            {
                trace_printf("[%s] - Unsolicitied Message overrun!\n", MyName);
//                abort();
            }

            UnsolMsgRecv = true;
            lock_test();
            if ( TestWait )
            {
                int handle;
                struct sync_buf_def *buf;
                handle = recv_msg->u.request.u.unsolicited_tm_sync.handle;
                buf = (struct sync_buf_def*)recv_msg->u.request.u.unsolicited_tm_sync.data;
                trace_printf("[%s] QUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                    recv_msg->u.request.u.unsolicited_tm_sync.nid,
                    recv_msg->u.request.u.unsolicited_tm_sync.pid,
                    handle,
                    buf->seq_number,
                    buf->length,
                    buf->string);
                unsolicited_queue.push( *recv_msg );
                unlock_test();
            }
            else
            {
                unlock_test();
                process_unsolicited_msg( recv_msg );

                lock_unsolicited();
                rc = signal_unsolicited();
                if ( rc == -1 )
                {
                    exit( 1);
                }
                unlock_unsolicited();
            }
            break;

         default:
            MessageTypeString( msgTypeStr, recv_msg->type );
            RequestTypeString( reqTypeStr, recv_msg->u.request.type );
            trace_printf("Notice: Type=%d(%s), RequestType=%d(%s)\n",recv_msg->type,msgTypeStr,recv_msg->u.request.type,reqTypeStr);
            if ( NoticeMsgRecv )
            {
                printf("[%s] - Notice Message overrun!\n", MyName);
//                abort();
            }
            NoticeMsgRecv = true;
            process_notice( recv_msg );
            lock_notice();
            rc = signal_notice();
            if ( rc == -1 )
            {
                exit( 1);
            }
            unlock_notice();
    }
}

MPI_Status tm_sync( int seq_number )
{
    struct sync_buf_def buf;
    MPI_Status status;
    struct message_def *msg;

    trace_printf ("[%s] sending tm_sync request.\n", MyName);
    fflush (stdout);

    // check if we need a new leader
    if (node_down)
    {
        request_to_become_TmLeader();
        node_down = false;
    }

    gp_local_mon_io->acquire_msg( &msg );

    // build seq buffer
    buf.seq_number = seq_number;
    sprintf(buf.string,"Master TmSync start #%d from Node %d",trans_starts,MyNid);
    buf.length = strlen(buf.string);
    
    // build monitor request
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_TmSync;
    msg->u.request.u.tm_sync.nid = MyNid;
    msg->u.request.u.tm_sync.pid = MyPid;
    msg->u.request.u.tm_sync.length = sizeof(struct sync_buf_def);
    memmove(msg->u.request.u.tm_sync.data,&buf,msg->u.request.u.tm_sync.length);
    trace_printf ("[%s] tm_sync data length=%d\n",MyName,msg->u.request.u.tm_sync.length);
 
    gp_local_mon_io->send_recv( msg );
    status.MPI_TAG = msg->reply_tag;

    if ((msg->type == MsgType_Service) &&
        (msg->u.reply.type == ReplyType_TmSync))
    {
        if (msg->u.reply.u.tm_sync.return_code == MPI_SUCCESS)
        {
            trace_printf ("[%s] tm_sync request successfully. rc=%d\n",
                    MyName, msg->u.reply.u.tm_sync.return_code);
            trace_printf("[%s] TmSync Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                MyNid,
                MyPid,
                msg->u.reply.u.tm_sync.handle,
                buf.seq_number,
                buf.length,
                buf.string);
            request[MyNid][seq_number].handle = msg->u.reply.u.tm_sync.handle;
            trans_count++;
            trans_active++;
            status.MPI_ERROR = msg->u.reply.u.tm_sync.return_code;        
        }
        else
        {
            trace_printf ("[%s] tm_sync request failed, rc=%d\n", MyName,
                    msg->u.reply.u.tm_sync.return_code);
            status.MPI_ERROR = msg->u.reply.u.tm_sync.return_code;        
        }
    }
    else
    {
        status.MPI_ERROR = -1;
        trace_printf ("[%s] Invalid MsgType(%d)/ReplyType(%d) for tm_sync request\n",
             MyName, msg->type, msg->u.reply.type);
    }

    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

    msg = NULL;
    return status;
}

bool wait_for_event()
{
    MPI_Status status;
    struct message_def *msg;

    trace_printf ("[%s] Waiting for event message.\n", MyName);

    status.MPI_ERROR = gp_local_mon_io->wait_for_event( &msg );

    process_event(msg);
    if (status.MPI_ERROR != MPI_SUCCESS)
    {
        trace_printf ("[%s] Recv failed, rc = %d\n", MyName, status.MPI_ERROR);
    }
    delete msg;
    return (status.MPI_ERROR == MPI_SUCCESS);
}

bool wait_for_notice()
{
    int rc = -1;
    trace_printf ("[%s] Waiting for tm_sync completion notice.\n", MyName);

    lock_notice();
    rc = wait_on_notice();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_notice();

    return ( rc == 0 );
}

bool wait_for_unsolicited_msg()
{
    int rc = -1;
    trace_printf ("[%s] Waiting for unsolicited message.\n", MyName);
    
    lock_unsolicited();
    rc = wait_on_unsolicited();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_unsolicited();

    return ( rc == 0 );
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
                trace_printf("[%s], Failed to load cluster configuration.\n", MyName);
                abort();
            }
        }
        else
        {
            trace_printf( "[%s] Warning: No cluster.conf found\n",MyName);
            if (MyNid == -1)
            {
                trace_printf( "[%s] Warning: set default virtual node ID = 0\n",MyName);
                MyNid = 0;
            }
            abort();
        }

        lnodeConfig = ClusterConfig.GetLNodeConfig( MyNid );
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        gv_ms_su_nid = MyPNid = pnodeConfig->GetPNid();
        trace_printf ("[%s] Local IO pnid = %d\n", MyName, MyPNid);
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
    MPI_Status status;
    bool done = false;
    bool end_of_test = false;
    
    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);

    trace_init((char *)"STDOUT", false, NULL, false);

    MyName = new char [MAX_PROCESS_PATH];
    strcpy( MyName, argv[5] );
    MyNid = atoi(argv[3]);

    int rc = pthread_mutex_init( &notice_mutex, NULL );
    if (rc)
    {
        trace_printf("[%s] Error initializing notice mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
        exit(1);
    }
    
    rc = pthread_mutex_init( &test_mutex, NULL );
    if (rc)
    {
        trace_printf("[%s] Error initializing test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
        exit(1);
    }
    
    rc = pthread_mutex_init( &unsolicited_mutex, NULL );
    if (rc)
    {
        trace_printf("[%s] Error initializing unsolicited mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
        exit(1);
    }
    
    // Check if we are using virtual nodes
    char *cmd_buffer = getenv("SQ_VIRTUAL_NODES");
    if (cmd_buffer && isdigit(cmd_buffer[0]))
    {
        VirtualNodes = atoi(cmd_buffer);
        if (VirtualNodes > 8) VirtualNodes = 8;
    }
    else
    {
        VirtualNodes = 0;
    }

    if ( ! gp_local_mon_io )
    {
        InitLocalIO();
        assert (gp_local_mon_io);
    }
 
    gp_local_mon_io->set_cb(recv_localio_msg, "notice");
    gp_local_mon_io->set_cb(recv_localio_msg, "recv");
    gp_local_mon_io->set_cb(recv_localio_msg, "unsol");

#if 0
    struct rlimit rl;
    int rc = getrlimit( RLIMIT_SIGPENDING, &rl );
    if ( rc == 0 )
    {
        trace_printf("[%s] RLIMIT_SIGPENDING cur=%d, max=%d\n", MyName, (int)rl.rlim_cur, (int)rl.rlim_max);
    }
#endif

    process_startup (argc, argv);
    NumNodes = GetNumOfNode();

    do
    {
        end_of_test = false;
        initialize_test();
        // wait for event signal to start test across all TMs
        wait_for_event();
        request_to_become_TmLeader();
        switch ( event_id )
        {
        case 1:
             // *** test 1 -- failed TM test w/ restart of TMs
             trace_printf("[%s] Test1 - TM process death test w/restart of TM processes - Waiting to start test.\n",MyName);
             if ( get_tm_processes( MAX_TEST_NODES ) )
             {
                abort();
             }
             if ( MyNid == 2 ||
                 (!VirtualNodes && MyNid == 3) )
             {
                // Wait for other TMs to start, then die!
                sleep(3);
                trace_printf("[%s] - Test1 - Stopping to halt node\n",MyName);
                fflush (stdout);
                MPI_Abort (MPI_COMM_WORLD, 99);
             }
             else
             {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                trace_printf("[%s] Test1 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test1(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    trace_printf("[%s] Test1(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }  
                else
                {
                    trace_printf("[%s] Test1 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
                trace_printf("[%s] Test1 - Wait to commit or abort TM sync data.\n",MyName);
             }
             while (((trans_commit+trans_abort)<NumNodes || trans_active) && ! end_of_test )
             {
                fflush (stdout);
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        trace_printf("[%s] Test1 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99 );
                    }
                }
                trace_printf("[%s] Test1 - Dead TM count =%d\n",MyName,dead_tm_count);
                if ( dead_tm_count == 2 && up_node_count == 2 )
                {
                    up_node_count = 0;
                    if ( IamTmLeader )
                    {
                        trace_printf("[%s] Test1 - TM leader is restarting dead TM processes.\n",MyName);
                        if ( restart_all_dead_tm() )
                        {
                            trace_printf("[%s] Test1 - Failed to restart dead TM processes! Aborting\n",MyName);
                            fflush (stdout);
                            MPI_Abort( MPI_COMM_WORLD, 99 );
                        }
                        wait_for_event(); // event 2
                        end_of_test = true;
                    }
                    else
                    {
                        trace_printf("[%s] Test1 - Wait for restart of dead TM processes.\n",MyName);
                        wait_for_event(); // event 2
                        end_of_test = true;
                    }
                }
                //printf("[%s] Test1 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
                if ( TestShutdown == true )
                {
                    end_of_test = true;
                }
             }
             trace_printf("[%s] Test1 - TM process death test w/restart of TM processes - Completed test.\n",MyName);
             break;
        case 2:
             // *** test 1 -- failed TM test w/ restart of TMs
             // Restarted TMs execute this path (event 2), surviving TMs are in case 1: above
             trace_printf("[%s] Test1 - TM process death test w/restart of TM processes - Completed test.\n",MyName);
             break;

        case 3:
            // *** test 3 - No collision Commit test
            Abort_transaction = false;
            trace_printf("[%s] Test3 - No collision Commit test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            // TM on logical node 2 to start sync
            if ( MyNid == 2 )
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                trace_printf("[%s] Test3 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test3 - Started TM Sync operation.\n",MyName);
                }
                else
                {
                    trace_printf("[%s] Test3 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
            }
            else
            {
                trace_printf("[%s] TM Test3 - Sync already started ... waiting for TM Sync data.\n",MyName);
                if ( !wait_for_unsolicited_msg() )
                {
                    trace_printf("[%s] Test3 - Failed to receive unsolicited message! Aborting\n",MyName);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99 );
                }
            }    
            trace_printf("[%s] Test3 - Wait to commit or abort TM sync data.\n",MyName);
            while( !wait_for_notice() )
            {
                if (trans_active)
                {
                    trace_printf("[%s] Test3 - Failed to receive notice! Aborting\n",MyName);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99 );
                }
                else
                {
                    trace_printf("[%s] Test3 - No transaction active.\n",MyName);
                }
            }
            Test_Initialized = false;
            trace_printf("[%s] Test3 - No collision Commit test - Completed test.\n",MyName);
            break;
            
        case 4:
            // *** test 4 -- No collision Abort test
            Abort_transaction = (MyNid==5?true:false);
            trace_printf("[%s] Test4 - No collision Abort test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            // TM on logical node 0 to start sync
            if ( MyNid == 1 )
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                trace_printf("[%s] Test4 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test4 - Started TM Sync operation.\n",MyName);
                }
                else
                {
                    trace_printf("[%s] Test4 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
            }
            else
            {
                trace_printf("[%s] TM Test4 - Sync already started ... waiting for TM Sync data.\n",MyName);
                if ( !wait_for_unsolicited_msg() )
                {
                    trace_printf("[%s] Test4 - Failed to receive unsolicited message! Aborting\n",MyName);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99 );
                }
            }    
            trace_printf("[%s] Test4 - Wait to commit or abort TM sync data.\n",MyName);
            while( !wait_for_notice() )
            {
                if (trans_active)
                {
                    trace_printf("[%s] Test4 - Failed to receive notice! Aborting\n",MyName);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99 );
                }
                else
                {
                    trace_printf("[%s] Test4 - No transaction active.\n",MyName);
                }
            }
            Test_Initialized = false;
            trace_printf("[%s] Test4 - No collision Abort test - Completed test.\n",MyName);
            break;
        
        case 5:
            // *** test 5 -- Collision Commit test
            Abort_transaction = false;
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            trace_printf("[%s] Test5 - Collision Commit test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                trace_printf("[%s] Test5(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test5(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    trace_printf("[%s] TM Test5(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    trace_printf("[%s] Test5 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
                trans_starts++;
                if ( trans_starts%(MyNid+1) == 0 ) usleep(1000*MyNid);
            }
            while ( trans_starts < MAX_SYNCS );
            trace_printf("[%s] Test5 - Wait to commit or abort TM sync data.\n",MyName);
            while ( !completed_test() )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        trace_printf("[%s] Test5 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99 );
                    }
                }
            }
            trace_printf("[%s] Test5 - Collision Commit test - Completed test.\n",MyName);
            break;
            
        case 6:
            // *** test 6 -- Collision Abort test
            Abort_transaction = (MyNid==2?true:false);
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            trace_printf("[%s] Test6 - Collision Abort test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                trace_printf("[%s] Test6(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test6(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    trace_printf("[%s] TM Test6(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    trace_printf("[%s] Test6 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
                trans_starts++;
                if ( trans_starts%(MyNid+1) == 0 ) usleep(1000*MyNid);
            }
            while ( trans_starts < MAX_SYNCS );
            trace_printf("[%s] Test6 - Wait to commit or abort TM sync data.\n",MyName);
            while ( !completed_test() )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        trace_printf("[%s] Test6 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99 );
                    }
                }
            }
            trace_printf("[%s] Test6 - Collision Abort test - Completed test.\n",MyName);
            break;
            
       case 7:     
            // *** test 7 -- failed TM test
            trace_printf("[%s] Test7 - TM process death test w/ node failure - Waiting to start test.\n",MyName);
            if ( MyNid == 2 ||
                (!VirtualNodes && MyNid == 3) )
            {
                usleep(SYNC_DELAY*2); // delay to allow at least two sync cycles
                trace_printf("[%s] - Test7 - Stopping to halt node\n",MyName);
                fflush (stdout);
                MPI_Abort (MPI_COMM_WORLD, 99);
            }
            else
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                trace_printf("[%s] Test7 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test7(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    trace_printf("[%s] TM Test7(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }  
                else
                {
                    trace_printf("[%s] Test7 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
                trace_printf("[%s] Test7 - Wait to commit or abort TM sync data.\n",MyName);
            }
            while ( ((trans_commit+trans_abort) < (NumNodes-1)) || trans_active )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        trace_printf("[%s] Test7 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99 );
                    }
                }
#if 0
                if ( down_node_count )
                {
                    down_node_count = 0;
                    NumNodes = GetNumOfNode();
                }
#endif
                trace_printf("[%s] Test7 - Counters: Commit#=%d, Abort#=%d, NumNodes#=%d, Active=%d\n",
                    MyName,trans_commit,trans_abort,NumNodes,trans_active);
            }
            trace_printf("[%s] Test7 - TM process death test w/ node failure - Completed test.\n",MyName);
            break;

        case 8:
             // *** test 8 -- failed TM test w/ restart of TMs while TmSync is in process
             trace_printf("[%s] Test8 - Collision Abort TM process death test w/restart of TM processes - Waiting to start test.\n",MyName);
             // suppress processing of other TmSyncs
             lock_test();
             TestWait = true;
             // abort processing of other TmSyncs
             Abort_transaction = false;
             if ( get_tm_processes( MAX_TEST_NODES ) )
             {
                abort();
             }
             unlock_test();
             if ( MyNid == 2 )
             {
                sleep(3);
                // delay to allow at least three sync cycles
                // and die in the middle of a TmSync
                usleep(SYNC_DELAY*3); 
                trace_printf("[%s] - Test8 - Stopping to halt node\n",MyName);
                fflush (stdout);
                MPI_Abort (MPI_COMM_WORLD, 99);
             }
             else
             {
                if ( MyNid == 3 )
                {
                    // delay to allow nid 2 to abort
                    sleep(3);
                    usleep(SYNC_DELAY*3); 
                    if ( !VirtualNodes )
                    {
                        // on a real cluster the watchdog process 
                        // does not know about the testtm program 
                        usleep(SYNC_DELAY*3); 
                        MPI_Abort (MPI_COMM_WORLD, 99);
                    }
                }
                else
                {
                    sleep(1);
                }
                trace_printf("[%s] Test8 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
                do
                {
                    trace_printf("[%s] Test8(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                    status = tm_sync(trans_starts);
                    if( status.MPI_ERROR == MPI_SUCCESS )
                    {
                        trace_printf("[%s] Test8(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                    }
                    else if( status.MPI_ERROR == MPI_ERR_PENDING )
                    {
                        trace_printf("[%s] TM Test8(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                    }
                    else
                    {
                        trace_printf("[%s] Test8 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99);
                    }
                    trans_starts++;
                } while ( trans_starts < MAX_SYNCS );
             }

             sleep(1);
             if ( TestWait )
             {
                 // Nid 2 may abort while TmSyncs are handled in this loop
                 while (!unsolicited_queue.empty())
                 {
                    struct message_def msg = unsolicited_queue.front();
                    int handle;
                    struct sync_buf_def *buf;
                    handle = msg.u.request.u.unsolicited_tm_sync.handle;
                    buf = (struct sync_buf_def*)msg.u.request.u.unsolicited_tm_sync.data;
                    trace_printf("[%s] DEQUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                        msg.u.request.u.unsolicited_tm_sync.nid,
                        msg.u.request.u.unsolicited_tm_sync.pid,
                        handle,
                        buf->seq_number,
                        buf->length,
                        buf->string);
                    process_unsolicited_msg( &msg );
                    unsolicited_queue.pop();
                 }
                 lock_test();
                 TestWait = false;
                 unlock_test();
             }
             sleep(2);
             
             while ( !completed_test() && !end_of_test  )
             {
                fflush (stdout);
                // wait death notice or TmSync notice
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        trace_printf("[%s] Test8 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        MPI_Abort( MPI_COMM_WORLD, 99 );
                    }
                }
                trace_printf("[%s] Test8 - Dead TM count =%d\n",MyName,dead_tm_count);
                if ( dead_tm_count == 2)
                {
                    if ( IamTmLeader )
                    {
                        trace_printf("[%s] Test8 - TM leader is restarting dead TM processes.\n",MyName);
                        if ( restart_all_dead_tm() )
                        {
                            trace_printf("[%s] Test8 - Failed to restart dead TM processes! Aborting\n",MyName);
                            fflush (stdout);
                            MPI_Abort( MPI_COMM_WORLD, 99 );
                        }
                        wait_for_event(); // event 9
                    }
                    else
                    {
                        trace_printf("[%s] Test8 - Wait for restart of dead TM processes.\n",MyName);
                        wait_for_event(); // event 9
                    }
                    trace_printf("[%s] Test8 - Start#=%d, Abort#=%d, Commit#=%d, Total Transaction=%d\n",
                        MyName, trans_starts,trans_abort,trans_commit,trans_count);
                    fflush (stdout);
                    initialize_test();
                    usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                    trace_printf("[%s] Test8 - Resending TM Sync request (abort=%d)\n",MyName, Abort_transaction);
                    do
                    {
                        trace_printf("[%s] Test8(%d) - Sending  TM Sync request.\n",MyName,trans_starts);
                        status = tm_sync(trans_starts);
                        if( status.MPI_ERROR == MPI_SUCCESS )
                        {
                            trace_printf("[%s] Test8(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                        }
                        else if( status.MPI_ERROR == MPI_ERR_PENDING )
                        {
                            trace_printf("[%s] TM Test8(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                        }
                        else
                        {
                            trace_printf("[%s] Test8 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                            fflush (stdout);
                            MPI_Abort( MPI_COMM_WORLD, 99);
                        }
                        trans_starts++;
                    } while ( trans_starts < MAX_SYNCS );
                }
                printf("[%s] Test8 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
                if ( TestShutdown == true )
                {
                    end_of_test = true;
                }
             }
            trace_printf("[%s] Test8 - Collision Abort TM process death test w/restart of TM processes - Completed test.\n",MyName);
            break;
        case 9:
            // *** test 8 -- failed TM test w/ restart of TMs
            // Restarted TMs execute this path (event 9), surviving TMs are in case 8: above
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            trace_printf("[%s] Test8 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                trace_printf("[%s] Test8(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    trace_printf("[%s] Test8(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    trace_printf("[%s] TM Test8(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    trace_printf("[%s] Test8 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    MPI_Abort( MPI_COMM_WORLD, 99);
                }
                trans_starts++;
            } while ( trans_starts < MAX_SYNCS );

            while ( !completed_test() )
            {
               fflush (stdout);
               if( !wait_for_notice() )
               {
                   if (trans_active)
                   {
                       trace_printf("[%s] Test8 - Failed to receive notice! Aborting\n",MyName);
                       fflush (stdout);
                       MPI_Abort( MPI_COMM_WORLD, 99 );
                   }
               }
               printf("[%s] Test8 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
            }
            trace_printf("[%s] Test8 - Collision Abort TM process death test w/restart of TM processes - Completed test.\n",MyName);
            break;

        case 10:
            // *** test 10 -- failed TM test w/out restart of TMs
            // This test must be performed after test1, so that there is no spare node
            // and TmSyncs are in processs when a node goes down
            trace_printf("[%s] Test10 - Collision Abort TM process death test w/ node failure - Waiting to start test.\n",MyName);
            // suppress processing of other TmSyncs
            lock_test();
            TestWait = true;
            // abort processing of other TmSyncs
            Abort_transaction = false;
            
            if ( get_tm_processes( MAX_TEST_NODES ) )
            {
               abort();
            }
            unlock_test();
            if ( MyNid == 2 )
            {
               sleep(3);
               // delay to allow at least three sync cycles
               // and die in the middle of a TmSync
               usleep(SYNC_DELAY*3); 
               trace_printf("[%s] - Test10 - Stopping to halt node\n",MyName);
               fflush (stdout);
               MPI_Abort (MPI_COMM_WORLD, 99);
            }
            else
            {
               if ( MyNid == 3 )
               {
                   // delay to allow nid 2 to abort
                   sleep(3);
                   usleep(SYNC_DELAY*3); 
               }
               else
               {
                   sleep(1);
               }
               trace_printf("[%s] Test10 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
               do
               {
                   trace_printf("[%s] Test10(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                   status = tm_sync(trans_starts);
                   if( status.MPI_ERROR == MPI_SUCCESS )
                   {
                       trace_printf("[%s] Test10(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                   }
                   else if( status.MPI_ERROR == MPI_ERR_PENDING )
                   {
                       trace_printf("[%s] TM Test10(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                   }
                   else
                   {
                       trace_printf("[%s] Test10 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                       fflush (stdout);
                       MPI_Abort( MPI_COMM_WORLD, 99);
                   }
                   trans_starts++;
               } while ( trans_starts < MAX_SYNCS );
            }

            sleep(1);
            if ( TestWait )
            {
                // Nid 2 may abort while TmSyncs are handled in this loop
                while (!unsolicited_queue.empty())
                {
                   struct message_def msg = unsolicited_queue.front();
                   int handle;
                   struct sync_buf_def *buf;
                   handle = msg.u.request.u.unsolicited_tm_sync.handle;
                   buf = (struct sync_buf_def*)msg.u.request.u.unsolicited_tm_sync.data;
                   trace_printf("[%s] DEQUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                       msg.u.request.u.unsolicited_tm_sync.nid,
                       msg.u.request.u.unsolicited_tm_sync.pid,
                       handle,
                       buf->seq_number,
                       buf->length,
                       buf->string);
                   process_unsolicited_msg( &msg );
                   unsolicited_queue.pop();
                }
                lock_test();
                TestWait = false;
                unlock_test();
            }
            sleep(2);
            
            while ( !completed_test() && !end_of_test )
            {
               fflush (stdout);
               if( !wait_for_notice() )
               {
                   if (trans_active)
                   {
                       trace_printf("[%s] Test10 - Failed to receive notice! Aborting\n",MyName);
                       fflush (stdout);
                       MPI_Abort( MPI_COMM_WORLD, 99 );
                   }
               }
               trace_printf("[%s] Test10 - Dead TM count =%d\n",MyName,dead_tm_count);
               if ( dead_tm_count == 2)
               {
                   trace_printf("[%s] Test10 - Waiting for node to die.\n",MyName);
                   wait_for_event(); // event 11
     
                   usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                   trace_printf("[%s] Test10 - Start#=%d, Abort#=%d, Commit#=%d, Total Transaction=%d\n",
                       MyName, trans_starts,trans_abort,trans_commit,trans_count);
                   fflush (stdout);
                   initialize_test();
                   // force completion of requests from node 2 and 3 since they are dead
                   end_requests( 2 );
                   end_requests( 3 );
                   trace_printf("[%s] Test10 - Resending TM Sync request (abort=%d)\n",MyName, Abort_transaction);
                   do
                   {
                       trace_printf("[%s] Test10(%d) - Sending  TM Sync request.\n",MyName,trans_starts);
                       status = tm_sync(trans_starts);
                       if( status.MPI_ERROR == MPI_SUCCESS )
                       {
                           trace_printf("[%s] Test10(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                       }
                       else if( status.MPI_ERROR == MPI_ERR_PENDING )
                       {
                           trace_printf("[%s] TM Test10(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                       }
                       else
                       {
                           trace_printf("[%s] Test10 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                           fflush (stdout);
                           MPI_Abort( MPI_COMM_WORLD, 99);
                       }
                       trans_starts++;
                   } while ( trans_starts < MAX_SYNCS );
               }
               printf("[%s] Test10 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
               if ( TestShutdown == true )
               {
                   end_of_test = true;
               }
            }
            trace_printf("[%s] Test10 - Collision Abort TM process death test w/ node failure - Completed test.\n",MyName);
            break;
        default:
            trace_printf("[%s] Event = %d, No test defined, stopping\n",MyName, event_id);
            done = true;
        }

        if (!done)
        {
            int test_num;
            test_num = (event_id == 2) ? 1 : event_id;
            test_num = (test_num == 9) ? 8 : test_num;
            test_num = (test_num == 11) ? 10 : test_num;
            trace_printf("[%s] Test%d - Start#=%d, Abort#=%d, Commit#=%d, Total Transaction=%d\n",
                MyName, test_num, trans_starts,trans_abort,trans_commit,trans_count);
            fflush (stdout);
        }
    }
    while (!done);    
                
    // exit my process
    exit_process ();

    trace_printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port( MyPort );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
