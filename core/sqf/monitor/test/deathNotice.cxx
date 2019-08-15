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

// todo:
//   make sure monitor deletes death notice interest for $SERV0
//   use semaphore or other instead of delay while waiting for notice
//   possibly start several deathUnreg processes rather than one at a time
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"
#include "deathNotice.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;
int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

const int MAX_WORKERS = 5;
bool workerStartProcess[] = {false, false, false, false, false};
int workerReqNotice[] = { 1, 0, 0, 0, 0 };
const char *workerName[] = {"$SERV0","$SERV1", "$TM00", "$TM01", "$TM02"};
#ifdef USE_NOTICE_TRANSID
const int workerExpNotices[] = { 1, 0, 0, 1, 3};
#else
const int workerExpNotices[] = { 1, 0, 0, 1, 1};
#endif


class WorkerProcess
{
public:
    WorkerProcess( const char * name
                 , int nid
                 , int pid
                 , Verifier_t verifier
                 , int expNotices);
    ~WorkerProcess(){}
    const char * getName() { return name_.c_str(); }
    int getNid() { return nid_; }
    int getPid() { return pid_; }
    int getVerifier() { return verifier_; }
    void incrDeathNotice() { ++deathNotices_; }
    int getExpNoticeCount() { return expNotices_; }
    int getDeathNoticeCount() { return deathNotices_; }

private:
    string name_;
    int nid_;
    int pid_;
    Verifier_t verifier_;
    int expNotices_;
    int deathNotices_;
};

WorkerProcess::WorkerProcess( const char * name
                            , int nid
                            , int pid
                            , Verifier_t verifier
                            , int expNotices)
              : name_(name)
              , nid_(nid)
              , pid_(pid)
              , verifier_(verifier)
              , expNotices_(expNotices)
              , deathNotices_ (0)
{
}

WorkerProcess* procList[50];
int procListCount = 0;



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

        bool found = false;
        for (int i=0; i<procListCount; i++)
        {
            if (procList[i]->getNid() == recv_msg->u.request.u.death.nid
             && procList[i]->getPid() == recv_msg->u.request.u.death.pid
             && procList[i]->getVerifier() == recv_msg->u.request.u.death.verifier)
            {
                procList[i]->incrDeathNotice();
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("[%s] Could not find procList object for (%d, %d)\n", 
                   MyName, recv_msg->u.request.u.death.nid,
                   recv_msg->u.request.u.death.pid);
        }
    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}

void sendCommand(int command, const char *msg, MPI_Comm &comm)
{

    int sendbuf[6];
    int recvbuf[100];
    int rc;
    const int clientTag = 99;
    MPI_Status status;

    sendbuf[0] = command;
    strcpy((char *) &sendbuf[1], msg);

    rc = XMPI_Sendrecv (sendbuf, 6, MPI_INT, 0, clientTag,
                       recvbuf, 100, MPI_CHAR, MPI_ANY_SOURCE,
                       MPI_ANY_TAG, comm, &status);
    if (rc != MPI_SUCCESS)
    {
        printf("[%s] SendRecv failed to deathUnreg, rc = (%d) %s\n",
               MyName, rc, util.MPIErrMsg(rc));
    }
}

bool testUnregister ()
{
    const int UNREG_COUNT = 5;
    char *childArgs[1] = {(char *) "-t"};
    bool testSuccess = true;

    int nid;
    int pid;
    Verifier_t verifier;
    MPI_Comm comm;
    char procName[25];

    for (int i = 0; i < UNREG_COUNT; i++)
    {
        if (util.requestNewProcess (1, ProcessType_Generic, false, (char *) "",
                                    "deathUnreg", "", "",
                                    ((tracing) ? 1: 0), childArgs, nid, pid,
                                    verifier,
                                    procName))
        {
            if ( tracing )
                printf("[%s] created new process %s (%d, %d)\n",
                       MyName, procName, nid, pid);
            procList[0] = new WorkerProcess ( procName, nid, pid, verifier, 1);
            procListCount = 1;


            // Open process and send commands to it
            if ( util.openProcess (procName, verifier, 0, comm) )
            {
                for ( int p=0; p < MAX_WORKERS; p++ )
                {
                    sendCommand(CMD_REG_INTEREST, workerName[p], comm);
                }
                sendCommand(CMD_END, "", comm);

                // Disconnect from process
                util.closeProcess ( comm );
            }

            for (int j = 0; j < 5; j++)
            {
                if ( procList[0]->getDeathNoticeCount() != 0)
                {
                    if ( tracing )
                        printf("[%s] death notice recognized for process %s"
                               " (%d, %d)\n",
                               MyName, procName, nid, pid);
                    break;
                }
                else
                {
                    if ( tracing )
                        printf("[%s] delaying while waiting for notice\n",
                               MyName);
                    sleep(1);
                }
            }
        }
        else
        {
            printf ("[%s] error starting deathUnreg process\n", MyName);
            
            testSuccess = false;
        }
    }

    return testSuccess;
}

bool testDeathNotices ()
{
    MPI_Comm manager_comm[MAX_WORKERS];
    int workerNid[MAX_WORKERS];
    int workerPid[MAX_WORKERS];
    Verifier_t workerVerifier[MAX_WORKERS];
    bool workerUp[MAX_WORKERS];
    char *serverArgs[1] = {(char *) "-t"};
    int workerChildren = 0;
    _TM_Txid_External transid = {{0LL, 0LL, 0LL, 0LL}};
    int sendbuf[3];
    char recvbuf[100];
    MPI_Status status;
    bool testSuccess = true;
    char procName[25];

    for (int i = 0; i < MAX_WORKERS; i++)
    {
        manager_comm[i] = MPI_COMM_NULL;
        workerUp[i] = false;

        if ( workerStartProcess[i] )
        {
            util.requestNewProcess ( 1, ProcessType_Generic, false, 
                                     workerName[i], "server", "", "",
                                     ((tracing) ? 1: 0), serverArgs,
                                     workerNid[i], workerPid[i], 
                                     workerVerifier[i], procName);
            ++workerChildren;

        }

        if (util.requestProcInfo ( workerName[i], workerNid[i], workerPid[i], workerVerifier[i]))
        {
            procList[procListCount]
                = new WorkerProcess ( workerName[i], workerNid[i], workerPid[i],
                                      workerVerifier[i], workerExpNotices[i] );
            if ( tracing )
                printf ("[%s] Worker #%d: %s is (%d, %d:%d)\n", MyName,
                        procListCount, workerName[i],
                        workerNid[i], workerPid[i], workerVerifier[i]);
            ++procListCount;
        }
        else
        {
            printf ("[%s] Unable to get process info for %s\n", MyName,
                    workerName[i]);
            testSuccess = false;
        }

        if ( util.openProcess( workerName[i]
                             , workerVerifier[i]
                             , workerReqNotice[i]
                             , manager_comm[i] ))
        {
            workerUp[i] = true;
            if ( tracing ) printf ("[%s] worker %d connected.\n", MyName, i);
        }
        else
        {
            workerUp[i] = false;
            testSuccess = false;
            printf ("[%s] worker %d failed to connect.\n", MyName, i);
        }
    }


    // do the work
    if ( tracing )
    {
        printf ("[%s] Starting work with %d workers\n", MyName, MAX_WORKERS);
    }

    util.requestNotice(workerNid[3], workerPid[3], workerVerifier[3], "", false, transid);
    util.requestNotice(-1, -1, workerVerifier[4], workerName[4], false, transid);

#ifdef USE_NOTICE_TRANSID
    transid.txid[0] = 1;
    util.requestNotice(workerNid[2], workerPid[2], workerVerifier[2], false, transid);
    util.requestNotice(workerNid[3], workerPid[3], workerVerifier[3], false, transid);
    util.requestNotice(workerNid[4], workerPid[4], workerVerifier[4], false, transid);

    transid.txid[0] = 2;
    util.requestNotice(workerNid[4], workerPid[4], workerVerifier[4], false, transid);

    transid.txid[0] = 3;
    util.requestNotice(workerNid[4], workerPid[4], workerVerifier[4], false, transid);

    // Cancel all death notices associated with transaction id 1
    transid.txid[0] = 1;
    util.requestNotice( -1, -1, -1, true, transid );
#endif

    // close $SERV1
    sendbuf[0] = MyRank;
    sendbuf[1] = 1;
    sendbuf[2] = 2; // CMD_END;
    XMPI_Sendrecv (sendbuf, 3, MPI_INT, 0, 100  /* USER_TAG */,
                  recvbuf, 100, MPI_CHAR, MPI_ANY_SOURCE,
                  100 /* USER_TAG */, manager_comm[1], &status);

    util.requestClose( workerName[1], workerVerifier[1]);

    util.requestKill ( workerName[0], -1 );
    util.requestKill ( workerName[2], workerVerifier[2] );
    util.requestKill ( workerName[3], workerVerifier[3] );
    util.requestKill ( workerName[4], -1 );


    // close the server processes
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (workerUp[i])
        {
            util.requestClose ( workerName[i], workerVerifier[i] );
        }
    }

    // Wait for a while so can receive death notices
    sleep(2);

    // Verify that got all death notices
    for (int i=0; i<procListCount; i++)
    {
        if (procList[i]->getDeathNoticeCount()
            != procList[i]->getExpNoticeCount())
        {
            printf("[%s] For %s (%d, %d:%d) expected %d notices but got %d.\n",
                   MyName, procList[i]->getName(), procList[i]->getNid(),
                   procList[i]->getPid(), procList[i]->getVerifier(), 
                   procList[i]->getExpNoticeCount(),
                   procList[i]->getDeathNoticeCount());
            testSuccess = false;
        }
    }

    return testSuccess;
}


bool testMultipleDeathNotices ()
{
    const int MAX_WATCHERS = 3 ;
    MPI_Comm deathWatcherComm[MAX_WATCHERS];
    int deathWatcherNid[MAX_WATCHERS];
    int deathWatcherPid[MAX_WATCHERS];
    Verifier_t deathWatcherVerifier[MAX_WATCHERS];
    char deathWatcherName[MAX_WATCHERS][25];
    char *serverArgs[1] = {(char *) "-t"};
    int deathWatchers = 0;

    const int victimReqNid = 1;
    int victimNid;
    int victimPid;
    Verifier_t victimVerifier;
    char victimName[25];
    int resultNid;
    int resultPid;
    Verifier_t resultVerifier;
    bool testSuccess = true;

    // Create victim process
    if (!util.requestNewProcess ( victimReqNid, ProcessType_Generic, false, "",
                                  "server", "", "", ((tracing) ? 1: 0),
                                  serverArgs,
                                  victimNid, victimPid, victimVerifier, victimName))
    {
        printf("[%s] Failed to create victim process\n", MyName);
        return false;
    }

    if ( tracing )
        printf ("[%s] Created victim process: %s (%d, %d:%d)\n", MyName,
                victimName, victimNid, victimPid, victimVerifier);

    if ( !util.requestProcInfo ( victimName, resultNid, resultPid, resultVerifier))
    {
        printf ("[%s] Unable to get process info for victim process %s\n",
                MyName, victimName);

        // Clean up and return

        // Kill victim process
        util.requestKill ( victimName, victimVerifier );

        return false;
    }
    else
    {
        procList[procListCount]
            = new WorkerProcess( victimName, victimNid
                               , victimPid, victimVerifier, true );
        ++procListCount;
    }

    // Create death watcher processes
    for (int i = 0; i < MAX_WATCHERS; i++)
    {
        deathWatcherComm[i] = MPI_COMM_NULL;

        if (!util.requestNewProcess( i  // created death watcher in different nids
                                   , ProcessType_Generic
                                   , false
                                   , ""
                                   , "deathWatch"
                                   , ""
                                   , ""
                                   , ((tracing) ? 1: 0)
                                   , serverArgs
                                   , deathWatcherNid[i]
                                   , deathWatcherPid[i]
                                   , deathWatcherVerifier[i]
                                   , deathWatcherName[i]))
        {  // Failed to create process.  
            testSuccess = false;
            break;
        }

        ++deathWatchers;

        if ( tracing )
            printf ("[%s] Death watcher #%d process: %s is (%d, %d:%d)\n",
                    MyName, i, deathWatcherName[i], deathWatcherNid[i],
                    deathWatcherPid[i], deathWatcherVerifier[i] );

        // Verify process start by getting process info
        if (!util.requestProcInfo( deathWatcherName[i]
                                 , resultNid
                                 , resultPid
                                 , resultVerifier))
        {
            printf ("[%s] Unable to get process info for %s\n", MyName,
                    deathWatcherName[i]);
            testSuccess = false;
            break;
        }
        else
        {
            procList[procListCount] = new WorkerProcess( deathWatcherName[i]
                                                       , deathWatcherNid[i]
                                                       , deathWatcherPid[i]
                                                       , deathWatcherVerifier[i]
                                                       , true );
            ++procListCount;
        }

        // Open process and send command to it
        if ( util.openProcess( deathWatcherName[i]
                             , deathWatcherVerifier[i]
                             , 0
                             , deathWatcherComm[i]) )
        {
            if ( tracing ) printf ("[%s] connected to death watcher %s.\n",
                                   MyName, deathWatcherName[i]);

            // Send command to death watcher processes so that it registers
            // interest in death of victim process.
            sendCommand(CMD_REG_INTEREST, victimName, deathWatcherComm[i]);
        }
        else
        {
            printf ("[%s] Unable to communicate with death watcher "
                    "process %s (%d, %d)\n", MyName, deathWatcherName[i],
                    deathWatcherNid[i], deathWatcherPid[i] );
            testSuccess = false;
            break;
        }
    }
    
    if (!testSuccess)
    {
        // Clean up and return.
        //util.requestKill ( victimName, victimVerifier );
        util.requestKill ( victimName, -1 );

        for (int i=0; i<deathWatchers; ++i)
        {
            util.requestKill( deathWatcherName[i], -1 );
        }

        return testSuccess;
    }


    sleep(1);

    // kill victim process
    util.requestKill( victimName, victimVerifier );

    sleep(1);

    int sendbuf[6];
    char recvbuf[100];
    int rc;
    const int clientTag = 99;
    MPI_Status status;

    // get results from child processes
    for ( int i=0; i < MAX_WATCHERS; i++ )
    {
        sendbuf[0] = CMD_GET_STATUS;
        rc = XMPI_Sendrecv (sendbuf, 6, MPI_INT, 0, clientTag,
                           recvbuf, 100, MPI_CHAR, MPI_ANY_SOURCE,
                           MPI_ANY_TAG, deathWatcherComm[i], &status);
        if (rc == MPI_SUCCESS)
        {
            if ( tracing )
                printf ("[%s] For deathWatcher %s process death status=%s\n",
                        MyName, deathWatcherName[i], recvbuf );

            if ( strcmp(recvbuf, "OK") != 0 )
            {
                printf("[%s] deathWatcher #%d failed to get process death "
                       "notice for %s\n", MyName, i, victimName );
                testSuccess = false;
            }
        }
        else
        {
            printf ("[%s] Unable to communicate with death watcher "
                    "process %s (%d, %d), rc=%d\n", MyName,
                    deathWatcherName[i],
                    deathWatcherNid[i], deathWatcherPid[i], rc );
            testSuccess = false;
        }
    }

    // terminate death watchers
    for ( int i=0; i < MAX_WATCHERS; i++ )
    {
        sendCommand(CMD_END, "", deathWatcherComm[i]);

        // Disconnect from process
        util.closeProcess ( deathWatcherComm[i] );
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

    // verify correct number of nodes
    testSuccess = util.validateNodeCount(3);

    if ( testSuccess )
    {
        printf("[%s] Beginning sub-test #1 (register/unregister death notice)"
               ".\n", MyName);

        testSuccess = testUnregister();

        if (testSuccess)
        {
            printf("[%s] Sub-test #1 passed\n", MyName);
        }
    }

    if ( testSuccess )
    {
        printf("[%s] Beginning sub-test #2 (death notice via open)\n",
               MyName);

        testSuccess = testDeathNotices();

        if (testSuccess)
        {
            printf("[%s] Sub-test #2 passed\n", MyName);
        }
    }

    if ( testSuccess )
    {
        printf("[%s] Beginning sub-test #3 (multiple death notice).\n", MyName);

        testSuccess = testMultipleDeathNotices();

        if (testSuccess)
        {
            printf("[%s] Sub-test #3 passed\n", MyName);
        }
    }

    printf("Death Notice Test:\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
