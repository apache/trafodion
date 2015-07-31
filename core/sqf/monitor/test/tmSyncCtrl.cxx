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

// Test TM sync requests with and without collisions

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
using namespace std;

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "tmSyncCtrl.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;
bool realCluster = false;

const char *MyName;
int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

struct NodeInfo_reply_def * nodeData = NULL;

typedef struct expectedResults_s
{
    int transStart; int transAbort; int transCommit; int transCount;
} expectedResults_t;

typedef enum { Unknown, Up, Down } tmState_t;
const char * tmStateViewable [] = { "Unknown", "Up", "Down" };


typedef struct
{
    const char * procName;
    const char * outFile;
    pid_t pid;
    tmState_t state;
    MPI_Comm comm;
    tmSyncResults_t results;

} tmProcess_t;

const int MAX_TEST_NODES = 6;

list<struct message_def> deathMsgs;
list<struct message_def> nodeDownMsgs;

pthread_mutex_t     notice_mutex;
pthread_cond_t      notice_cv;
bool                notice_signaled = false;

bool                processDied = false;
bool                nodeDown = false;

// "transaction manager" process info.  Indexed by node number.
tmProcess_t tmProcess[MAX_TEST_NODES] = {
    { "",      "TM00.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}},
    { "",      "TM01.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}},
    { "$TM02", "TM02.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}},
    { "$TM03", "TM03.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}},
    { "",      "TM04.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}},
    { "",      "TM05.lst", -1, Unknown, MPI_COMM_NULL, {0, 0, 0, 0, 0, 0, 0}}
};

MPI_Request workerReq[MAX_TEST_NODES];
int workerResponses=0;


const int MAX_TESTS = 10;

bool okOnVirtual[MAX_TESTS+1] = {false,  // test 0, no such test
                                 false,
                                 false,  // Test 2 -- no such test
                                 true,
                                 true,
                                 true,
                                 true,
                                 true,
                                 false,
                                 false,
                                 false};

// Spare node requirement for each test:
//   -1: no spare node requirement
//    0: spare node should not be available
//    1: spare node needed
int sparesNeeded[MAX_TESTS+1] = {-1, // Test 0 -- no such test
                                1,   // Test 1
                                -1,  // Test 2 -- no such test
                                -1,  // Test 3
                                -1,  // Test 4
                                -1,  // Test 5
                                -1,  // Test 6
                                -1,  // Test 7
                                1,   // Test 8
                                -1,  // Test 9 -- no such test
                                0};  // Test 10

tmState_t expectedState[MAX_TESTS+2][MAX_TEST_NODES] =
    {
        { Up, Up, Up, Up, Up, Up }, // Test 0
        { Up, Up, Down, Down, Up, Up }, // Test 1
        { Up, Up, Up, Up, Up, Up }, // Test 2
        { Up, Up, Up, Up, Up, Up }, // Test 3
        { Up, Up, Up, Up, Up, Up }, // Test 4
        { Up, Up, Up, Up, Up, Up }, // Test 5
        { Up, Up, Up, Up, Up, Up }, // Test 6
        { Up, Up, Down, Down, Up, Up }, // Test 7
        { Up, Up, Down, Down, Up, Up }, // Test 8
        { Up, Up, Up, Up, Up, Up }, // Test 9
        { Up, Up, Down, Down, Up, Up }, // Test 10
        { Up, Up, Down, Down, Up, Up }, // Test 11
    };

tmState_t expectedVirtualState[MAX_TESTS+2][MAX_TEST_NODES] =
    {
        { Up, Up, Up, Up, Up, Up }, // Test 0
        { Up, Up, Down, Down, Up, Up }, // Test 1
        { Up, Up, Up, Up, Up, Up }, // Test 2
        { Up, Up, Up, Up, Up, Up }, // Test 3
        { Up, Up, Up, Up, Up, Up }, // Test 4
        { Up, Up, Up, Up, Up, Up }, // Test 5
        { Up, Up, Up, Up, Up, Up }, // Test 6
        { Up, Up, Down, Up, Up, Up }, // Test 7
        { Up, Up, Down, Down, Up, Up }, // Test 8
        { Up, Up, Up, Up, Up, Up }, // Test 9
        { Up, Up, Down, Down, Up, Up }, // Test 10
        { Up, Up, Down, Down, Up, Up }, // Test 11
    };

int expectedInitialTms[MAX_TESTS+2] =
    { 6, // Test 0
      6, // Test 1
      6, // Test 2
      6, // Test 3
      6, // Test 4
      6, // Test 5
      6, // Test 6
      6, // Test 7
      6, // Test 8
      6, // Test 9
      6, // Test 10
      4  // Test 11
    };

expectedResults_t expectedResults[MAX_TESTS+1][MAX_TEST_NODES] =
    {
        { // Test 0 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 1
            {1, 0, 4, 4},
            {1, 0, 4, 4},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 0, 4, 4},
            {1, 0, 4, 4}
        },
        { // Test 2 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 3
            {0, 0, 1, 1},
            {0, 0, 1, 1},
            {1, 0, 1, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 1}
        },
        { // Test 4
            {0, 1, 0, 1},
            {1, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
        },
        { // Test 5
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60}
        },
        { // Test 6
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60}
        },
        { // Test 7
            {1, 0, 4, 4},
            {1, 0, 4, 4},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 0, 4, 4},
            {1, 0, 4, 4}
        },
        { // Test 8
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60}
        },
        { // Test 9 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 10
            {10, 0, 40, 40},
            {10, 0, 40, 40},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {10, 0, 40, 40},
            {10, 0, 40, 40}
        }
    };

expectedResults_t expectedVirtualResults[MAX_TESTS+1][MAX_TEST_NODES] =
    {
        { // Test 0 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 1
            {1, 0, 4, 4},
            {1, 0, 4, 4},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {1, 0, 4, 4},
            {1, 0, 4, 4}
        },
        { // Test 2 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 3
            {0, 0, 1, 1},
            {0, 0, 1, 1},
            {1, 0, 1, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 1},
            {0, 0, 1, 1}
        },
        { // Test 4
            {0, 1, 0, 1},
            {1, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
            {0, 1, 0, 1},
        },
        { // Test 5
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60},
            {10, 0, 60, 60}
        },
        { // Test 6
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60},
            {10, 50, 10, 60}
        },
        { // Test 7
            {1, 5, 0, 5},
            {1, 5, 0, 5},
            {0, 0, 0, 0},
            {1, 5, 0, 5},
            {1, 5, 0, 5},
            {1, 5, 0, 5}
        },
        { // Test 8 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 9 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        { // Test 10 -- unused
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        }
    };

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
        {
            printf("[%s] Process death notice received for %s (%d, %d:%d),"
                   " trans_id=%lld.%lld.%lld.%lld., aborted=%d\n",
                   MyName,
                   recv_msg->u.request.u.death.process_name,
                   recv_msg->u.request.u.death.nid,
                   recv_msg->u.request.u.death.pid,
                   recv_msg->u.request.u.death.verifier,
                   recv_msg->u.request.u.death.trans_id.txid[0],
                   recv_msg->u.request.u.death.trans_id.txid[1],
                   recv_msg->u.request.u.death.trans_id.txid[2],
                   recv_msg->u.request.u.death.trans_id.txid[3],
                   recv_msg->u.request.u.death.aborted);
        }
        deathMsgs.push_back( *recv_msg );
        processDied = true;
        
    }
    else if ( recv_msg->type == MsgType_NodeDown )
    {
        printf("[%s] Node DOWN notice received for %d (%s)\n", MyName,
               recv_msg->u.request.u.down.nid,
               recv_msg->u.request.u.down.node_name);
        nodeDownMsgs.push_back( *recv_msg );
        nodeDown = true;
    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }

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
bool checkConfig(int testNum)
{
    int virtualNodes;

    bool result = true;

    char *env = getenv("SQ_VIRTUAL_NODES");
    if (env && isdigit(env[0]))
    {
        virtualNodes = atoi(env);
        if ( tracing )
        {
            printf ("[%s] %d virtual nodes defined\n", MyName, virtualNodes);
        }
    }
    else
    {
        virtualNodes = 0;
        realCluster = true;
    }

    util.requestNodeInfo( -1, false, -1, -1, nodeData);

    if ( tracing )
    {
        printf ("[%s] Configuration contains %d logical nodes in %d "
                "physical nodes, %d spares, %d spares available\n",
                MyName, nodeData->num_nodes, nodeData->num_pnodes,
                nodeData->num_spares, nodeData->num_available_spares);
    }

    if ( virtualNodes )
    {
        // Verify number of virtual nodes matches number of logical nodes
        if ( virtualNodes != nodeData->num_nodes )
        {
            printf ("[%s] *** ERROR *** %d virtual nodes were specified but "
                    "configuration has  %d\n", MyName, virtualNodes,
                    nodeData->num_nodes);
            result = false;
        }
    }
    else
    {   // This test requires 3 physical nodes w/ 2 logical nodes each
        // and one spare node.
        if ( nodeData->num_pnodes != 4)
        {
            printf ("[%s] *** ERROR *** This test requires a configuration "
                  " with 3 physical nodes but only %d are configured\n",
                    MyName, nodeData->num_pnodes);
            result = false;
        }
        if ( testNum != -1 && sparesNeeded[testNum] != -1 )
        {
            if ( sparesNeeded[testNum] != nodeData->num_available_spares )
            {
                printf ("[%s] *** ERROR *** Test %d requires %d spare nodes "
                        "but %d are available.\n", MyName,
                        testNum, sparesNeeded[testNum],
                        nodeData->num_available_spares);
                result = false;
            }
        }
    }

    if ( nodeData->num_nodes != 6 )
    {
        printf ("[%s] *** ERROR *** This test requires a configuration "
                " with 6 logical nodes but only %d are configured\n",
                MyName, nodeData->num_nodes);
        result = false;
    }

    return result;
}

bool initChildComm ( )
{
    int rc;
    bool result = true;
    MPI_Comm comm;

    rc = XMPI_Comm_accept( util.getPort(), MPI_INFO_NULL, 0, MPI_COMM_SELF,
                          &comm );

    if (rc == MPI_SUCCESS)
    {
        XMPI_Comm_set_errhandler (comm, MPI_ERRORS_RETURN);

        MPI_Status status;
        tmSyncResults_t recvBuf;

        rc = XMPI_Recv (&recvBuf, sizeof(tmSyncResults_t)/sizeof(int),
                       MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       comm, &status);
        if (rc == MPI_SUCCESS)
        {
            tmProcess[recvBuf.nid].comm = comm;
            tmProcess[recvBuf.nid].pid = recvBuf.pid;
            if ( tracing )
            {
                printf("[%s] Identified child as (%d, %d)\n", MyName,
                       recvBuf.nid, recvBuf.pid);
            }
        }
        else
        {
            printf ("[%s] XMPI_Recv failed for child, rc=%d\n", MyName, rc);
            result = false;
        }
    }
    else
    {
        printf ("[%s] XMPI_Comm_accept failed, rc=%d\n", MyName, rc);
        result = false;
    }

    return result;
}


void initChildRecv()
{
    int rc;

    for (int i = 0; i < MAX_TEST_NODES; ++i)
    {
        rc = XMPI_Irecv(&tmProcess[i].results, sizeof(tmSyncResults_t)/sizeof(int),
                       MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       tmProcess[i].comm, &workerReq[i] );

        if (rc != MPI_SUCCESS)
        {
            printf ("[%s] XMPI_Irecv failed for tmProcess #%d, rc=%d\n", MyName,
                    i, rc);
        }
    }
}

void checkChildComm ( )
{

    int rc;
    int outcount;
    int completedOp[MAX_TEST_NODES];
    MPI_Status completedStatus[MAX_TEST_NODES];
    int errClass;

    do
    {
        outcount = 0;
        rc = XMPI_Testsome(MAX_TEST_NODES, workerReq, &outcount, completedOp,
                          completedStatus);
        MPI_Error_class(rc, &errClass);

        if (errClass == MPI_SUCCESS && outcount == MPI_UNDEFINED)
        {   // XMPI_Testsome returned no results
            break;
        }
        else if (errClass == MPI_SUCCESS)
        {
            for (int i=0; i<outcount; ++i)
            {
                if ( tracing )
                {
                    printf("[%s] got input from (%d, %d): test #%d,"
                           " start=%d, abort=%d, commit=%d, count=%d\n",
                           MyName,
                           tmProcess[completedOp[i]].results.nid,
                           tmProcess[completedOp[i]].results.pid,
                           tmProcess[completedOp[i]].results.testNum,
                           tmProcess[completedOp[i]].results.transStart,
                           tmProcess[completedOp[i]].results.transAbort,
                           tmProcess[completedOp[i]].results.transCommit,
                           tmProcess[completedOp[i]].results.transCount);
                }

                ++workerResponses;
            }
        }
        else if (errClass == MPI_ERR_IN_STATUS)
        {
            char buf[200];
            int count;

            for (int i=0; i<outcount; ++i)
            {
                if ( tracing )
                {
                    MPI_Error_string(completedStatus[i].MPI_ERROR, buf, &count);

                    printf("[%s] Status for tmProcess #%d: MPI_SOURCE=%d, "
                           "MPI_TAG=%d, MPI_ERROR=%d (%s)\n",
                           MyName, completedOp[i],
                           completedStatus[i].MPI_SOURCE,
                           completedStatus[i].MPI_TAG,
                           completedStatus[i].MPI_ERROR, buf);
                }
                ++workerResponses;
            }
        }
        else
        {
            char buf[200];
            int count;
            MPI_Error_string(rc, buf, &count);
            printf ("[%s] XMPI_Testsome failed, rc=%d (%s), outcount=%d, errClass=%d\n", MyName, rc, buf, outcount, errClass);

            break;
        }

    } while (outcount != 0);

}

bool createTMProcess ( int tmNum )
{
    int nid;
    int pid;
    Verifier_t verifier;
    char procName[25];
    char *tmSyncTestArgs[1] = {(char *) "-t"};
    bool result = false;

    tmProcess[tmNum].comm = MPI_COMM_NULL;

    if (util.requestNewProcess (tmNum, ProcessType_DTM, false,
                                tmProcess[tmNum].procName,
                                "tmSyncTest", "", tmProcess[tmNum].outFile,
                                ((tracing) ? 1: 0), tmSyncTestArgs, nid, pid,
                                verifier, procName) )
    {
        printf("[%s] created process %s (%d, %d:%d), output file %s\n",
               MyName, procName, nid, pid,verifier, tmProcess[tmNum].outFile);

        if ( initChildComm ( ) )
        {
            result = true;
        }
    }

    return result;
}

bool createProcesses ( void )
{
    for (int i = 0; i < MAX_TEST_NODES; ++i)
    {
        if  (!createTMProcess ( i ) )
        {
            return false;
        }
    }

    return true;
}

bool getTmProcStatus( int expectedTms )
{
    int count;
    MPI_Status status;
    struct message_def *msg;
    bool result = false;
    int readyTMs = 0;

    if ( gp_local_mon_io->acquire_msg( &msg ) != 0 )
    {
        printf ("[%s] Unable to acquire message buffer.\n", MyName);
        return result;
    }

    for (int i = 0; i < MAX_TEST_NODES; ++i)
    {
        tmProcess[i].state = Down;
    }

    msg->type = MsgType_Service;
    msg->noreply = false;
    msg->reply_tag = REPLY_TAG;
    msg->u.request.type = ReqType_ProcessInfo;
    msg->u.request.u.process_info.nid = util.getNid();
    msg->u.request.u.process_info.pid = util.getPid();
    msg->u.request.u.process_info.verifier = util.getVerifier();
    msg->u.request.u.process_info.process_name[0] = '\0';
    msg->u.request.u.process_info.target_nid = -1;
    msg->u.request.u.process_info.target_pid = -1;
    msg->u.request.u.process_info.target_verifier = -1;
    msg->u.request.u.process_info.type = ProcessType_DTM;
    msg->u.request.u.process_info.target_process_name[0] = '\0';

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
                int nid;

                for ( int i = 0; i < msg->u.reply.u.process_info.num_processes; i++ )
                {
                    nid = msg->u.reply.u.process_info.process[i].nid;

                    if ( tracing )
                    {
                        printf ( "[%s] TM ProcessInfo: nid=%d, pid=%d, Name=%s,"
                                 " program=%s, state=%d\n", MyName, nid,
                                 msg->u.reply.u.process_info.process[i].pid,
                                 msg->u.reply.u.process_info.process[i].process_name,
                                 msg->u.reply.u.process_info.process[i].program,
                                 msg->u.reply.u.process_info.process[i].state);
                    }

                    tmProcess[nid].state
                        = msg->u.reply.u.process_info.process[i].state
                          == State_Up ? Up : Down;
                    if (tmProcess[nid].state == Up) ++readyTMs;
                }

                if ( readyTMs == expectedTms )
                {
                    result = true;
                }
                else
                {
                    printf ( "[%s] *** Error *** Expected %d TM processes but only found %d\n", MyName, expectedTms, readyTMs);
                }
            }
            else
            {
                printf ("[%s] TM ProcessInfo failed, error=%s\n", MyName,
                    util.MPIErrMsg(msg->u.reply.u.process_info.return_code));
            }
        }
        else
        {
            printf("[%s] Invalid MsgType(%d)/ReplyType(%d) for TM ProcessInfo message\n",
                    MyName, msg->type, msg->u.reply.type);
        }
    }
    else
    {
        printf ("[%s] ProcessInfo reply message invalid.  Reply tag=%d, "
                "count=%d (expected %d)\n", MyName, msg->reply_tag,
                count, (int) sizeof (struct message_def));
    }

    gp_local_mon_io->release_msg(msg);
    msg = NULL;

    return result;
}


bool doTmSyncTest ( int test )
{
    bool result = true;

    workerResponses = 0;

    if ( getTmProcStatus( expectedInitialTms[test] ) )
    {  // expected number of tm processes are running

        initChildRecv();

        sleep(1);

        if ( tracing )
        {
            printf("[%s] sending event %d to all DTM processes.\n",
                   MyName, test);
        }
        util.requestSendEvent(-1, -1, -1, "", ProcessType_DTM, test, "");

        int retries = 0;
        // temp for debugging
        //        while ( workerResponses != MAX_TEST_NODES )
        while ( workerResponses != MAX_TEST_NODES && retries < 50)
        {
            sleep(2);
            if ( tracing )
            {
                printf("[%s] Checking tmProcess responses (have %d)\n", MyName,
                       workerResponses);
            }
            checkChildComm();
            ++retries;
        }
        if ( workerResponses != MAX_TEST_NODES )
        {
            result = false;
            printf ( "[%s] *** Error *** Expected %d responses from TM processes but "
                     "only got %d\n", MyName, MAX_TEST_NODES, workerResponses);
        }


        if ( test == 7  || test == 10 )
        {
            struct message_def msg;
            fflush (stdout);
            if ( !processDied )
            {
                printf("[%s] Waiting for process death.\n", MyName);
                if( !wait_for_notice() )
                {
                    printf("[%s] Failed to receive notice! Aborting\n",MyName);
                }
            }
            while (!deathMsgs.empty())
            {
                msg = deathMsgs.front();
                deathMsgs.pop_front();
                if ( tracing )
                {
                    printf("[%s] tmProcess #%d is down\n", MyName,
                           msg.u.request.u.death.nid);
                }

                tmProcess[msg.u.request.u.death.nid].state = Down;
            }
            if ( !nodeDown )
            {
                printf("[%s] Waiting for Node DOWN.\n", MyName);
                if( !wait_for_notice() )
                {
                    printf("[%s] Failed to receive notice! Aborting\n",MyName);
                }
            }
            while (!nodeDownMsgs.empty())
            {
                msg = nodeDownMsgs.front();
                nodeDownMsgs.pop_front();
                if ( tracing )
                {
                    printf("[%s] Node %d (%s) is DOWN.\n", MyName, 
                           msg.u.request.u.down.nid,
                           msg.u.request.u.down.node_name);
                }
            }
        }

        for (int i = 0; i < MAX_TEST_NODES; ++i)
        {
            if ( realCluster )
            {
                if  ( tmProcess[i].state != expectedState[test][i] )
                {
                    printf("[%s] After test #%d expected tmProcess[%d] to be %s "
                           "but it was %s.\n", MyName, test, i,
                           tmStateViewable[expectedState[test][i]],
                           tmStateViewable[tmProcess[i].state] );

                    result = false;
                }
            }
            else
            {
                if  ( tmProcess[i].state != expectedVirtualState[test][i] )
                {
                    printf("[%s] After test #%d expected tmProcess[%d] to be %s "
                           "but it was %s.\n", MyName, test, i,
                           tmStateViewable[expectedVirtualState[test][i]],
                           tmStateViewable[tmProcess[i].state] );

                    result = false;
                }
            }

            if ( tmProcess[i].state == Down  &&
                 test != 7  &&
                 test != 10 )
            {
                if ( tracing )
                {
                    printf("[%s] Restarting TM process on node %d\n",
                           MyName, i );
                }

                createTMProcess ( i );
            }
        }
    }

    return result;
}


int main (int argc, char *argv[])
{
    bool testSuccess;

    XMPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    XMPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);

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
    
    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    // Send startup message to monitor
    util.requestStartup ();

    // Verify correct number of nodes
    testSuccess = checkConfig( -1 );

    if ( testSuccess )
    {
        testSuccess = createProcesses();
    }

    if ( testSuccess )
    {
        sleep(1);

        // Execute the specified set of tests
        bool subTestSuccess;
        list<int> testSet;
        testSet = util.getTests();
        list<int>::iterator it;
        int testNum;

        for (it = testSet.begin(); it != testSet.end(); ++it)
        {
            testNum = *it;

            // Reset results
            for (int i=0; i<MAX_TEST_NODES; ++i)
            {
                tmProcess[i].results.transStart = 0;
                tmProcess[i].results.transAbort = 0;
                tmProcess[i].results.transCommit = 0;
                tmProcess[i].results.transCount = 0;
            }
            subTestSuccess = true;

            if ( testNum < 1  || testNum > MAX_TESTS
              || testNum == 2 || testNum == 9 )
            {
                printf("[%s] Test #%d is not a valid test.\n", MyName, testNum);
                continue;
            }

            // Check node configuration
            if ( !checkConfig ( testNum ) )
            {
                printf("[%s] Sub-test #%d failed\n", MyName, testNum);
                testSuccess = false;
                continue;
            }

            if ( realCluster || okOnVirtual[testNum])
            {
                printf("\n[%s] Beginning sub-test #%d\n", MyName, testNum);

                subTestSuccess = doTmSyncTest(testNum);
                if ( subTestSuccess && testNum == 8 )
                {
                    // Test 8 part 1 completed, need to initiate "test 9"
                    // so TM processes complete part 2 of the test.
                    printf("\n[%s] Beginning sub-test #%d, part 2\n",
                           MyName, testNum);
                    subTestSuccess = doTmSyncTest( 9 );
                }
                else if ( subTestSuccess && testNum == 10 )
                {
                    // Test 10 part 1 completed, need to initiate "test 11"
                    // so TM processes complete part 2 of the test.
                    printf("\n[%s] Beginning sub-test #%d, part 2\n",
                           MyName, testNum);
                    subTestSuccess = doTmSyncTest( 11 );
                }

                if ( subTestSuccess )
                {
                    // Verify results
                    if ( realCluster )
                    {
                        for (int i=0; i<MAX_TEST_NODES; ++i)
                        {
                            if (tmProcess[i].results.transStart
                                != expectedResults[testNum][i].transStart)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transStart=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedResults[testNum][i].transStart,
                                       tmProcess[i].results.transStart);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transAbort
                                != expectedResults[testNum][i].transAbort)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transAbort=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedResults[testNum][i].transAbort,
                                       tmProcess[i].results.transAbort);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transCommit
                                != expectedResults[testNum][i].transCommit)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transCommit=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedResults[testNum][i].transCommit,
                                       tmProcess[i].results.transCommit);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transCount
                                != expectedResults[testNum][i].transCount)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transTotal=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedResults[testNum][i].transCount,
                                       tmProcess[i].results.transCount);
                                subTestSuccess = false;
                            }
                        }
                    }
                    else
                    {
                        for (int i=0; i<MAX_TEST_NODES; ++i)
                        {
                            if (tmProcess[i].results.transStart
                                != expectedVirtualResults[testNum][i].transStart)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transStart=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedVirtualResults[testNum][i].transStart,
                                       tmProcess[i].results.transStart);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transAbort
                                != expectedVirtualResults[testNum][i].transAbort)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transAbort=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedVirtualResults[testNum][i].transAbort,
                                       tmProcess[i].results.transAbort);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transCommit
                                != expectedVirtualResults[testNum][i].transCommit)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transCommit=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedVirtualResults[testNum][i].transCommit,
                                       tmProcess[i].results.transCommit);
                                subTestSuccess = false;
                            }
                            if (tmProcess[i].results.transCount
                                != expectedVirtualResults[testNum][i].transCount)
                            {
                                printf("[%s] Test #%d, worker #%d, expected transTotal=%d,"
                                       " but got %d instead.\n", MyName, testNum, i,
                                       expectedVirtualResults[testNum][i].transCount,
                                       tmProcess[i].results.transCount);
                                subTestSuccess = false;
                            }
                        }
                    }
                }

                if (subTestSuccess)
                {
                    printf("[%s] Sub-test #%d passed\n", MyName, testNum);
                }
                else
                {
                    printf("[%s] Sub-test #%d failed\n", MyName, testNum);
                    testSuccess = false;
                }
            }
            else
            {
                printf("[%s] Skipping sub-test #%d because it is not "
                       "appropriate on a virtual cluster.\n", MyName, testNum);
            }
        }
    }

    printf("TmSync Test:\t\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    // tell monitor we are exiting
    util.requestExit ( );

    util.requestShutdown ( ShutdownLevel_Normal );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit ( testSuccess ? 0 : 1 );
}
