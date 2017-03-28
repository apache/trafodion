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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "tmSyncCtrl.h"

#include <sys/time.h>
#include <sys/resource.h>

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0};

#include <queue>
using namespace std;

#define MAX_SYNCS 10
#define MAX_TEST_NODES 6
#define SYNC_DELAY 1500000 // delay (1500 ms)


int NumNodes = 0;

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
struct tm_process_def {
    bool dead;
    bool leader;
    int  pid;
    char process_name[MAX_PROCESS_NAME];    // TM process's Name
    char program[MAX_PROCESS_PATH];
} tmProcess[MAX_TEST_NODES];
bool Abort_transaction = false;
bool Test_Initialized = false;
bool TestWait = false;
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

pthread_mutex_t     unsolicited_mutex;
pthread_cond_t      unsolicited_cv;
bool                unsolicited_signaled = false;

pthread_mutex_t     event_mutex;
pthread_cond_t      event_cv;
bool                event_signaled = false;


struct sync_buf_def {
    int seq_number;
    int length;
    char string[132];
};

queue<struct message_def> unsolicited_queue; // unsolicited msg queue

void util_set_core_free() {
    struct rlimit limit;
    limit.rlim_cur = 0;
    limit.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &limit);
}

void lock_notice()
{
    int rc = pthread_mutex_lock(&notice_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to lock notice mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_notice()
{
    int rc = pthread_mutex_unlock(&notice_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to unlock notice mutex: %s (%d)\n",
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
        printf("[%s] - Unable to signal notice: %s (%d)\n",
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
            printf("[%s] - Unable to signal notice: %s (%d)\n",
                         MyName, strerror(errno), errno);
            rc = -1;
        }
    }
    notice_signaled = false;

    return( rc );
}

void lock_event()
{
    int rc = pthread_mutex_lock(&event_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to lock event mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_event()
{
    int rc = pthread_mutex_unlock(&event_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to unlock event mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}

int signal_event()
{
    int rc = 0;

    event_signaled = true;
    rc = pthread_cond_broadcast(&event_cv);
    if ( rc != 0)
    {
        errno = rc;
        printf("[%s] - Unable to signal event: %s (%d)\n",
                     MyName, strerror(errno), errno);
        rc = -1;
    }

    return( rc );
}

int wait_on_event( void )
{
    int rc = 0;

    if ( ! event_signaled )
    {
        rc = pthread_cond_wait(&event_cv, &event_mutex);
        if ( rc != 0)
        {
            errno = rc;
            printf("[%s] - Unable to signal event: %s (%d)\n",
                         MyName, strerror(errno), errno);
            rc = -1;
        }
    }
    event_signaled = false;

    return( rc );
}

void lock_test()
{
    int rc = pthread_mutex_lock(&test_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to lock test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_test()
{
    int rc = pthread_mutex_unlock(&test_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to unlock test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}

void lock_unsolicited()
{
    int rc = pthread_mutex_lock(&unsolicited_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to lock unsolicited mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
    }
}


void unlock_unsolicited()
{
    int rc = pthread_mutex_unlock(&unsolicited_mutex);

    if (rc != 0)
    {
        printf("[%s] - Unable to unlock unsolicited mutex: %s (%d)\n",
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
        printf("[%s] - Unable to signal unsolicited: %s (%d)\n",
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
            printf("[%s] - Unable to signal unsolicited: %s (%d)\n",
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
    printf("[%s] - Completing request: node=%d, handle=%d, abort=%d\n", MyName, nid, handle, abort);

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

    printf("[%s] - Can't find TmSync handle=%d\n", MyName, handle);
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
                    printf("[%s] - Test not completed @ node=%d,  handle index=%d\n",MyName,nid,j);
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
    printf( "[%s] - Ending requests for: node=%d\n", MyName, nid );

    for(j=0; j<MAX_SYNCS; j++)
    {
        request[nid][j].completed = true;
        request[nid][j].aborted = true;
    }
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
    msg->u.request.u.process_info.nid = util.getNid();
    msg->u.request.u.process_info.pid = util.getPid();
    msg->u.request.u.process_info.verifier = util.getVerifier();
    msg->u.request.u.process_info.process_name[0] = 0;
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
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
                        printf ( "[%s] TM ProcessInfo: nid=%d, pid=%d, Name=%s, program=%s\n", MyName
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
                    printf( "[%s] TM ProcessInfo failed, invalid number of TM processes, count=%d\n", MyName
                          , msg->u.reply.u.process_info.num_processes);
                    return 1;
                }
            }
            else
            {
                printf ("[%s] TM ProcessInfo failed, error=%s\n", MyName,
                    util.MPIErrMsg(msg->u.reply.u.process_info.return_code));
                return 1;
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for TM ProcessInfo message\n",
                    MyName, msg->type, msg->u.reply.type);
            return 1;
        }
    }
    else
    {
        printf ("[%s] TM ProcessInfo reply message invalid\n", MyName);
        return 1;
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return 0;
}


void reset_trans_counts ( void )
{
    trans_count = 0;
    trans_starts = 0;
    trans_abort = 0;
    trans_commit = 0;
    trans_active=0;
}

void initialize_test( void )
{
    int nid;
    int j;

    printf ("[%s] Initializing for test\n",MyName);
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
    reset_trans_counts();
    dead_tm_count = 0;
    Test_Initialized = true;
}


void process_unsolicited_msg( struct message_def *msg )
{
    int rc;
    int handle;
    struct message_def *reply;
    struct sync_buf_def *buf;

    gp_local_mon_io->acquire_msg( &reply );

    printf ("[%s] Processing unsolicited tm_sync message.\n", MyName);
    if( msg->u.request.type == ReqType_TmSync )
    {
        if( msg->type == MsgType_UnsolicitedMessage )
        {
            handle = msg->u.request.u.unsolicited_tm_sync.handle;

            buf = (struct sync_buf_def*)msg->u.request.u.unsolicited_tm_sync.data;
            request[msg->u.request.u.unsolicited_tm_sync.nid][buf->seq_number].handle = handle;
            printf("[%s] TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
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
            reply->u.reply.u.unsolicited_tm_sync.nid = util.getNid();
            reply->u.reply.u.unsolicited_tm_sync.pid = util.getPid();
            reply->u.reply.u.unsolicited_tm_sync.handle = handle;
            if (Abort_transaction)
            {
                printf("[%s] Sending Abort to monitor.\n",MyName);
                reply->u.reply.u.unsolicited_tm_sync.return_code = 99;
            }
            else
            {
                printf("[%s] Sending Commit to monitor.\n",MyName);
                reply->u.reply.u.unsolicited_tm_sync.return_code = MPI_SUCCESS;
            }
            printf ("[%s] sending TmSync Unsolicited Message reply to monitor.\n", MyName);
            fflush (stdout);

            rc = gp_local_mon_io->send( reply );

            if( rc != MPI_SUCCESS )
            {
                printf ("[%s] Send failed, rc = %d\n", MyName, rc);
            }
        }
        else
        {
            printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }
    }
    else
    {
        printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }
}

void process_event( struct message_def *msg )
{
    printf ("[%s] Processing event message.\n", MyName);
    event_id = -1;
    if( msg->u.request.type == ReqType_Notice )
    {
        if( msg->type == MsgType_Event )
        {
            printf("[%s] Event %d (%s) received\n",
                   MyName,
                   msg->u.request.u.event_notice.event_id,
                   msg->u.request.u.event_notice.data);
            event_id = msg->u.request.u.event_notice.event_id;
            lock_event();
            signal_event();
            unlock_event();

        }
        else
        {
            printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }
    }
    else
    {
        printf("[%s] Invalid Event received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }
}

void request_to_become_TmLeader(void)
{
    int count;
    MPI_Status status;
    struct message_def *msg;

    printf ("[%s] sending TmLeader request.\n", MyName);
    fflush (stdout);


    gp_local_mon_io->acquire_msg( &msg );

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_TmLeader;
    msg->u.request.u.leader.nid = util.getNid();
    msg->u.request.u.leader.pid = util.getPid();

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
                printf ("[%s] I'm the new TmLeader.\n", MyName);
            }
            else
            {
                printf ("[%s] I'm not the TmLeader.\n", MyName);
            }
            if (msg->u.reply.u.generic.return_code == -1)
            {
                printf ("[%s] Failed to get the TmLeader\n", MyName);
            }
            else
            {
                printf ("[%s] The new TmLeader is %s(%d,%d)\n", MyName,
                    msg->u.reply.u.generic.process_name,
                    msg->u.reply.u.generic.nid,
                    msg->u.reply.u.generic.pid);
            }
        }
        else
        {
            printf
                ("[%s] Invalid MsgType(%d)/ReplyType(%d) for TmLeader message\n",
                 MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] TmLeader process reply invalid.\n", MyName);
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

    printf ("[%s] Processing tm_sync completion notice.\n", MyName);
    if( msg->u.request.type == ReqType_Notice )
    {
        if( msg->type == MsgType_TmSyncAbort )
        {
            lock_test();
            printf ("[%s] Processing %d Abort notice(s).\n", MyName, msg->u.request.u.tm_sync_notice.count);
            for(j=0; j < msg->u.request.u.tm_sync_notice.count; j++)
            {
                seq=completed_request(msg->u.request.u.tm_sync_notice.nid[j],
                                      msg->u.request.u.tm_sync_notice.handle[j],
                                      true);
                printf("[%s] TmSyncAbort seq#=%d, handle=%d\n",
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
            printf ("[%s] Processing %d Commit notice(s).\n", MyName, msg->u.request.u.tm_sync_notice.count);
            for(j=0; j < msg->u.request.u.tm_sync_notice.count; j++)
            {
                seq=completed_request(msg->u.request.u.tm_sync_notice.nid[j],
                                      msg->u.request.u.tm_sync_notice.handle[j],
                                      false);
                if ( seq > -1 )
                {
                    trans_active--;
                    assert( trans_active >= 0 );
                    printf("[%s] TmSyncCommit seq#=%d, handle=%d\n",
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
                }
                printf("[%s] TM Process Death Notification for Nid=%d, Pid=%d\n",
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
            up_node_count++;
        }
        else
        {
            printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
                MyName, msg->type, msg->noreply, msg->u.request.type);
        }
    }
    else
    {
        printf("[%s] Invalid TmSync Notice received - msgtype=%d, noreply=%d, reqtype=%d\n",
            MyName, msg->type, msg->noreply, msg->u.request.type);
    }
    return completed;
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

void recv_localio_msg(struct message_def *recv_msg, int )
{
    int  rc = -1;
    char reqTypeStr[30] = {'\0'};

    // CHECK TO SEE THAT WE HAVE STARTED THE TEST BEFORE WE CONTINUE
    while ( !Test_Initialized ) usleep(1000);

    // Format message timestamp
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char tmbuf[64], buf[64];

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof buf, "%s.%06ld", tmbuf, tv.tv_usec);

    printf("[%s] %s Message received: ", MyName, buf);
    switch ( recv_msg->type )
    {
        case MsgType_Service:
            printf("Service Reply: Type=%d, ReplyType=%d\n",recv_msg->type, recv_msg->u.reply.type);
            break;

        case MsgType_Event:
            printf("Event - %d\n",recv_msg->u.request.u.event_notice.event_id);
            process_event( recv_msg );
            break;

        case MsgType_UnsolicitedMessage:
            printf("Unsolicited Message:\n");
            lock_test();
            if ( TestWait )
            {
                int handle;
                struct sync_buf_def *buf;
                handle = recv_msg->u.request.u.unsolicited_tm_sync.handle;
                buf = (struct sync_buf_def*)recv_msg->u.request.u.unsolicited_tm_sync.data;
                printf("[%s] QUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
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

        case MsgType_Shutdown:
            printf("Shutdown notice received\n");
            printf("[%s] Simulating event 100\n", MyName);
            event_id = 100;
            lock_event();
            signal_event();
            unlock_event();

            break;

         default:
            RequestTypeString( reqTypeStr, recv_msg->u.request.type );
            printf("Notice: Type=%d(%s), RequestType=%d(%s)\n",recv_msg->type,MessageTypeString(recv_msg->type),recv_msg->u.request.type,reqTypeStr);
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

    printf ("[%s] sending tm_sync request.\n", MyName);
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
    sprintf(buf.string,"Master TmSync start #%d from Node %d",trans_starts,util.getNid());
    buf.length = strlen(buf.string);

    // build monitor request
    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_TmSync;
    msg->u.request.u.tm_sync.nid = util.getNid();
    msg->u.request.u.tm_sync.pid = util.getPid();
    msg->u.request.u.tm_sync.length = sizeof(struct sync_buf_def);
    memmove(msg->u.request.u.tm_sync.data,&buf,msg->u.request.u.tm_sync.length);
    printf ("[%s] tm_sync data length=%d\n",MyName,msg->u.request.u.tm_sync.length);

    gp_local_mon_io->send_recv( msg );
    status.MPI_TAG = msg->reply_tag;

    if ((msg->type == MsgType_Service) &&
        (msg->u.reply.type == ReplyType_TmSync))
    {
        if (msg->u.reply.u.tm_sync.return_code == MPI_SUCCESS)
        {
            printf ("[%s] tm_sync request successfully. rc=%d\n",
                    MyName, msg->u.reply.u.tm_sync.return_code);
            printf("[%s] TmSync Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
                util.getNid(),
                util.getPid(),
                msg->u.reply.u.tm_sync.handle,
                buf.seq_number,
                buf.length,
                buf.string);
            request[util.getNid()][seq_number].handle = msg->u.reply.u.tm_sync.handle;
            trans_count++;
            trans_active++;
            status.MPI_ERROR = msg->u.reply.u.tm_sync.return_code;
        }
        else
        {
            printf ("[%s] tm_sync request failed, rc=%d\n", MyName,
                    msg->u.reply.u.tm_sync.return_code);
            status.MPI_ERROR = msg->u.reply.u.tm_sync.return_code;
        }
    }
    else
    {
        status.MPI_ERROR = -1;
        printf ("[%s] Invalid MsgType(%d)/ReplyType(%d) for tm_sync request\n",
             MyName, msg->type, msg->u.reply.type);
    }

    fflush (stdout);

    gp_local_mon_io->release_msg(msg);

    msg = NULL;
    return status;
}

bool wait_for_event()
{
    int rc = -1;

    lock_event();
    rc = wait_on_event();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_event();

    return ( rc == 0 );
}

bool wait_for_notice()
{
    int rc = -1;
    printf ("[%s] Waiting for tm_sync completion notice.\n", MyName);

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
    printf ("[%s] Waiting for unsolicited message.\n", MyName);

    lock_unsolicited();
    rc = wait_on_unsolicited();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_unsolicited();

    return ( rc == 0 );
}

MPI_Comm CtrlComm = MPI_COMM_NULL;

bool openCtrl ()
{
    char   ctrlPort[MPI_MAX_PORT_NAME];
    bool result = false;

    if (util.requestOpen ( "$CTRLR", -1, 0, ctrlPort ))
    {
        printf ("[%s] opened $CTRLR port=%s\n", MyName, ctrlPort);

        int rc = XMPI_Comm_connect (ctrlPort, MPI_INFO_NULL,
                                   0, MPI_COMM_SELF, &CtrlComm);
        if (rc == MPI_SUCCESS)
        {
            XMPI_Comm_set_errhandler (CtrlComm, MPI_ERRORS_RETURN);

            printf ("[%s] connected to process.\n", MyName);
            result = true;

            printf("[%s] sending id to CTRLR\n", MyName );

            // Send worker identification to controller
            tmSyncResults_t sendBuf
                = {util.getNid(), util.getPid(), 0, 0, 0, 0, 0};

            rc = XMPI_Send ((void *) &sendBuf,
                           sizeof(tmSyncResults_t)/sizeof(int),
                           MPI_INT, 0, 100 /* USER_TAG */, CtrlComm);
            if (rc != MPI_SUCCESS)
            {
                printf ("[%s] XMPI_Send failed. rc = (%d)\n", MyName, rc);
            }

        }
        else
        {
            printf ("[%s] failed to connect. rc = (%d)\n", MyName, rc);
        }
    }
    else
    {
        printf ("[%s] failed to open $CTRLR\n", MyName);
    }

    return result;
}

void doTest1(  )
{
    MPI_Status status;
    bool end_of_test = false;

             // *** test 1 -- failed TM test w/ restart of TMs
             printf("[%s] Test1 - TM process death test w/restart of TM processes - Waiting to start test.\n",MyName);
             if ( get_tm_processes( MAX_TEST_NODES ) )
             {
                abort();
             }
             if ( util.getNid() == 2 ||
                 (!VirtualNodes && util.getNid() == 3) )
             {
                // Wait for other TMs to start, then die!
                sleep(3);
                printf("[%s] - Test1 - Stopping to halt node\n",MyName);
                fflush (stdout);
                util_set_core_free();
                abort();
             }
             else
             {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                printf("[%s] Test1 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test1(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    printf("[%s] Test1(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    printf("[%s] Test1 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
                printf("[%s] Test1 - Wait to commit or abort TM sync data.\n",MyName);
             }
             while (((trans_commit+trans_abort)<NumNodes || trans_active) && ! end_of_test )
             {
                fflush (stdout);
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        printf("[%s] Test1 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        abort();
                    }
                }
                printf("[%s] Test1 - Dead TM count =%d\n",MyName,dead_tm_count);
                if ( dead_tm_count == 2 && up_node_count == 2 )
                {
                    up_node_count = 0;
                    end_of_test = true;

                }
                //printf("[%s] Test1 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
             }
             printf("[%s] Test1 - TM process death test w/restart of TM processes - Completed test.\n",MyName);
}

void doTest3()
{
    MPI_Status status;

            // *** test 3 - No collision Commit test
            Abort_transaction = false;
            printf("[%s] Test3 - No collision Commit test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            // TM on logical node 2 to start sync
            if ( util.getNid() == 2 )
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                printf("[%s] Test3 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test3 - Started TM Sync operation.\n",MyName);
                }
                else
                {
                    printf("[%s] Test3 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
            }
            else
            {
                printf("[%s] TM Test3 - Sync already started ... waiting for TM Sync data.\n",MyName);
                if ( !wait_for_unsolicited_msg() )
                {
                    printf("[%s] Test3 - Failed to receive unsolicited message! Aborting\n",MyName);
                    fflush (stdout);
                    abort();
                }
            }
            printf("[%s] Test3 - Wait to commit or abort TM sync data.\n",MyName);
            while( !wait_for_notice() )
            {
                if (trans_active)
                {
                    printf("[%s] Test3 - Failed to receive notice! Aborting\n",MyName);
                    fflush (stdout);
                    abort();
                }
                else
                {
                    printf("[%s] Test3 - No transaction active.\n",MyName);
                }
            }
            Test_Initialized = false;
            printf("[%s] Test3 - No collision Commit test - Completed test.\n",MyName);
}

void doTest4()
{
    MPI_Status status;

            // *** test 4 -- No collision Abort test
            Abort_transaction = (util.getNid()==5?true:false);
            printf("[%s] Test4 - No collision Abort test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            // TM on logical node 0 to start sync
            if ( util.getNid() == 1 )
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                printf("[%s] Test4 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test4 - Started TM Sync operation.\n",MyName);
                }
                else
                {
                    printf("[%s] Test4 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
            }
            else
            {
                printf("[%s] TM Test4 - Sync already started ... waiting for TM Sync data.\n",MyName);
                if ( !wait_for_unsolicited_msg() )
                {
                    printf("[%s] Test4 - Failed to receive unsolicited message! Aborting\n",MyName);
                    fflush (stdout);
                    abort();
                }
            }
            printf("[%s] Test4 - Wait to commit or abort TM sync data.\n",MyName);
            while( !wait_for_notice() )
            {
                if (trans_active)
                {
                    printf("[%s] Test4 - Failed to receive notice! Aborting\n",MyName);
                    fflush (stdout);
                    abort();
                }
                else
                {
                    printf("[%s] Test4 - No transaction active.\n",MyName);
                }
            }
            Test_Initialized = false;
            printf("[%s] Test4 - No collision Abort test - Completed test.\n",MyName);
}

void doTest5()
{
    MPI_Status status;

            // *** test 5 -- Collision Commit test
            Abort_transaction = false;
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            printf("[%s] Test5 - Collision Commit test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                printf("[%s] Test5(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test5(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    printf("[%s] TM Test5(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    printf("[%s] Test5 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
                trans_starts++;
                if ( trans_starts%(util.getNid()+1) == 0 ) usleep(1000*util.getNid());
            }
            while ( trans_starts < MAX_SYNCS );
            printf("[%s] Test5 - Wait to commit or abort TM sync data.\n",MyName);
            while ( !completed_test() )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        printf("[%s] Test5 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        abort();
                    }
                }
            }
            printf("[%s] Test5 - Collision Commit test - Completed test.\n",MyName);
}

void doTest6()
{
    MPI_Status status;

            // *** test 6 -- Collision Abort test
            Abort_transaction = (util.getNid()==2?true:false);
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            printf("[%s] Test6 - Collision Abort test (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                printf("[%s] Test6(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test6(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    printf("[%s] TM Test6(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    printf("[%s] Test6 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
                trans_starts++;
                if ( trans_starts%(util.getNid()+1) == 0 ) usleep(1000*util.getNid());
            }
            while ( trans_starts < MAX_SYNCS );
            printf("[%s] Test6 - Wait to commit or abort TM sync data.\n",MyName);
            while ( !completed_test() )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        printf("[%s] Test6 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        abort();
                    }
                }
            }
            printf("[%s] Test6 - Collision Abort test - Completed test.\n",MyName);
}

void doTest7()
{
    MPI_Status status;

            // *** test 7 -- failed TM test
            printf("[%s] Test7 - TM process death test w/ node failure - Waiting to start test.\n",MyName);
            if ( util.getNid() == 2 ||
                (!VirtualNodes && util.getNid() == 3) )
            {
                usleep(SYNC_DELAY*2); // delay to allow at least two sync cycles
                printf("[%s] - Test7 - Stopping to halt node\n",MyName);
                fflush (stdout);
                util_set_core_free();
                abort();
            }
            else
            {
                usleep(SYNC_DELAY); // delay to allow at least one sync cycle
                printf("[%s] Test7 - Sending TM Sync request.\n",MyName);
                status = tm_sync(trans_starts++);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test7(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    printf("[%s] TM Test7(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    printf("[%s] Test7 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
                }
                printf("[%s] Test7 - Wait to commit or abort TM sync data.\n",MyName);
            }
            while ( ((trans_commit+trans_abort) < (NumNodes-1)) || trans_active )
            {
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        printf("[%s] Test7 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        abort();
                    }
                }
#if 0
                if ( down_node_count )
                {
                    down_node_count = 0;
                    NumNodes = GetNumOfNode();
                }
#endif
                printf("[%s] Test7 - Counters: Commit#=%d, Abort#=%d, NumNodes#=%d, Active=%d\n",
                    MyName,trans_commit,trans_abort,NumNodes,trans_active);
            }
            printf("[%s] Test7 - TM process death test w/ node failure - Completed test.\n",MyName);
}

void doTest8()
{
    MPI_Status status;

             // *** test 8 -- failed TM test w/ restart of TMs while TmSync is in process
             printf("[%s] Test8 - Collision Abort TM process death test w/restart of TM processes - Waiting to start test.\n",MyName);
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
             if ( util.getNid() == 2 )
             {
                sleep(3);
                // delay to allow at least three sync cycles
                // and die in the middle of a TmSync
                usleep(SYNC_DELAY*3);
                printf("[%s] - Test8 - Stopping to halt node\n",MyName);
                fflush (stdout);
                util_set_core_free();
                abort();
             }
             else
             {
                if ( util.getNid() == 3 )
                {
                    // delay to allow nid 2 to abort
                    sleep(3);
                    usleep(SYNC_DELAY*3);
                    if ( !VirtualNodes )
                    {
                        // on a real cluster the watchdog process
                        // does not know about the testtm program
                        usleep(SYNC_DELAY*3);
                        util_set_core_free();
                        abort();
                    }
                }
                else
                {
                    sleep(1);
                }
                printf("[%s] Test8 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
                do
                {
                    printf("[%s] Test8(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                    status = tm_sync(trans_starts);
                    if( status.MPI_ERROR == MPI_SUCCESS )
                    {
                        printf("[%s] Test8(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                    }
                    else if( status.MPI_ERROR == MPI_ERR_PENDING )
                    {
                        printf("[%s] TM Test8(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                    }
                    else
                    {
                        printf("[%s] Test8 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                        fflush (stdout);
                        abort();
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
                    printf("[%s] DEQUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
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

             while ( !completed_test() )
             {
                fflush (stdout);
                // wait death notice or TmSync notice
                if( !wait_for_notice() )
                {
                    if (trans_active)
                    {
                        printf("[%s] Test8 - Failed to receive notice! Aborting\n",MyName);
                        fflush (stdout);
                        abort();
                    }
                }
                printf("[%s] Test8 - Dead TM count =%d\n",MyName,dead_tm_count);
                if ( dead_tm_count == 2)
                {
                    // Completed part 1 of test8, send results to controller
                    break;
                }
                printf("[%s] Test8 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
             }
            printf("[%s] Test8 - Collision Abort TM process death test w/restart of TM processes - Completed test.\n",MyName);
}

// Test 9 is the continuation of test 8.  The controller process invokes
// test 9 once it completes the first part of test 8 and has restarted the
// two TM processes that exited.
void doTest9()
{
    MPI_Status status;

            // *** test 9 -- failed TM test w/ restart of TMs
            usleep(SYNC_DELAY); // delay to allow at least one sync cycle
            printf("[%s] Test9 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
            do
            {
                printf("[%s] Test9(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                status = tm_sync(trans_starts);
                if( status.MPI_ERROR == MPI_SUCCESS )
                {
                    printf("[%s] Test9(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                }
                else if( status.MPI_ERROR == MPI_ERR_PENDING )
                {
                    printf("[%s] TM Test9(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                }
                else
                {
                    printf("[%s] Test9 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                    fflush (stdout);
                    abort();
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
                       printf("[%s] Test9 - Failed to receive notice! Aborting\n",MyName);
                       fflush (stdout);
                       abort();
                   }
               }
               printf("[%s] Test9 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
            }
            printf("[%s] Test9 - Collision Abort TM process death test w/restart of TM processes - Completed test.\n",MyName);
}

void doTest10()
{
    MPI_Status status;

            // *** test 10 -- failed TM test w/out restart of TMs
            // This test must be performed after test1, so that there is no spare node
            // and TmSyncs are in process when a node goes down
            printf("[%s] Test10 - Collision Abort TM process death test w/ node failure - Waiting to start test.\n",MyName);
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
            if ( util.getNid() == 2 )
            {
               sleep(3);
               // delay to allow at least three sync cycles
               // and die in the middle of a TmSync
               usleep(SYNC_DELAY*3);
               printf("[%s] - Test10 - Stopping to halt node\n",MyName);
               fflush (stdout);
               util_set_core_free();
               abort();
            }
            else
            {
               if ( util.getNid() == 3 )
               {
                   // delay to allow nid 2 to abort
                   sleep(3);
                   usleep(SYNC_DELAY*3);
               }
               else
               {
                   sleep(1);
               }
               printf("[%s] Test10 - Sending TM Sync request (abort=%d) - Waiting to start test.\n",MyName, Abort_transaction);
               do
               {
                   printf("[%s] Test10(%d) - Sending TM Sync request.\n",MyName,trans_starts);
                   status = tm_sync(trans_starts);
                   if( status.MPI_ERROR == MPI_SUCCESS )
                   {
                       printf("[%s] Test10(%d) - Started TM Sync operation.\n",MyName,trans_starts);
                   }
                   else if( status.MPI_ERROR == MPI_ERR_PENDING )
                   {
                       printf("[%s] TM Test10(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
                   }
                   else
                   {
                       printf("[%s] Test10 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
                       fflush (stdout);
                       abort();
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
                   printf("[%s] DEQUEUE TmSync Unsolicited Message - nid=%d pid=%d handle=%d data='seq#=%d, len=%d string=%s'\n", MyName,
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

            while ( !completed_test() )
            {
               fflush (stdout);
               if( !wait_for_notice() )
               {
                   if (trans_active)
                   {
                       printf("[%s] Test10 - Failed to receive notice! Aborting\n",MyName);
                       fflush (stdout);
                       abort();
                   }
               }
               printf("[%s] Test10 - Dead TM count =%d\n",MyName,dead_tm_count);
               if ( dead_tm_count == 2)
               {
                   // Completed part 1 of test10, send results to controller
                   break;
               }
               printf("[%s] Test10 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
            }
            printf("[%s] Test10 - Collision Abort TM process death test w/ node failure - Completed test.\n",MyName);

}

// Test 11 is the continuation of test 10.  The controller process invokes
// test 10 once it completes the first part of test 10.
void doTest11()
{
    MPI_Status status;

    // Delay to allow all tmSyncTest processes to get event 11
    sleep(3);

    // force completion of requests from node 2 and 3 since they are dead
    end_requests( 2 );
    end_requests( 3 );

    printf("[%s] Test11 - Resending TM Sync request (abort=%d)\n",MyName, Abort_transaction);
    do
    {
        printf("[%s] Test11(%d) - Sending  TM Sync request.\n",MyName,trans_starts);
        status = tm_sync(trans_starts);
        if( status.MPI_ERROR == MPI_SUCCESS )
        {
            printf("[%s] Test11(%d) - Started TM Sync operation.\n",MyName,trans_starts);
        }
        else if( status.MPI_ERROR == MPI_ERR_PENDING )
        {
            printf("[%s] TM Test11(%d) - Sync already started ... waiting for TM Sync data.\n",MyName,trans_starts);
        }
        else
        {
            printf("[%s] Test11 - Can't start TM Sync, err=%d\n",MyName,status.MPI_ERROR);
            fflush (stdout);
            abort();
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
                printf("[%s] Test11 - Failed to receive notice! Aborting\n",MyName);
                fflush (stdout);
                abort();
            }
        }
        printf("[%s] Test11 - DEBUG commit=%d, abort=%d, nodes=%d, active=%d\n",MyName,trans_commit,trans_abort,NumNodes,trans_active);
    }
    printf("[%s] Test11 - failed TM test w/out restart of TMs - Completed test.\n",MyName);

}

int main (int argc, char *argv[])
{

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    int rc = pthread_mutex_init( &notice_mutex, NULL );
    if (rc)
    {
        printf("[%s] Error initializing notice mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
        exit(1);
    }

    rc = pthread_mutex_init( &test_mutex, NULL );
    if (rc)
    {
        printf("[%s] Error initializing test mutex: %s (%d)\n",
                     MyName, strerror(errno), errno);
        exit(1);
    }

    rc = pthread_mutex_init( &unsolicited_mutex, NULL );
    if (rc)
    {
        printf("[%s] Error initializing unsolicited mutex: %s (%d)\n",
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

    // Set local io callback function
    gp_local_mon_io->set_cb(recv_localio_msg, "notice");
    gp_local_mon_io->set_cb(recv_localio_msg, "recv");
    gp_local_mon_io->set_cb(recv_localio_msg, "unsol");
    gp_local_mon_io->set_cb(recv_localio_msg, "event");

    // Send startup message to monitor
    util.requestStartup ();

    // Open control process
    if (!openCtrl())
    {
        printf ("[%s] FAILED opened $CTRLR\n", MyName);
        //        exit(1);
    }

    NumNodes = util.getNodeCount();
    bool done = false;

    do
    {
        initialize_test();
        // wait for event signal to start test across all TMs
        wait_for_event();
        reset_trans_counts();
        request_to_become_TmLeader();
        switch ( event_id )
        {
        case 1:
            doTest1();
            break;

        case 2: // Unused test
            printf("[%s] *** Test2 - No such test ***\n",MyName);
            break;

        case 3:
            // *** test 3 - No collision Commit test
            doTest3();
            break;

        case 4:
            // *** test 4 -- No collision Abort test
            doTest4();
            break;

        case 5:
            // *** test 5 -- Collision Commit test
            doTest5();
            break;

        case 6:
            // *** test 6 -- Collision Abort test
            doTest6();
            break;

        case 7:
            doTest7();
            break;

        case 8:
            // *** test 8 -- failed TM test w/ restart of TMs while TmSync
            // is in process (part 1)
            doTest8();
            break;

        case 9:
            // *** test 9 -- failed TM test w/ restart of TMs while TmSync
            // is in process (part 2)
            doTest9();
            break;

        case 10:
            // *** test 10 -- failed TM test w/out restart of TMs (part 1)
            doTest10();
            break;

        case 11:
            // *** test 11 -- failed TM test w/out restart of TMs (part 2)
            doTest11();
            break;

        default:
            printf("[%s] Event = %d, No test defined, stopping\n",MyName, event_id);
            done = true;
        }

        if (!done)
        {
            printf("[%s] Test%d - Start#=%d, Abort#=%d, Commit#=%d, Total Transaction=%d\n",
                MyName, event_id, trans_starts,trans_abort,trans_commit,trans_count);
            fflush (stdout);

            if (CtrlComm != MPI_COMM_NULL)
            {
                tmSyncResults_t sendBuf = {util.getNid(), util.getPid(),
                                           event_id, trans_starts, trans_abort,
                                           trans_commit, trans_count};

                printf("[%s] Sending results to controller, elements=%d\n",
                       MyName, (int)(sizeof(tmSyncResults_t)/sizeof(int)));
                rc = XMPI_Send ((void *) &sendBuf,
                               sizeof(tmSyncResults_t)/sizeof(int),
                               MPI_INT, 0, 100 /* USER_TAG */, CtrlComm);
                if (rc != MPI_SUCCESS)
                {
                    printf ("[%s] XMPI_Send failed. rc = (%d)\n", MyName, rc);
                }
            }
        }
    }
    while (!done);

    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
