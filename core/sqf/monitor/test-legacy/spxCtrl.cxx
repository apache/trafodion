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

// Test SPX process behavior
//   1.  Verify ability to start an SPX process on each of the physical nodes
//   2.  Verify that if an SPX process dies each of the other SPX
//       process receives a process death notification.
//   3.  Verify that can only start one spx process on a given logical node.
//   4.  Verify that cannot start an spx process on a logical node that
//       shares a physical node with another logical node where an spx
//       process is running.


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "spxCtrl.h"

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

typedef struct spx_process_def {
    bool dead;
    int  nid;
    int  pid;
    Verifier_t verifier;
    char procName[MAX_PROCESS_NAME];    // SPX process Name
    MPI_Comm comm;
} spxProcess_t;

spxProcess_t *spxProcess;
int spxProcessCount = 0;


// Routine for handling notices:
//   NodeDown, NodeUp, ProcessDeath, Shutdown, TmSyncAbort, TmSyncCommit
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_ProcessDeath )
    {
        if ( tracing )
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

        for (int i=0; i < spxProcessCount; ++i)
        {
            if ( spxProcess[i].nid == recv_msg->u.request.u.death.nid
                 && spxProcess[i].pid == recv_msg->u.request.u.death.pid )
            {
                printf("[%s] Marking dead SPX process %s (%d, %d)\n",
                       MyName, spxProcess[i].procName, spxProcess[i].nid,
                       spxProcess[i].pid);

                spxProcess[i].dead = true;
            }
        }


    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}


// Verify that the configuration contains the desired number and
// type of nodes required for this test.
bool checkConfig(int & lnodes, int & pnodes)
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
    }

    util.requestNodeInfo( -1, false, -1, -1, nodeData);

    // process node info data
    lnodes = nodeData->num_nodes;  // # logical nodes
    pnodes = nodeData->num_pnodes; // # physical nodes

    if ( tracing )
    {
        printf ("[%s] Configuration contains %d logical nodes in %d "
                "physical nodes\n", MyName, lnodes, pnodes);
    }

    if ( virtualNodes )
    {
        // Verify number of virtual nodes matches number of logical nodes
        if ( virtualNodes != lnodes )
        {
            printf ("[%s] *** ERROR *** %d virtual nodes were specified but "
                    "configuration has  %d\n", MyName, virtualNodes, lnodes);
            result = false;
        }
    }

    if (pnodes < 3)
    {
        printf ("[%s] *** ERROR *** This test requires a minimum of 3 "
                "physical nodes but only %d are configured.\n", MyName, pnodes);
        result = false;
    }

    // Allocate array to store spx process info.
    spxProcess = new spxProcess_t[pnodes];

    return result;
}


// Start SPX processes on each of the physical nodes
bool SPX_test1 ()
{
    int prevPnid = -1;
    int reqNid;
    char *childArgs[1] = {(char *) "-t"};
    bool testSuccess = true;

    for (int i = 0; i < nodeData->num_nodes; i++)
    {
        if (nodeData->node[i].pnid != prevPnid)
        {  // Found new physical node, start an SPX process

            if ( tracing )
            {
                printf("[%s] Will start SPX process on nid=%d, pnid=%d\n",
                       MyName, nodeData->node[i].nid, nodeData->node[i].pnid);
            }

            prevPnid = nodeData->node[i].pnid;
            reqNid = nodeData->node[i].nid;

            spxProcess[i].dead = false;
            if (util.requestNewProcess (reqNid, ProcessType_SPX, false,
                                        (char *) "", // Name
                                        "spxProc", "", "",
                                        ((tracing) ? 1: 0), childArgs,
                                        spxProcess[spxProcessCount].nid,
                                        spxProcess[spxProcessCount].pid,
                                        spxProcess[spxProcessCount].verifier,
                                        spxProcess[spxProcessCount].procName))
            {
                printf("[%s] Started SPX process %s (%d, %d:%d)\n", MyName,
                       spxProcess[spxProcessCount].procName,
                       spxProcess[spxProcessCount].nid,
                       spxProcess[spxProcessCount].pid,
                       spxProcess[spxProcessCount].verifier);

                // Open the SPX process
                if ( util.openProcess( spxProcess[spxProcessCount].procName
                                     , spxProcess[spxProcessCount].verifier
                                     , 0
                                     , spxProcess[spxProcessCount].comm) )
                {
                    if ( tracing ) printf ("[%s] connected to SPX process %s.\n",
                                           MyName,
                                           spxProcess[spxProcessCount].procName);

                }
                else
                {
                    printf ("[%s] Unable to communicate with SPX "
                            "process %s (%d, %d)\n", MyName,
                            spxProcess[spxProcessCount].procName,
                            spxProcess[spxProcessCount].nid,
                            spxProcess[spxProcessCount].pid );
                    testSuccess = false;
                    break;
                }

                ++spxProcessCount;

            }
            else
            {
                printf("[%s] Failed to start SPX process on node %d\n", MyName,
                       reqNid);
                testSuccess = false;
            }
        }
        else
        {
            if ( tracing )
            {
                printf ("[%s] Node #%d shares a physical node with "
                        "another logical node on physical node %d.\n", MyName,
                        nodeData->node[i].nid, nodeData->node[i].pnid);
            }
            multLogicalPerPhysical = true;
        }
    }

    return testSuccess;
}

bool SPX_test2 ()
{
    bool testSuccess = true;

    int sendbuf;
    replyMsg_t recvbuf;
    int rc;
    const int clientTag = 99;
    MPI_Status status;

    // Cause one SPX process to terminate
    sendbuf = CMD_END;
    rc = XMPI_Sendrecv (&sendbuf, 1, MPI_INT, 0, clientTag,
                       &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE,
                       MPI_ANY_TAG, spxProcess[2].comm, &status);

    sleep(1);

    // Ask each remaining SPX process for count of death notices received
    for (int i=0; i < spxProcessCount; ++i)
    {
        if ( i != 2)
        {
            sendbuf = CMD_GET_STATUS;
            rc = XMPI_Sendrecv (&sendbuf, 1, MPI_INT, 0, clientTag,
                               &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE,
                               MPI_ANY_TAG, spxProcess[i].comm, &status);
            if (rc == MPI_SUCCESS)
            {
                if (tracing)
                {
                    printf("[%s] SPX process %d, got %d notifications\n",
                           MyName, i, recvbuf.noticeCount);
                }

                if (recvbuf.noticeCount != 1) testSuccess = false;
            }
        }
    }

    return testSuccess;
}

// Attempt to start a second spx process on a given physical node (should fail)
bool SPX_test3 ()
{
    char *childArgs[1] = {(char *) "-t"};

    bool testSuccess = true;
    int nid;
    int pid;
    Verifier_t verifier;
    char procName[25];

    if (util.requestNewProcess (0, ProcessType_SPX, false,
                                (char *) "", // Name
                                "spxProc", "", "",
                                ((tracing) ? 1: 0), childArgs,
                                nid, pid, verifier, procName))
    {
        printf("[%s] *** Error *** successfully started second SPX process %s "
               "(%d, %d:%d) on node 0.\n", MyName, procName, nid, pid, verifier);
        testSuccess = false;
    }
    else
    {
        if ( tracing )
        {
            printf ("[%s] As expected, could not start a second SPX process "
                    "on node %d.\n", MyName, 0);
        }
    }

    return testSuccess;
}


// Attempt to start an spx process on a logical node that is on the
// same physical node as another spx process (should fail).
bool SPX_test4 ()
{
    int prevPnid = -1;
    char *childArgs[1] = {(char *) "-t"};
    bool testSuccess = true;

    for (int i = 0; i < nodeData->num_nodes; i++)
    {
        if (nodeData->node[i].pnid == prevPnid)
        {  // Found a logical node on same physical node as another
           // logical node.
            int nid;
            int pid;
            Verifier_t verifier;
            char procName[25];

            if ( tracing )
            {
                printf ("[%s] Node #%d shares a physical node with "
                        "another logical node on physical node %d.  "
                        "Attempting to start an SPX process on node #%d.\n",
                        MyName, nodeData->node[i].nid, nodeData->node[i].pnid,
                        nodeData->node[i].nid);
            }

            if (util.requestNewProcess (nodeData->node[i].nid, ProcessType_SPX,
                                        false, (char *) "", // Name
                                        "spxProc", "", "",
                                        ((tracing) ? 1: 0), childArgs,
                                        nid, pid, verifier, procName))
            {
                printf("[%s] *** Error *** successfully started second SPX "
                       "process %s (%d, %d) on physical node %d / logical "
                       "node %d.\n", MyName,
                       procName, nid, pid, nodeData->node[i].pnid,
                       nodeData->node[i].nid);

                testSuccess = false;
            }
            else
            {
                if ( tracing )
                {
                    printf ("[%s] As expected, could not start a second SPX "
                            "process on physical node %d / logical node %d.\n",
                            MyName, nodeData->node[i].pnid,
                            nodeData->node[i].nid);
                }
            }

            break;
        }

        prevPnid = nodeData->node[i].pnid;
    }

    return testSuccess;
}

int main (int argc, char *argv[])
{

    bool testSuccess = true;

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    // Send startup message to monitor
    util.requestStartup ();

    // Verify the node configuration
    int lnodes;
    int pnodes;
    testSuccess = checkConfig(lnodes, pnodes);

    if ( testSuccess )
    {
        printf("[%s] Beginning SPX sub-test 1\n", MyName);

        testSuccess = SPX_test1();
    }

    if ( testSuccess )
    {
        printf("[%s] Beginning SPX sub-test 2\n", MyName);

        testSuccess = SPX_test2();
    }

    if ( testSuccess )
    {
        printf("[%s] Beginning SPX sub-test 3\n", MyName);

        testSuccess = SPX_test3();
    }

    if ( testSuccess )
    {
        if ( multLogicalPerPhysical )
        {
            printf("[%s] Beginning SPX sub-test 4\n", MyName);

            testSuccess = SPX_test4();
        }
        else
        {
            printf("[%s] SPX sub-test 4 skipped because current Trafodion "
                   "configuration does not contain multiple logical nodes "
                   "per physical node.\n", MyName);
        }
    }

    int sendbuf;
    replyMsg_t recvbuf;
    const int clientTag = 99;
    MPI_Status status;

    for (int i=0; i < spxProcessCount; ++i)
    {
        // Tell the SPX process to exit
        sendbuf = CMD_END;
        XMPI_Sendrecv (&sendbuf, 1, MPI_INT, 0, clientTag,
                      &recvbuf, 1, MPI_INT, MPI_ANY_SOURCE,
                      MPI_ANY_TAG, spxProcess[i].comm, &status);
    }

    printf("SPX Process Test:\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
