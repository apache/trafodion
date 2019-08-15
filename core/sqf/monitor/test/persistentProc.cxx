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

// Persistent process test for the monitor

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect


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

    }
    else if ( recv_msg->type == MsgType_NodeDown )
    {
        printf("[%s] Node %d (%s) is DOWN.\n", MyName,
               recv_msg->u.request.u.down.nid,
               recv_msg->u.request.u.down.node_name);
    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}


bool testPersistent ()
{
    bool testSuccess = true;
    int procNid;
    int procPid;
    Verifier_t procVerifier;
    char procName[25];
    char *serverArgs[1] = {(char *) "-t"};
    enum { TEST_FAILED=0 };
    const int persistNode1 = 1;
    const int persistNode2 = 2;

    // Start the server process
    if (!util.requestNewProcess( persistNode1
                               , ProcessType_PERSIST
                               , false
                               , "$ABC"
                               , "server"
                               , ""
                               , ""
                               , ((tracing) ? 1: 0)
                               , serverArgs
                               , procNid
                               , procPid
                               , procVerifier
                               , procName) )
    {
        testSuccess = false;
        return testSuccess;
    }

    // Allow time for process creation
    sleep(1);

    // Verify process is running
    int statNid1;
    int statPid1;
    Verifier_t statVerifier1;
    if (util.requestProcInfo ( "$ABC", statNid1, statPid1, statVerifier1))
    {

        if ( statNid1 != persistNode1 )
        {
            printf ("[%s] process $ABC (%d, %d:%d) is not running on node %d "
                    "as expected.\n", MyName, statNid1, statPid1,
                    statVerifier1, persistNode1);
            testSuccess = false;
            return testSuccess;
        }
        else
        {
            printf ("[%s] Started persistent process $ABC (%d, %d:%d)\n",
                    MyName, statNid1, statPid1, statVerifier1 );
        }
    }
    else
    {
        printf ("[%s] Started persisten process $ABC but unable to get "
                "process info for it.\n", MyName);
        testSuccess = false;
        return testSuccess;
    }

    printf ("[%s] Killing process $ABC\n", MyName);

    // Kill the server process, expect it to be restarted on the same node
    util.requestKill ( "$ABC", procVerifier );

    // Allow time for process kill and after-effects
    sleep(1);

    // Verify process was restarted on original node
    int statNid2;
    int statPid2;
    Verifier_t statVerifier2;
    if (util.requestProcInfo ( "$ABC", statNid2, statPid2, statVerifier2))
    {
        if ( statNid2 != persistNode1 )
        {
            printf ("[%s] process $ABC (%d, %d:%d) is not running on node %d "
                    "as expected.\n", MyName, statNid2, statPid2,
                    statVerifier2, persistNode1);
            testSuccess = false;
            return testSuccess;
        }
        if ( statPid2 == statPid1 )
        {
            printf ("[%s] process $ABC apparently not restarted, old pid is "
                     "the same as the current pid (%d)\n", MyName, statPid2);
            testSuccess = false;
            return testSuccess;
        }
        else
        {
            printf ("[%s] Persistent process $ABC (%d, %d:%d) was restarted as "
                    "expected.\n",
                    MyName, statNid2, statPid2, statVerifier2);
        }
    }
    else
    {
        printf ("[%s] Unable to get process info for $ABC\n", MyName);
        testSuccess = false;
        return testSuccess;
    }

    printf ("[%s] Killing process $ABC\n", MyName);

    // Kill the server process again, do not expect restart since persistent
    // retry count has been exceeded.
    util.requestKill ( "$ABC", -1 );

    // Allow time for process kill and after-effects
    sleep(1);

    if (util.requestProcInfo ( "$ABC", statNid2, statPid2, statVerifier2))
    {
        printf("[%s] Unexpectedly got $ABC (%d, %d:%d) process status\n",
               MyName, statNid2, statPid2, statVerifier2);
        testSuccess = false;
        return testSuccess;
    }
    else
    {
        printf("[%s] Confirmed: process $ABC was not restarted.\n",
               MyName);
    }

    // Start the server process
    if (!util.requestNewProcess( persistNode1
                               , ProcessType_PERSIST
                               , false
                               , "$ABC"
                               , "server"
                               , ""
                               , ""
                               , ((tracing) ? 1: 0)
                               , serverArgs
                               , procNid
                               , procPid
                               , procVerifier
                               , procName) )
    {
        testSuccess = false;
        return testSuccess;
    }

    // Allow time for process creation
    sleep(1);

    // Verify process is running
    if (util.requestProcInfo ( "$ABC", statNid1, statPid1, statVerifier1))
    {
        if ( statNid1 != persistNode1 )
        {
            printf ("[%s] process $ABC (%d, %d:%d) is not running on node %d "
                    "as expected.\n", MyName, statNid1, statPid1, statVerifier1,
                    persistNode1);
            return TEST_FAILED;
        }
        else
        {
            printf ("[%s] Started persistent process $ABC (%d, %d:%d)\n",
                    MyName, statNid1, statPid1, statVerifier1);
        }
    }
    else
    {
        printf ("[%s] Unable to get process info for $ABC\n", MyName);
        testSuccess = false;
        return testSuccess;
    }

    printf ("[%s] Downing node %d\n", MyName, persistNode1);

    // Down the node
    util.requestNodeDown ( persistNode1 );

    // Verify node is down
    struct NodeInfo_reply_def * nodeData;
    for (int i=0; i<30; ++i)
    {
        if ( util.requestNodeInfo ( -1, false, -1, -1, nodeData ) )
        {  // Got node data
            if ( nodeData->node[persistNode1].state == State_Down )
            {
                printf( "[%s] Node status for node %d is %s.\n"
                      , MyName
                      , persistNode1
                      , StateString(nodeData->node[persistNode1].state) );
                break;
            }
            else
            {
                printf("[%s] Node status for node %d is %s, expecting %s\n"
                      , MyName
                      , persistNode1
                      , StateString(nodeData->node[persistNode1].state)
                      , StateString(State_Down) );
            }
        }
        else
        {   // Failed to get node data
            printf ("[%s] Unable to get node info\n", MyName);
            testSuccess = false;
            return testSuccess;
        }
        sleep(2);
    }
    if ( nodeData->node[persistNode1].state != State_Down )
    {
        printf ("[%s] After downing node %d, node state=%s but expected "
                "state=%s\n"
                , MyName
                , persistNode1
                , StateString(nodeData->node[persistNode1].state)
                , StateString(State_Down) );
        testSuccess = false;
        return testSuccess;
    }
    
    // Verify process was restarted on new node
    if (util.requestProcInfo ( "$ABC", statNid1, statPid1, statVerifier1))
    {
        if ( statNid1 != persistNode2 )
        {
            printf( "[%s] process $ABC (%d, %d:%d) is not running on node %d "
                    "as expected.\n"
                   , MyName
                   , statNid1
                   , statPid1
                   , statVerifier1
                   , persistNode2 );
            testSuccess = false;
            return testSuccess;
        }
        else
        {
            printf( "[%s] Persistent process $ABC (%d, %d:%d) was restarted as "
                    "expected.\n"
                  , MyName
                  , statNid1
                  , statPid1
                  , statVerifier1);
        }
    }
    else
    {
        printf ("[%s] Unable to get process info for $ABC\n", MyName);
        testSuccess = false;
        return testSuccess;
    }

    printf ("[%s] Killing process $ABC\n", MyName);

    // Kill the server process again, do not expect restart because
    // restart count exceeded.
    util.requestKill ( "$ABC", -1 );

    // Allow time for process kill and after-effects
    sleep(1);

    // Verify process not restarted
    if (util.requestProcInfo( "$ABC", statNid2, statPid2, statVerifier2 ))
    {
        printf( "[%s] Persistent process $ABC (%d, %d:%d) restart was not "
                "expected.\n"
              , MyName
              , statNid2
              , statPid2
              , statVerifier2);
        testSuccess = false;
        return testSuccess;
    }
    else
    {
        printf( "[%s] Confirmed: process $ABC was not restarted.\n",  MyName);
    }

    return testSuccess;

}

int main (int argc, char *argv[])
{

    bool testSuccess;

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    // Send startup message to monitor
    util.requestStartup ();

    // Verify correct number of nodes
    testSuccess = util.validateNodeCount(6);

    if ( testSuccess )
    {
        testSuccess = testPersistent();
    }

    printf("Persistent Process Test:\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit ( testSuccess ? 0 : 1 );
}
