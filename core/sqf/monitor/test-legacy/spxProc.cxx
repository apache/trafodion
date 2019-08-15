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
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
SB_Verif_Type  gv_ms_su_verif = -1;
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0};

int deathNoticesReceived = 0;


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

        ++deathNoticesReceived;

    }
    else
    {
        printf("[%s] unexpected notice, type=%s\n", MyName,
               MessageTypeString( recv_msg->type));
    }
}


void processCommands()
{
    MPI_Comm ctrlComm;
    int rc;
    MPI_Status status;
    int recvbuf[6];
    char sendbuf[100];
    bool done = false;
    const int serverTag = 100;

    if ( tracing )
    {
        printf ("[%s] Port: %s\n", MyName, util.getPort());
    }

    if ( tracing )
    {
        printf ("[%s] Wait to connect.\n", MyName);
    }

    XMPI_Comm_accept (util.getPort(), MPI_INFO_NULL, 0, MPI_COMM_SELF,
                     &ctrlComm);
    XMPI_Comm_set_errhandler (ctrlComm, MPI_ERRORS_RETURN);

    if ( tracing )
    {
        printf ("[%s] Connected.\n", MyName);
    }

    replyMsg_t replyMsg;

    do
    {
        rc = XMPI_Recv (recvbuf, 6, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       ctrlComm, &status);

        if (rc == MPI_SUCCESS)
        {
            switch (recvbuf[0])
            {
            case CMD_GET_STATUS:
                if ( tracing )
                {
                    printf("[%s] got command CMD_GET_STATUS.\n",MyName);
                }

                replyMsg.noticeCount = deathNoticesReceived;

               break;

            case CMD_END:
                if ( tracing )
                {
                    printf("[%s] got command CMD_END.\n",MyName);
                }

                done = true;
                break;
            default:
               sprintf (sendbuf, "[%s] Received (%d:%d) UNKNOWN", MyName,
                        recvbuf[0], recvbuf[1]);
            }

            rc = XMPI_Send (&replyMsg, (int) sizeof(replyMsg_t), MPI_CHAR, 0,
                           serverTag, ctrlComm);

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
    util.closeProcess ( ctrlComm );
}


int main (int argc, char *argv[])
{

    util.processArgs (argc, argv);
    tracing = util.getTrace();
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    util.requestStartup ();

    // Get and execute commands from controller process
    processCommands();

    // tell monitor we are exiting
    util.requestExit ( );

    printf ("[%s] calling Finalize!\n", MyName);
    fflush (stdout);
    XMPI_Close_port( util.getPort() );
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
    exit (0);
}
