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

// deathUnreg: does process startup, get commands from controller process
// to register interest in death of processes, and exits upon command.

// Used to exercise monitor's ability to register death notice interest and
// then unregister when requesting process exits.

#include <stdio.h>
#include <stdlib.h>
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
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0};


void processCommands()
{
    MPI_Comm worker_comm;
    int servNid;
    int servPid;
    Verifier_t servVerifier;
    int rc;
    MPI_Status status;
    int recvbuf[6];
    char sendbuf[100];
    bool done = false;
    const int serverTag = 100;
    char pName[12];

    if ( tracing )
    {
        printf ("[%s] Port: %s\n", MyName, util.getPort());
    }

    if ( tracing )
    {
        printf ("[%s] Wait to connect.\n", MyName);
    }

    XMPI_Comm_accept (util.getPort(), MPI_INFO_NULL, 0, MPI_COMM_SELF,
                     &worker_comm);
    XMPI_Comm_set_errhandler (worker_comm, MPI_ERRORS_RETURN);

    if ( tracing )
    {
        printf ("[%s] Connected.\n", MyName);
    }

    do
    {
        rc = XMPI_Recv (recvbuf, 6, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       worker_comm, &status);

        if (rc == MPI_SUCCESS)
        {
            switch (recvbuf[0])
            {
            case CMD_REG_INTEREST:
                strcpy(pName, (char *)&recvbuf[1]);
                if ( tracing )
                {
                    printf("[%s] got command CMD_REG_INTEREST for %s.\n",MyName,
                           pName);
                }

                if (util.requestProcInfo ( pName, servNid, servPid, servVerifier) )
                {
                    _TM_Txid_External transid = {{0LL, 0LL, 0LL, 0LL}};

                    util.requestNotice(servNid, servPid, servVerifier, pName, false, transid);
                }
                strcpy(sendbuf, "OK");
                break;

            case CMD_END:
                if ( tracing )
                {
                    printf("[%s] got command CMD_END.\n",MyName);
                }
                strcpy(sendbuf, "OK");
                done = true;
                break;
            default:
               sprintf (sendbuf, "[%s] Received (%d:%d) UNKNOWN", MyName,
                        recvbuf[0], recvbuf[1]);
            }
            rc = XMPI_Send (sendbuf, (int) strlen (sendbuf) + 1, MPI_CHAR, 0,
                           serverTag, worker_comm);
        }
        else
        {  // Receive failed
            printf("[%s] XMPI_Recv failed, rc = (%d) %s\n",
                   MyName, rc, util.MPIErrMsg(rc));
            done = true;
        }

    }
    while (!done);

    if ( tracing )
    {
        printf ("[%s] disconnecting.\n", MyName);
    }
    util.closeProcess ( worker_comm );
}

int main (int argc, char *argv[])
{

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    util.requestStartup ();

    // Get and execute commands from controller process
    processCommands();

    // tell monitor we are exiting
    util.requestExit ( );

    fflush (stdout);
    XMPI_Close_port( util.getPort() );
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
