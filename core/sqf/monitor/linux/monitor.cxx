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

#include <iostream>
#include <map>

using namespace std;

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/resource.h> //add for getrlimit, strange centos6/gcc4.4 don't need this

//#include <sys/stat.h>
#include "monlogging.h"
#include "monprof.h"
#include "monsonar.h"
#include "montrace.h"
#include "redirector.h"
#include "healthcheck.h"
#include "config.h"
#include "device.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "tmsync.h"
#include "cluster.h"
#include "monitor.h"
#include "props.h"

#ifdef DMALLOC
#include "dm.h"
#endif
#include "replicate.h"
#include "robsem.h"
#include "commaccept.h"

#include <assert.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sched.h>
#include "localio.h"
#include "lock.h"
#include "mlio.h"
#include "redirector.h"
#include "intprocess.h"

#include "reqqueue.h"
#include "reqworker.h"
#include "zclient.h"

#include "SCMVersHelp.h"

#define LOG_ERROR
#define RidRecvPrioritySlot 4096
#define MinimumSlotToPrioritize 4096


// Global Variables
struct rlimit Rl;
bool PidMap=false;
bool usingCpuAffinity=false;
bool usingTseCpuAffinity=false;
bool genSnmpTrapEnabled = false;
int Measure=0;
long trace_level = 0;
char MyPath[MAX_PROCESS_PATH];
char MyCommPort[MPI_MAX_PORT_NAME] = {'\0'};
char MyMPICommPort[MPI_MAX_PORT_NAME] = {'\0'};
char MySyncPort[MPI_MAX_PORT_NAME] = {'\0'};
char Node_name[MPI_MAX_PROCESSOR_NAME] = {'\0'};
sigset_t SigSet;
bool Emulate_Down = false;
long next_test_delay = 100000; // in usec. (default 100 msec)
CClusterConfig *ClusterConfig = NULL;
bool IAmIntegrating = false;
bool IAmIntegrated = false;
char IntegratingMonitorPort[MPI_MAX_PORT_NAME] = {'\0'};
bool IsRealCluster = true;
bool IsAgentMode = false;
bool IsMaster = false;
bool IsMPIChild = false;
char MasterMonitorName[MAX_PROCESS_PATH]= {'\0'};
CommType_t CommType = CommType_Undefined;
bool SMSIntegrating = false;
int  CreatorShellPid = -1;
Verifier_t CreatorShellVerifier = -1;
bool SpareNodeColdStandby = true;
bool ZClientEnabled = true;

// Lock to manage memory modifications during fork/exec
CLock MemModLock;
CMonitor *Monitor = NULL;
CNodeContainer *Nodes = NULL;
CConfigContainer *Config = NULL;
CDeviceContainer *Devices = NULL;
int MyPNID = -1;
CNode *MyNode;
CMonLog *MonLog =  NULL;
CMonStats * MonStats = NULL;
extern CMonTrace *MonTrace;
CRedirector Redirector;
CIntProcess IntProcess;
CReqQueue ReqQueue;
CHealthCheck HealthCheck;
CCommAccept CommAccept;
extern CReplicate Replicator;
CZClient  *ZClient = NULL;
// Seabed disconnect semaphore
RobSem * sbDiscSem = NULL;



DEFINE_EXTERN_COMP_DOVERS(monitor)
DEFINE_EXTERN_COMP_GETVERS2(monitor)


_TM_Txid_External invalid_trans( void )
{
    _TM_Txid_External trans1;
    
    trans1.txid[0] = -1LL;
    trans1.txid[1] = -1LL;
    trans1.txid[2] = -1LL;
    trans1.txid[3] = -1LL;

    return trans1;
}

_TM_Txid_External null_trans( void )
{
    _TM_Txid_External trans1;
    
    trans1.txid[0] = 0LL;
    trans1.txid[1] = 0LL;
    trans1.txid[2] = 0LL;
    trans1.txid[3] = 0LL;

    return trans1;
}

bool isEqual( _TM_Txid_External trans1, _TM_Txid_External trans2 )
{
    return (memcmp(&trans1,&trans2,sizeof(_TM_Txid_External)) == 0);
}

bool isNull( _TM_Txid_External transid )
{
    _TM_Txid_External trans_null = null_trans();

    return isEqual(transid,trans_null);
}

bool isInvalid( _TM_Txid_External transid )
{
    _TM_Txid_External trans_invalid = invalid_trans();

    return isEqual(transid,trans_invalid);
}

char *ErrorMsg (int error_code)
{
    int rc;
    int length;
    static char buffer[MPI_MAX_ERROR_STRING];

    rc = MPI_Error_string (error_code, buffer, &length);
    if (rc != MPI_SUCCESS)
    {
        snprintf(buffer, sizeof(buffer), 
                 "MPI_Error_string: Invalid error code (%d)\n", error_code);
        length = strlen(buffer);
    }
    buffer[length] = '\0';

    return buffer;
}

void child_death_signal_handler2 (int signal, siginfo_t *info, void *)
{
    pid_t pid;
    int saveerrno;
    int status;
    pid_t whichPid;

    const char method_name[] = "child_death_signal_handler2";
    if (trace_settings & TRACE_ENTRY_EXIT)
       trace_nolock_printf("%s@%d\n", method_name, __LINE__);

    saveerrno = errno; // waitpid/etc sets errno

    if (trace_settings & TRACE_SIG_HANDLER)
        trace_nolock_printf("%s@%d - signal=%d, code=%d, status=%d, pid=%d\n",
                            method_name, __LINE__, signal, info->si_code,
                            info->si_status, info->si_pid);

    // Handle the process that triggered the signal handler as well
    // as any other exited children.
    whichPid = info->si_pid;
    do
    {
        pid = waitpid (whichPid, &status, WNOHANG);
        if (trace_settings & TRACE_SIG_HANDLER)
        {
            if ( pid != -1 )
            {
                trace_nolock_printf("%s@%d - waitpid(%d) returned %d\n",
                                    method_name, __LINE__, whichPid, pid);
            }
            else
            {
                trace_nolock_printf("%s@%d - waitpid(%d) error, %s (%d)\n",
                                    method_name, __LINE__, whichPid,
                                    strerror(errno), errno);
            }
        }
        if (pid > 0)
        {
            IntProcess.handle_signal(pid, status);

            // Add this pid to a list to be examined outside the
            // signal-handler (to avoid spending excessive time in
            // signal handler).  When that list is examined the work
            // required to handle the terminated process will be done.
            SQ_theLocalIOToClient->handleDeadPid(pid);

            if (trace_settings & TRACE_SIG_HANDLER)
            { 
                if ( WIFEXITED(status) )
                {   // Process exited normally
                    trace_nolock_printf("%s@%d - process %d exited, exit"
                                        " status=%d\n", method_name,__LINE__,
                                        pid, WEXITSTATUS(status));
                }
                if ( WIFSIGNALED(status) )
                {   // Process was terminated by a signal
                   trace_nolock_printf("%s@%d - process %d terminated by "
                                       "signal #%d\n", method_name, __LINE__,
                                       pid, WTERMSIG(status));
                }
            }            
        }
        else if ( whichPid != -1 && pid == 0 )
        {
            // Process that triggered the signal handler has not yet changed
            // state.  Remember the "pid" so we can handle it later.
            if (trace_settings & TRACE_SIG_HANDLER)
                trace_nolock_printf("%s@%d - adding pid=%d to unreaped list\n",
                                    method_name, __LINE__, pid);
            SQ_theLocalIOToClient->handleAlmostDeadPid(whichPid);
            pid = 1; // force at least one more waitpid to be done
        }
        whichPid = -1; // now do waitpid on any child process
    }
    while (pid > 0);

    errno = saveerrno;

    if (trace_settings & TRACE_ENTRY_EXIT)
        trace_nolock_printf("%s@%d - Exit\n", method_name, __LINE__);
}

void monMallocStats()
{
    // Log current malloc statistics in stderr
    time_t mytime = time(NULL);
    char *timestamp = ctime(&mytime);
    timestamp[strlen(timestamp)-1] = '\0';

    printf("monitor malloc statistics at %s:\n", timestamp);
    malloc_stats();
}

const char *CommTypeString( CommType_t commType)
{
    const char *str;
    
    switch( commType )
    {
        case CommType_InfiniBand:
            str = "InfiniBand";
            break;
        case CommType_Sockets:
            str = "Sockets";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
}


CMonitor::CMonitor (int procTermSig)
    : CTmSync_Container (),
      OpenCount (0),
      NoticeCount (0),
      ProcessCount (0),
      NumOutstandingIO (0),
      NumOutstandingSends (0),
      Last_error (MPI_SUCCESS),
      processMapFd ( -1 ),
      procTermSig_ ( procTermSig )
{
    const char method_name[] = "CMonitor::CMonitor";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "MNTR", 4);

#ifdef USE_SEQUENCE_NUM
    clock_gettime(CLOCK_REALTIME, &savedTime_);
#endif

    TRACE_EXIT;
}

CMonitor::~CMonitor (void)
{
    const char method_name[] = "CMonitor::~CMonitor";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "mntr", 4);

    if ( processMapFd != -1)
    {
        close ( processMapFd );
    }

    TRACE_EXIT;
}

void CMonitor::IncOpenCount (void)
{
    OpenCount++;
}
void CMonitor::IncNoticeCount (void)
{      
    NoticeCount++;
}
void CMonitor::IncProcessCount (void)
{
    ProcessCount++;
}
void CMonitor::DecrOpenCount (void)
{
    OpenCount--;
}
void CMonitor::DecrNoticeCount (void)
{      
    NoticeCount--;
}
void CMonitor::DecrProcessCount (void)
{
    ProcessCount--;
}

void CMonitor::openProcessMap ( void )
{
    char fname[MAX_PROCESS_PATH];

    char *env;
    env = getenv("SQ_PIDMAP");
    if ( env && *env == '1' )
    {
        PidMap = true;
    }

    snprintf( fname, sizeof(fname), "%s/monitor.map.%d.%s",
             getenv("MPI_TMPDIR"), MyPNID, Node_name );
    remove(fname);
    processMapFd = open(fname, O_WRONLY | O_APPEND | O_CREAT,
                        S_IRUSR | S_IWUSR );
    if ( processMapFd == -1 )
    {  // File open error
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf),
                 "[CMonitor::openProcessMap], Error opening %s, %s (%d).\n",
                fname, strerror(errno), errno);
        mon_log_write(MON_PROCESS_COMPLETEPSTARTUP_2, SQ_LOG_ERR, buf);
    }
}

void CMonitor::writeProcessMapEntry ( const char * buf )
{
    if ( processMapFd != -1 )
        write( processMapFd, buf, strlen(buf));
}

void CMonitor::writeProcessMapBegin( const char *name
                                   , int nid
                                   , int pid
                                   , int verifier
                                   , int parentNid
                                   , int parentPid
                                   , int parentVerifier
                                   , const char *program )
{
    char buf[55+MAX_PROCESS_NAME+MAX_PROCESS_PATH];

    time_t mytime = time(NULL);
    char *timestamp = ctime(&mytime);
    timestamp[strlen(timestamp)-1] = '\0';

    snprintf( buf, sizeof(buf)
            , "BEGIN %s %-8s (%d, %d:%d) P(%d, %d:%d) %s\n"
            , timestamp, name, nid, pid, verifier
            , parentNid, parentPid, parentVerifier, program);
    writeProcessMapEntry ( buf );
}

void CMonitor::writeProcessMapEnd( const char *name
                                 , int nid
                                 , int pid
                                 , int verifier
                                 , int parentNid
                                 , int parentPid
                                 , int parentVerifier
                                 , const char *program )
{
    char buf[55+MAX_PROCESS_NAME+MAX_PROCESS_PATH];

    time_t mytime = time(NULL);
    char *timestamp = ctime(&mytime);
    timestamp[strlen(timestamp)-1] = '\0';

    snprintf( buf, sizeof(buf)
            , "END   %s %-8s (%d, %d:%d) P(%d, %d:%d) %s\n"
            , timestamp, name, nid, pid, verifier
            , parentNid, parentPid, parentVerifier, program);
    writeProcessMapEntry ( buf );
}

bool CMonitor::CompleteProcessStartup (struct message_def * msg)
{
    bool status = FAILURE;
    CProcess *process;
    CLNode  *lnode;

    const char method_name[] = "CMonitor::CompleteProcessStartup";
    TRACE_ENTRY;
    
    lnode = Nodes->GetLNode( msg->u.request.u.startup.nid );
    if ( lnode )
    {
        process = lnode->
            CompleteProcessStartup ( msg->u.request.u.startup.process_name,
                                     msg->u.request.u.startup.port_name,
                                     msg->u.request.u.startup.os_pid,
                                     msg->u.request.u.startup.event_messages,
                                     msg->u.request.u.startup.system_messages,
                                     NULL );
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], Invalid Node ID=%d\n",
                 method_name, msg->u.request.u.startup.nid);
        mon_log_write(MON_MONITOR_COMPLETEPSTARTUP_1, SQ_LOG_ERR, buf);

        process = NULL;
    }
    
    if (process)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d - Process %s started on port %s\n",
                         method_name, __LINE__,
                         msg->u.request.u.startup.process_name,
                         msg->u.request.u.startup.port_name);
        }

        CProcessContainer::ParentNewProcReply( process, MPI_SUCCESS);
        status = SUCCESS;
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[CMonitor::CompleteProcessStartup], Error= Can't find process: %s!\n", msg->u.request.u.startup.process_name);
        mon_log_write(MON_MONITOR_COMPLETEPSTARTUP_2, SQ_LOG_ERR, buf);
           
        msg->u.reply.type = ReplyType_Generic;
        msg->u.reply.u.generic.nid = -1;
        msg->u.reply.u.generic.pid = -1;
        msg->u.reply.u.generic.verifier = -1;
        msg->u.reply.u.generic.process_name[0] = '\0';
        msg->u.reply.u.generic.return_code = MPI_ERR_NAME;
        status = FAILURE;
    }
    TRACE_EXIT;

    return status;
}

char * CMonitor::ProcCopy(char *bufPtr, CProcess *process)
{
    const char method_name[] = "CMonitor::ProcCopy";
    TRACE_ENTRY;

    int  stringDataLen = 0;
    struct clone_def *procObj = (struct clone_def *)bufPtr;

    procObj->nid = process->GetNid();
    procObj->type = process->GetType();
    procObj->priority = process->GetPriority();
    procObj->backup = process->IsBackup();
    procObj->unhooked = process->IsUnhooked();
    procObj->pathStrId = process->pathStrId();
    procObj->ldpathStrId = process->ldPathStrId();
    procObj->programStrId = process->programStrId();
    procObj->os_pid = process->GetPid();
    procObj->verifier = process->GetVerifier();
    procObj->prior_pid = process->GetPriorPid ();
    procObj->parent_nid = process->GetParentNid();
    procObj->parent_pid = process->GetParentPid();
    procObj->parent_verifier = process->GetParentVerifier();
    procObj->persistent_retries = process->GetPersistentRetries();
    procObj->event_messages = process->IsEventMessages();
    procObj->system_messages = process->IsSystemMessages();
    procObj->argc = process->argc();
    procObj->creation_time = process->GetCreationTime();


    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Packing process %s (%d,%d:%d)\n"
                        , method_name, __LINE__
                        , process->GetName()
                        , process->GetNid()
                        , process->GetPid()
                        , process->GetVerifier() );

    char *stringData = &procObj->stringData;

    if (strlen(process->GetName()))
    {
        // Copy the program name
        procObj->nameLen = strlen(process->GetName()) + 1;
        memcpy(stringData, process->GetName(),  procObj->nameLen );
        stringData += procObj->nameLen;
        stringDataLen = procObj->nameLen;
    }
    else
    {
        procObj->nameLen = 0;
    }

    if (strlen(process->GetPort()))
    {
        // Copy the port
        procObj->portLen = strlen(process->GetPort()) + 1;
        memcpy(stringData, process->GetPort(),  procObj->portLen );
        stringData += procObj->portLen;
        stringDataLen += procObj->portLen;
    }
    else
    {
        procObj->portLen = 0;
    }

    if (process->IsPersistent())
    {
        if (strlen(process->infile()))
        {
            // Copy the standard in file name
            procObj->infileLen = strlen(process->infile()) + 1;
            memcpy(stringData, process->infile(), procObj->infileLen);
            stringData += procObj->infileLen;
            stringDataLen += procObj->infileLen;
        }
        else
        {
            procObj->infileLen = 0;
        }

        if (strlen(process->outfile()))
        {
            // Copy the standard out file name
            procObj->outfileLen = strlen(process->outfile()) + 1;
            memcpy(stringData, process->outfile(),  procObj->outfileLen );
            stringData += procObj->outfileLen;
            stringDataLen += procObj->outfileLen;
        }
        else
        {
            procObj->outfileLen = 0;
        }

        procObj->argvLen =  process->userArgvLen();
        if (procObj->argvLen)
        {
            // Copy the program argument strings
            memcpy(stringData, process->userArgv(), procObj->argvLen);
            stringData += procObj->argvLen;
            stringDataLen += procObj->argvLen;
        }

        procObj->persistent = true;

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d - Packing process string data:\n"
                              "        name(%d)       =%s\n"
                              "        port(%d)       =%s\n"
                              "        infile(%d)     =%s\n"
                              "        outfile(%d)    =%s\n"
                              "        userArgv(%d)   =%s\n"
                              "        stringData(%d) =%s\n"
                            , method_name, __LINE__
                            , procObj->nameLen
                            , process->GetName()
                            , procObj->portLen
                            , process->GetPort()
                            , procObj->infileLen
                            , process->infile()
                            , procObj->outfileLen
                            , process->outfile()
                            , procObj->argvLen
                            , procObj->argvLen?process->userArgv():"" 
                            , stringDataLen
                            , stringDataLen?&procObj->stringData:"" );
    }
    else
    {
        procObj->infileLen = 0;
        procObj->outfileLen = 0;
        procObj->argvLen = 0;
        procObj->persistent = false;
    }

    TRACE_EXIT;
    return stringData;
}

int CMonitor::PackProcObjs( char *&buffer )
{
    const char method_name[] = "CMonitor::PackProcObjs";
    TRACE_ENTRY;

    CLNode *lnode = NULL;
    CProcess *process = NULL;
    int procCount = 0;

    char *bufPtr = buffer;

    // first copy all primary and generic processes
    lnode = Nodes->GetFirstLNode();
    for ( ; lnode ; lnode = lnode->GetNext() )
    {
        process = lnode->GetFirstProcess();
        while (process)
        {
            if (!process->IsBackup())
            {
                buffer = ProcCopy(buffer, process);
                ++procCount;
            }

            process = process->GetNext();
        }
    }

    // copy all the backup processes
    lnode = Nodes->GetFirstLNode();
    for ( ; lnode ; lnode = lnode->GetNext() )
    {
        process = lnode->GetFirstProcess();
        while (process)
        {
            if (process->IsBackup())
            {
                buffer = ProcCopy(buffer, process);
                ++procCount;
            }

            process = process->GetNext();
        }
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Total Procs = %d, Total Size = %ld, Avg = %ld bytes\n",
                       method_name, __LINE__, procCount, buffer - bufPtr, (buffer - bufPtr)/procCount);

    TRACE_EXIT;
    return procCount;
}

void CMonitor::UnpackProcObjs( char *&buffer, int procCount )
{
    const char method_name[] = "CMonitor::UnpackProcObjs";
    TRACE_ENTRY;

    CNode * node = NULL;
    CProcess * process = NULL;
    int  stringDataLen;
    char *name = NULL;
    char *port = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    char *userargv = NULL;
    char *stringData = NULL;

    struct clone_def *procObj;

    int i;

    for (i=0; i<procCount; i++)
    {
        procObj = (struct clone_def *)buffer;

        stringDataLen = 0;
        stringData = &procObj->stringData;
  
        node = Nodes->GetLNode (procObj->nid)->GetNode();

        if (procObj->nameLen)
        {
            name = &procObj->stringData;
            stringDataLen += procObj->nameLen;
        }
          
        if (procObj->portLen)
        {
            port = &stringData[stringDataLen];
            stringDataLen += procObj->portLen;
        }
          
        if (procObj->infileLen)
        {
            infile = &stringData[stringDataLen];
            stringDataLen += procObj->infileLen;
        }
          
        if (procObj->outfileLen)
        {
            outfile = &stringData[stringDataLen];
            stringDataLen += procObj->outfileLen;
        }

        if (procObj->argvLen)
        {
            userargv = &stringData[stringDataLen];
            stringDataLen += procObj->argvLen;
        }

        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d - Unpacking process string data:\n"
                              "        stringData(%d) =%s\n"
                              "        name(%d)       =%s\n"
                              "        port(%d)       =%s\n"
                              "        infile(%d)     =%s\n"
                              "        outfile(%d)    =%s\n"
                              "        userArgc       =%d\n"
                              "        userArgv(%d)   =%s\n"
                            , method_name, __LINE__
                            , stringDataLen
                            , stringDataLen?&procObj->stringData:""
                            , procObj->nameLen
                            , procObj->nameLen?name:""
                            , procObj->portLen
                            , procObj->portLen?port:""
                            , procObj->infileLen
                            , procObj->infileLen?infile:""
                            , procObj->outfileLen
                            , procObj->outfileLen?outfile:""
                            , procObj->argc
                            , procObj->argvLen
                            , procObj->argvLen?userargv:"" );

        process = node->CloneProcess (procObj->nid,
                                      procObj->type,
                                      procObj->priority,
                                      procObj->backup,
                                      procObj->unhooked,
                                      procObj->nameLen?name:(char *)"",
                                      procObj->portLen?port:(char *)"",
                                      procObj->os_pid,
                                      procObj->verifier, 
                                      procObj->parent_nid,
                                      procObj->parent_pid,
                                      procObj->parent_verifier,
                                      procObj->event_messages,
                                      procObj->system_messages,
                                      procObj->pathStrId,
                                      procObj->ldpathStrId,
                                      procObj->programStrId,
                                      procObj->infileLen?infile:(char *)"",
                                      procObj->outfileLen?outfile:(char *)"",
                                      &procObj->creation_time);

        if ( process && procObj->argvLen )
        {
            process->userArgs ( procObj->argc, procObj->argvLen, userargv );
        }

        if ( process && procObj->persistent )
        {
            process->SetPersistent(true);
        }

        buffer = &stringData[stringDataLen];
    }

    TRACE_EXIT;
    return;
}

void CMonitor::StartPrimitiveProcesses( void )
{
    const char method_name[] = "CMonitor::StartPrimitiveProcesses";
    TRACE_ENTRY;

    if ( !MyNode->IsSpareNode() )
    {
        // Queue the Create primitive processes request for 
        // processing by a worker thread.
        ReqQueue.enqueueCreatePrimitiveReq( MyPNID );
    }
    
    TRACE_EXIT;
}

void HandleMyNodeExpiration( void )
{
    const char method_name[] = "HandleMyNodeExpiration";
    TRACE_ENTRY;
    ReqQueue.enqueueDownReq(MyPNID);
    TRACE_EXIT;
}

void HandleNodeExpiration( const char *nodeName )
{
    const char method_name[] = "HandleNodeExpiration";
    TRACE_ENTRY;
    CNode *node = Nodes->GetNode((char *)nodeName);
    if (node)
    {
        ReqQueue.enqueueDownReq(node->GetPNid());
    }
    TRACE_EXIT;
}

void CreateZookeeperClient( void )
{
    const char method_name[] = "CreateZookeeperClient";
    TRACE_ENTRY;

    if ( ZClientEnabled )
    {
        string       hostName;
        string       zkQuorumHosts;
        stringstream zkQuorumPort;
        char *env;
        char  hostsStr[MAX_PROCESSOR_NAME * 3] = { 0 };
        char *tkn = NULL;

        int zport;
        env = getenv("ZOOKEEPER_PORT");
        if ( env && isdigit(*env) )
        {
            zport = atoi(env);
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Zookeeper quorum port is not defined!\n"
                    , method_name);
            mon_log_write(MON_MONITOR_CREATEZCLIENT_1, SQ_LOG_CRIT, buf);

            ZClientEnabled = false;
            TRACE_EXIT;
            return;
        }
        
        env = getenv("ZOOKEEPER_NODES");
        if ( env )
        {
            zkQuorumHosts = env;
            if ( zkQuorumHosts.length() == 0 )
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], Zookeeper quorum hosts are not defined!\n"
                        , method_name);
                mon_log_write(MON_MONITOR_CREATEZCLIENT_2, SQ_LOG_CRIT, buf);

                ZClientEnabled = false;
                TRACE_EXIT;
                return;
            }
            
            strcpy( hostsStr, zkQuorumHosts.c_str() );
            zkQuorumPort.str( "" );
            
            tkn = strtok( hostsStr, "," );
            do
            {
                if ( tkn != NULL )
                {
                    hostName = tkn;
                    zkQuorumPort << hostName.c_str()
                                 << ":" 
                                 << zport;
                }
                tkn = strtok( NULL, "," );
                if ( tkn != NULL )
                {
                    zkQuorumPort << ",";
                }
                
            }
            while( tkn != NULL );
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf( "%s@%d zkQuorumPort=%s\n"
                            , method_name, __LINE__
                            , zkQuorumPort.str().c_str() );
            }
        }
    
        ZClient = new CZClient( zkQuorumPort.str().c_str()
                              , ZCLIENT_TRAFODION_ZNODE
                              , ZCLIENT_INSTANCE_ZNODE );
        if ( ZClient == NULL )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Failed to allocate ZClient object!\n"
                    , method_name);
            mon_log_write(MON_MONITOR_CREATEZCLIENT_3, SQ_LOG_CRIT, buf);
            abort();
        }
    }

    TRACE_EXIT;
}

void StartZookeeperClient( void )
{
    const char method_name[] = "StartZookeeperClient";
    TRACE_ENTRY;

    int rc = -1;

    if ( ZClientEnabled )
    {
        if ( ZClient )
        {
            rc = ZClient->StartWork();
            if (rc == 0)
            {
                ZClient->StartMonitoring();

                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], ZClient node monitoring started\n"
                        , method_name);
                mon_log_write(MON_MONITOR_STARTZCLIENT_1, SQ_LOG_INFO, buf);
            }
        }
    }

    TRACE_EXIT;
}

#ifdef USE_SEQUENCE_NUM
long long CMonitor::GetTimeSeqNum()
{
    const char method_name[] = "CMonitor::GetTimeSeqNum";
    TRACE_ENTRY;

    TimeSeqNumLock_.lock();

    struct timespec currTime;
    clock_gettime(CLOCK_REALTIME, &currTime);
    
    if ( (currTime.tv_sec > savedTime_.tv_sec) ||
         ( (currTime.tv_sec == savedTime_.tv_sec) &&   
           (currTime.tv_nsec > savedTime_.tv_nsec) ) ) 
    {
        savedTime_.tv_sec = currTime.tv_sec;
        savedTime_.tv_nsec = currTime.tv_nsec;
    }
    else
    {   // time drifted back. just increment nanoseconds
        savedTime_.tv_nsec++; 
        // overflow check is not required as it would take
        // at least 3 billion requests to overflow
    }

    long long result;
    int* value = (int *)&result;
    value[0] = savedTime_.tv_nsec;  // little endian
    value[1] = savedTime_.tv_sec;

    TimeSeqNumLock_.unlock();

    if (trace_settings & TRACE_REQUEST_DETAIL)
        trace_printf("%s@%d Time seq num = %Lx\n", method_name, __LINE__, result);
    
    TRACE_EXIT;

    return result;
}
#endif

int main (int argc, char *argv[])
{
    int i;
    int rc;
    bool done = false;
    char *env;
    char *nodename = NULL;
    char fname[MAX_PROCESS_PATH];
    char short_node_name[MPI_MAX_PROCESSOR_NAME];
    char port_fname[MAX_PROCESS_PATH];
    char temp_fname[MAX_PROCESS_PATH];
    char buf[MON_STRING_BUF_SIZE];
    unsigned int initSleepTime = 1; // 1 second

    mallopt(M_ARENA_MAX, 4); // call to limit the number of arena's of  monitor to 4.This call doesn't seem to have any effect !
 
    CALL_COMP_DOVERS(monitor, argc, argv);

    const char method_name[] = "main";

    if (argc < 2) {
      printf("error: monitor needs an argument...exitting...\n");
      exit(0);
    }

    int lv_arg_index = 1;
    while ( lv_arg_index < argc )
    {
        // Installations like Cloudera Manager, the monitor is started in AGENT mode
        if ( strcmp( argv[lv_arg_index], "COLD_AGENT" ) == 0 )
        {
            IsAgentMode = true;
        }

        lv_arg_index++;
    }

    // Set flag to indicate whether we are operating in a real cluster
    // or a virtual cluster.   This is used throughout the monitor when
    // behavior differs for a real vs. virtual cluster environment.
    if ( !IsAgentMode )
    {
        if ( getenv( "SQ_VIRTUAL_NODES" ) )
        {
            IsRealCluster = false;
            Emulate_Down = true;
        }
        if (IsRealCluster)
        {
            // The monitor processes may be started by MPIrun utility
            env = getenv("SQ_MON_CREATOR");
            if ( env != NULL && strcmp(env, "MPIRUN") == 0 )
            {
                IsMPIChild = true;
            }
            // The monitor can be set to run in AGENT mode
            env = getenv("SQ_MON_RUN_MODE");
            if ( env != NULL && strcmp(env, "AGENT") == 0 )
            {
                IsAgentMode = true;
            }
        }
    }

    if ( IsAgentMode )
    {
        MON_Props xprops( true );
        xprops.load( "monitor.env" );
        MON_Smap_Enum xenum( &xprops );
        while ( xenum.more( ) )
        {
            char *xkey = xenum.next( );
            const char *xvalue = xprops.get( xkey );
            if ( xkey && xkey[0] && xvalue )
            {
                setenv( xkey, xvalue, 1 );
            }
        }
    }

    MonLog = new CMonLog( "log4cxx.monitor.mon.config", "MON", "alt.mon", -1, -1, getpid(), "$MONITOR" );

    MonLog->setupInMemoryLog();

#ifdef MULTI_TRACE_FILES
    initVariableKey();
#endif

#ifdef DMALLOC
    util_dmalloc_start();
#endif

    // Save our execution path
    env = getenv("PWD");
    if ( env )
    {
        STRCPY(MyPath,env);
    }
    for(i=strlen(argv[0])-1; i>=0; i--)
    {
        if (argv[0][i] == '/')
        {
            argv[0][i] = '\0';
            strncpy(MyPath,argv[0],sizeof(MyPath));
            argv[0][i] = '/';
            break;
        }
    }
    env = getenv("SQ_MEASURE");
    if ( env && *env == '1' )
    {
        Measure = 1;
        snprintf(fname, sizeof(fname), "%s/monitor.P%d",
                 getenv("MPI_TMPDIR"),getpid());
        setenv("MPI_INSTR", fname, 1);
    }
    if ( env && *env == '2' )
    {
        Measure = 2;
        snprintf(fname, sizeof(fname), "%s/monitor.cpu.P%d:cpu",
                 getenv("MPI_TMPDIR"),getpid());
        setenv("MPI_INSTR", fname, 1);
    }

    env = getenv("SQ_IC");
    if ( env != NULL && strcmp(env, "IBV") == 0 )
    {
        CommType = CommType_InfiniBand;
    }
    else
    {
        CommType = CommType_Sockets;
    }

    // Mask all allowed signals
    sigset_t              mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    sigdelset(&mask, SIGUSR2);
    rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], pthread_sigmask error=%d\n",
                 method_name, rc);
        mon_log_write(MON_MONITOR_MAIN_1, SQ_LOG_ERR, buf);
    }

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    // Initialize MPI environment
    MPI_Init (&argc, &argv);

    env = getenv("MON_PROF_ENABLE");
    if ( env )
    {
        mon_profiler_init(env); // LCOV_EXCL_LINE
    }

    env = getenv("MON_SNMP_ENABLE");
    if ( env )
    {
        genSnmpTrapEnabled = true;
    }
    
    env = getenv("MON_INIT_SLEEP");
    if ( env && isdigit(*env) )
    {
        initSleepTime = atoi(env);
    }

    env = getenv("SQ_COLD_STANDBY_SPARE");
    if ( env && isdigit(*env) )
    {
        if ( strcmp(env,"0")==0 )
        {
            SpareNodeColdStandby = false;
        }
    }

    // We need to delay some to make sure all monitor processes have initialized before
    // any monitor tries to perform an Allgather operation.
    sleep( initSleepTime );

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    MPI_Comm_rank (MPI_COMM_WORLD, &MyPNID);

    MonLog->setPNid( MyPNID );

    gethostname(Node_name, MPI_MAX_PROCESSOR_NAME);
    char *tmpptr = Node_name;
    while ( *tmpptr )
    {
        *tmpptr = (char)tolower( *tmpptr );
        tmpptr++;
    }

    // Remove the domain portion of the name if any
    char str1[MPI_MAX_PROCESSOR_NAME];
    memset( str1, 0, MPI_MAX_PROCESSOR_NAME );
    memset( short_node_name, 0, MPI_MAX_PROCESSOR_NAME );
    strcpy (str1, Node_name );

    char *str1_dot = strchr( (char *) str1, '.' );
    if ( str1_dot )
    {
        memcpy( short_node_name, str1, str1_dot - str1 );
    }
    else
    {
        strcpy (short_node_name, str1 );
    }

#ifdef MULTI_TRACE_FILES
    setThreadVariable( (char *)"mainThread" );
#endif

    // Without mpi daemon the monitor has no default standard output.
    // We create a standard output file here.
    if ( IsRealCluster )
    {
        snprintf(fname, sizeof(fname), "%s/logs/sqmon.%s.log",
                 getenv("TRAF_HOME"), Node_name);
    }
    else
    {
        snprintf(fname, sizeof(fname), "%s/logs/sqmon.%d.%s.log",
                 getenv("TRAF_HOME"), MyPNID, Node_name);
    }
    remove(fname);
    if( freopen (fname, "w", stdout) == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], can't open stdout (%s).\n",
                 method_name, fname);
        mon_log_write(MON_MONITOR_MAIN_2, SQ_LOG_ERR, buf);
    }
    setlinebuf(stdout);

    // Send stderr output to same file as stdout.  (Note: the monitor does
    // not write to stderr but perhaps there could be components included in
    // the monitor build that do write to stderr.)
    if ( dup2 ( 1, 2 ) == -1 )
    {
        printf ( "dup2 failed for stderr: %s (%d)\n", strerror(errno), errno);
    }

    switch( CommType )
    {
        case CommType_Sockets: // Valid communication protocol
            break;
        case CommType_InfiniBand: // Currenly disabled - requires HA MPI
            //MPI_Open_port (MPI_INFO_NULL, MyMPICommPort);
        default:
            printf( "SQ_IC contains invalid communication protocol: %s\n"
                   , CommTypeString(CommType));
            abort();
    }

    if ((!IsAgentMode) && (argc > 3 && strcmp (argv[2], "-integrate") == 0))
    {
        switch( CommType )
        {
            case CommType_InfiniBand:
                if (argc == 4 && strstr(argv[3], "$port#"))
                {
                    SMSIntegrating = IAmIntegrating = true;
                    strcpy( IntegratingMonitorPort, argv[3] );
                }
                else
                {
                    printf ( "Invalid integrating monitor MPI port: %s\n", argv[3]);
                    abort();
                }
                break;
            case CommType_Sockets:
                if ( IsAgentMode || isdigit (*argv[3]) )
                {
                    // In agent mode and when re-integrating (node up), all
                    // monitors processes start as a cluster of 1 and join to the 
                    // creator monitor to establish the real cluster.
                    // Therefore, MyPNID will always be zero them it is 
                    // necessary to use the node name to obtain the correct
                    // <pnid> from the configuration which occurs when creating the
                    // CMonitor object down below. By setting MyPNID to -1, when the 
                    // CCluster::InitializeConfigCluster() invoked during the creation
                    // of the CMonitor object it will set MyPNID using Node_name.
                    MyPNID = -1;
                    SMSIntegrating = IAmIntegrating = true;
                    strcpy( IntegratingMonitorPort, argv[3] );
                }
                else
                {
                    printf ( "Invalid integrating monitor socket port: %s\n", argv[3]);
                    abort();
                }
                break;
            default:
                // Programmer bonehead!
                abort();
        }
        if ( isdigit (*argv[4]) )
        {
            
            CreatorShellPid = atoi( argv[4] );
        }
        else
        {
            printf ( "Invalid creator shell pid: %s\n", argv[4]);
            abort();
        }
        if ( isdigit (*argv[5]) )
        {
            
            CreatorShellVerifier = atoi( argv[5] );
        }
        else
        {
            printf ( "Invalid creator shell verifier: %s\n", argv[5]);
            abort();
        }

        // Trace cannot be specified on startup command but need to
        // check for trace environment variable settings.
        MonTrace->mon_trace_init("0", NULL);

    }

    if (IsAgentMode)
    {    
        CreatorShellPid = 1000; // per monitor.sh
        CreatorShellVerifier = 0;
    }

    if (argc == 3 && isdigit(*argv[2]) )
    {
        MonTrace->mon_trace_init(argv[2], "STDOUT");
    }
    else if (argc >= 3 && strcmp (argv[2], "TRACEFILE") == 0)
    {
        if (argc == 4 && isdigit (*argv[3]))
        {
            MonTrace->mon_trace_init(argv[3], NULL);
        }
    }
    else
    {   // Trace not specified on startup command but need to check
        // for trace environment variable settings.
        MonTrace->mon_trace_init("0", NULL);
    }

    if ((env = getenv("MON_SYNC_DELAY")) != NULL)
    {   // Sync cycle delay value specified
        int delay;
        errno = 0;
        delay = strtol(env, NULL, 10);
        if ( errno == 0) next_test_delay = delay;
    }
    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d next_test_delay=%ld\n", method_name, __LINE__,
                     next_test_delay);

#ifdef USE_SONAR
    if ( (env = getenv("SQ_SONAR") ) != NULL )
    {
        sonar_state_init();
        if ( IsRealCluster && sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
        {  // Not a virtual cluster and sonar is enabled.

            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d Enabling Sonar\n", method_name, __LINE__);
            }

            MonStats = new CMonSonarStats();
        }
    }
#endif
    if ( MonStats == NULL )
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d Using non-Sonar counters\n",
                         method_name, __LINE__);
        }

        MonStats = new CMonStats();
    }

    // Normal process termination uses SIGKILL by default.  Environment
    // variable PROC_TERM_SIG can be used to override this.  Valid values
    // are SIGKILL or SIGTERM (as either string or integer values).
    int procTermSig = SIGKILL;
    env = getenv("PROC_TERM_SIG");
    if  ( env )
    {
        if (strcasecmp(env, "SIGKILL") == 0)
        {
            procTermSig = SIGKILL;
        }
        else if (strcasecmp(env, "SIGTERM") == 0)
        {
            procTermSig = SIGTERM;
        }
        else
        {
            procTermSig = atoi( env );
            if ( !(procTermSig == SIGKILL || procTermSig == SIGTERM) )
            {
                procTermSig = SIGKILL;
            }
        }
    }
    if (trace_settings & TRACE_INIT)
    {
        trace_printf("%s@%d Using signal %d for normal processes "
                     "termination.\n", method_name, __LINE__, procTermSig);
    }

    // Record statistics (sonar counters): monitor is busy
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->MonitorBusyIncr();

    snprintf(buf, sizeof(buf),
                 "[CMonitor::main], %s, Started! CommType: %s (%s%s%s)\n"
                , CALL_COMP_GETVERS2(monitor)
                , CommTypeString( CommType )
                , IsRealCluster?"RealCluster":"VirtualCluster"
                , IsAgentMode?"/AgentMode":""
                , IsMPIChild?"/MPIChild":"" );
    mon_log_write(MON_MONITOR_MAIN_3, SQ_LOG_INFO, buf);
       
#ifdef DMALLOC
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d" "DMALLOC Option set" "\n", method_name, __LINE__);
#endif

#ifdef DEBUGGING
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d" "DEBUGGING Option set" "\n", method_name, __LINE__);
#endif

    if ( Emulate_Down )
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" "EMULATE_DOWN Option set" "\n", method_name, __LINE__);

    {
        // Create thread for monitoring redirected i/o.
        // This is also used for monitor logs, so start it early. 
        Redirector.start();
        
        // Create global configuration now
        ClusterConfig = new CClusterConfig();
        if (ClusterConfig)
        {
            bool traceEnabled = (trace_settings & TRACE_TRAFCONFIG) ? true : false;
            if (ClusterConfig->Initialize( traceEnabled, MonTrace->getTraceFileName()))
            {
                if (!ClusterConfig->LoadConfig())
                {
                     char la_buf[MON_STRING_BUF_SIZE];
                     sprintf(la_buf, "[%s], Failed to load cluster configuration.\n", method_name);
                     mon_log_write(MON_MONITOR_MAIN_12, SQ_LOG_CRIT, la_buf);
                
                     abort();
                }
            }
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Failed to open cluster configuration.\n", method_name);
                mon_log_write(MON_MONITOR_MAIN_13, SQ_LOG_CRIT, la_buf);
            
                abort();
            }
        }
       else  
       {
           char la_buf[MON_STRING_BUF_SIZE];
           sprintf(la_buf, "[%s], Failed to allocate cluster configuration.\n", method_name);
           mon_log_write(MON_MONITOR_MAIN_14, SQ_LOG_CRIT, la_buf);
          
           abort();
        }

        // Set up zookeeper and determine the master 
         if ( IsAgentMode || IsRealCluster )
        {
            // Zookeeper client is enabled only in a real cluster
            env = getenv("SQ_MON_ZCLIENT_ENABLED");

            if ( env )
            {
                if ( env && isdigit(*env) )
                {
                    if ( strcmp(env,"0")==0 )
                    {
                        ZClientEnabled = false;
                    }
                }
            }

            if ( ZClientEnabled )
            {
                 CreateZookeeperClient( );
            }
        }
        else
        {
            ZClientEnabled = false;
        }
        
        if (IsAgentMode)
        {
            if ((ZClientEnabled) && (ZClient != NULL))
            {
                // Do not wait, just see if one exists
                const char *masterMonitor = ZClient->WaitForAndReturnMaster(false);

                if (masterMonitor)
                {
                    strcpy (MasterMonitorName, masterMonitor);
                    // unfortunately, we have to do this to see if we are the master before
                    // other things are set up.   This is how we must do that
                    if (strcmp(Node_name, masterMonitor) == 0)
                    {
                        IsMaster = true;
                    }
                    else 
                    {
                        IsMaster = false;
                    }
                }
                else
                {
                    strcpy (MasterMonitorName, ClusterConfig->GetConfigMasterByName());  
                    if (strcmp (Node_name,  ClusterConfig->GetConfigMasterByName()) == 0)
                    {
                        IsMaster = true;
                    }
                    else
                    {
                        IsMaster = false;
                    }
                }
      
             }
         }

         if (IsAgentMode)
         {
            if (!IsMaster)
            {
                MyPNID=-1;
                SMSIntegrating = IAmIntegrating = true;
                char *monitorPort = getenv ("MONITOR_COMM_PORT");
                if (monitorPort)
                {
                    strcpy( IntegratingMonitorPort, MasterMonitorName);
                    strcat( IntegratingMonitorPort, ":");
                    strcat( IntegratingMonitorPort, monitorPort);
                }
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d (MasterMonitor) IsAgentMode = TRUE, I am NOT the master, "
                                  "MyPNID=%d, master port=%s\n"
                                , method_name, __LINE__
                                , MyPNID, IntegratingMonitorPort );
                }
            }
            else
            {
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d (MasterMonitor) IsAgentMode = TRUE, I am the master, MyPNID=%d\n"
                                , method_name, __LINE__, MyPNID );
                }
                IAmIntegrating = false; 
            }
        }
        Nodes = new CNodeContainer (); 
        Config = new CConfigContainer ();
        Monitor = new CMonitor (procTermSig);  

        if ( IsAgentMode )
        {
            if (trace_settings & TRACE_INIT)
            {
                trace_printf( "%s@%d MyPNID=%d\n"
                            , method_name, __LINE__, MyPNID );
            }
            MonLog->setPNid( MyPNID );
        }
        
        if (IsAgentMode)
        {
            CNode *myNode = Nodes->GetNode(MyPNID);
            const char *masterMonitor=NULL;
            if (myNode == NULL)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf
                       , "[%s], Failed to get my Node, MyPNID=%d\n"
                       , method_name, MyPNID );
                mon_log_write(MON_MONITOR_MAIN_15, SQ_LOG_CRIT, la_buf);
                
                abort();
            }
            
            if ((ZClientEnabled) && (ZClient != NULL))
            {
                CNode *masterNode = Nodes->GetNode(MasterMonitorName);    
                if (!masterNode)
                {
                    if (trace_settings & TRACE_INIT)
                    {
                          trace_printf("%s@%d (MasterMonitor) IsMaster == %d, masterNode is NULL, with MasterMonitorName %s\n", method_name, __LINE__, IsMaster, MasterMonitorName);
                    }
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf(la_buf, "[%s], Failed to get my Master Node.\n", method_name);
                    mon_log_write(MON_MONITOR_MAIN_16, SQ_LOG_CRIT, la_buf);
                
                    abort();
                }
                else
                {
                    if (trace_settings & TRACE_INIT)
                    {
                          trace_printf("%s@%d (MasterMonitor) IsMaster == %d, masterNode=%s\n", method_name, __LINE__, IsMaster, masterNode->GetName() );
                    }
                }
                Monitor->SetMonitorLeader( masterNode->GetPNid() );
                if (MyPNID == masterNode->GetPNid())
                {
                     ZClient->CreateMasterZNode ( myNode->GetName() );
                     strcpy (MasterMonitorName, myNode->GetName());
                     if (trace_settings & TRACE_INIT)
                     {
                         trace_printf("%s@%d (MasterMonitor) IsMaster == %d, set monitor lead to %d\n", method_name, __LINE__, IsMaster, MyPNID);
                     }           
                 }
                 else
                 {
                     masterMonitor = ZClient->WaitForAndReturnMaster(true);
                     CNode *masterNode = NULL;
                     if (masterMonitor)
                     {
                         strcpy (MasterMonitorName, masterMonitor);
                         masterNode = Nodes->GetNode(MasterMonitorName); 
                     }
                
                     if (masterNode)
                     {
                          if (trace_settings & TRACE_INIT)
                          {
                              trace_printf("%s@%d (MasterMonitor) IsMaster == %d, set monitor lead to %d\n", method_name, __LINE__, IsMaster, masterNode->GetPNid());
                          } 
                          Monitor->SetMonitorLeader( masterNode->GetPNid() );
                     }
                     else
                     {
                          if (trace_settings & TRACE_INIT)
                          {
                              trace_printf("%s@%d (MasterMonitor) IsMaster == %d, masterNode is NULL, with MasterMonitorName %s\n", method_name, __LINE__, IsMaster, MasterMonitorName);
                          }
                          char la_buf[MON_STRING_BUF_SIZE];
                          sprintf(la_buf, "[%s], Failed to get my Master Node.\n", method_name);
                          mon_log_write(MON_MONITOR_MAIN_17, SQ_LOG_CRIT, la_buf);
                 
                          abort();
                     }
                }
            }
        }
        if (!IAmIntegrating)
        {
            Config->Init ();
        }
        Devices = new CDeviceContainer ();
        if ( !Devices->IsInitialized() )
        {
            if ( IAmIntegrating ) 
            {   // Problem unmounting devices, let creator monitor know then abort
                Monitor->ReIntegrate( CCluster::Reintegrate_Err13 );
            }
            else
            {
                MPI_Abort(MPI_COMM_SELF,99); // too early to call failsafe node down.
            }
        }
        nodename = new char [Monitor->GetConfigPNodesCount() * MPI_MAX_PROCESSOR_NAME];

        // Create health check thread
        HealthCheck.start();

        // Create thread to accept connections from other monitors
        CommAccept.start();
        // Open file used to record process start/end times
        Monitor->openProcessMap ();

        // Always using localio now, no other option
        SQ_theLocalIOToClient = new SQ_LocalIOToClient( MyPNID );
        assert (SQ_theLocalIOToClient);
 
        #define BLOCK_SIZE  512
        char *ioBuffer = NULL;
        int fd;
            
        rc = posix_memalign( (void**)&ioBuffer, BLOCK_SIZE, BLOCK_SIZE);
        if ( rc == -1 )
        {
            int err = rc;
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], can't allocate aligned %d bytes "
                     "buffer , Error=%d(%s)\n",
                     method_name, BLOCK_SIZE, err, ErrorMsg(err));
            mon_log_write(MON_MONITOR_MAIN_4, SQ_LOG_CRIT, buf);

            MPI_Abort(MPI_COMM_SELF,99);
        }

        memset( (void *)ioBuffer, 0 , BLOCK_SIZE );

// start ok
        if (IsRealCluster)
        {
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.port.%s",
                     getenv("MPI_TMPDIR"), short_node_name );
        }
        else
        {
            // Write out our port number so other processes can attach.
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.port.%d.%s",
                     getenv("MPI_TMPDIR"),MyPNID,Node_name);
        }

        // Change Node_name what we have in our configuration
        CNode *myNode = Nodes->GetNode(MyPNID);
        if (myNode)
        {
            strcpy (Node_name, myNode->GetName()); 
        }
        // create with no caching, user read/write, group read/write, other read
        fd = open( port_fname
                   , O_RDWR | O_TRUNC | O_CREAT | O_DIRECT 
                   , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH );
        if ( fd != -1 )
        {
            snprintf( ioBuffer, BLOCK_SIZE, "%s", MyCommPort );
            rc = write( fd, ioBuffer, BLOCK_SIZE );
            if ( rc == -1 )
            {
                int err = errno;
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf), "[%s], can't write %d bytes to "
                         "port file (%s), Error=%d(%s)\n",
                         method_name, BLOCK_SIZE, port_fname, err,
                         ErrorMsg(err));
                mon_log_write(MON_MONITOR_MAIN_5, SQ_LOG_CRIT, buf);

                if ( IAmIntegrating )
                    // This monitor is reintegrating into cluster.  Inform
                    // creator monitor of error, then abort.
                    Monitor->ReIntegrate( CCluster::Reintegrate_Err10 );
                else
                    MPI_Abort(MPI_COMM_SELF,99);
            }
            close( fd );
            if (trace_settings & TRACE_INIT)
                trace_printf("%s@%d" " Port file created, pnid=%d, port=%s" "\n", method_name, __LINE__, MyPNID, MyCommPort );
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], can't open port file (%s), "
                     "Error= %s\n", method_name, port_fname,
                     ErrorMsg(errno));
            mon_log_write(MON_MONITOR_MAIN_6, SQ_LOG_CRIT, buf);

            if ( IAmIntegrating )
                // This monitor is reintegrating into cluster.  Inform
                // creator monitor of error, then abort.
                Monitor->ReIntegrate( CCluster::Reintegrate_Err11 );
            else
                MPI_Abort(MPI_COMM_SELF,99);
        }
        free( ioBuffer );
        int ret = SQ_theLocalIOToClient->initWorker();
        if (ret)
        {
            if (trace_settings & TRACE_INIT)
               trace_printf("%s@%d" " Cannot start localio worker, aborting "  "%d" "\n", method_name, __LINE__, ret);
            delete SQ_theLocalIOToClient;
            SQ_theLocalIOToClient = NULL;
            if ( IAmIntegrating )
                // This monitor is reintegrating into cluster.  Inform
                // creator monitor of error, then abort.
                Monitor->ReIntegrate( CCluster::Reintegrate_Err12 );
            else
                assert (false);
        }

        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d" "started LocalIOToClient environment\n" "\n", method_name, __LINE__);

        if (trace_settings & TRACE_INIT)
        {
            int rc = getrlimit( RLIMIT_SIGPENDING, &Rl );
            if ( rc == 0 )
            {
                printf("%s@%d" " RLIMIT_SIGPENDING cur=%d, max=%d\n", method_name, __LINE__, (int)Rl.rlim_cur, (int)Rl.rlim_max);
            }
        }
      if ( IAmIntegrating )
        {
            // This monitor is integrating to (joining) an existing cluster
            Monitor->ReIntegrate( 0 );
            MyNode->SetPhase( Phase_Activating );

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf("%s@%d" " After UpdateCluster" "\n", method_name, __LINE__);
        }
        else
        {  
            Monitor->EnterSyncCycle();
            done = Monitor->exchangeNodeData();
            Monitor->ExitSyncCycle();

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf("%s@%d" " After ImAlive " "\n", method_name, __LINE__);
        }
    }

    // Enable Zookeeper client in real cluster only and
    // after the integration phase on a node 'up'
    if ( IsRealCluster )
    {
        if ( ZClientEnabled )
        {
            {
                StartZookeeperClient();
                // Set watch for master
                if (IsAgentMode)
                {
                    ZClient->WatchMasterNode( MasterMonitorName );
                }
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                {
                    trace_printf( "%s@%d (MasterMonitor) set watch for MasterMonitorName %s\n", method_name, __LINE__, MasterMonitorName );
                }
            }
        }
    }

    // Initialize Seabed disconnect semaphore
    char *port;
    switch( CommType )
    {
        case CommType_InfiniBand:
            port = strstr(MyCommPort, "$port#");
            if (port) port += 5;
            break;
        case CommType_Sockets:
            port = strchr(MyCommPort, ':');
            break;
        default:
            // Programmer bonehead!
            abort();
    }
    if (port != NULL)
    {
        int myPortNum;
        int semMax = 50;
        unsigned int segKey;

        myPortNum = strtol(&port[1], NULL, 10);

        env = getenv("MS_DISC_SEM");
        if ( env )
        {
            errno = 0;
            semMax = strtol(env, NULL, 10);
            if ( errno != 0) semMax = 50;
        }

        segKey = RobSem::getSegKey(0x40000000, myPortNum, MyPNID);
        rc = RobSem::create_sem(segKey, IPC_CREAT + IPC_EXCL, sbDiscSem, semMax);

        if ( rc != 0 )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Failed to create seabed disconnect semaphore: %s "
                     "(%d)\n", method_name, strerror(errno), errno);
            mon_log_write(MON_MONITOR_MAIN_10, SQ_LOG_CRIT, buf);
            sbDiscSem = NULL;
        }
    }


    // Create request worker threads
    CReqWorker::startReqWorkers();

    if ( ! IAmIntegrating )
    {
        Monitor->StartPrimitiveProcesses();
    }

    env = getenv( "SQ_USE_CPU_AFFINITY" );
    if ( env && strcmp( env, "1" ) == 0 )
    {   // Set flag to indicate that logical node CPU affinity is used for 
        // processes.
        // (see CNode::SetAffinity)
        usingCpuAffinity = true;
    }

    env = getenv( "SQ_USE_TSE_CPU_AFFINITY" );
    if ( env && strcmp( env, "1" ) == 0 )
    {   // Set flag to indicate that CPU affinity is used for TSE processes.
        // (see CNode::SetAffinity)
        usingTseCpuAffinity = true;
    }

    nice(MON_BASE_NICE);

    Monitor->setMonInitComplete(true); 

    struct timeval awakenedAt;
    struct timeval awakeTime;
    struct timeval now;
    gettimeofday(&awakenedAt, NULL);

    while (!done)
    {
        // Record statistics (sonar counters): monitor is NOT busy
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->MonitorBusyDecr();

        // Sleep for a maximum of next_test_delay.  Reduce
        // next_test_delay by the amount of time since awoke for
        // current sync cycle.  If current sync cycle took a long
        // time don't sleep at all.
        gettimeofday(&now, NULL);
        if ( (now.tv_usec - awakenedAt.tv_usec )  < 0 )
        {
            awakeTime.tv_sec = now.tv_sec - awakenedAt.tv_sec - 1;
            awakeTime.tv_usec = 1000000 + now.tv_usec - awakenedAt.tv_usec;
        }
        else
        {
            awakeTime.tv_sec = now.tv_sec - awakenedAt.tv_sec;
            awakeTime.tv_usec = now.tv_usec - awakenedAt.tv_usec;
        }

        if ( awakeTime.tv_sec == 0
             && awakeTime.tv_usec < next_test_delay)
        {
            usleep( next_test_delay - awakeTime.tv_usec );
        }
        gettimeofday(&awakenedAt, NULL);

        // Record statistics (sonar counters): monitor is busy
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->MonitorBusyIncr();

        Monitor->EnterSyncCycle();
        if ( Monitor->TmSyncPending() )
        {
            Monitor->TmSync ();
        }
        Monitor->ExitSyncCycle();

        if ( !Monitor->GetPendingSlaveTmSync() &&
             Monitor->GetTotalSlaveTmSyncCount() == 0 )
        {
            Monitor->EnterSyncCycle();
            done = Monitor->exchangeNodeData();
            Monitor->ExitSyncCycle();
        }

        if (done)
            break; 


        // Check to see if 'ckillall' is executing and disable the watchdog
        if ( !SQ_theLocalIOToClient->isWDTEnabled() )
        {
           // HealthCheck.setState(MON_EXIT_WATCHDOG);
        }

    }

    if (trace_settings & TRACE_STATS)
    {   // Write malloc statistics info to stderr
        monMallocStats();
    }

    if ( ZClientEnabled )
    {
        ZClient->StopMonitoring();
        ZClient->ShutdownWork();
    }

    Redirector.shutdownWork();

    // shut down health check thread before shutting down reqWorker thread.
    HealthCheck.shutdownWork();

    ReqQueue.stats();
    // Stop request worker threads
    CReqWorker::shutdownWork();
    Replicator.stats();

    Monitor->stats();

    // Tell the LIO worker threads to exit
    SQ_theLocalIOToClient->shutdownWork();

    CommAccept.shutdownWork();

    // Rename the monitor "port" file
    sprintf(temp_fname, "%s.bak", port_fname);
    remove(temp_fname);
    rename(port_fname, temp_fname);

    delete [] nodename;
    delete Devices;
    delete Nodes;
    delete ZClient;
    delete Monitor;
    Monitor = NULL; // TRACE uses this
    delete Config;

    if ( sbDiscSem != NULL )
    {
        RobSem::destroy_sem( sbDiscSem );
    }
    if ( CommType == CommType_InfiniBand )
    {
        MPI_Close_port( MyCommPort );
    } 
#if 0
    // TODO: MPICH cannot handle a node down and subsequent shutdown
    //       MPI_Finalize() hangs so its currently disabled, but
    //       causes an abnormal termination in the monitor process at exit.
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d" "- Calling MPI_Finalize()" "\n", method_name, __LINE__);
    MPI_Finalize ();
#endif
    if (trace_settings & TRACE_STATS)
    {
      trace_printf("%s@%d" "- LIO Stats: shared_buffers_total="  "%d" "\n", method_name, __LINE__, SQ_theLocalIOToClient->getSharedBufferCount());
      trace_printf("%s@%d" "- LIO Stats: shared_buffers_acquired_max="  "%d" "\n", method_name, __LINE__, SQ_theLocalIOToClient->getAcquiredBufferCount());
      trace_printf("%s@%d" "- LIO Stats: shared_buffers_available_min="  "%d" "\n", method_name, __LINE__, SQ_theLocalIOToClient->getAvailableBufferCount());
      trace_printf("%s@%d" "- LIO Stats: shared_buffer_misses="  "%d" "\n", method_name, __LINE__, SQ_theLocalIOToClient->getMissedBufferCount());
      trace_printf("%s@%d" "- LIO Stats: max queued child death=%d\n",
                   method_name, __LINE__,
                   SQ_theLocalIOToClient->getMaxChildDeathCount());
      trace_printf("%s@%d" "- LIO Stats: almost-dead pids=%d\n",
                   method_name, __LINE__,
                   SQ_theLocalIOToClient->getAlmostDeadPids());
      trace_printf("%s@%d" "- LIO Stats: verifierMap="  "%d" "\n", method_name, __LINE__, SQ_theLocalIOToClient->getVerifierMapCount());
    }
    delete SQ_theLocalIOToClient;

    snprintf(buf, sizeof(buf), "[CMonitor::main], Shutdown normally.\n");
    mon_log_write(MON_MONITOR_MAIN_11, SQ_LOG_INFO, buf);

    if (trace_settings & TRACE_STATS)
    {
        if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
           MonStats->displayStats();
    }

    // Record statistics (sonar counters): monitor is NOT busy
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->MonitorBusyDecr();

    delete MonLog;

    return 0;
}

#ifdef DELAY_TP
void CMonitor::Delay_TP(char *tpName)
{
    char nodename[20];
    char keyname[MAX_KEY_NAME];
    char valueC[MAX_VALUE_SIZE];
    int value;

    if (Config)
    {  
       snprintf(nodename, sizeof(nodename), "NODE%d",MyPNID);
       CConfigGroup *group = Config->GetGroup(nodename);
       if (group)
       {  
          strcpy(keyname,"REQUESTDELAY_TP");
          CConfigKey *key = group->GetKey(keyname);
          if (key)
          {
            strcpy(valueC,key->Value);
            value = atoi(valueC);
            sleep(value);
          }
       }
    }
}
#endif
