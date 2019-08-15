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

// Test DTM process behavior
//   1.  Verify ability to start an DTM process on each of the logical nodes
//   2.  Verify that if an DTM process dies each of the other DTM
//       process DOES NOT receiv a process death OR tmRestarted notification.
//   3.  Verify that DTM as a persistent process when restarted
//       sends TmReady request to monitor.
//   4.  Verify that only one DTM process can be started on a logical node.
//   5.  Verify that DTM as a persistent process and exceeds restart limits
//       brings node down.


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "dtmCtrl.h"

#define DTM_PROC_NAME_PREFIX        "$TM"
#define DTM_RESTART_NID             2
#define DTM_DOWN_NID                4
#define DTM_KILL_DELAY              3
#define DTM_RESTART_DELAY           3
#define DTM_PERSIST_RETRIES         2
#define DTM_OLD_TEST                0

MonTestUtil util;

long trace_settings = 0;

FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

struct NodeInfo_reply_def * nodeData = NULL;
bool multLogicalPerPhysical = false;

typedef struct testProcess_s
{
    bool dead;
    int  nid;
    int  pid;
    Verifier_t verifier;
    char procName[MAX_PROCESS_NAME];    // DTM process Name
    MPI_Comm comm;
} testProcess_t;

testProcess_t *dtmProcess;
testProcess_t *persistentProcess;

int lnodes;
int pnodes;
int dtmProcessCount = 0;
int persistentProcessCount = 0;
int nidDown = -1;
int nidUp = -1;

pthread_mutex_t     notice_mutex;
pthread_cond_t      notice_cv;
bool                notice_signaled = false;

bool                nodeDown = false;

bool closeDTM( int nid );
bool closePersistent( int nid );
bool openDTM( int nid );
bool openPersistent( int nid );

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
        if ( tracing )
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

        for ( int i=0; i < lnodes; ++i )
        {
            if (  dtmProcess[i].nid == recv_msg->u.request.u.death.nid
               && dtmProcess[i].pid == recv_msg->u.request.u.death.pid )
            {
                closeDTM( dtmProcess[i].nid );
                printf( "[%s] Marking dead DTM process %s (%d, %d)\n"
                      , MyName, dtmProcess[i].procName, dtmProcess[i].nid
                      , dtmProcess[i].pid );
                --dtmProcessCount;
                dtmProcess[i].nid = -1;
                dtmProcess[i].pid = -1;
                dtmProcess[i].verifier = -1;
                dtmProcess[i].dead = true;
            }
        }
        for ( int i=0; i < lnodes; ++i )
        {
            if (  persistentProcess[i].nid == recv_msg->u.request.u.death.nid
               && persistentProcess[i].pid == recv_msg->u.request.u.death.pid )
            {
                closePersistent( persistentProcess[i].nid );
                printf( "[%s] Marking dead process %s (%d, %d)\n"
                      , MyName, persistentProcess[i].procName
                      , persistentProcess[i].nid
                      , persistentProcess[i].pid );
                --persistentProcessCount;
                persistentProcess[i].nid = -1;
                persistentProcess[i].pid = -1;
                persistentProcess[i].verifier = -1;
                persistentProcess[i].dead = true;
            }
        }
    }
    else if ( recv_msg->type == MsgType_NodeDown )
    {
        printf("[%s] Node %d (%s) is DOWN.\n", MyName,
               recv_msg->u.request.u.down.nid,
               recv_msg->u.request.u.down.node_name);
        nidDown = recv_msg->u.request.u.down.nid;
        nodeDown = true;
    }
    else if ( recv_msg->type == MsgType_NodeUp )
    {
        printf("[%s] Node %d (%s) is Up.\n", MyName,
               recv_msg->u.request.u.up.nid,
               recv_msg->u.request.u.up.node_name);
        nidUp = recv_msg->u.request.u.up.nid;
        nodeDown = false;
        //++dtmProcessCount;
    }
    else
    {
        printf( "[%s] unexpected notice, type=%s\n"
              , MyName, MessageTypeString( recv_msg->type ) );
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


// Verify that the configuration contains the desired number and
// type of nodes required for this test.
bool checkConfig( void )
{
    bool result = true;
    int virtualNodes;

    char *env = getenv( "SQ_VIRTUAL_NODES" );
    if ( env && isdigit( env[0] ) )
    {
        virtualNodes = atoi( env );
        if ( tracing )
        {
            printf( "[%s] %d virtual nodes defined\n", MyName, virtualNodes );
        }
    }
    else
    {
        virtualNodes = 0;
    }

    util.requestNodeInfo( -1, false, -1, -1, nodeData );

    // process node info data
    lnodes = nodeData->num_nodes;  // # logical nodes
    pnodes = nodeData->num_pnodes; // # physical nodes

    if ( tracing )
    {
        printf( "[%s] Configuration contains %d logical nodes in %d "
                "physical nodes\n", MyName, lnodes, pnodes );
    }

    if ( virtualNodes )
    {
        // Verify number of virtual nodes matches number of logical nodes
        if ( virtualNodes != lnodes )
        {
            printf( "[%s] *** ERROR *** %d virtual nodes were specified but "
                    "configuration has  %d\n", MyName, virtualNodes, lnodes );
            result = false;
        }
    }

    if ( pnodes < 3 )
    {
        printf( "[%s] *** ERROR *** This test requires a minimum of 3 "
                "physical nodes but only %d are configured.\n", MyName, pnodes );
        result = false;
    }

    // Allocate array to store DTM and test persistent process info.
    dtmProcess = new testProcess_t[lnodes];
    persistentProcess = new testProcess_t[lnodes];

    return result;
}

bool createPersistent( int nid )
{
    bool testSuccess = true;
    char *childArgs[1] = {(char *) "-t"};
    char procName[MAX_PROCESS_NAME] = {0};

    sprintf( procName, "$PP%d", nid );
    
    if ( util.requestNewProcess( nid
                               , ProcessType_PERSIST
                               , false
                               , procName
                               , "dtmProc"
                               , ""
                               , ""
                               , ((tracing) ? 1: 0)
                               , childArgs
                               , persistentProcess[nid].nid
                               , persistentProcess[nid].pid
                               , persistentProcess[nid].verifier
                               , persistentProcess[nid].procName ) )
    {
        printf( "[%s] Started process %s (%d, %d:%d)\n"
              , MyName
              , persistentProcess[nid].procName
              , persistentProcess[nid].nid
              , persistentProcess[nid].pid
              , persistentProcess[nid].verifier );

        persistentProcess[nid].dead = false;

        testSuccess = openPersistent( nid );
    }
    else
    {
        persistentProcess[nid].dead = true;
        printf( "[%s] Failed to start process %s on node %d\n"
              , MyName, procName, nid );
        testSuccess = false;
    }

    return testSuccess;
}

bool closePersistent( int nid )
{
    bool testSuccess = true;
    char procName[MAX_PROCESS_NAME] = {0};

    if ( ! persistentProcess[nid].dead )
    {
        printf( "[%s] Closing process %s (%d, %d:%d)\n"
              , MyName
              , persistentProcess[nid].procName
              , persistentProcess[nid].nid
              , persistentProcess[nid].pid
              , persistentProcess[nid].verifier );

        if ( util.closeProcess( persistentProcess[nid].comm ) )
        {
            persistentProcess[nid].comm = -1;
            if ( tracing )
            {
                printf( "[%s] Closed process %s.\n"
                      , MyName, persistentProcess[nid].procName );
            }
        }
        else
        {
            printf( "[%s] Unable to close process %s (%d, %d)\n"
                  , MyName
                  , persistentProcess[nid].procName
                  , persistentProcess[nid].nid
                  , persistentProcess[nid].pid );
            testSuccess = false;
        }
    }
    else
    {
        sprintf( procName, "$PP%03d", nid );
        printf( "[%s] Did not attempt to close process %s on node %d\n"
              , MyName, procName, nid );
    }

    return testSuccess;
}

bool openPersistent( int nid )
{
    bool testSuccess = true;
    char procName[MAX_PROCESS_NAME] = {0};

    if ( ! persistentProcess[nid].dead )
    {
        printf( "[%s] Opening process %s (%d, %d:%d)\n"
              , MyName
              , persistentProcess[nid].procName
              , persistentProcess[nid].nid
              , persistentProcess[nid].pid
              , persistentProcess[nid].verifier );

        if ( util.openProcess( persistentProcess[nid].procName
                             , persistentProcess[nid].verifier
                             , 1
                             , persistentProcess[nid].comm ) )
        {
            if ( tracing )
            {
                printf( "[%s] connected to process %s.\n"
                      , MyName, persistentProcess[nid].procName );
            }
        }
        else
        {
            printf( "[%s] Unable to communicate with "
                "process %s (%d, %d)\n", MyName,
                persistentProcess[nid].procName,
                persistentProcess[nid].nid,
                persistentProcess[nid].pid );
            testSuccess = false;
        }
    }
    else
    {
        sprintf( procName, "$PP%03d", nid );
        printf( "[%s] Did not attempt to open process %s on node %d\n"
              , MyName, procName, nid );
    }

    return testSuccess;
}

bool infoPersistent( int nid )
{
    bool testSuccess = true;

    printf( "[%s] Getting info on process %s\n"
          , MyName
          , persistentProcess[nid].procName );

    if ( util.requestProcInfo( persistentProcess[nid].procName
                             , persistentProcess[nid].nid
                             , persistentProcess[nid].pid
                             , persistentProcess[nid].verifier ) )
    {
        if ( persistentProcess[nid].nid != nid )
        {
            printf ( "[%s] process %s is not running on node %d "
                     "as expected.\n"
                   , MyName
                   , persistentProcess[nid].procName
                   , nid );
            testSuccess = false;
        }
        else
        {
            printf( "[%s] Found persistent process %s (%d, %d:%d)\n"
                  , MyName
                  , persistentProcess[nid].procName
                  , persistentProcess[nid].nid
                  , persistentProcess[nid].pid
                  , persistentProcess[nid].verifier );
        }
    }
    else
    {
        testSuccess = false;
    }

    return( testSuccess );
}

bool infoPersistentRequiredDTM( int node_id )
{
    bool testSuccess = true;
    int  nid = -1;
    int  pid = -1;
    int  verifier = -1;
    char procName[MAX_PROCESS_NAME] = {0};

    sprintf( procName, "$PSD%d", node_id );
    
    printf( "[%s] Getting info on process %s\n"
          , MyName
          , procName );

    if ( util.requestProcInfo( procName
                             , nid
                             , pid
                             , verifier ) )
    {
        if ( nid != node_id )
        {
            printf ( "[%s] process %s is not running on node %d "
                     "as expected, nid=%d\n"
                   , MyName
                   , procName
                   , node_id
                   , nid );
            testSuccess = false;
        }
        else
        {
            printf( "[%s] Found persistent process %s (%d, %d:%d)\n"
                  , MyName
                  , procName
                  , nid
                  , pid
                  , verifier );
        }
    }
    else
    {
        printf( "[%s] No info returned on process %s\n"
              , MyName
              , procName );
        testSuccess = false;
    }

    return( testSuccess );
}

void killPersistent( int nid )
{
    printf( "[%s] Killing process %s:%d\n"
          , MyName
          , persistentProcess[nid].procName
          , persistentProcess[nid].verifier );

    // Kill the server process, expect it to be restarted on the same node
    util.requestKill( persistentProcess[nid].procName
                    , persistentProcess[nid].verifier );

    // Allow time for process kill and after-effects
    sleep( DTM_KILL_DELAY );
}

bool checkDTM( int nid , const char *procNamePrefix )
{
    bool testSuccess = true;
    char *childArgs[1] = {(char *) "-t"};
    char procName[MAX_PROCESS_NAME] = {0};

    sprintf( procName, "%s%d", procNamePrefix, nid );
    
    if ( util.requestNewProcess( nid
                               , ProcessType_DTM
                               , false
                               , procName
                               , "dtmProc"
                               , ""
                               , ""
                               , ((tracing) ? 1: 0)
                               , childArgs
                               , dtmProcess[nid].nid
                               , dtmProcess[nid].pid
                               , dtmProcess[nid].verifier
                               , dtmProcess[nid].procName
                               , true ) )
    {
        printf( "[%s] Started DTM process %s (%d, %d:%d)\n"
              , MyName
              , dtmProcess[nid].procName
              , dtmProcess[nid].nid
              , dtmProcess[nid].pid
              , dtmProcess[nid].verifier );

        testSuccess = false;
    }
    else
    {
        printf( "[%s] Failed to start DTM process %s on node %d\n"
              , MyName
              , procName
              , nid );
        strcpy(dtmProcess[nid].procName,procName);
        if( util.requestProcInfo( dtmProcess[nid].procName   
                                , dtmProcess[nid].nid
                                , dtmProcess[nid].pid
                                , dtmProcess[nid].verifier ))
        {
            printf( "[%s] Existing DTM process with name %s on node %d with PID=%d and Verifier=%d\n"
                  , MyName
                  , dtmProcess[nid].procName
                  , dtmProcess[nid].nid
                  , dtmProcess[nid].pid
                  , dtmProcess[nid].verifier );
      
            dtmProcess[nid].dead = false;
            //testSuccess = true;
            testSuccess = openDTM( nid );
        }
        else 
        {
            printf( "[%s] Failed to find DTM process %s on node %d\n"
                  , MyName
                  , procName
                  , nid );
            dtmProcess[nid].dead = true;
            testSuccess = false;
        }
    }

    return testSuccess;
}

bool closeDTM( int nid )
{
    bool testSuccess = true;
    char procName[MAX_PROCESS_NAME] = {0};

    if ( ! dtmProcess[nid].dead )
    {
        printf( "[%s] Closing process %s (%d, %d:%d)\n"
              , MyName
              , dtmProcess[nid].procName
              , dtmProcess[nid].nid
              , dtmProcess[nid].pid
              , dtmProcess[nid].verifier );

        if ( util.closeProcess( dtmProcess[nid].comm ) )
        {
            dtmProcess[nid].comm = -1;
            if ( tracing )
            {
                printf( "[%s] Closed process %s.\n"
                      , MyName, dtmProcess[nid].procName );
            }
        }
        else
        {
            printf( "[%s] Unable to close process %s (%d, %d)\n"
                  , MyName
                  , dtmProcess[nid].procName
                  , dtmProcess[nid].nid
                  , dtmProcess[nid].pid );
            testSuccess = false;
        }
    }
    else
    {
        sprintf( procName, "$DTM%03d", nid );
        printf( "[%s] Did not attempt to close process %s on node %d\n"
              , MyName, procName, nid );
    }

    return testSuccess;
}

bool openDTM( int nid )
{
    bool testSuccess = true;
    char procName[MAX_PROCESS_NAME] = {0};

    if ( ! dtmProcess[nid].dead )
    {
        printf( "[%s] Opening process %s (%d, %d:%d)\n"
              , MyName
              , dtmProcess[nid].procName
              , dtmProcess[nid].nid
              , dtmProcess[nid].pid
              , dtmProcess[nid].verifier );

        if ( util.openProcess( dtmProcess[nid].procName
                             , dtmProcess[nid].verifier
                             , 1
                             , dtmProcess[nid].comm ) )
        {
            if ( tracing )
            {
                printf( "[%s] connected to process %s.\n"
                      , MyName, dtmProcess[nid].procName );
            }
        }
        else
        {
            printf( "[%s] Unable to communicate with "
                "process %s (%d, %d)\n", MyName,
                dtmProcess[nid].procName,
                dtmProcess[nid].nid,
                dtmProcess[nid].pid );
            testSuccess = false;
        }
    }
    else
    {
        sprintf( procName, "$DTM%03d", nid );
        printf( "[%s] Did not attempt to open process %s on node %d\n"
              , MyName, procName, nid );
    }

    return testSuccess;
}

bool infoDTM( int nid )
{
    bool testSuccess = true;

    printf( "[%s] Getting info on DTM process %s\n"
          , MyName
          , dtmProcess[nid].procName );

    if ( util.requestProcInfo( dtmProcess[nid].procName
                             , dtmProcess[nid].nid
                             , dtmProcess[nid].pid
                             , dtmProcess[nid].verifier ) )
    {
        if ( dtmProcess[nid].nid != nid )
        {
            printf ( "[%s] DTM process %s is not running on node %d "
                     "as expected.\n"
                   , MyName
                   , dtmProcess[nid].procName
                   , nid );
            testSuccess = false;
        }
        else
        {
            printf( "[%s] Found DTM process %s (%d, %d:%d)\n"
                  , MyName
                  , dtmProcess[nid].procName
                  , dtmProcess[nid].nid
                  , dtmProcess[nid].pid
                  , dtmProcess[nid].verifier );
        }
    }
    else
    {
        testSuccess = false;
    }

    return( testSuccess );
}

void killDTM( int nid )
{
    printf( "[%s] Killing process %s:%d\n"
          , MyName, dtmProcess[nid].procName, dtmProcess[nid].verifier );

    // Kill the server process, expect it to be restarted on the same node
    util.requestKill ( dtmProcess[nid].procName, dtmProcess[nid].verifier );

    // Allow time for process kill and after-effects
    sleep( DTM_KILL_DELAY );
}

//   1.  Verify that primitive DTM process cannot be created on each logical node
//   2.  Verify that primitive DTM process exists on each logical nodexs
//
//   DTM process created must fail in each logical node. 
//   A DTM process should already have been created by Monitor as one of its 
//   primitive processes.
bool DTM_test1 ()
{
    int prevNid = -1;
    int prevPNid = -1;
    int reqNid;
    bool testSuccess = true;

    for ( int i = 0; i < lnodes; i++ )
    {
        if ( nodeData->node[i].pnid == prevPNid )
        {
            prevPNid = nodeData->node[i].pnid;
            if ( tracing )
            {
                printf( "[%s] Node #%d shares a physical node with "
                        "another logical node on physical node %d.\n"
                      , MyName, nodeData->node[i].nid
                      , nodeData->node[i].pnid );
            }
            multLogicalPerPhysical = true;
        }

        if ( nodeData->node[i].nid != prevNid )
        {  // Found new logical node, start an DTM process

            if ( tracing )
            {
                printf( "[%s] Will start DTM process on nid=%d, pnid=%d\n"
                      , MyName, nodeData->node[i].nid
                      , nodeData->node[i].pnid );
            }

            prevNid = reqNid = nodeData->node[i].nid;

            if ( checkDTM( reqNid, DTM_PROC_NAME_PREFIX ) )
            {
                ++dtmProcessCount;
            }
            else
            {
                printf( "[%s] Failed to find primitive DTM process on node %d\n"
                      , MyName, reqNid );
                testSuccess = false;
                break;
            }

            if ( createPersistent( reqNid ) )
            {
                ++persistentProcessCount;
            }
            else
            {
                testSuccess = false;
                break;
            }
        }
    }

    if ( dtmProcessCount != lnodes )
    {
        printf( "[%s] Only %d DTM processes created, expecting %d\n"
              , MyName, dtmProcessCount, lnodes );
        testSuccess = false;
    }
    if ( persistentProcessCount != lnodes )
    {
        printf( "[%s] Only %d persistent processes created, expecting %d\n"
              , MyName, persistentProcessCount, lnodes );
        testSuccess = false;
    }

    return testSuccess;
}

//   2.  Verify that if an DTM process dies none of the other DTM
//       process receives a process death or tmRestarted notifications.
//
//       Request the notice counts from each DTM process. All non-restarted 
//       DTMs will return the notices received counts equal to zero.
bool DTM_test2 ()
{
    bool testSuccess = true;

    int dtmDeathNoticeCount = 0;
    int tmRestartedNoticeCount = 0;
    int sendbuf;
    replyMsg_t recvbuf;
    int rc;
    const int clientTag = 99;
    MPI_Status status;

    // Cause one DTM process to terminate abnormally (DTM in nid 2)
    sendbuf = CMD_EXIT;
    rc = XMPI_Sendrecv( &sendbuf, 1, MPI_INT, 0, clientTag
                      , &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE
                      , MPI_ANY_TAG, dtmProcess[DTM_RESTART_NID].comm, &status );

    // Wait for cluster to execute DTM restart protocol
    sleep( 5 );

    dtmDeathNoticeCount = 0;
    tmRestartedNoticeCount = 0;

    // Ask each remaining DTM process for count of death notices received
    for ( int i=0; i < lnodes; ++i )
    {
        if ( ! dtmProcess[i].dead )
        {
            sendbuf = CMD_GET_STATUS;
            rc = XMPI_Sendrecv( &sendbuf, 1, MPI_INT, 0, clientTag
                              , &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE
                              , MPI_ANY_TAG, dtmProcess[i].comm, &status );
            if ( rc == MPI_SUCCESS )
            {
                if ( tracing )
                {
                    printf( "[%s] DTM process %s, got %d death and %d TM "
                            "restarted notifications\n"
                          , MyName
                          , dtmProcess[i].procName
                          , recvbuf.deathNoticeCount
                          , recvbuf.tmRestartedNoticeCount );
                }

                dtmDeathNoticeCount += recvbuf.deathNoticeCount;
                tmRestartedNoticeCount += recvbuf.tmRestartedNoticeCount;
            }
            else
            {
                printf( "[%s] Communication with %s failed!\n"
                      , MyName, dtmProcess[i].procName );
            }
        }
    }

    if ( dtmDeathNoticeCount != 0 )
    {
        printf( "[%s] Got %d DTM death notifications, expecting %d\n"
              , MyName, dtmDeathNoticeCount, 0 );
        testSuccess = false;
    }
    if ( tmRestartedNoticeCount != 0 )
    {
        printf( "[%s] Got %d DTM restarted notifications, expecting %d\n"
              , MyName, tmRestartedNoticeCount, 0 );
        testSuccess = false;
    }

    return testSuccess;
}

//   3.  Verify that DTM as a persistent process when restarted
//       sends TmReady request to monitor.
//
//       Test 2 sets up the scenario.
//
//       Checks that a DTM process exists in each
//       logical node. 
//       Checks that a persistent process exists in each
//       logical node. The recreation of the persistent process is
//       triggered by the TmReady sent by the restarted DTM.
bool DTM_test3 ()
{
    bool testSuccess = true;

    dtmProcessCount = 0;

    for ( int i = 0; i < lnodes; i++ )
    {
        if ( tracing )
        {
            printf( "[%s] Checking DTM process info on nid=%d\n"
                  , MyName, i );
        }

        if ( infoDTM( i ) )
        {
            dtmProcess[i].dead = false;
            ++dtmProcessCount;
            if ( dtmProcess[i].comm == -1)
            {
                openDTM( i );
            }
        }
        else
        {
            printf( "[%s] Failed to find DTM process on node %d\n"
                  , MyName, i );
            dtmProcess[i].dead = true;
        }
    }

    if ( dtmProcessCount != lnodes )
    {
        printf( "[%s] Only %d DTM processes exist, expecting %d\n"
              , MyName, dtmProcessCount, lnodes );
        testSuccess = false;
    }

    sleep( 5 );

    if ( !infoPersistentRequiredDTM( DTM_RESTART_NID ) )
    {
        printf( "[%s] Failed to find persistent process on node %d\n"
              , MyName, DTM_RESTART_NID );
        testSuccess = false;
    }

    return testSuccess;
}

//   4.  Verify that only one DTM process can be started on a logical node.
//
//       Attempt to create a 2nd DTM in where one already exists.
bool DTM_test4 ()
{
    char *childArgs[1] = {(char *) "-t"};

    bool testSuccess = true;
    int nid;
    int pid;
    Verifier_t verifier;
    char procName[25];

    // Try to create a new DTM process in nid 0
    if ( util.requestNewProcess( 0
                               , ProcessType_DTM
                               , false
                               , (char *) "" // Name
                               , "dtmProc"
                               , ""
                               , ""
                               , ((tracing) ? 1: 0)
                               , childArgs
                               , nid
                               , pid
                               , verifier
                               , procName ) )
    {
        printf( "[%s] *** Error *** successfully started second DTM process "
                "%s (%d, %d:%d) on node 0.\n"
              , MyName, procName, nid, pid, verifier );
        testSuccess = false;
    }
    else
    {
        printf( "[%s] As expected, could not start a second DTM process "
                "on node %d.\n", MyName, 0 );
    }

    return testSuccess;
}


//   5.  Verify that DTM as a persistent process and exceeds restart limits
//       brings node down.
//
//       Sends CMD_END to DTM repeatedly after restart.
//          (test PERSIST_RETRIES=DTM_PERSIST_RETRIES,DTM_PERSIST_DELAY)
//       After the 3rd send node should go down.
bool DTM_test5 ()
{
    bool testSuccess = true;

    for ( int i = 0; i < (DTM_PERSIST_RETRIES+1); i++ )
    {
        if ( tracing )
        {
            printf( "[%s] Killing DTM process %s (%d, %d:%d) retry=%d\n"
                  , MyName
                  , dtmProcess[DTM_DOWN_NID].procName
                  , dtmProcess[DTM_DOWN_NID].nid
                  , dtmProcess[DTM_DOWN_NID].pid
                  , dtmProcess[DTM_DOWN_NID].verifier, i+1 );
        }

        killDTM( DTM_DOWN_NID );
        
        // Wait for the process to be recreated
        sleep( DTM_RESTART_DELAY );
        if ( infoDTM( DTM_DOWN_NID ) )
        {
            printf( "[%s] Found restarted persistent process %s (%d, %d:%d)\n"
                  , MyName
                  , dtmProcess[DTM_DOWN_NID].procName
                  , dtmProcess[DTM_DOWN_NID].nid
                  , dtmProcess[DTM_DOWN_NID].pid
                  , dtmProcess[DTM_DOWN_NID].verifier );
            dtmProcess[DTM_DOWN_NID].dead = false;
        }
        else
        {
            printf( "[%s] Failed to find persistent process on node %d\n"
                  , MyName, DTM_DOWN_NID );
            dtmProcess[DTM_DOWN_NID].dead = true;
        }
    }

    // Wait for node down notice
    if ( !nodeDown )
    {
        printf("[%s] Waiting for Node DOWN.\n", MyName);
        if( !wait_for_notice() )
        {
            printf("[%s] Failed to receive notice! Aborting\n",MyName);
        }
    }

    if ( nidDown != DTM_DOWN_NID )
    {
        printf( "[%s] Node %d down, expecting %d\n"
              , MyName, nidDown, DTM_DOWN_NID );
        testSuccess = false;
    }

    return testSuccess;
}


int main (int argc, char *argv[])
{
    bool testSuccess = true;

    util.processArgs( argc, argv );
    tracing = util.getTrace( );
    MyName = util.getProcName( );

    util.InitLocalIO( );
    assert( gp_local_mon_io );

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb( recv_notice_msg, "notice" );

    // Send startup message to monitor
    util.requestStartup( );

    // Verify the node configuration
    testSuccess = checkConfig();
    fflush (stdout );

    if ( testSuccess )
    {
        printf( "[%s] BEGIN DTM sub-test 1\n", MyName );

        testSuccess = DTM_test1( );

        printf( "[%s] END DTM sub-test 1, test: ", MyName );
        if (testSuccess)
        {
            printf( "PASS\n" );
        }
        else
        {
            printf( "FAIL\n" );
        }
    }
    fflush (stdout );
    if ( testSuccess )
    {
        printf( "[%s] BEGIN DTM sub-test 2\n", MyName );

        testSuccess = DTM_test2( );

        printf( "[%s] END DTM sub-test 2, test: ", MyName );
        if (testSuccess)
        {
            printf( "PASS\n" );
        }
        else
        {
            printf( "FAIL\n" );
        }
    }
    fflush (stdout );
    if ( testSuccess )
    {
        printf( "[%s] BEGIN DTM sub-test 3\n", MyName );

        testSuccess = DTM_test3( );

        printf( "[%s] END DTM sub-test 3, test: ", MyName );
        if (testSuccess)
        {
            printf( "PASS\n" );
        }
        else
        {
            printf( "FAIL\n" );
        }
    }
    fflush (stdout );
    if ( testSuccess )
    {
        printf( "[%s] BEGIN DTM sub-test 4\n", MyName );

        testSuccess = DTM_test4( );

        printf( "[%s] END DTM sub-test 4, test: ", MyName );
        if (testSuccess)
        {
            printf( "PASS\n" );
        }
        else
        {
            printf( "FAIL\n" );
        }
    }
    fflush (stdout );
    if ( testSuccess )
    {
        printf( "[%s] BEGIN DTM sub-test 5\n", MyName );

        testSuccess = DTM_test5( );

        printf( "[%s] END DTM sub-test 5, test: ", MyName );
        if (testSuccess)
        {
            printf( "PASS\n" );
        }
        else
        {
            printf( "FAIL\n" );
        }
    }
    fflush (stdout );

    int sendbuf;
    replyMsg_t recvbuf;
    const int clientTag = 99;
    MPI_Status status;

    for ( int i=0; i < lnodes; ++i )
    {
        if ( dtmProcess[i].comm  != -1 )
        {
            printf( "[%s] Sending CMD_END to process %s\n"
                  , MyName, dtmProcess[i].procName );
            sendbuf = CMD_END;
            XMPI_Sendrecv( &sendbuf, 1, MPI_INT, 0, clientTag,
                &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE,
                MPI_ANY_TAG, dtmProcess[i].comm, &status );
        }
        else
        {
            printf( "[%s] NOT sending CMD_END to process %s (%d, %d:%d), "
                    "comm=%d, dead=%d\n"
                  , MyName
                  , dtmProcess[i].procName
                  , dtmProcess[i].nid
                  , dtmProcess[i].pid
                  , dtmProcess[i].verifier
                  , dtmProcess[i].comm
                  , dtmProcess[i].dead );
         }
        if ( persistentProcess[i].comm  != -1 )
        {
            printf( "[%s] Sending CMD_END to process %s\n"
                  , MyName, persistentProcess[i].procName );
            sendbuf = CMD_END;
            XMPI_Sendrecv( &sendbuf, 1, MPI_INT, 0, clientTag,
                &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE,
                MPI_ANY_TAG, persistentProcess[i].comm, &status );
        }
        else
        {
            printf( "[%s] NOT sending CMD_END to process %s (%d, %d:%d), "
                    "comm=%d, dead=%d\n"
                  , MyName
                  , persistentProcess[i].procName
                  , persistentProcess[i].nid
                  , persistentProcess[i].pid
                  , persistentProcess[i].verifier
                  , persistentProcess[i].comm
                  , persistentProcess[i].dead );
         }
     }

     sleep( 5 );
     printf( "DTM Process Test:\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED" );

     // tell monitor we are exiting
     util.requestExit( );

     XMPI_Close_port( util.getPort( ) );
     if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit( 0 );
}
