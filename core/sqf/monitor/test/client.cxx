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
#include <unistd.h>

#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"
#include "xmpi.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

// Server message tags
#define USER_TAG 100

// Server process commands
#define CMD_CONT 1
#define CMD_END  2
#define CMD_ABORT 3

const char *MyName;
int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

const int MAX_WORKERS = 4;
int deathNoticeCount = 0;
int workerNid[MAX_WORKERS];
int workerPid[MAX_WORKERS];
int workerVerifier[MAX_WORKERS];
bool workerDeathNotice[] = { false, false, false, false };

bool testSuccess = true;

void connectServer (char *port, MPI_Comm * comm)
{
    int rc;

    rc = XMPI_Comm_connect (port, MPI_INFO_NULL, 0, MPI_COMM_SELF, comm);
    if (rc == MPI_SUCCESS)
    {
        XMPI_Comm_set_errhandler (*comm, MPI_ERRORS_RETURN);
    }
    else
    {
        printf ("[%s] failed to connect to %s. rc = (%d)\n", MyName, port, rc);
        abort();
    }
}

// Routine for handling notices
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_ProcessDeath )
    {
        if ( tracing )
        {
            printf("[%s] Process death notice received for %s (%d, %d:%d)\n", 
                   MyName,
                   recv_msg->u.request.u.death.process_name,
                   recv_msg->u.request.u.death.nid,
                   recv_msg->u.request.u.death.pid,
                   recv_msg->u.request.u.death.verifier);
        }

        bool found = false;
        for (int i=0; i<MAX_WORKERS; i++)
        {
            if (workerNid[i] == recv_msg->u.request.u.death.nid
             && workerPid[i] == recv_msg->u.request.u.death.pid
             && workerVerifier[i] == recv_msg->u.request.u.death.verifier)
            {
                workerDeathNotice[i] = true;
                ++deathNoticeCount;
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("[%s] Could not find worker for (%d, %d:%d)\n", 
                   MyName, 
                   recv_msg->u.request.u.death.nid,
                   recv_msg->u.request.u.death.pid,
                   recv_msg->u.request.u.death.verifier);
        }
    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
        testSuccess = false;
    }
}


int main (int argc, char *argv[])
{
    const int MAX_CYCLES = 60;

    int i;
    int j;
    int rc;
    bool workerUp[MAX_WORKERS];
    int sendbuf[3];
    char recvbuf[100];
    MPI_Comm manager_comm[MAX_WORKERS];
    MPI_Status status;
    char workerPort[MAX_WORKERS][MPI_MAX_PORT_NAME];
    bool workerStartProcess[] = {false, false, true, true};
    const char *workerName[] = {"$SERV0","$SERV1", "$SERV2", "$SERV3"};
    int workerMsgCount[MAX_WORKERS];
    char *serverArgs[1] = {(char *) "-t"};
    int workerChildren = 0;
    char procName[25];

    for (i = 0; i < MAX_WORKERS; i++)
    {
        workerVerifier[i] = -1;
    }
    
    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    util.requestStartup ();

    sleep (2);
    for (i = 0; i < MAX_WORKERS; i++)
    {
        manager_comm[i] = MPI_COMM_NULL;
        workerUp[i] = false;
        workerMsgCount[i] = 0;

        if ( workerStartProcess[i] )
        {
            if ( tracing ) printf ("[%s] util.requestNewProcess(workerName[%d]=%s)\n", MyName, i, workerName[i]);
            fflush (stdout);
            util.requestNewProcess( 1
                                  , ProcessType_Generic
                                  , false
                                  , workerName[i]
                                  , "server"
                                  , ""
                                  , ""
                                  , ((tracing) ? 1: 0)
                                  , serverArgs
                                  , workerNid[i]
                                  , workerPid[i]
                                  , workerVerifier[i]
                                  , procName);
            ++workerChildren;
        }

        if ( tracing ) printf ("[%s] util.requestOpen(workerName[%d]=%s)\n", MyName, i, workerName[i]);
        fflush (stdout);
        if (util.requestOpen( workerName[i]
                            , workerVerifier[i]
                            , 0
                            , workerPort[i] ))
        {
            if ( tracing ) printf ("[%s] connectServer(workerName[%d]=%s)\n", MyName, i, workerName[i]);
            fflush (stdout);
            connectServer (workerPort[i], &manager_comm[i]);
        }

        if (manager_comm[i] != MPI_COMM_NULL)
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
        fflush (stdout);
    }

    // do the work
    if ( tracing )
    {
        printf ("[%s] Starting work with %d workers\n", MyName, MAX_WORKERS);
    }
    for (j = 0; j < MAX_CYCLES + 1; j++)
    {
        for (i = 0; i < MAX_WORKERS; i++)
        {
            if (workerUp[i])
            {
                sendbuf[0] = MyRank;
                sendbuf[1] = i;
                switch (j)
                {
                case 1:
                    sendbuf[2] = CMD_CONT;
                    break;
                case MAX_CYCLES:
                    sendbuf[2] = CMD_END;
                    break;
                default:
                    sendbuf[2] = CMD_CONT;
                }
                rc = XMPI_Sendrecv (sendbuf, 3, MPI_INT, 0, USER_TAG,
                                   recvbuf, 100, MPI_CHAR, MPI_ANY_SOURCE,
                                   USER_TAG, manager_comm[i], &status);
                if (rc != MPI_SUCCESS)
                {
                    printf
                        ("[%s] SendRecv failed to worker %d, rc = (%d)%s\n",
                         MyName, i, rc, util.MPIErrMsg(rc));

                    util.closeProcess( manager_comm[i] );
                    sleep (5);

                    if (util.requestOpen( workerName[i]
                                        , workerVerifier[i]
                                        , 0
                                        , workerPort[i] ))
                    {
                        connectServer (workerPort[i], &manager_comm[i]);
                    }
                    testSuccess = false;
                }
                else
                {
                    ++workerMsgCount[i];
                }
            }
            else
            {
                printf ("[%s] Not sending to worker %d\n", MyName, i);
            }
        }
    }

    // close the server processes
    for (i = 0; i < MAX_WORKERS; i++)
    {
        if (workerUp[i])
        {
            util.closeProcess ( manager_comm[i] );
        }

        if ( tracing )
        {
            printf("[%s] %d messages sent/received for worker %d\n", MyName,
                   workerMsgCount[i], i);
        }
        if ( workerMsgCount[i] != ( MAX_CYCLES + 1 ))
        {
            printf ("[%s] for worker %d, expecting %d messages but got %d\n",
                    MyName, i, ( MAX_CYCLES + 1 ), workerMsgCount[i]);
            testSuccess = false;            
        }
    }

    // Wait until all death notices received or time-out
    for (int i=0; i<5; i++)
    {
        if (deathNoticeCount == workerChildren) break;
        sleep(1);
    }

    // Verify that got all death notices
    for (int i=0; i<MAX_WORKERS; i++)
    {
        if (workerStartProcess[i] && !workerDeathNotice[i])
        {
            printf("[%s] No death notice received for worker %d, "
                   " %s (%d, %d)\n",
                   MyName, i, workerName[i], workerNid[i], workerPid[i]);

            testSuccess = false;
        }
    }

    printf("Multi-Node Test:\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");


    // tell monitor we are exiting
    util.requestExit ( );

    XMPI_Close_port (util.getPort());
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
