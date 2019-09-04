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
#include <unistd.h>
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "dtmCtrl.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0};

int deathNoticesReceived = 0;
int tmRestartedNoticesReceived = 0;

pthread_mutex_t     notice_mutex;
pthread_cond_t      notice_cv;
bool                notice_signaled = false;

bool                shutdownSent = false;


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

bool wait_for_notice()
{
    int rc = -1;
    printf ("[%s] Waiting for notice.\n", MyName);

    lock_notice();
    rc = wait_on_notice();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_notice();

    return ( rc == 0 );
}

// Routine for handling notices:
//   NodeDown, NodeUp, ProcessDeath, Shutdown, TmSyncAbort, TmSyncCommit
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_ProcessDeath )
    {
        printf( "[%s] Process death notice received for %s (%d, %d:%d),"
                " trans_id=%lld.%lld.%lld.%lld., aborted=%d\n"
              , MyName 
              , recv_msg->u.request.u.death.process_name 
              , recv_msg->u.request.u.death.nid
              , recv_msg->u.request.u.death.pid
              , recv_msg->u.request.u.death.verifier
              , recv_msg->u.request.u.death.trans_id.txid[0]
              , recv_msg->u.request.u.death.trans_id.txid[1]
              , recv_msg->u.request.u.death.trans_id.txid[2]
              , recv_msg->u.request.u.death.trans_id.txid[3]
              , recv_msg->u.request.u.death.aborted );
        ++deathNoticesReceived;
    }
    else if ( recv_msg->type == MsgType_NodeDown )
    {
        printf("[%s] Node %d (%s) is DOWN.\n", MyName,
               recv_msg->u.request.u.down.nid,
               recv_msg->u.request.u.down.node_name);
    }
    else if ( recv_msg->type == MsgType_NodeUp )
    {
        printf("[%s] Node %d (%s) is UP.\n", MyName,
               recv_msg->u.request.u.up.nid,
               recv_msg->u.request.u.up.node_name);
    }
    else if ( recv_msg->type == MsgType_Shutdown )
    {
        printf("[%s] Shutdown (%d)!\n", MyName,
               recv_msg->u.request.u.shutdown.level);
        shutdownSent = true;
    }
    else
    {
        printf( "[%s] unexpected notice, type=%s\n"
              , MyName
              , MessageTypeString( recv_msg->type ) );
    }
    fflush (stdout );

    lock_notice();
    int rc = signal_notice();
    if ( rc == -1 )
    {
        exit( 1);
    }
    unlock_notice();
}


void processCommands()
{
    MPI_Comm ctrlComm;
    int rc;
    MPI_Status status;
    int recvbuf[6];
    char sendbuf[100];
    bool done = false;
    const int serverTag = 100;

    if ( tracing )
    {
        printf( "[%s] Port: %s\n", MyName, util.getPort( ) );
    }

    if ( tracing )
    {
        printf( "[%s] Wait to connect.\n", MyName );
    }

    XMPI_Comm_accept( util.getPort( ), MPI_INFO_NULL, 0, MPI_COMM_SELF
                    , &ctrlComm );
    XMPI_Comm_set_errhandler( ctrlComm, MPI_ERRORS_RETURN );

    if ( tracing )
    {
        printf( "[%s] Connected.\n", MyName );
    }

    if ( !util.requestTmReady( ) )
    {
        done = true;
    }

    fflush (stdout );
    replyMsg_t replyMsg;

    do
    {
        rc = XMPI_Recv( recvbuf, 6, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG
                      , ctrlComm, &status );
        if ( rc == MPI_SUCCESS )
        {
            switch ( recvbuf[0] )
            {
            case CMD_GET_STATUS:
                printf( "[%s] got command CMD_GET_STATUS: death count=%d, "
                        "DTM restarted count=%d\n"
                      , MyName
                      , deathNoticesReceived
                      , tmRestartedNoticesReceived );
                replyMsg.deathNoticeCount = deathNoticesReceived;
                replyMsg.tmRestartedNoticeCount = tmRestartedNoticesReceived;
                break;
            case CMD_EXIT:
                printf( "[%s] got command CMD_EXIT.\n",MyName );
                exit( 1 );
                break;
            case CMD_END:
                printf( "[%s] got command CMD_END.\n",MyName );
                done = true;
                break;
            default:
                sprintf( sendbuf, "[%s] Received (%d:%d) UNKNOWN"
                       , MyName, recvbuf[0], recvbuf[1] );
                fflush (stdout );
                abort();
            }

            rc = XMPI_Send( &replyMsg, (int) sizeof(replyMsg_t), MPI_CHAR, 0
                          , serverTag, ctrlComm );
        }
        else
        {  // Receive failed
            printf( "[%s] XMPI_Recv failed, rc = (%d) %s\n"
                  , MyName, rc, util.MPIErrMsg( rc ) );
            done = true;
        }
        fflush (stdout );
    }
    while ( !done );

    if ( tracing )
    {
        printf( "[%s] disconnecting.\n", MyName );
    }
    util.closeProcess( ctrlComm );
}


int main (int argc, char *argv[])
{
    util.processArgs( argc, argv );
    tracing = util.getTrace( );
    MyName = util.getProcName( );

    util.InitLocalIO( );
    assert( gp_local_mon_io );

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb( recv_notice_msg, "notice" );

    util.requestStartup( );

    //pause();

    // Get and execute commands from controller process
    processCommands( );

    // Wait for node down notice
    if ( !shutdownSent )
    {
        printf("[%s] Waiting for shutdown notice!\n", MyName);
        if( !wait_for_notice() )
        {
            printf("[%s] Failed to receive shutdown notice! Aborting\n",MyName);
        }
    }

    // tell monitor we are exiting
    util.requestExit( );

    printf( "[%s] calling Finalize!\n", MyName );
    fflush( stdout );
    XMPI_Close_port( util.getPort( ) );
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }

    exit( 0 );
}
