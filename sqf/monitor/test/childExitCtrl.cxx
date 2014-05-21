// controller:
//    create process A on node 0
//    after children created
//    kill process A
//    verify that process A killed and children killed

    // get process status, verify the parent was created ok
    // (no need to register death notification since we are parent)
    // done: childExitParent sends us a message for each process created
    // done: register for each child death notification
    // done: kill process
    // done: verify death notification for childExitParent
    // done: verify death notification for each child process

// testing:
//  test failure in parent process creation
//  test other failure scenarios, make sure test reports failure


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
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect


class CreatedProcess
{
public:
    CreatedProcess(const char * name, int nid, int pid);
    ~CreatedProcess(){}
    const char * getName() { return name_.c_str(); }
    int getNid() { return nid_; }
    int getPid() { return pid_; }
    void setDeathNotice() { deathNotice_ = true; }
    bool getDeathNotice() { return deathNotice_; }

private:
    string name_;
    int nid_;
    int pid_;
    bool deathNotice_;
};

CreatedProcess::CreatedProcess (const char * name, int nid, int pid)
    : name_(name), nid_(nid), pid_(pid), deathNotice_ (false)
{
}

CreatedProcess* childDeathParent;
CreatedProcess* procList[10];
int procListCount = 0;
int deathNoticeCount = 0;


// Routine for handling notices:
//   NodeDown, NodeUp, ProcessDeath, Shutdown, TmSyncAbort, TmSyncCommit
void recv_notice_msg(struct message_def *recv_msg, int )
{
    if ( recv_msg->type == MsgType_ProcessDeath )
    {
        printf("[%s] Process death notice received for (%d, %d)\n", 
               MyName,
               recv_msg->u.request.u.death.nid,
               recv_msg->u.request.u.death.pid);

        bool found = false;
        for (int i=0; i<procListCount; i++)
        {
            if (procList[i]->getNid() == recv_msg->u.request.u.death.nid
             && procList[i]->getPid() == recv_msg->u.request.u.death.pid)
            {
                procList[i]->setDeathNotice();
                ++deathNoticeCount;
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

// Get messages from "childExitParent" which give the process
// names of child processes it has created.
bool getProcesses ( )
{
    MPI_Comm CtrlComm = MPI_COMM_NULL;
    int rc;
    char recvbuf[25];
    MPI_Status status;
    bool done = false;
    int nid;
    int pid;
    _TM_Txid_External transid = {{0LL, 0LL, 0LL, 0LL}};

    rc = MPI_Comm_accept( util.getPort(), MPI_INFO_NULL, 0, MPI_COMM_SELF,
                          &CtrlComm);
    if (rc == MPI_SUCCESS)
    {
        MPI_Comm_set_errhandler (CtrlComm, MPI_ERRORS_RETURN);
        if ( tracing )
            printf ("[%s] Connected to childExitParent\n", MyName);
    }
    else
    {
        printf ("[%s] MPI_Comm_accept failed, rc=%d\n", MyName, rc);
        return false;
    }

    do
    {
        rc = MPI_Recv (recvbuf, 25, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG,
                       CtrlComm, &status);
        if (rc == MPI_SUCCESS)
        {
            if ( strcmp("FINIS", recvbuf) == 0 )
            {
                done = true;
            }
            else
            {   // Got a process name, keep track of it
                for (int i=0; i<3; i++)
                {
                    if (util.requestProcInfo ( recvbuf, nid, pid ))
                    {
                        printf ("[%s] registering process %s (%d, %d)\n",
                                MyName, recvbuf, nid, pid);

                        procList[procListCount]
                            = new CreatedProcess ( recvbuf, nid, pid );
                        ++procListCount;
                        util.requestNotice ( nid, pid, false, transid );
                        break;
                    }
                    // Allow more time for process startup
                    sleep(1);
                }
            }
        }
        else
        {
            printf ("[%s] MPI_Recv failed, rc=%d\n", MyName, rc);
        }

    }
    while (!done);

    return true;
}

void cleanup ()
{
    // tell monitor we are exiting
    util.requestExit ( );

    if ( tracing ) printf ("[%s] calling Finalize!\n", MyName);

    MPI_Close_port( util.getPort() );
    MPI_Finalize ();
    if ( gp_local_mon_io )
    {
        delete gp_local_mon_io;
    }
}

int main (int argc, char *argv[])
{
    bool testSuccess = true;
    int MyRank = -1;

    if ( tracing ) util.setTrace (tracing);

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyRank);

    util.processArgs (argc, argv);
    MyName = util.getProcName();

    util.InitLocalIO( );
    assert (gp_local_mon_io);

    // Set local io callback function for "notices"
    gp_local_mon_io->set_cb(recv_notice_msg, "notice");

    util.requestStartup ();

    // Start process "childExitParent" that will start several child processes
    int nid;
    int pid;
    char *childArgs[0];
    char procName[25];
    childDeathParent = NULL;
    if (util.requestNewProcess (1, ProcessType_Generic, false,
                                (char *) "$PROCA", "childExitParent", "", "", 
                                0, childArgs, nid, pid, procName))
    {
        procList[0] = new CreatedProcess ( "$PROCA", nid, pid );
        ++procListCount;
    }
    else
    {
        printf ("[%s] error starting childExitParent process\n", MyName);
        printf ("[%s] test failed\n", MyName);
        
        printf("[%s] Test FAILED\n", MyName);

        cleanup();
        return 1;
    }

    // Get created process info from "childExitParent"
    if (!getProcesses ( ))
    {
        printf ("[%s] could not get process info from childExitParent\n",
                MyName);

        printf("[%s] Test FAILED\n", MyName);

        cleanup();
        return 1;
    }

    // Kill "childExitParent"
    util.requestKill ( "$PROCA" );

    // Wait until all death notices received or time-out
    for (int i=0; i<5; i++)
    {
        if (deathNoticeCount == procListCount) break;
        sleep(1);
    }

    // Verify that got all death notices
    for (int i=0; i<procListCount; i++)
    {
        if (!procList[i]->getDeathNotice())
        {
            printf("[%s] No death notice received for process %s (%d, %d)\n",
                   MyName, procList[i]->getName(), procList[i]->getNid(),
                   procList[i]->getPid());

            testSuccess = false;
        }
    }

    printf("Child Exit Test:\t\t%s\n", (testSuccess) ? "PASSED" : "FAILED");

    cleanup();
    exit (0);
}
