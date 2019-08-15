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

// Process creation test.
// Create processes on local and remote nodes using both
// waited and no-waited create.
//


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

const int MAX_WORKERS = 5;
const char *workerName[] = {"$SERV0","$SERV1", "$SERV2", "$SERV3", "$SERVX"};
int workerCreateNid[MAX_WORKERS];  // Node on which to create the process
bool workerNowait[MAX_WORKERS] = {false, false, true, true, true};
int workerNid[MAX_WORKERS];
int workerPid[MAX_WORKERS];
int workerVerifier[MAX_WORKERS];

bool testSuccess = true;

class WorkerProcess
{
public:
    WorkerProcess(const char * name, int nid, int pid, Verifier_t verifier);
    ~WorkerProcess(){}
    const char * getName() { return name_.c_str(); }
    int getNid() { return nid_; }
    int getPid() { return pid_; }
    int getVerifier() { return verifier_; }
    void incrDeathNotice() { ++deathNotices_; }
    int getDeathNoticeCount() { return deathNotices_; }
    void incrCreatedNotice() { ++createdNotices_; }
    int getCreatedNoticeCount() { return createdNotices_; }

private:
    string name_;
    int nid_;
    int pid_;
    Verifier_t verifier_;
    int deathNotices_;
    int createdNotices_;
};

WorkerProcess::WorkerProcess( const char *name
                            , int nid
                            , int pid
                            , Verifier_t verifier)
              : name_(name)
              , nid_(nid)
              , pid_(pid)
              , verifier_(verifier)
              , deathNotices_ (0)
              , createdNotices_(0) 
{
}

WorkerProcess* procList[50];
int procListCount = 0;


// Routine for handling notices
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_Shutdown )
    {
        if ( tracing )
            printf("[%s] Shutdown notice received, level=%d\n",
                   MyName,
                   recv_msg->u.request.u.shutdown.level);
    }
    else if ( recv_msg->type == MsgType_ProcessDeath )
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
             && procList[i]->getPid() == recv_msg->u.request.u.death.pid)
            {
                procList[i]->incrDeathNotice();
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("[%s] Could not find procList object for %s (%d, %d)\n",
                   MyName,
                   recv_msg->u.request.u.death.process_name,
                   recv_msg->u.request.u.death.nid,
                   recv_msg->u.request.u.death.pid);
        }

    }
    else if ( recv_msg->type == MsgType_ProcessCreated )
    {
        if ( tracing )
            printf("[%s] Process creation notice received for %s (%d, %d:%d),"
                   " port=%s, tag=%lld, ret code=%d\n",
                   MyName,
                   recv_msg->u.request.u.process_created.process_name,
                   recv_msg->u.request.u.process_created.nid,
                   recv_msg->u.request.u.process_created.pid,
                   recv_msg->u.request.u.process_created.verifier,
                   recv_msg->u.request.u.process_created.port,
                   recv_msg->u.request.u.process_created.tag,
                   recv_msg->u.request.u.process_created.return_code);
        bool found = false;
        for (int i=0; i<procListCount; i++)
        {
            if ( strcmp(procList[i]->getName(),
                        recv_msg->u.request.u.process_created.process_name) == 0)
            {
                printf("[%s] *** ERROR *** Got process creation notice for "
                       " %s (%d, %d) but process already in list.\n",
                       MyName,
                       recv_msg->u.request.u.process_created.process_name,
                       recv_msg->u.request.u.process_created.nid,
                       recv_msg->u.request.u.process_created.pid);

                testSuccess = false;

                found = true;
                break;
            }
        }
        if (!found)
        {
            int i;
            for (i=0; i<MAX_WORKERS; i++)
            {
                if ( strcmp(workerName[i],
                            recv_msg->u.request.u.process_created.process_name) == 0)
                {
                    printf("[%s] Got process creation notice for "
                           " %s (%d, %d) adding process to procList[%d].\n",
                           MyName, 
                           recv_msg->u.request.u.process_created.process_name,
                           recv_msg->u.request.u.process_created.nid,
                           recv_msg->u.request.u.process_created.pid, i);
    
                    found = true;
                    break;
                }
            }
            if ( found )
            {
                procList[i]
                = new WorkerProcess ( 
                          recv_msg->u.request.u.process_created.process_name,
                          recv_msg->u.request.u.process_created.nid,
                          recv_msg->u.request.u.process_created.pid,
                          recv_msg->u.request.u.process_created.verifier);
                procList[i]->incrCreatedNotice();

                ++procListCount;

                if ( tracing )
                    printf ("[%s] Worker #%d: %s (%d, %d:%d)\n", MyName,
                            i, 
                            recv_msg->u.request.u.process_created.process_name,
                            recv_msg->u.request.u.process_created.nid,
                            recv_msg->u.request.u.process_created.pid,
                            recv_msg->u.request.u.process_created.verifier);
            }
        }

    }
    else if ( recv_msg->type == MsgType_Change ) 
    {
        if (tracing) 
            printf("[%s] Message Type Change: ConfigType=%d GroupName=%s Key=%s Value=%s\n", MyName, 
                   recv_msg->u.request.u.change.type,
                   recv_msg->u.request.u.change.group,
                   recv_msg->u.request.u.change.key,
                   recv_msg->u.request.u.change.value );
    }
    else {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
        testSuccess = false;
    }
}


int main (int argc, char *argv[])
{
    char *serverArgs[1] = {(char *) "-t"};
    char *server5ArgsTrace[2] = {(char *) "-t",(char *) "-x"};
    char *server5ArgsNoTrace[1] = {(char *) "-x"};
    char procName[25];
    
    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");
    gp_local_mon_io->set_cb(recv_notice_msg, "event");
    gp_local_mon_io->set_cb(recv_notice_msg, "recv");
    gp_local_mon_io->set_cb(recv_notice_msg, "unsol");

    util.requestStartup ();

    // verify correct number of nodes
    testSuccess = util.validateNodeCount(2);
    
    if(!testSuccess)
    {
       printf("Process Creation Test:\t\t%s\n", "FAILED due to an invalid configuration");

        // tell monitor we are exiting
        util.requestExit ( );

        XMPI_Close_port (util.getPort());
        if ( gp_local_mon_io )
        {
            delete gp_local_mon_io;
        }
        exit (0);
    }

    int myNid = util.getNid();
    int otherNid = ( myNid == 0 ) ? 1 : 0;

    if ( util.getShutdownBeforeStartup() )
    {
        printf("[%s] ShutdownBeforeStartup Test\n", MyName);
        
        workerCreateNid[0] = myNid;
        workerCreateNid[1] = otherNid;
        workerCreateNid[2] = myNid;
        workerCreateNid[3] = otherNid;
        workerCreateNid[4] = otherNid;

        // Create all processes except the 5th process
        for (int i = 0; i < (MAX_WORKERS-1); i++)
        {
            printf("[%s] Starting process %s on node %d, %s\n",
                   MyName, workerName[i], workerCreateNid[i],
                   (workerNowait[i] ? "nowait" : "wait"));

            testSuccess = util.requestNewProcess( workerCreateNid[i]
                                                , ProcessType_Generic
                                                , workerNowait[i]
                                                , workerName[i]
                                                , "server"
                                                , ""      // inFile
                                                , ""      // outFile
                                                , ((tracing) ? 1: 0)
                                                , serverArgs
                                                , workerNid[i]
                                                , workerPid[i]
                                                , workerVerifier[i]
                                                , procName);

            if ( testSuccess )
            {
                if ( workerNowait[i] == false )
                {
                    procList[procListCount] = new WorkerProcess( workerName[i]
                                                               , workerNid[i]
                                                               , workerPid[i]
                                                               , workerVerifier[i] );
                    ++procListCount;
                    
                    if ( tracing )
                        printf ("[%s] Worker #%d: %s (%d, %d:%d)\n", MyName,
                                procListCount-1, 
                                workerName[i], workerNid[i], 
                                workerPid[i], workerVerifier[i] );
                }
            }
        }

        sleep(2);

        // Kill the processes previously created
        for (int j=0; j<procListCount; j++)
        {
            printf("[%s] Killing process %s (%d, %d)\n", MyName,
                   procList[j]->getName(),
                   procList[j]->getNid(), procList[j]->getPid());

            util.requestKill( procList[j]->getName(), procList[j]->getVerifier() );
        }

        // Create the the 5th process
        printf("[%s] Starting process %s on node %d, %s, args=%s %s\n",
               MyName, workerName[(MAX_WORKERS-1)], workerCreateNid[(MAX_WORKERS-1)],
               (workerNowait[(MAX_WORKERS-1)] ? "nowait" : "wait"),
               server5ArgsTrace[0],((tracing)?server5ArgsTrace[1]:"") );

        testSuccess = util.requestNewProcess
            ( workerCreateNid[(MAX_WORKERS-1)], ProcessType_Generic,
              workerNowait[(MAX_WORKERS-1)],
              workerName[(MAX_WORKERS-1)],
              "server",
              "",         // inFile
              "",         // outFile
              ((tracing)?2:1), ((tracing)?server5ArgsTrace:server5ArgsNoTrace),
              workerNid[(MAX_WORKERS-1)], workerPid[(MAX_WORKERS-1)], 
              workerVerifier[(MAX_WORKERS-1)], procName);

        if ( testSuccess )
        {
            if ( workerNowait[(MAX_WORKERS-1)] == false )
            {
                procList[procListCount]
                    = new WorkerProcess ( workerName[(MAX_WORKERS-1)]
                                        , workerNid[(MAX_WORKERS-1)]
                                        , workerPid[(MAX_WORKERS-1)]
                                        , workerVerifier[(MAX_WORKERS-1)] );
                ++procListCount;

                if ( tracing )
                    printf ( "[%s] Worker #%d: %s (%d, %d:%d)\n", MyName
                           , procListCount-1
                           , workerName[(procListCount-1)]
                           , workerNid[(procListCount-1)]
                           , workerPid[(procListCount-1)]
                           , workerVerifier[(procListCount-1)] );
            }
        }

        // Wait for a while so process will be created
        sleep(1);

        util.requestShutdown ( ShutdownLevel_Normal );

        // Wait for a while so can receive notices
        sleep(3);

        // Verify that got all notices and that processes were created
        // on the correct nodes
        bool found;
        for (int i = 0; i < MAX_WORKERS; i++)
        {
            found = false;
            for (int j=0; j<procListCount; j++)
            {
                if ( strcmp(workerName[i], procList[j]->getName()) ==0)
                {
                    found = true;

                    if (workerCreateNid[i] != workerNid[j])
                    {
                        printf("[%s] *** ERROR *** For worker %s requested "
                               "process creation on node %d but it was created "
                               "on node %d.\n",
                               MyName, workerName[i], workerCreateNid[i],
                               workerNid[j]);
                        testSuccess = false;
                    }

                    if ( workerNowait[i]
                         && procList[j]->getCreatedNoticeCount() == 0)
                    {
                        printf("[%s] *** ERROR *** For worker %s did not get "
                               "process creation notice as expected.\n",
                               MyName, workerName[i]);
                        testSuccess = false;
                    }

                    if ( procList[j]->getDeathNoticeCount() == 0 )
                    {
                        printf("[%s] *** ERROR *** For worker %s did not get "
                               "process death notice as expected.\n",
                               MyName, workerName[i]);
                        testSuccess = false;
                    }
                }
            }
            if ( !found )
            {
                printf("[%s] *** ERROR *** Worker process %s not created "
                       "as expected.\n",
                       MyName, workerName[i]);
                testSuccess = false;
            }
        }

        printf("Process Creation Test:\t\t%s\n",
           (testSuccess) ? "PASSED" : "FAILED");
    }
    
    if ( util.getNodedownBeforeStartup() )
    {
        printf("[%s] NodedownBeforeStartup Test\n", MyName);
        
        char *serverArgsNodeDownNotrace[1] = {(char *) "-y"};
        char *serverArgsNodeDown2[2] = {(char *) "-t", (char *) "-y"};
            
        workerCreateNid[0] = otherNid;
        workerCreateNid[1] = otherNid;
        workerNowait[0]  = true;
        workerNowait[1]  = false;
        int maxworkers = 2;
        procListCount = 0;
        
        // Create all processes except the 5th process
        for (int i = 0; i < maxworkers; i++)
        {
            printf("[%s] Starting process %s on node %d, %s\n",
                   MyName, workerName[i], workerCreateNid[i],
                   (workerNowait[i] ? "nowait" : "wait"));

            testSuccess = util.requestNewProcess( workerCreateNid[i]
                                                , ProcessType_Generic
                                                , workerNowait[i]
                                                , workerName[i]
                                                , "server"
                                                , ""      // inFile
                                                , ""      // outFile
                                                , ((tracing) ? 2: 1)  //argument count
                                                , (tracing)?serverArgsNodeDown2:serverArgsNodeDownNotrace
                                                , workerNid[i]
                                                , workerPid[i]
                                                , workerVerifier[i]
                                                , procName);

            if ( testSuccess )
            {
                if ( workerNowait[i] == false )
                {
                    procList[procListCount] = new WorkerProcess( workerName[i]
                                                               , workerNid[i]
                                                               , workerPid[i]
                                                               , workerVerifier[i] );
                    ++procListCount;
                    
                    if ( tracing )
                        printf ("[%s] Worker #%d: %s (%d, %d:%d)\n", MyName,
                                procListCount-1, 
                                workerName[i], workerNid[i], 
                                workerPid[i], workerVerifier[i] );
                }
            }
        }
       
        //sleep for x sec for external script to issue node down.
        // Wait for a while so can receive notices
        sleep(5); 

        // Verify that we got all notices 
        bool found;
        for (int i = 0; i < maxworkers; i++)
        {
            found = false;
            for (int j=0; j<procListCount; j++)
            {
                if ( strcmp(workerName[i], procList[j]->getName()) ==0)
                {
                    found = true;

                    if (workerCreateNid[i] != workerNid[j])
                    {
                        printf("[%s] *** ERROR *** For worker %s requested "
                               "process creation on node %d but it was created "
                               "on node %d.\n",
                               MyName, workerName[i], workerCreateNid[i],
                               workerNid[j]);
                        testSuccess = false;
                    }

                    if ( workerNowait[i] 
                         && procList[j]->getCreatedNoticeCount() == 0)
                    {
                        printf("[%s] *** ERROR *** For worker %s did not get "
                               "process creation notice as expected.\n",
                               MyName, workerName[i]);
                        testSuccess = false;
                    }

                    if ( procList[j]->getDeathNoticeCount() == 0 )
                    {
                        printf("[%s] *** ERROR *** For worker %s did not get "
                               "process death notice as expected.\n",
                               MyName, workerName[i]);
                        testSuccess = false;
                    }
                }
            }
            if ( !found )
            {
                printf("[%s] Worker process %s not created "
                       "as expected.\n",
                       MyName, workerName[i]);
                testSuccess = true; //not finding it is expected here.
            }
        }
        printf("Node Down Before Startup Test :\t%s\n",
           (testSuccess) ? "PASSED" : "FAILED");
    }

    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
