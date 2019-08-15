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
#include "nameserverconfig.h"
#include "lnode.h"
#include "pnode.h"
#include "cluster.h"
#include "monitor.h"
#include "props.h"

#ifdef DMALLOC
#include "dm.h"
#endif
#include "replicate.h"
#include "robsem.h"
#include "commaccept.h"
#ifdef NAMESERVER_PROCESS
#include "nscommacceptmon.h"
#endif

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
#include "nameserver.h"
#include "meas.h"

#include "reqqueue.h"
#include "reqworker.h"
#include "zclient.h"

#include "SCMVersHelp.h"

#ifndef NAMESERVER_PROCESS
#include "ptpcommaccept.h"
#include "ptpclient.h"
#endif

#define LOG_ERROR
#define RidRecvPrioritySlot 4096
#define MinimumSlotToPrioritize 4096


// Global Variables
struct rlimit Rl;
bool PidMap = false;
bool usingCpuAffinity = false;
bool usingTseCpuAffinity = false;
bool genSnmpTrapEnabled = false;
int Measure = 0;
long trace_level = 0;
char MyPath[MAX_PROCESS_PATH];
char MyCommPort[MPI_MAX_PORT_NAME] = {'\0'};
char MyMPICommPort[MPI_MAX_PORT_NAME] = {'\0'};
char MySyncPort[MPI_MAX_PORT_NAME] = {'\0'};
#ifdef NAMESERVER_PROCESS
char MyMon2NsPort[MPI_MAX_PORT_NAME] = {'\0'};
#else
char MyPtPPort[MPI_MAX_PORT_NAME] = {'\0'};
#endif
char Node_name[MPI_MAX_PROCESSOR_NAME] = {'\0'};
sigset_t SigSet;
bool Emulate_Down = false;
#ifdef NAMESERVER_PROCESS
long next_test_delay = 10000; // in usec. (default 10 msec)
#else
long next_test_delay = 100000; // in usec. (default 100 msec)
#endif
CClusterConfig *ClusterConfig = NULL;
bool IAmIntegrating = false;
bool IAmIntegrated = false;
char IntegratingMonitorPort[MPI_MAX_PORT_NAME] = {'\0'};
bool IsRealCluster = true;
bool IsAgentMode = false;
bool IsNameServer = false;
AgentType_t AgentType = AgentType_Undefined;
bool IsMaster = false;
bool IsMPIChild = false;
char MasterMonitorName[MAX_PROCESS_PATH]= {'\0'};
CommType_t CommType = CommType_Undefined;
bool SMSIntegrating = false;
int  CreatorShellPid = -1;
Verifier_t CreatorShellVerifier = -1;
bool SpareNodeColdStandby = true;
bool ZClientEnabled = true;
#ifndef NAMESERVER_PROCESS
bool NameServerEnabled = false;
#endif
int  ClusterId = -1;
int  InstanceId = -1;

// Lock to manage memory modifications during fork/exec
CLock MemModLock;
CMonitor *Monitor = NULL;
#ifndef NAMESERVER_PROCESS
CNameServer *NameServer = NULL;
CProcess *NameServerProcess = NULL;
CPtpClient *PtpClient = NULL;
#endif
CNodeContainer *Nodes = NULL;
CConfigContainer *Config = NULL;
CNameServerConfigContainer *NameServerConfig = NULL;
#ifndef NAMESERVER_PROCESS
CDeviceContainer *Devices = NULL;
#endif
int MyPNID = -1;
CNode *MyNode;
CMonLog *MonLog =  NULL;
CMonStats * MonStats = NULL;
extern CMonTrace *MonTrace;
#ifndef NAMESERVER_PROCESS
CRedirector Redirector;
#endif
CIntProcess IntProcess;
CReqQueue ReqQueue;
CHealthCheck HealthCheck;
CCommAccept CommAccept;
#ifdef NAMESERVER_PROCESS
CCommAcceptMon CommAcceptMon;
#else
CPtpCommAccept PtpCommAccept;
#endif
extern CReplicate Replicator;
CZClient  *ZClient = NULL;
#ifndef NAMESERVER_PROCESS
// Seabed disconnect semaphore
RobSem * sbDiscSem = NULL;
#endif
int monitorArgc = 0;
char monitorArgv[MAX_ARGS][MAX_ARG_SIZE];
CMeas Meas;


#ifdef NAMESERVER_PROCESS
DEFINE_EXTERN_COMP_DOVERS(trafns)
DEFINE_EXTERN_COMP_GETVERS2(trafns)
#else
DEFINE_EXTERN_COMP_DOVERS(monitor)
DEFINE_EXTERN_COMP_GETVERS2(monitor)
#endif


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

#ifndef NAMESERVER_PROCESS
void sigtermSignalHandler(int signal, siginfo_t *info, void *)
{
    const char method_name[] = "sigtermSignalHandler";

    if (trace_settings & TRACE_ENTRY_EXIT)
       trace_nolock_printf("%s@%d\n", method_name, __LINE__);

    if (trace_settings & TRACE_SIG_HANDLER)
        trace_nolock_printf("%s@%d - signal=%d, code=%d, status=%d, pid=%d\n",
                            method_name, __LINE__, signal, info->si_code,
                            info->si_status, info->si_pid);

    char la_buf[MON_STRING_BUF_SIZE*2];
    snprintf( la_buf, sizeof(la_buf)
            , "[%s], Initiating node down on Node %s, pnid=%d (received SIGTERM signal)\n"
            , method_name
            , MyNode->GetName()
            , MyPNID );
    mon_log_write(MON_MONITOR_SIGTERMSIGNALHANDLER_1, SQ_LOG_CRIT, la_buf); 

    Monitor->HardNodeDown( MyPNID, true );

    if (trace_settings & TRACE_ENTRY_EXIT)
        trace_nolock_printf("%s@%d - Exit\n", method_name, __LINE__);
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
#endif

void monMallocStats()
{
    // Log current malloc statistics in stderr
    time_t mytime = time(NULL);
    char *timestamp = ctime(&mytime);
    timestamp[strlen(timestamp)-1] = '\0';

    printf("monitor malloc statistics at %s:\n", timestamp);
    malloc_stats();
}

const char *AgentTypeString( AgentType_t agentType)
{
    const char *str;
    
    switch( agentType )
    {
        case AgentType_Ambari:
            str = "Ambari";
            break;
        case AgentType_CM:
            str = "CM";
            break;
        case AgentType_MPI:
            str = "MPI";
            break;
        default:
            str = "Undefined";
            break;
    }

    return( str );
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


#ifdef NAMESERVER_PROCESS
CMonitor::CMonitor ()
    : CCluster (),
#else
CMonitor::CMonitor (int procTermSig)
    : CCluster (),
#endif
      OpenCount (0)
    , NoticeCount (0)
    , ProcessCount (0)
    , NumOutstandingIO (0)
    , NumOutstandingSends (0)
    , Last_error (MPI_SUCCESS)
#ifndef NAMESERVER_PROCESS
    , processMapFd ( -1 )
    , procTermSig_ ( procTermSig )
#endif
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

#ifndef NAMESERVER_PROCESS
    if ( processMapFd != -1)
    {
        close ( processMapFd );
    }
#endif

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

#ifndef NAMESERVER_PROCESS
void CMonitor::openProcessMap ( void )
{
    char fname[MAX_PROCESS_PATH];

    char *env;
    env = getenv("SQ_PIDMAP");
    if ( env && *env == '1' )
    {
        PidMap = true;
    }

    if ( IsRealCluster )
    {
        snprintf( fname, sizeof(fname), "%s/monitor.map.%s",
                 getenv("TRAF_LOG"), Node_name );
    }
    else
    {
        snprintf( fname, sizeof(fname), "%s/monitor.map.%d.%s",
                 getenv("TRAF_LOG"), MyPNID, Node_name );
    }
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
#endif

#ifndef NAMESERVER_PROCESS
void CMonitor::writeProcessMapEntry ( const char * buf )
{
    if ( processMapFd != -1 )
        write( processMapFd, buf, strlen(buf));
}
#endif

#ifndef NAMESERVER_PROCESS
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
#endif

#ifndef NAMESERVER_PROCESS
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
#endif

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
                                     NULL,
                                     -1 );
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
#ifndef NAMESERVER_PROCESS
        CProcessContainer::ParentNewProcReply( process, MPI_SUCCESS);
        status = SUCCESS;
#endif
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
#ifndef NAMESERVER_PROCESS
    procObj->pathStrId = process->pathStrId();
    procObj->ldpathStrId = process->ldPathStrId();
    procObj->programStrId = process->programStrId();
#endif
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

#ifdef NAMESERVER_PROCESS
    if (strlen(process->path()))
    {
        // Copy the path
        procObj->pathLen = strlen(process->path()) + 1;
        memcpy(stringData, process->path(),  procObj->pathLen );
        stringData += procObj->pathLen;
        stringDataLen = procObj->pathLen;
    }
    else
    {
        procObj->pathLen = 0;
    }

    if (strlen(process->ldpath()))
    {
        // Copy the ldpath
        procObj->ldpathLen = strlen(process->ldpath()) + 1;
        memcpy(stringData, process->ldpath(),  procObj->ldpathLen );
        stringData += procObj->ldpathLen;
        stringDataLen = procObj->ldpathLen;
    }
    else
    {
        procObj->ldpathLen = 0;
    }

    if (strlen(process->program()))
    {
        // Copy the program
        procObj->programLen = strlen(process->program()) + 1;
        memcpy(stringData, process->program(),  procObj->programLen );
        stringData += procObj->programLen;
        stringDataLen = procObj->programLen;
    }
    else
    {
        procObj->programLen = 0;
    }
#endif
    
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
    }
    else
    {
        procObj->infileLen = 0;
        procObj->outfileLen = 0;
        procObj->argvLen = 0;
        procObj->persistent = false;
    }

#ifdef NAMESERVER_PROCESS
    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Packing process string data:\n"
                          "        name(%d)       =%s\n"
                          "        port(%d)       =%s\n"
                          "        path(%d)       =%s\n"
                          "        ldpath(%d)     =%s\n"
                          "        program(%d)    =%s\n"
                          "        infile(%d)     =%s\n"
                          "        outfile(%d)    =%s\n"
                          "        userArgv(%d)   =%s\n"
                          "        stringData(%d) =%s\n"
                        , method_name, __LINE__
                        , procObj->nameLen
                        , process->GetName()
                        , procObj->portLen
                        , process->GetPort()
                        , procObj->pathLen
                        , process->path()
                        , procObj->ldpathLen
                        , process->ldpath()
                        , procObj->programLen
                        , process->program()
                        , procObj->infileLen
                        , process->infile()
                        , procObj->outfileLen
                        , process->outfile()
                        , procObj->argvLen
                        , procObj->argvLen?process->userArgv():""
                        , stringDataLen
                        , stringDataLen?&procObj->stringData:"" );
#else
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
#endif

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

#ifndef NAMESERVER_PROCESS
    if (!NameServerEnabled)
#endif
    {
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
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
    {
        long avg = 0;
        if ( procCount > 0 )
            avg = (buffer - bufPtr) / procCount;
        trace_printf("%s@%d - Total Procs = %d, Total Size = %ld, Avg = %ld bytes\n",
                     method_name, __LINE__, procCount, buffer - bufPtr, avg);
    }

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
#ifdef NAMESERVER_PROCESS
    char *path = NULL;
    char *ldpath = NULL;
    char *program = NULL;
#endif
    char *name = NULL;
    char *port = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    char *userargv = NULL;
    char *stringData = NULL;

    struct clone_def *procObj;

    int i;

    for (i = 0; i < procCount; i++)
    {
        procObj = (struct clone_def *)buffer;

        stringDataLen = 0;
        stringData = &procObj->stringData;

        node = Nodes->GetLNode (procObj->nid)->GetNode();

        if (procObj->nameLen)
        {
            name = &stringData[stringDataLen];
            stringDataLen += procObj->nameLen;
        }

        if (procObj->portLen)
        {
            port = &stringData[stringDataLen];
            stringDataLen += procObj->portLen;
        }

#ifdef NAMESERVER_PROCESS
        if (procObj->pathLen)
        {
            path = &stringData[stringDataLen];
            stringDataLen += procObj->pathLen;
        }

        if (procObj->ldpathLen)
        {
            ldpath = &stringData[stringDataLen];
            stringDataLen += procObj->ldpathLen;
        }

        if (procObj->programLen)
        {
            program = &stringData[stringDataLen];
            stringDataLen += procObj->programLen;
        }
#endif

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

#ifdef NAMESERVER_PROCESS
        if (trace_settings & (TRACE_REQUEST | TRACE_INIT | TRACE_RECOVERY))
                trace_printf( "%s@%d - Unpacking process string data:\n"
                              "        stringData(%d) =%s\n"
                              "        name(%d)       =%s\n"
                              "        port(%d)       =%s\n"
                              "        path(%d)       =%s\n"
                              "        ldpath(%d)     =%s\n"
                              "        program(%d)    =%s\n"
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
                            , procObj->pathLen
                            , procObj->pathLen?path:""
                            , procObj->ldpathLen
                            , procObj->ldpathLen?ldpath:""
                            , procObj->programLen
                            , procObj->programLen?program:""
                            , procObj->infileLen
                            , procObj->infileLen?infile:""
                            , procObj->outfileLen
                            , procObj->outfileLen?outfile:""
                            , procObj->argc
                            , procObj->argvLen
                            , procObj->argvLen?userargv:"" );
#else
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
#endif

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
#ifdef NAMESERVER_PROCESS
                                      path,
                                      ldpath,
                                      program,
#else
                                      procObj->pathStrId,
                                      procObj->ldpathStrId,
                                      procObj->programStrId,
#endif
                                      procObj->infileLen?infile:(char *)"",
                                      procObj->outfileLen?outfile:(char *)"",
                                      &procObj->creation_time,
                                      procObj->origPNidNs);

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

#ifndef NAMESERVER_PROCESS
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
#endif

void HandleMyNodeExpiration( void )
{
    const char method_name[] = "HandleMyNodeExpiration";
    TRACE_ENTRY;
    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (!MyNode->IsPendingNodeDown())
        {
            ReqQueue.enqueueDownReq(MyPNID);
        }
    }
    TRACE_EXIT;
}

void HandleNodeChange( const char *nodeName )
{
    const char method_name[] = "HandleNodeChange";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling CHANGE for node=%s\n"
                        , method_name, __LINE__
                        , nodeName );
        }
    }

    TRACE_EXIT;
}

void HandleNodeConfigurationChange( void )
{
    const char method_name[] = "HandleNodeConfigurationChange";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling CONFIGURATION CHANGE\n"
                        , method_name, __LINE__ );
        }
    }

    TRACE_EXIT;
}

void HandleNodeError( const char *nodeName )
{
    const char method_name[] = "HandleNodeError";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling ERROR for node=%s\n"
                        , method_name, __LINE__
                        , nodeName );
        }

        CNode *node = Nodes->GetNode((char *)nodeName);
        if (node)
        {
            if (node->GetState() != State_Down)
            {
                node->SetPendingNodeDown(true);
            }
            ReqQueue.enqueueDownReq(node->GetPNid());
        }
    }
    TRACE_EXIT;
}

void HandleNodeChild( const char *nodeName )
{
    const char method_name[] = "HandleNodeChild";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling CHILD for node=%s\n"
                        , method_name, __LINE__
                        , nodeName );
        }
    }
    TRACE_EXIT;
}

void HandleNodeCreated( const char *nodeName )
{
    const char method_name[] = "HandleNodeCreated";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling CREATE for node=%s\n"
                        , method_name, __LINE__
                        , nodeName );
        }
    }
    TRACE_EXIT;
}

void HandleNodeExpiration( const char *nodeName )
{
    const char method_name[] = "HandleNodeExpiration";
    TRACE_ENTRY;

    if (ZClientEnabled )
    {
        if (ZClient->StateGet() == CZClient::ZC_SHUTDOWN)
        {
            return;
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d Handling EXPIRATION of node=%s\n"
                        , method_name, __LINE__
                        , nodeName );
        }

        CNode *node = Nodes->GetNode((char *)nodeName);
        if (node)
        {
            if (node->GetState() != State_Down)
            {
                node->SetPendingNodeDown(true);
            }
            ReqQueue.enqueueDownReq(node->GetPNid());
        }
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
        string       instanceId;
        string       trafodionRootZNode;
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

        env = getenv("TRAF_ROOT_ZNODE");
        if ( env )
        {
            stringstream ss;
            ss.str( "" );
            ss << env;
            trafodionRootZNode = ss.str();
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Environment variable TRAF_ROOT_ZNODE is undefined, exiting!\n", method_name);
            mon_log_write(MON_MONITOR_CREATEZCLIENT_3, SQ_LOG_CRIT, la_buf);
 
            printf( "%s", la_buf);
            exit(EXIT_FAILURE);
        }

        if (InstanceId != -1)
        {
            stringstream ss;
            ss.str( "" );
            ss << "/" << InstanceId;
            instanceId = ss.str();
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Instance Id is undefined, exiting!\n", method_name);
            mon_log_write(MON_MONITOR_CREATEZCLIENT_4, SQ_LOG_CRIT, la_buf);
 
            printf( "%s", la_buf);
            exit(EXIT_FAILURE);
        }

        ZClient = new CZClient( zkQuorumPort.str().c_str()
                              , trafodionRootZNode.c_str()
                              , instanceId.c_str() );
        if ( ZClient == NULL )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Failed to allocate ZClient object!\n"
                    , method_name);
            mon_log_write(MON_MONITOR_CREATEZCLIENT_5, SQ_LOG_CRIT, buf);
            mon_failure_exit();
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
    int configMasterPNid = -1;
    bool done = false;
    char *env;
    char *nodename = NULL;
    char fname[MAX_PROCESS_PATH];
    char short_node_name[MPI_MAX_PROCESSOR_NAME];
#ifndef NAMESERVER_PROCESS
    int portFileOpenDelay = DEFAULT_PORTFILEOPEN_DELAY;
    char port_fname[MAX_PROCESS_PATH];
    char temp_fname[MAX_PROCESS_PATH];
#endif
    char buf[MON_STRING_BUF_SIZE];
#ifndef NAMESERVER_PROCESS
    unsigned int initSleepTime = 1; // 1 second
#endif

    mallopt(M_ARENA_MAX, 4); // call to limit the number of arena's of  monitor to 4.This call doesn't seem to have any effect !

#ifdef NAMESERVER_PROCESS
    CALL_COMP_DOVERS(trafns, argc, argv);
#else
    CALL_COMP_DOVERS(monitor, argc, argv);
#endif

    const char method_name[] = "main";

#ifdef NAMESERVER_PROCESS
    IsNameServer = true;
#endif

    if (argc < 2) {
      printf("error: monitor needs an argument...exitting...\n");
      exit(EXIT_FAILURE);
    }

    int lv_arg_index = 1;
    while ( lv_arg_index < argc )
    {
        // In installations like Cloudera Manager, the monitor is started in AGENT mode
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
        if ( IsRealCluster )
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
#ifdef NAMESERVER_PROCESS
        else
        {
            env = getenv("SQ_MON_RUN_MODE");
            if ( env != NULL && strcmp(env, "AGENT") == 0 )
            {
                IsAgentMode = true;
            }
        }
#endif
    }

#ifndef NAMESERVER_PROCESS
    env = getenv("SQ_NAMESERVER_ENABLED");
    if ( env && isdigit(*env) )
    {
        int val = atoi(env);
        NameServerEnabled = (val != 0) ? true : false;
    }
#endif

    if ( IsAgentMode || IsNameServer )
    {
        MON_Props xprops( true );
        char *envfile;
        env = getenv( "TRAF_CONF" );
        if (env)
        {
           envfile = new char [strlen(env)+16];
           strcpy(envfile, env);
        } else {
           envfile = new char [17];
           strcpy(envfile, ".");
        }
#ifdef NAMESERVER_PROCESS
        strcat(envfile, "/nameserver.env");
#else
        strcat(envfile, "/monitor.env");
#endif
        xprops.load( envfile );
        delete [] envfile;
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
    for (i = strlen(argv[0])-1; i >= 0; i--)
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
                 getenv("TRAF_LOG"),getpid());
        setenv("MPI_INSTR", fname, 1);
    }
    if ( env && *env == '2' )
    {
        Measure = 2;
        snprintf(fname, sizeof(fname), "%s/monitor.cpu.P%d:cpu",
                 getenv("TRAF_LOG"),getpid());
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

    env = getenv("TRAF_AGENT");
    if ( env != NULL && strcmp(env, "CM") == 0 )
    {
        AgentType = AgentType_CM;
    }
    else if ( env != NULL && strcmp(env, "Ambari") == 0 )
    {
        AgentType = AgentType_Ambari;
    }
    else
    {
        AgentType = AgentType_MPI;
    }

#ifdef NAMESERVER_PROCESS
    env = getenv("NS_GENERATE_CORE_ON_FAILURE_EXIT");
#else
    env = getenv("MON_GENERATE_CORE_ON_FAILURE_EXIT");
#endif
    if ( env )
    {
        if ( *env == '0' )
        {
            GenCoreOnFailureExit = false;
        }
        else
        {
            GenCoreOnFailureExit = true;
        }
    }

#ifndef NAMESERVER_PROCESS
    if (AgentType == AgentType_CM)
    {
        // Set sigaction such that SIGTERM signal is caught.
        // In a Cloudera Manager (CM) environment, the supervisord
        // will send the monitor (NODE ROLE) a SIGTERM to indicate
        // is time to shutdown the instance
        struct sigaction act;
        act.sa_sigaction = sigtermSignalHandler;
        act.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigemptyset (&act.sa_mask);
        sigaddset (&act.sa_mask, SIGTERM);
        sigaction (SIGTERM, &act, NULL);
    }
#endif

    // Mask all allowed signals
    sigset_t              mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGPROF); // allows profiling such as google profiler
    sigdelset(&mask, SIGUSR2);
#ifndef NAMESERVER_PROCESS
    sigdelset(&mask, SIGTERM);
#endif
    rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        printf( "[%s],  Unable to set signal mask, "
                "pthread_sigmask() error=%d (%s)\n"
               ,method_name, rc, strerror(rc) );
        exit(EXIT_FAILURE);
    }

    // Setup HP_MPI software license
    int key = 413675219; //413675218 to display banner
    MPI_Initialized(&key);

    // Initialize MPI environment
    MPI_Init (&argc, &argv);

#ifdef NAMESERVER_PROCESS
    if ( argc > 1 )
    {
        if ( strcmp(argv[1], "SQMON1.1") == 0 )
        {
            MyPNID = atoi( argv[2] );
            int arg = 1;
            for ( ; argv[arg+11]; arg++ )
            {
                argv[arg] = argv[arg+11];
            }
            argv[arg] = NULL;
            argc -= 11;
        }
    }
#else
    monitorArgc = argc;
    STRCPY(monitorArgv[0], "trafns");
    for ( int arg = 1; arg < argc; arg++ )
        STRCPY(monitorArgv[arg], argv[arg]);
#endif

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

#ifndef NAMESERVER_PROCESS
    env = getenv("MON_INIT_SLEEP");
    if ( env && isdigit(*env) )
    {
        initSleepTime = atoi(env);
    }
#endif

    env = getenv("SQ_COLD_STANDBY_SPARE");
    if ( env && isdigit(*env) )
    {
        if ( strcmp(env,"0")==0 )
        {
            SpareNodeColdStandby = false;
        }
    }

#ifndef NAMESERVER_PROCESS
    env = getenv("MON_PORT_OPEN_FILE_DELAY");
    if ( env && isdigit(*env) )
    {
        portFileOpenDelay = atoi(env);
        if (portFileOpenDelay < MIN_PORTFILEOPEN_DELAY)
        {
            portFileOpenDelay = MIN_PORTFILEOPEN_DELAY;
        }
        else if (portFileOpenDelay > MAX_PORTFILEOPEN_DELAY)
        {
            portFileOpenDelay = MAX_PORTFILEOPEN_DELAY;
        }
    }

    // We need to delay some to make sure all monitor processes have initialized before
    // any monitor tries to perform an Allgather operation.
    sleep( initSleepTime );
#endif

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
#ifndef NAMESERVER_PROCESS
    MPI_Comm_rank (MPI_COMM_WORLD, &MyPNID);
#endif

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

    if (IsRealCluster) 
    {
      strcpy (Node_name, short_node_name );
    }

#ifdef MULTI_TRACE_FILES
    setThreadVariable( (char *)"mainThread" );
#endif

#ifdef NAMESERVER_PROCESS
    MonLog = new CMonLog( "log4cxx.monitor.trafns.config", "NS", "alt.mon", MyPNID, -1, getpid(), "$TNS" );
#else
    MonLog = new CMonLog( "log4cxx.monitor.mon.config", "MON", "alt.mon", MyPNID, -1, getpid(), "$MONITOR" );
#endif
    MonLog->setPNid( MyPNID );
    MonLog->setupInMemoryLog();

#ifdef NAMESERVER_PROCESS
    // Without mpi daemon the monitor has no default standard output.
    // We create a standard output file here.
    if ( IsRealCluster )
    {
        snprintf(fname, sizeof(fname), "%s/trafns.%s.log",
                 getenv("TRAF_LOG"), Node_name);
    }
    else
    {
        snprintf(fname, sizeof(fname), "%s/trafns.%d.%s.log",
                 getenv("TRAF_LOG"), MyPNID, Node_name);
    }
#else
    // Without mpi daemon the monitor has no default standard output.
    // We create a standard output file here.
    if ( IsRealCluster )
    {
        snprintf(fname, sizeof(fname), "%s/sqmon.%s.log",
                 getenv("TRAF_LOG"), Node_name);
    }
    else
    {
        snprintf(fname, sizeof(fname), "%s/sqmon.%d.%s.log",
                 getenv("TRAF_LOG"), MyPNID, Node_name);
    }
#endif
    if( freopen(fname, "a", stdout) == NULL )
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], can't open stdout (%s).\n",
                 method_name, fname);
        mon_log_write(MON_MONITOR_MAIN_2, SQ_LOG_ERR, buf);
    }
    setlinebuf(stdout);

#ifndef NAMESERVER_PROCESS
    // Send stderr output to same file as stdout.  (Note: the monitor does
    // not write to stderr but perhaps there could be components included in
    // the monitor build that do write to stderr.)
    if ( dup2 ( 1, 2 ) == -1 )
    {
        printf ( "dup2 failed for stderr: %s (%d)\n", strerror(errno), errno);
    }
#else
    // Name Server is a child process of the monitor, the process create logic
    // will establish IO redirection between the monitor process and the child.
#endif

    switch( CommType )
    {
        case CommType_Sockets: // Valid communication protocol
            break;
        case CommType_InfiniBand: // Currenly disabled - requires HA MPI
            //MPI_Open_port (MPI_INFO_NULL, MyMPICommPort);
        default:
            printf( "SQ_IC contains invalid communication protocol: %s\n"
                   , CommTypeString(CommType));
            exit(EXIT_FAILURE);
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
                    exit(EXIT_FAILURE);
                }
                break;
            case CommType_Sockets:
                if ( IsAgentMode || isdigit (*argv[3]) )
                {
                    // In agent mode and when re-integrating (node up), all
                    // monitors processes start as a cluster of 1 and join to the
                    // creator monitor to establish the real cluster.
                    SMSIntegrating = IAmIntegrating = true;
                    strcpy( IntegratingMonitorPort, argv[3] );
                }
                else
                {
                    printf ( "Invalid integrating monitor socket port: %s\n", argv[3]);
                    exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
        }
        if ( isdigit (*argv[5]) )
        {

            CreatorShellVerifier = atoi( argv[5] );
        }
        else
        {
            printf ( "Invalid creator shell verifier: %s\n", argv[5]);
            exit(EXIT_FAILURE);
        }

        // Trace cannot be specified on startup command but need to
        // check for trace environment variable settings.
        MonTrace->mon_trace_init("0", NULL);

    }

    if ( IsRealCluster )
    {
        // Set MyPNID to -1 to use the node name to obtain the correct
        // <pnid> from the configuration which occurs when creating the
        // CMonitor object down below. By setting MyPNID to -1, when the
        // CCluster::InitializeConfigCluster() invoked during the creation
        // of the CMonitor object it will set MyPNID using Node_name.
        MyPNID = -1;
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

    const char message_tag[] = "Trafodion";
    snprintf( buf, sizeof(buf), "[%s] - monitor Started!\n", message_tag );
    mon_log_write(MON_MONITOR_MAIN_3, SQ_LOG_INFO, buf);
#ifdef NAMESERVER_PROCESS
    snprintf(buf, sizeof(buf), "[%s] - %s, Started! CommType: %s\n"
                , message_tag
                , CALL_COMP_GETVERS2(trafns), CommTypeString( CommType ));
#else
    snprintf(buf, sizeof(buf), "[%s] - %s, Started! CommType: %s (%s%s%s%s%s)\n"
                , message_tag
                , CALL_COMP_GETVERS2(monitor)
                , CommTypeString( CommType )
                , IsRealCluster?"RealCluster":"VirtualCluster"
                , IsAgentMode?"/AgentMode:":""
                , IsAgentMode?AgentTypeString( AgentType ):""
                , IsMPIChild?"/MPIChild":""
                , NameServerEnabled?"/NameServerEnabled":"" );
#endif
    mon_log_write(MON_MONITOR_MAIN_3, SQ_LOG_INFO, buf);
    snprintf( buf, sizeof(buf), "[%s] - monitor Started!\n", message_tag );
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
#ifndef NAMESERVER_PROCESS
        // Create thread for monitoring redirected i/o.
        // This is also used for monitor logs, so start it early.
        Redirector.start();
#endif

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

                     mon_failure_exit();
                }
            }
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Failed to open cluster configuration.\n", method_name);
                mon_log_write(MON_MONITOR_MAIN_13, SQ_LOG_CRIT, la_buf);

                mon_failure_exit();
            }
        }
       else
       {
           char la_buf[MON_STRING_BUF_SIZE];
           sprintf(la_buf, "[%s], Failed to allocate cluster configuration.\n", method_name);
           mon_log_write(MON_MONITOR_MAIN_14, SQ_LOG_CRIT, la_buf);

           mon_failure_exit();
        }

        //Moved creation of the below to later on
        //Nodes = new CNodeContainer (); 
        //Config = new CConfigContainer ();
        //Monitor = new CMonitor (procTermSig);
        
        ClusterId = ClusterConfig->GetClusterId();
        InstanceId = ClusterConfig->GetInstanceId();

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
            configMasterPNid = ClusterConfig->GetConfigMaster();
            if ((ZClientEnabled) && (ZClient != NULL))
            {
                // Do not wait, just see if one exists
                const char *masterMonitor = ZClient->MasterWaitForAndReturn(false);

                if (masterMonitor)
                {
                    strcpy (MasterMonitorName, masterMonitor);

                    if (trace_settings & TRACE_INIT)
                    {
                        trace_printf("%s@%d (MasterMonitor) IsAgentMode=%s, IAmIntegrating=%s, masterMonitor from ZK: %s, Node_name: %s\n"
                                     , method_name
                                     , __LINE__
                                     , IsAgentMode?"TRUE":"FALSE"
                                     , IAmIntegrating?"TRUE":"FALSE"
                                     , MasterMonitorName
                                     , Node_name);
                    }

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

                    if (trace_settings & TRACE_INIT)
                    {
                        trace_printf("%s@%d (MasterMonitor) IsAgentMode=%s, IAmIntegrating=%s, ConfigMasterMonitor: %s, Node_name:%s \n"
                                     , method_name
                                     , __LINE__
                                     , IsAgentMode?"TRUE":"FALSE"
                                     , IAmIntegrating?"TRUE":"FALSE"
                                     , MasterMonitorName
                                     , Node_name);
                    }

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
#ifdef NAMESERVER_PROCESS
             else
             {
                strcpy (MasterMonitorName, ClusterConfig->GetConfigMasterByName());

                if (trace_settings & TRACE_INIT)
                {
                    trace_printf("%s@%d (MasterMonitor) IsAgentMode=%s, IAmIntegrating=%s, ConfigMasterMonitor: %s, Node_name:%s \n"
                                 , method_name
                                 , __LINE__
                                 , IsAgentMode?"TRUE":"FALSE"
                                 , IAmIntegrating?"TRUE":"FALSE"
                                 , MasterMonitorName
                                 , Node_name);
                }

                if ( IsRealCluster )
                {
                    if (strcmp (Node_name,  ClusterConfig->GetConfigMasterByName()) == 0)
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
                     IsMaster = ( MyPNID == 0 );
                 }
             }
#endif
         }

         if (IsAgentMode)
         {
            if (!IsMaster)
            {
                SMSIntegrating = IAmIntegrating = true;
#ifdef NAMESERVER_PROCESS
                char *monitorPort = getenv ("NS_COMM_PORT");
#else
                char *monitorPort = getenv ("MONITOR_COMM_PORT");
#endif
                if (monitorPort)
                {
#ifdef NAMESERVER_PROCESS
                    if ( IsRealCluster )
                    {
                        strcpy( IntegratingMonitorPort, MasterMonitorName);
                    }
                    else
                    {
                        char localHost[MAX_PROCESSOR_NAME];
                        gethostname( localHost, MAX_PROCESSOR_NAME );
                        strcpy( IntegratingMonitorPort, localHost);
                    }
#else
                    strcpy( IntegratingMonitorPort, MasterMonitorName);
#endif
                    strcat( IntegratingMonitorPort, ":");
                    strcat( IntegratingMonitorPort, monitorPort);
                }
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d (MasterMonitor) IsAgentMode=%s, IAmIntegrating=%s, "
                                  "I am NOT the master, MyPNID=%d, master port=%s\n"
                                , method_name, __LINE__
                                , IsAgentMode?"TRUE":"FALSE"
                                , IAmIntegrating?"TRUE":"FALSE"
                                , MyPNID, IntegratingMonitorPort );
                }
            }
            else
            {
                IAmIntegrating = false;
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d (MasterMonitor) IsAgentMode=%s, IAmIntegrating=%s, I AM the master, MyPNID=%d\n"
                                , method_name, __LINE__, IsAgentMode?"TRUE":"FALSE", IAmIntegrating?"TRUE":"FALSE", MyPNID );
                }
            }
        }

        NameServerConfig = new CNameServerConfigContainer ();
        Nodes = new CNodeContainer ();
        Config = new CConfigContainer ();
#ifdef NAMESERVER_PROCESS
        Monitor = new CMonitor ();
#else
        if (NameServerEnabled)
        {
            PtpClient  = new CPtpClient ();
            Monitor    = new CMonitor (procTermSig);
            NameServer = new CNameServer ();
        }
        else
        {
            Monitor = new CMonitor (procTermSig);
        }
#endif

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
            int monitorLead = -1;
            CNode *myNode = Nodes->GetNode(MyPNID);
            const char *masterMonitor = NULL;
            if (myNode == NULL)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf( la_buf
                       , "[%s], Failed to get myNode object, MyPNID=%d\n"
                       , method_name, MyPNID );
                mon_log_write(MON_MONITOR_MAIN_15, SQ_LOG_CRIT, la_buf);

                mon_failure_exit();
            }
            if ((ZClientEnabled) && (ZClient != NULL))
            {
                bool newMasterSelected = false;
retryMaster:
                CNode *masterNode = Nodes->GetNode(MasterMonitorName);
                if (!masterNode)
                {
                    if (trace_settings & TRACE_INIT)
                    {
                        trace_printf("%s@%d (MasterMonitor) IsMaster=%d, IAmIntegrating=%s, masterNode is NULL, with MasterMonitorName %s\n"
                                    , method_name, __LINE__, IsMaster, IAmIntegrating?"TRUE":"FALSE", MasterMonitorName);
                    }
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf(la_buf, "[%s], Failed to get my Master Node.\n", method_name);
                    mon_log_write(MON_MONITOR_MAIN_16, SQ_LOG_CRIT, la_buf);

                    mon_failure_exit();
                }
                else
                {
                    if (trace_settings & TRACE_INIT)
                    {
                          trace_printf("%s@%d (MasterMonitor) IsMaster=%d, IAmIntegrating=%s, masterNode=%s\n"
                                      , method_name, __LINE__, IsMaster, IAmIntegrating?"TRUE":"FALSE", masterNode->GetName() );
                    }
                }
                monitorLead = masterNode->GetPNid();
                if (MyPNID == monitorLead)
                {
                     ZClient->MasterZNodeDelete( myNode->GetName() ); // just in case of stale info
                     ZClient->MasterZNodeCreate( myNode->GetName() );
                     strcpy (MasterMonitorName, myNode->GetName());
                     if (newMasterSelected)
                     {
                        newMasterSelected = false;
                        IsMaster = true;
                        IAmIntegrating = false;
                        // This monitor is now the master, therefore load the 
                        // logical node (lnodes) as per the static configuration
                        Monitor->InitializeConfigCluster( monitorLead );
                     }
                     if (trace_settings & TRACE_INIT)
                     {
                         trace_printf("%s@%d (MasterMonitor) IsMaster=%d, IAmIntegrating=%s, master monitor pnid=%d\n"
                                     , method_name, __LINE__, IsMaster, IAmIntegrating?"TRUE":"FALSE", MyPNID);
                     }
                 }
                 else
                 {
                     masterMonitor = ZClient->MasterWaitForAndReturn(true);
                     CNode *masterNode = NULL;
                     if (masterMonitor)
                     {
                         strcpy (MasterMonitorName, masterMonitor);
                         masterNode = Nodes->GetNode(MasterMonitorName);
                        if (newMasterSelected)
                        {
                            newMasterSelected = false;
                        }
                     }

                     if (masterNode)
                     {
                          if (trace_settings & TRACE_INIT)
                          {
                            trace_printf( "%s@%d (MasterMonitor) IsMaster=%d, IAmIntegrating=%s, "
                                          "master monitor pnid=%d\n"
                                        , method_name, __LINE__
                                        , IsMaster
                                        , IAmIntegrating?"TRUE":"FALSE"
                                        , masterNode->GetPNid());
                          }
                          monitorLead = masterNode->GetPNid();
                          if (monitorLead != configMasterPNid)
                          {
                            char *commPort = getenv ("MONITOR_COMM_PORT");
                            if (commPort)
                            {
                                strcpy( IntegratingMonitorPort, MasterMonitorName);
                                strcat( IntegratingMonitorPort, ":");
                                strcat( IntegratingMonitorPort, commPort);
                            }
                          }
                     }
                     else
                     {
                          if (trace_settings & TRACE_INIT)
                          {
                            trace_printf( "%s@%d (MasterMonitor) IsMaster=%d, IAmIntegrating=%s, "
                                          "masterNode is NULL, with MasterMonitorName %s\n"
                                        , method_name, __LINE__
                                        , IsMaster
                                        , IAmIntegrating?"TRUE":"FALSE"
                                        , MasterMonitorName);
                          }
                          char la_buf[MON_STRING_BUF_SIZE];
                          sprintf( la_buf, "[%s], Master monitor (%s) is not available, selecting a new master\n"
                                 , method_name, MasterMonitorName );
                          mon_log_write(MON_MONITOR_MAIN_17, SQ_LOG_INFO, la_buf);

                          ZClient->MasterZNodeDelete( MasterMonitorName ); // just in case of stale info
                          CPNodeConfig * pnodeConfig = ClusterConfig->GetNextPNodeConfigByName( MasterMonitorName );
                          assert( pnodeConfig );
                          strcpy( MasterMonitorName, pnodeConfig->GetName() );

                          newMasterSelected = true;

                          goto retryMaster;
                      }
                }
            }
#ifdef NAMESERVER_PROCESS
            else
            {
                if ( !IsRealCluster )
                {
                    monitorLead = 0;
                }
            }
#endif
            CNode *masterNode = Nodes->GetNode(monitorLead);
            char    buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                           , "[%s], Master Monitor is %s on node %d\n"
                           , method_name
                           , masterNode?masterNode->GetName():""
                           , monitorLead);
            mon_log_write(MON_MONITOR_MAIN_18, SQ_LOG_INFO, buf);
        }
        if (!IAmIntegrating)
        {
            Config->Init ();
        }
#ifndef NAMESERVER_PROCESS
        Devices = new CDeviceContainer ();
        if ( !Devices->IsInitialized() )
        {
            if ( IAmIntegrating )
            {   // Problem unmounting devices, let creator monitor know then abort
                Monitor->ReIntegrate( CCluster::Reintegrate_Err13 );
            }
            else
            {
                mon_failure_exit();
            }
        }
#endif
        nodename = new char [Monitor->GetConfigPNodesCount() * MPI_MAX_PROCESSOR_NAME];

        // Create health check thread
        HealthCheck.start();

        // Create request worker threads
        CReqWorker::startReqWorkers();

        // Create thread to accept connections from other monitors
        CommAccept.start();
#ifdef NAMESERVER_PROCESS
        // Create thread to accept connections from other name servers
        CommAcceptMon.start();
        if (IsMaster)
        {
            CommAcceptMon.startAccepting();
        }
#else
        if (NameServerEnabled)
        {
            // Create thread to accept point-2-point connections from other monitors
            PtpCommAccept.start();
        }
#endif
#ifndef NAMESERVER_PROCESS
        // Open file used to record process start/end times
        Monitor->openProcessMap ();
#endif

#ifndef NAMESERVER_PROCESS

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

            mon_failure_exit();
        }

        memset( (void *)ioBuffer, 0 , BLOCK_SIZE );
#endif

// start ok
#ifndef NAMESERVER_PROCESS
        if (IsRealCluster)
        {
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.port.%s",
                     getenv("TRAF_LOG"), short_node_name );
        }
        else
        {
            // Write out our port number so other processes can attach.
            snprintf(port_fname, sizeof(port_fname), "%s/monitor.port.%d.%s",
                     getenv("TRAF_LOG"),MyPNID,Node_name);
        }
#endif

        // Change Node_name what we have in our configuration
        CNode *myNode = Nodes->GetNode(MyPNID);
        if (myNode)
        {
            strcpy (Node_name, myNode->GetName());
        }

#ifndef NAMESERVER_PROCESS
        for ( i = 0; i < MAX_PORTFILEOPEN_RETRIES; i++ )
        {
            // create with no caching, user read/write, group read/write, other read
            fd = open( port_fname
                       , O_RDWR | O_TRUNC | O_CREAT | O_DIRECT | O_SYNC
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
                             "port file (%s), error=%d(%s)\n",
                             method_name, BLOCK_SIZE, port_fname, err,
                             ErrorMsg(err));
                    mon_log_write(MON_MONITOR_MAIN_5, SQ_LOG_CRIT, buf);
    
                    if ( IAmIntegrating )
                    {
                        // This monitor is reintegrating into cluster.  Inform
                        // creator monitor of error, then abort.
                        Monitor->ReIntegrate( CCluster::Reintegrate_Err10 );
                    }
                    else
                    {
                        mon_failure_exit();
                    }
                }
                close( fd );
                if (trace_settings & TRACE_INIT)
                    trace_printf("%s@%d" " Port file created, pnid=%d, port=%s" "\n", method_name, __LINE__, MyPNID, MyCommPort );
                break;    
            }
            else
            {
                int err = errno;
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], can't open port file (%s), "
                          "error=%d (%s), retry=%d, max retries=%d\n"
                        , method_name, port_fname, err, ErrorMsg(err)
                        , i, MAX_PORTFILEOPEN_RETRIES );
                mon_log_write(MON_MONITOR_MAIN_6, SQ_LOG_ERR, buf);
                sleep(portFileOpenDelay);
            }
        }
        if ( fd == -1 )
        {
            if ( IAmIntegrating )
            {
                // This monitor is reintegrating into cluster.  Inform
                // creator monitor of error, then abort.
                Monitor->ReIntegrate( CCluster::Reintegrate_Err11 );
            }
            else
            {
                mon_failure_exit();
            }
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
#endif

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
                trace_printf("%s@%d" " After ReIntegrate" "\n", method_name, __LINE__);
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
#ifndef NAMESERVER_PROCESS
            Nodes->AddConfiguredZNodes();
#endif
            StartZookeeperClient();
        }
    }

#ifndef NAMESERVER_PROCESS
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
#endif

#ifndef NAMESERVER_PROCESS
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
#endif

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
        done = Monitor->exchangeNodeData();
        Monitor->ExitSyncCycle();

        if (done)
            break;


#ifndef NAMESERVER_PROCESS
        // Check to see if 'ckillall' is executing and disable the watchdog
        if ( !SQ_theLocalIOToClient->isWDTEnabled() )
        {
           // HealthCheck.setState(MON_EXIT_WATCHDOG);
        }
#endif

    }

    if (trace_settings & TRACE_STATS)
    {   // Write malloc statistics info to stderr
        monMallocStats();
    }

    if ( ZClientEnabled )
    {
        ZClient->ShutdownWork();
    }

#ifndef NAMESERVER_PROCESS
    Redirector.shutdownWork();
#endif

    // shut down health check thread before shutting down reqWorker thread.
    HealthCheck.shutdownWork();

    ReqQueue.stats();
    // Stop request worker threads
    CReqWorker::shutdownWork();
    Replicator.stats();

    Monitor->stats();

#ifndef NAMESERVER_PROCESS
    // Tell the LIO worker threads to exit
    SQ_theLocalIOToClient->shutdownWork();
    if (NameServerEnabled)
    {
        PtpCommAccept.shutdownWork();
    }
#endif

    CommAccept.shutdownWork();
#ifdef NAMESERVER_PROCESS
    CommAcceptMon.shutdownWork();
#endif

#ifndef NAMESERVER_PROCESS
    // Rename the monitor "port" file
    sprintf(temp_fname, "%s.bak", port_fname);
    remove(temp_fname);
    rename(port_fname, temp_fname);
#endif

    delete [] nodename;
#ifndef NAMESERVER_PROCESS
    delete Devices;
#endif
    delete Nodes;
    delete ZClient;
    delete Monitor;
    Monitor = NULL; // TRACE uses this
    delete Config;

#ifndef NAMESERVER_PROCESS
    if ( sbDiscSem != NULL )
    {
        RobSem::destroy_sem( sbDiscSem );
    }
#endif

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
#ifndef NAMESERVER_PROCESS
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
#endif

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
