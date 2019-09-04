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
bool shutdownBeforeStartup = false;
bool shutdownSent = false;

// Server message tags
#define USER_TAG 100

// Server process commands
#define CMD_CONT  1
#define CMD_END   2
#define CMD_ABORT 3

// global variables
int MyRank = -1;
const char *MyName;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
MPI_Comm worker_comm;

// Routine for handling notices
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_Shutdown )
    {
        if ( tracing )
            printf("[%s] Shutdown notice received, level=%d\n",
                   MyName, recv_msg->u.request.u.shutdown.level);
        shutdownSent = true;
    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}

void server ()
{
    int j = 0,
        rc;
    int recvbuf[3];
    char sendbuf[100];
    bool done = false;
    MPI_Status status;

    do
    {
        j++;
        if ( tracing )
        {
            printf("[%s] waiting for message.\n",MyName);
        }
        rc = XMPI_Recv (recvbuf, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       worker_comm, &status);
        if (rc == MPI_SUCCESS)
        {
            switch (recvbuf[2])
            {
            case CMD_CONT:
                sprintf (sendbuf, "[%s] Received (%d:%d) CMD_CONT", MyName,
                         recvbuf[0], recvbuf[1]);
                break;
            case CMD_END:
                sprintf (sendbuf, "[%s] Received (%d:%d) CMD_END", MyName,
                         recvbuf[0], recvbuf[1]);
                done = true;
                break;
            case CMD_ABORT:
                sprintf (sendbuf, "[%s] Received (%d:%d) CMD_ABORT", MyName,
                         recvbuf[0], recvbuf[1]);
                if ( tracing )
                {
                    printf ("[%s] Received CMD_ABORT, exiting\n", MyName);
                }
                exit (1);
            default:
                sprintf (sendbuf, "[%s] Received (%d:%d) UNKNOWN", MyName,
                         recvbuf[0], recvbuf[1]);
            }
            if ( tracing )
            {
                printf("[%s] message received <%s>.\n", MyName, sendbuf);
            }
            rc = XMPI_Send (sendbuf, (int) strlen (sendbuf) + 1, MPI_CHAR, 0,
                           USER_TAG, worker_comm);
        }
        if (rc != MPI_SUCCESS)
        {
            printf ("[%s] try %d failed with rc = (%d)%s\n", MyName, j, rc,
                    util.MPIErrMsg(rc));
            util.closeProcess ( worker_comm );
            printf ("[%s] Waiting to Accept connection.\n", MyName);
            XMPI_Comm_accept (util.getPort(), MPI_INFO_NULL, 0, MPI_COMM_SELF,
                             &worker_comm);
            XMPI_Comm_set_errhandler (worker_comm, MPI_ERRORS_RETURN);
            printf ("[%s] Reconnected.\n", MyName);
        }
        else
        {
            if ( tracing )
            {
                printf("[%s] reply sent.\n",MyName);
            }
        }
    }
    while (!done);
}


int main (int argc, char *argv[])
{

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    if (util.getShutdownBeforeStartup())
    {
        sleep(2);  // wait for shutdown to be sent
        if ( tracing )
        {
            printf ("[%s] Sending startup message\n", MyName);
        }
        util.requestStartup ();

        gp_local_mon_io->set_cb(recv_notice_msg, "notice");
        do
        {
            printf ("[%s] Waiting for shutdown message.\n", MyName);
            fflush (stdout);
            sleep(1);
        }
        while (!shutdownSent);

        // tell monitor we are exiting
        util.requestExit ( );

        if ( tracing )
        {
            printf ("[%s] exiting.\n", MyName);
        }
        if (gp_local_mon_io)
        {
            delete gp_local_mon_io;
        }
        exit (0);
    }
    else if ( util.getNodedownBeforeStartup() )
    {
		if ( tracing )
        {
           printf ("[%s] nodedownBeforeStartup mode, going to sleep\n", MyName);
           fflush (stdout);
        } 
			
		sleep(8);
        
        // tell monitor we are exiting
		util.requestExit ( );
		
        // now exit
		if ( tracing )
		{
		  printf ("[%s] exiting.\n", MyName);
		}
		if (gp_local_mon_io)
		{
			   delete gp_local_mon_io;
		}
		exit (0);
    }
    else
    {
        if ( tracing )
        {
            printf ("[%s] Shutdown or Nodedown before startup not set\n", MyName);
        }
    }

    util.requestStartup ();

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

    /*
      // need to modify this so that we look for "waitforclose" or some other argument that means we should not process requests.
    if (argc==7 || argc==9)
    {
    */
        if ( tracing )
        {
            printf("[%s] Ready to process client requests\n",MyName);
        }
        server ();
        /*
    }
    else
    {
        printf("[%s] Not processing client requests\n",MyName);
        sleep(10);
    }
    */

    if ( tracing )
    {
        printf ("[%s] disconnecting.\n", MyName);
    }
    util.closeProcess ( worker_comm );

    XMPI_Close_port (util.getPort());

    // need to decide if any test requires the server to exit abnormally.
    // if so probably would be best to have a server argument that specifies
    // that.
    /*
    if ((strcmp("$SERV2",MyName)==0) ||
        (strcmp("$SERV3",MyName)==0)   )
    {
    */
        // tell monitor we are exiting
        util.requestExit ( );
    /*
    }
    */

    if ( tracing )
    {
        printf ("[%s] exiting.\n", MyName);
    }

    if (gp_local_mon_io)
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
