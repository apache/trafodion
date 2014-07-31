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

// childExitChild: does process startup then delays until killed.

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "clio.h"
#include "sqevlog/evl_sqlog_writer.h"
#include "montestutil.h"

MonTestUtil util;

long trace_settings = 0;
FILE *shell_locio_trace_file = NULL;

const char *MyName;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0};


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

    for (int i=0; i<10; i++)
    {
        printf ("[%s] delaying...\n", MyName);

        sleep(1);
    }

    // tell monitor we are exiting
    util.requestExit ( );

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    MPI_Close_port( util.getPort() );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
