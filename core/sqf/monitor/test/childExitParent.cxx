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

// childExitParent:
//    create several child processes then exit abnormally

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

MPI_Comm CtrlComm = MPI_COMM_NULL;

bool openCtrl ()
{
    bool result = false;
    if (util.openProcess( "$CTRLR", -1, 0, CtrlComm ))
    {
        if ( tracing )
            printf ("[%s] connected to process $CTRLR\n", MyName);
        result = true;
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

    rc = XMPI_Send ((void *) sendbuf, (int) strlen (sendbuf) + 1, MPI_CHAR, 0,
                   100 /* USER_TAG */, CtrlComm);
    if (rc != MPI_SUCCESS)
    {
        printf ("[%s] XMPI_Send failed. rc = (%d)\n", MyName, rc);
    }
}

int main (int argc, char *argv[])
{

    util.processArgs (argc, argv);
    MyName = util.getProcName();
    tracing = util.getTrace();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    util.requestStartup ();

#if 0
    sleep( 20 );
#endif    
    // Open control process
    if (!openCtrl())
    {
        printf ("[%s] FAILED open to $CTRLR\n", MyName);
        exit(0);
    }

    int nid;
    int pid;
    Verifier_t verifier;
    char *childArgs[1] = {(char *) "-t"};
    char procName[25];

    util.requestNewProcess ( 0      // target nid
                           , ProcessType_Generic
                           , false
                           , (char *) "$A01"
                           , "childExitChild"
                           , ""     // inFile
                           , ""     // outFile
                           , ((tracing) ? 1: 0)
                           , childArgs
                           , nid
                           , pid
                           , verifier
                           , procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A11",
                            "childExitChild", "", "", ((tracing) ? 1: 0), childArgs,
                           
                            nid, pid, verifier, procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A12",
                            "childExitChild", "", "", ((tracing) ? 1: 0), childArgs,
                           
                            nid, pid, verifier, procName);
    util.requestNewProcess (1, ProcessType_Generic, false, (char *) "$A13",
                            "childExitChild", "", "", ((tracing) ? 1: 0), childArgs,
                           
                            nid, pid, verifier, procName);

    sendToCtrl( "$A01" );
    sendToCtrl( "$A11" );
    sendToCtrl( "$A12" );
    sendToCtrl( "$A13" );
    sendToCtrl( "FINIS" );

    printf ("[%s] children created, ready to be killed ...\n", MyName);

    for (int i=0; i<7; i++)
//    while( 1 )
    {
        // temp print
        printf ("[%s] delaying...\n", MyName);

        sleep(1);
    }
                
    // exit my process
    util.requestExit ( );

    printf ("[%s] calling Finalize!\n", MyName);
    XMPI_Close_port( util.getPort() );
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
