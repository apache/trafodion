///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

// childExitParent:
//    create several child processes then exit abnormally

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;
bool tracing = false;

const char *MyName;

int MyRank = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect

MPI_Comm CtrlComm = MPI_COMM_NULL;

bool openCtrl ()
{
    char   ctrlPort[MPI_MAX_PORT_NAME];
    bool result = false;

    if (util.requestOpen ( "$CTRLR", 0, ctrlPort ))
    {
        if ( tracing )
            printf ("[%s] opened $CTRLR port=%s\n", MyName, ctrlPort);

        int rc = MPI_Comm_connect (ctrlPort, MPI_INFO_NULL,
                                   0, MPI_COMM_SELF, &CtrlComm);
        if (rc == MPI_SUCCESS)
        {
            MPI_Comm_set_errhandler (CtrlComm, MPI_ERRORS_RETURN);
            if ( tracing )
                printf ("[%s] connected to process.\n", MyName);
            result = true;
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

void sendToCtrl( const char * sendbuf )
{
    int rc;

    if ( tracing )
        printf("[%s] sending to CTRLR: %s\n", MyName, sendbuf );

    rc = MPI_Send ((void *) sendbuf, (int) strlen (sendbuf) + 1, MPI_CHAR, 0,
                   100 /* USER_TAG */, CtrlComm);
    if (rc != MPI_SUCCESS)
    {
        printf ("[%s] MPI_Send failed. rc = (%d)\n", MyName, rc);
    }
}

int main (int argc, char *argv[])
{
    int MyRank = -1;

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);

    util.processArgs (argc, argv);
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    util.requestStartup ();

    // Open control process
    if (!openCtrl())
    {
        printf ("[%s] FAILED opened $CTRLR\n", MyName);
        exit(0);
    }

    int nid;
    int pid;
    char *childArgs[0];
    char procName[25];

    util.requestNewProcess (0, ProcessType_Generic, false, (char *) "$A01",
                            "childExitChild", "", "", 0, childArgs,
                            nid, pid, procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A11",
                            "childExitChild", "", "", 0, childArgs,
                            nid, pid, procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A12",
                            "childExitChild", "", "", 0, childArgs,
                            nid, pid, procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A13",
                            "childExitChild", "", "", 0, childArgs,
                            nid, pid, procName);

    sendToCtrl( "$A01" );
    sendToCtrl( "$A11" );
    sendToCtrl( "$A12" );
    sendToCtrl( "$A13" );
    sendToCtrl( "FINIS" );

    //    start_process (2, (char *) "$A01");
    // start_process (2, (char *) "$A01");

    // temp print
    printf ("[%s] children created, ready to be killed ...\n", MyName);

    for (int i=0; i<7; i++)
    {
        // temp print
        printf ("[%s] delaying...\n", MyName);

        sleep(1);
    }

    // exit my process
    util.requestExit ( );

    printf ("[%s] calling Finalize!\n", MyName);
    MPI_Close_port( util.getPort() );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
