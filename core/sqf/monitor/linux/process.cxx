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

using namespace std;

#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/epoll.h>

#include "props.h"
#include "localio.h"
#include "mlio.h"
#include "monlogging.h"
#ifdef USE_FORK_SUSPEND_RESUME
#include "monrs.h"
#endif // USE_FORK_SUSPEND_RESUME
#include "monsonar.h"
#include "montrace.h"
#include "redirector.h"
#include "healthcheck.h"
#include "lock.h"
#include "config.h"
#include "device.h"
#include "monitor.h"
#include "msgdef.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "process.h"
#include "intprocess.h"
#include "gentrap.h"
#include "nameserver.h"

#include "reqqueue.h"
extern CReqQueue ReqQueue;

#include "replicate.h"

#ifndef NAMESERVER_PROCESS
#include "ptpclient.h"
#endif

extern bool IsAgentMode;
extern bool IsMaster;
extern bool IsRealCluster;

extern bool PidMap;
extern int Measure;
extern int trace_level;
extern int MyPNID;
extern int ClusterId ;
extern int InstanceId;
extern char MyCommPort[MPI_MAX_PORT_NAME];
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern sigset_t SigSet;
extern CLock MemModLock;
extern CMonitor *Monitor;
#ifndef NAMESERVER_PROCESS
extern bool NameServerEnabled;
extern CNameServer *NameServer;
extern CPtpClient *PtpClient;
#endif
extern CNodeContainer *Nodes;
extern CConfigContainer *Config;
#ifndef NAMESERVER_PROCESS
extern CDeviceContainer *Devices;
#endif
extern CNode *MyNode;
extern CMonStats *MonStats;
#ifndef NAMESERVER_PROCESS
extern CRedirector Redirector;
#endif
extern CHealthCheck HealthCheck;
extern CReplicate Replicator;
extern CIntProcess IntProcess;

extern char *ErrorMsg (int error_code);
extern _TM_Txid_External invalid_trans( void );
extern _TM_Txid_External null_trans( void );
extern bool isEqual( _TM_Txid_External trans1, _TM_Txid_External trans2 );
extern bool isNull( _TM_Txid_External transid );
extern bool isInvalid( _TM_Txid_External transid );

extern bool IAmIntegrated;
extern bool SMSIntegrating;

extern const char *NodePhaseString( NodePhase phase );
extern const char *ProcessTypeString( PROCESSTYPE type );
extern const char *StateString( STATE state);

extern int monitorArgc;
extern char monitorArgv[MAX_ARGS][MAX_ARG_SIZE];

CProcess::CProcess (CProcess * parent, int nid, int pid,
#ifdef NAMESERVER_PROCESS
                    Verifier_t verifier,
#endif
                    PROCESSTYPE type,
                    int priority, int backup, bool debug, bool unhooked,
                    char *name,
#ifdef NAMESERVER_PROCESS
                    char *path,
                    char *ldpath,
                    char *program,
#else
                    strId_t pathStrId, strId_t ldpathStrId, strId_t programStrId, 
#endif
                    char *infile, char *outfile)
  :
    Nid (nid),
    Pid (pid),
#ifdef NAMESERVER_PROCESS
    Verifier ( verifier ),
#else
    Verifier ( -1 ),
#endif
    PidAtFork_ (pid),
    Type (type),
    Event_messages (false),
    System_messages (false),
    Paired (false),
    Clone (false),
    Debug(debug),
    DeletePending (false),
    StartupCompleted (false),
    Backup (backup),
    Abended (false),
    Attached (false),
    abort_(false),
    Persistent (false),
    UnHooked (unhooked),
    Nowait (false),
    PersistentCreateTime (0),
    PersistentRetries (0),
    Tag ( 0 ),
    Parent (parent),
    PairParentNid (-1),
    PairParentPid (-1),
    PairParentVerifier (-1),
    ReplyTag (REPLY_TAG),           // will be set again when we have a pending reply
    OpenedCount (0),
    LastNid (nid),
    DumpState (Dump_Ready),
    DumpStatus (Dump_Success),
    DumperNid (-1),
    DumperPid (-1),
    DumperVerifier (-1),
    priorPid_ (0),
    State_ (State_Unknown),
    next_(NULL),
    prev_(NULL),
    nextL_(NULL),
    prevL_(NULL),
    Last_error (MPI_SUCCESS)
    , argc_(0)
    , userArgvLen_ (0)
    , userArgv_ (NULL)
#ifdef NAMESERVER_PROCESS
    , path_(path)
    , ldpath_(ldpath)
    , program_(program)
#else
    , path_()
    , ldpath_()
    , program_()
    , programStrId_(programStrId)
    , pathStrId_(pathStrId)
    , ldpathStrId_(ldpathStrId)
#endif
    , firstInstance_(true)
    , cmpOrEsp_(false)
    , trafRootZnode_()
    , trafConf_()
    , trafHome_()
    , trafLog_()
    , trafVar_()
    , fd_stdin_(-1)
    , fd_stdout_(-1)
    , fd_stderr_(-1)
    , owned_(false)
    , ownerId_(0)
    , replRefCount_(0)
    , requestBuf_ (NULL)
#ifndef NAMESERVER_PROCESS
    , NoticeHead(NULL)
    , NoticeTail(NULL)
#endif
#ifdef NAMESERVER_PROCESS
    , monSockFd_(-1)
    , origPNidNs_(-1)
#endif
{
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcess::CProcess";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PROC", 4);

    hangupTime_.tv_sec = 0;
    hangupTime_.tv_nsec = 0;

    Port[0] = '\0';
    STRCPY (Name, name);
    CreationTime.tv_sec = 0;
    CreationTime.tv_nsec = 0;
    if ( infile && strcmp(infile,"#default") != 0)
        infile_ = infile;
    if ( outfile && strcmp(outfile,"#default") != 0)
        outfile_ = outfile;

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - Process %s, infile=%s, infile_=%s\n"
                    , method_name, __LINE__
                    , Name, infile, infile_.c_str());
    }
    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - Process %s, outfile=%s, outfile_=%s\n"
                    , method_name, __LINE__
                    , Name, outfile, outfile_.c_str());
    }

#ifndef NAMESERVER_PROCESS
    Config->strIdToString(programStrId_, program_ );
#endif

    switch (Type)
    {
        case ProcessType_ASE:
        case ProcessType_TSE:
            Priority = (priority<TSE_BASE_NICE?TSE_BASE_NICE:priority);
            break;
        case ProcessType_DTM:
            Priority = (priority<DTM_BASE_NICE?DTM_BASE_NICE:priority);
            break;
        case ProcessType_NameServer:
        case ProcessType_Watchdog:
        case ProcessType_PSD:
            Priority = priority;
            break;
        case ProcessType_AMP:
        case ProcessType_Backout:
        case ProcessType_VolumeRecovery:
        case ProcessType_MXOSRVR:
        case ProcessType_PERSIST:
        case ProcessType_SMS:
        case ProcessType_SPX:
        case ProcessType_SSMP:
        case ProcessType_TMID:
        case ProcessType_Generic:
            Priority = (priority<APP_BASE_NICE?APP_BASE_NICE:priority);
            break;
        default:
            Priority = priority;
            snprintf(la_buf, sizeof(la_buf),
                     "[CProcess::CProcess], Invalid process type!\n");
            mon_log_write(MON_PROCESS_PROCESS_1, SQ_LOG_ERR, la_buf);
    }

    switch (Type)
    {
        case ProcessType_DTM:
        case ProcessType_PSD:
        case ProcessType_PERSIST:
        case ProcessType_SMS:
        case ProcessType_SPX:
        case ProcessType_SSMP:
        case ProcessType_TMID:
        case ProcessType_Watchdog:
        case ProcessType_NameServer:
            Persistent = true;
            break;
        default:
            break;
    }

    if (parent)
    {
        // the process is being started at the request of a parent process
        Parent_Nid = parent->Nid;
        Parent_Pid = parent->Pid;
        Parent_Verifier = parent->Verifier;
        if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
           trace_printf("%s@%d - Process (%d, %d) has parent (%d, %d)\n", method_name, __LINE__, Nid, Pid, Parent_Nid, Parent_Pid);
        if (Backup)
        {
            PairParentNid = parent->PairParentNid;
            PairParentPid = parent->PairParentPid;
            parent->Parent_Nid = Nid;
            parent->Parent_Pid = Pid;
            if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
                trace_printf("%s@%d - Backup process %s (%d, %d) has process "
                             "pair parent (%d, %d) and primary process "
                             "(%d, %d)\n",
                             method_name, __LINE__, Name, Nid, Pid,
                             PairParentNid, PairParentPid,
                             parent->Nid, parent->Pid);
        }
    }
    else
    {
        // the process is being started by the monitor at initiation time
        Parent_Nid = -1;
        Parent_Pid = -1;
        Parent_Verifier = -1;
        if (backup)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[CProcess::CProcess], No Primary for Backup process!\n");
            mon_log_write(MON_PROCESS_PROCESS_2, SQ_LOG_ERR, la_buf);
        }
    }
    if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
    {
        trace_printf( "%s@%d - Process %s created @ %p:\n"
                      "                          nid       =%d\n"
                      "                          priority  =%d\n"
                      "                          type      =%s\n"
                      "                          persistent=%d\n"
                      "                          unhooked  =%d\n"
                    , method_name, __LINE__
                    , Name
                    , this
                    , Nid
                    , Priority 
                    , ProcessTypeString(Type)
                    , Persistent
                    , UnHooked );
    }

    Monitor->IncProcessCount();

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumProcsIncr();

    TRACE_EXIT;
}

CProcess::~CProcess (void)
{
    const char method_name[] = "CProcess::~CProcess";
    TRACE_ENTRY;
    Monitor->DecrProcessCount();

    if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
       trace_printf("%s@%d" " - Process " "%s(%d,%d:%d)" " destroyed @ " "%p""\n", method_name, __LINE__, Name, Nid, Pid, Verifier, this);

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumProcsDecr();


#ifndef NAMESERVER_PROCESS
    deathInterestLock_.lock();
    CNotice *notice = NoticeHead;
    while (notice)
    {
        // Send death notice messages to all opened processes
        notice->DeLink (&NoticeHead, &NoticeTail);
        delete notice;

        notice = NoticeHead;
    }
    deathInterestLock_.unlock();
#endif

    // For SSM process, release any undelivered pending notices.
    struct message_def * deathNotice;
    while ((deathNotice = GetDeathNotice()) != NULL)
    {
        delete deathNotice;
    }

    delete [] userArgv_;

#ifndef NAMESERVER_PROCESS
    if (fd_stdin_ != -1 && !Clone)
    {
        Redirector.tryShutdownPipeFd(Pid, fd_stdin_, false);
    }

    if (fd_stdout_ != -1)
    {
        Redirector.tryShutdownPipeFd(Pid, fd_stdout_, true);
    }

    if (fd_stderr_ != -1)
    {
        Redirector.tryShutdownPipeFd(Pid, fd_stderr_, false);
    }
#endif

    // Remove the fifos associated with this process (if any)
    if (fifo_stdin_.size() != 0)
    {
        unlink(fifo_stdin_.c_str());
    }

    if (fifo_stdout_.size() != 0)
    {
        unlink(fifo_stdout_.c_str());
    }

    if (fifo_stderr_.size() != 0)
    {
        unlink(fifo_stderr_.c_str());
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "proc", 4);

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
const char* CProcess::path()
{
    Config->strIdToString(pathStrId_, path_ );
    return( path_.c_str() );
}
#endif

#ifndef NAMESERVER_PROCESS
const char* CProcess::ldpath()
{
    Config->strIdToString(ldpathStrId_, ldpath_ );
    return( ldpath_.c_str() );
}
#endif

int CProcess::getUserArgs( char user_argv[MAX_ARGS][MAX_ARG_SIZE] )
{
    const char *pUserArgv = userArgv_;
    int i, arglen;
    for (i = 0; i < argc_; i++)
    {
        arglen = strlen (pUserArgv) + 1;
        strcpy( user_argv[i], pUserArgv );
        pUserArgv += arglen;
    }
    strcpy( user_argv[i], "" );
    return(argc_);
}

void CProcess::userArgs ( int argc, int argvLen, const char * argvList )
{
    const char method_name[] = "CProcess::userArgs";
    TRACE_ENTRY;

    argc_ = argc;
    userArgvLen_ = argvLen;
    if ( userArgv_ != NULL )
    {
        delete[] userArgv_;
    }
    userArgv_ = new char[ argvLen ];
    memcpy(userArgv_, argvList, argvLen);

    TRACE_EXIT;
}

void CProcess::userArgs ( int argc, char user_argv[MAX_ARGS][MAX_ARG_SIZE] )
{
    const char method_name[] = "CProcess::userArgs";
    TRACE_ENTRY;

    argc_ = argc;

    // Compute amount of space need to store argument strings
    userArgvLen_ = 0;
    for (int i = 0; i < argc; i++)
    {
        userArgvLen_ += strlen(user_argv[i]) + 1;
    }
    if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Copying arguments argc=%d, argvSize=%d\n",
                     method_name, __LINE__, argc, userArgvLen_);
    if (userArgvLen_ != 0)
    {
        userArgv_ = new char[userArgvLen_];
    }

    char *pUserArgv = userArgv_;
    for (int i = 0; i < argc; i++)
    {
        if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL))
            trace_printf("%s@%d - name=%s, Copying user_argv[%d]='%s'\n", method_name, __LINE__, Name, i, user_argv[i]);
        strcpy (pUserArgv, user_argv[i]);
        pUserArgv += strlen(user_argv[i]) + 1;
    }

    TRACE_EXIT;
}

void CProcess::validateObj( void )
{
    if (strncmp((const char *)&eyecatcher_, "PROC", 4) !=0 )
    {  // Not a valid object
        abort();
    }
}

#ifndef NAMESERVER_PROCESS
bool CProcess::CancelDeathNotification( int nid
                                      , int pid
                                      , Verifier_t verifier
                                      , _TM_Txid_External trans_id )
{
    bool status = FAILURE;
    CNotice *next;

    const char method_name[] = "CProcess::CancelDeathNotification";
    TRACE_ENTRY;

    deathInterestLock_.lock();
    CNotice *notice = NoticeHead;

    while( notice )
    {
        if ((( notice->Nid == nid    ) &&
             ( notice->Pid == pid    ) &&
             ( notice->verifier_ == verifier ) &&
             ( isInvalid( trans_id ) || isEqual( notice->TransID, trans_id )))
         || (( nid == -1 || pid == -1             ) &&
             ( isEqual(notice->TransID, trans_id) ) ) )
        {
            next = notice->GetNext();

            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL
                                  | TRACE_PROCESS_DETAIL))
            {
                trace_printf( "%s@%d - Process %s (%d, %d:%d) deleting death "
                              "notice interest for %s (%d, %d:%d), "
                              "trans_id=%lld.%lld.%lld.%lld\n"
                            , method_name, __LINE__
                            , Name
                            , Nid
                            , Pid
                            , Verifier
                            , notice->name_.c_str()
                            , notice->Nid
                            , notice->Pid
                            , notice->verifier_
                            , notice->TransID.txid[0]
                            , notice->TransID.txid[1]
                            , notice->TransID.txid[2]
                            , notice->TransID.txid[3] );
            }

            notice->DeLink(&NoticeHead, &NoticeTail);
            delete notice;

            notice = next;

            status = SUCCESS;
        }
        else
        {
            notice = notice->GetNext();
        }
    }

    deathInterestLock_.unlock();

    TRACE_EXIT;
    return status;
}
#endif

#ifndef NAMESERVER_PROCESS
// Death notice registration for a process
bool CProcess::procExitReg(CProcess *targetProcess,
                           _TM_Txid_External transId)
{
    const char method_name[] = "CProcess::ProcExitReg";
    TRACE_ENTRY;

    bool status = FAILURE;

    if ( Nid != targetProcess->GetParentNid() ||
         Pid != targetProcess->GetParentPid())
    {   // This process is not the parent of the target process (parent
        // processes automatically get process death notifications.)

        nidPid_t target = { targetProcess->Nid, targetProcess->Pid };
        deathInterestLock_.lock();
        // Add entry to list of processes that are being monitored
        // by this process.
        deathInterest_.push_back( target );
        // Add entry to set of nids of processes that are being monitored
        // by this process.
        deathInterestNid_.insert( targetProcess->Nid );
        deathInterestLock_.unlock();

        // Register interest with the target process
        targetProcess->RegisterDeathNotification( Nid
                                                , Pid
                                                , Verifier
                                                , Name
                                                , transId );
        status = SUCCESS;

        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf("%s@%d - Process %s (%d, %d) registered interest "
                         "in death of process %s (%d, %d), "
                         "trans_id=%lld.%lld.%lld.%lld\n",
                         method_name, __LINE__, Name, Nid, Pid,
                         targetProcess->Name, targetProcess->Nid,
                         targetProcess->Pid,
                         transId.txid[0], transId.txid[1], transId.txid[2],
                         transId.txid[3] );
        }
    }


    TRACE_EXIT;
    return status;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::procExitNotifierNodes( void )
{
    const char method_name[] = "CProcess::procExitNotifierNodes";
    TRACE_ENTRY;

    CLNode *targetLNode = NULL;
    CNode  *targetNode = NULL;
    nidSet_t::iterator it;

    // Remove death notice registration for all entries on list
    deathInterestLock_.lock();
    for ( it = deathInterestNid_.begin(); it != deathInterestNid_.end(); ++it)
    {
        targetLNode = Nodes->GetLNode ( *it );
        if (targetLNode)
        {
            targetNode = targetLNode->GetNode();
        }

        if ( targetNode )
        {
            if (NameServerEnabled && targetNode->GetPNid() != MyPNID)
            {
                // Forward the process exit to the target node
                int rc = PtpClient->ProcessExit( this 
                                               , targetLNode->GetNid()
                                               , targetNode->GetName() ); 
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send process exit "
                              "for process %s (%d, %d) "
                              "to target node %s, nid=%d\n"
                            , method_name
                            , GetName()
                            , GetNid()
                            , GetPid()
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_PROCESS_PROCEXITNOTIFIERNODES_1, SQ_LOG_ERR, la_buf);
                }
            }
        }
    }
    deathInterestNid_.clear();
    deathInterestLock_.unlock();

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::procExitUnregAll ( _TM_Txid_External transId )
{
    const char method_name[] = "CProcess::procExitUnregAll";
    TRACE_ENTRY;

    CLNode *node;
    CProcess *targetProcess = NULL;
    nidPidList_t::iterator it;

    // Remove death notice registration for all entries on list
    deathInterestLock_.lock();
    for ( it = deathInterest_.begin(); it != deathInterest_.end(); ++it)
    {
        node = Nodes->GetLNode ( it->nid );
        targetProcess = NULL;
        if (node)
        {
            targetProcess = node->GetProcessL( it->pid );
        }

        if ( targetProcess )
        {
            if (NameServerEnabled && targetProcess->IsClone())
            {
                CLNode *targetLNode = Nodes->GetLNode( targetProcess->GetNid() );
            
                int rc = -1;
                // Forward the process cancel death notification to the target node
                rc = PtpClient->ProcessNotify( targetProcess->GetNid()
                                             , targetProcess->GetPid()
                                             , targetProcess->GetVerifier()
                                             , transId
                                             , true // cancel target's death notification
                                             , this // of this process
                                             , targetLNode->GetNid()
                                             , targetLNode->GetNode()->GetName() ); 
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send process notify request "
                              "for process %s (%d, %d) "
                              "to target node %s, nid=%d\n"
                            , method_name
                            , targetProcess->GetName()
                            , targetProcess->GetNid()
                            , targetProcess->GetPid()
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_PROCESS_PROCEXITUNREGALL_1, SQ_LOG_ERR, la_buf);
                }
            }
            
            targetProcess->CancelDeathNotification( Nid
                                                  , Pid
                                                  , Verifier
                                                  , transId );
        }
    }
    deathInterest_.clear();
    deathInterestLock_.unlock();

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::childAdd ( int nid, int pid )
{
    const char method_name[] = "CProcess::childAdd";
    TRACE_ENTRY;

    nidPid_t child = { nid, pid };
    childrenListLock_.lock();
    children_.push_back ( child );
    childrenListLock_.unlock();

    TRACE_EXIT;
}

int CProcess::childCount ( void )
{
    const char method_name[] = "CProcess::childCount";
    TRACE_ENTRY;

    childrenListLock_.lock();
    int count = children_.size();
    childrenListLock_.unlock();

    TRACE_EXIT;
    return(count);
}

void CProcess::childRemove ( int nid, int pid )
{
    const char method_name[] = "CProcess::childRemove";
    TRACE_ENTRY;

    nidPidList_t::iterator it;

    childrenListLock_.lock();
    for ( it = children_.begin(); it != children_.end(); ++it)
    {
        if (it->nid == nid && it->pid == pid )
        {
            children_.erase ( it );
            break;
        }
    }
    childrenListLock_.unlock();

    TRACE_EXIT;
}

bool CProcess::childRemoveFirst ( nidPid_t & child)
{
    const char method_name[] = "CProcess::childRemoveFirst";
    TRACE_ENTRY;

    bool result = false;

    childrenListLock_.lock();
    if ( !children_.empty() )
    {
        child = children_.front ();
        children_.pop_front ();
        result = true;

    }
    childrenListLock_.unlock();

    TRACE_EXIT;

    return result;
}

void CProcess::childUnHookedAdd( int nid, int pid )
{
    const char method_name[] = "CProcess::childUnHookedAdd";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
        trace_printf( "%s@%d adding unhooked child (%d:%d)\n"
                    , method_name, __LINE__
                    , nid, pid );

    nidPid_t child = { nid, pid };
    childrenListLock_.lock();
    childrenUnHooked_.push_back ( child );
    childrenListLock_.unlock();

    TRACE_EXIT;
}

int CProcess::childUnHookedCount( void )
{
    const char method_name[] = "CProcess::childUnHookedCount";
    TRACE_ENTRY;

    childrenListLock_.lock();
    int count = childrenUnHooked_.size();
    childrenListLock_.unlock();

    TRACE_EXIT;
    return(count);
}

void CProcess::childUnHookedRemove( int nid, int pid )
{
    const char method_name[] = "CProcess::childUnHookedRemove";
    TRACE_ENTRY;

    nidPidList_t::iterator it;

    childrenListLock_.lock();
    for ( it = childrenUnHooked_.begin(); it != childrenUnHooked_.end(); ++it)
    {
        if (it->nid == nid && it->pid == pid )
        {
            childrenUnHooked_.erase ( it );
            break;
        }
    }
    childrenListLock_.unlock();

    TRACE_EXIT;
}

bool CProcess::childUnHookedRemoveFirst( nidPid_t & child)
{
    const char method_name[] = "CProcess::childUnHookedRemoveFirst";
    TRACE_ENTRY;

    bool result = false;

    childrenListLock_.lock();
    if ( !childrenUnHooked_.empty() )
    {
        child = childrenUnHooked_.front ();
        childrenUnHooked_.pop_front ();
        result = true;

    }
    childrenListLock_.unlock();

    TRACE_EXIT;

    return result;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::CompleteDump(DUMPSTATUS status, char *core_file)
{
    CProcess           *dumper;
    struct message_def *msg;

    const char method_name[] = "CProcess::CompleteDump";
    TRACE_ENTRY;

    DumpStatus = status;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
       trace_printf("%s@%d - Dumper Process nid=%d, pid=%d:%d\n",
                    method_name, __LINE__, DumperNid, DumperPid, DumperVerifier);
    dumper = Nodes->GetLNode (DumperNid)->GetProcessL(DumperPid);
    if (dumper && MyNode->IsMyNode(DumperNid))
    {
        if ( (DumperVerifier == -1) || (DumperVerifier == dumper->GetVerifier()) )
        {
            msg = parentContext();
            if ( msg )
            { // reply to parent pending, so send reply
                msg->noreply = false;
                msg->type = MsgType_Service;
                msg->u.reply.type = ReplyType_Dump;
                msg->u.reply.u.dump.nid = Nid;
                msg->u.reply.u.dump.pid = Pid;
                msg->u.reply.u.dump.verifier = Verifier;
                if (status == Dump_Success)
                {
                    char coreFile[MAX_PROCESS_PATH];
                    CNode * node = Nodes->GetLNode(GetNid())->GetNode();
                    snprintf( coreFile, sizeof(coreFile)
                            , "%s:%s", node->GetFqdn(), core_file );
                    STRCPY(msg->u.reply.u.dump.core_file, coreFile);
                    msg->u.reply.u.dump.return_code = MPI_SUCCESS;
                }
                else
                {
                    msg->u.reply.u.dump.core_file[0] = 0;
                    msg->u.reply.u.dump.return_code = MPI_ERR_EXITED;
                }
                CRequest::lioreply (msg, dumper->GetPid());
                parentContext( NULL );
            }
        }
    }

    DumpState = Dump_Ready;

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::CompleteProcessStartup (char *port, int os_pid, bool event_messages,
                                       bool system_messages, bool preclone,
                                       struct timespec *creation_time, int /*origPNidNs*/)
{
    const char method_name[] = "CProcess::CompleteProcessStartup";
    TRACE_ENTRY;

    STRCPY (Port, port);
    Pid = os_pid;
    Event_messages = event_messages;
    System_messages = system_messages;

    if (preclone)
    {
        Clone = true;
    }

    if (!Clone)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
            trace_printf("%s@%d: process %s (%d, %d), preclone=%d"
                         ", clone=%d\n",
                         method_name, __LINE__, Name,
                         Nid, os_pid, preclone, Clone);
        StartupCompleted = true;
        if (os_pid != -1)
        {
            if ( MyNode->IsMyNode(Nid) )
            {
                if ( NameServerEnabled )
                {
                    int rc = -1;
                    // Register process in Name Server
                    rc = NameServer->ProcessNew(this); // in reqQueue thread (CExtStartupReq)
                    if (rc)
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        snprintf( la_buf, sizeof(la_buf)
                                , "[%s] - Can't register new process "
                                  "%s (%d, %d) "
                                  "to Name Server process\n"
                                , method_name
                                , GetName()
                                , GetNid()
                                , GetPid() );
                        mon_log_write(MON_PROCESS_COMPLETESTARTUP_1, SQ_LOG_ERR, la_buf);
                    }

                    if (Parent_Nid != -1)
                    {
                        if (Parent_Nid != Nid)
                        {
                            // Tell the parent node the current state of the process
                            rc = PtpClient->ProcessClone(this);
                            if (rc)
                            {
                                char la_buf[MON_STRING_BUF_SIZE];
                                CLNode *parentLNode = NULL;
                                parentLNode = Nodes->GetLNode( GetParentNid() );
                                snprintf( la_buf, sizeof(la_buf)
                                        , "[%s] - Can't send process clone request"
                                          "for process %s (%d, %d) "
                                          "to parent node %s, nid=%d\n"
                                        , method_name
                                        , GetName()
                                        , GetNid()
                                        , GetPid()
                                        , parentLNode->GetNode()->GetName()
                                        , parentLNode->GetNid() );
                                mon_log_write(MON_PROCESS_COMPLETESTARTUP_2, SQ_LOG_ERR, la_buf);
                            }
                        }
                    }
                }
                else
                {
                    // Replicate the clone to other nodes
                    CReplClone *repl = new CReplClone(this);
                    Replicator.addItem(repl);
                }
            }
            else
            {
                Clone = true;
            }
        }
        else
        {
            // TODO: What does an os_pid == -1 mean?
            if ( NameServerEnabled )
            {
                if (Parent_Nid != -1)
                {
                    if (Parent_Nid != Nid)
                    {
                        int rc = -1;
                        // Tell the parent node the current state of the process
                        rc = PtpClient->ProcessClone(this);
                        if (rc)
                        {
                            char la_buf[MON_STRING_BUF_SIZE];
                            CLNode *parentLNode = NULL;
                            parentLNode = Nodes->GetLNode( GetParentNid() );
                            snprintf( la_buf, sizeof(la_buf)
                                    , "[%s] - Can't send process clone request"
                                      "for process %s (%d, %d) "
                                      "to parent node %s, nid=%d\n"
                                    , method_name
                                    , GetName()
                                    , GetNid()
                                    , GetPid()
                                    , parentLNode->GetNode()->GetName()
                                    , parentLNode->GetNid() );
                            mon_log_write(MON_PROCESS_COMPLETESTARTUP_3, SQ_LOG_ERR, la_buf);
                        }
                    }
                }
            }
            else
            {
                // Replicate the clone to other nodes
                CReplClone *repl = new CReplClone(this);
                Replicator.addItem(repl);
            }
        }
    }

    if (!Clone)
    {
        // check if we need to setup any associated devices.
        if ((Type == ProcessType_TSE) ||
            (Type == ProcessType_ASE)   )
        {
             Devices->CreateDevice( this );
        }

        if ((Type == ProcessType_TSE) ||
            (Type == ProcessType_DTM) ||
            (Type == ProcessType_ASE) )
        {
             MyNode->addToQuiesceSendPids( GetPid(), GetVerifier() );

             if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
                 trace_printf("%s%d: pid %d added to quiesce send list\n", method_name, __LINE__, GetPid());
        }

        if ((Type == ProcessType_TSE) ||
            (Type == ProcessType_ASE) )
        {
             MyNode->addToQuiesceExitPids( GetPid(), GetVerifier() );

             if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
                 trace_printf("%s%d: pid %d added to quiesce exit list\n", method_name, __LINE__, GetPid());
        }
    }

    if ( Clone && !preclone )
    {
        StartupCompleted = true;
        if (creation_time != NULL)
            CreationTime = *creation_time;
    }

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d: process %s (%d, %d:%d), preclone=%d"
                     ", clone=%d, StartupCompleted=%d\n",
                     method_name, __LINE__, Name, Nid, os_pid, Verifier, preclone,
                     Clone, StartupCompleted);
    State_ = State_Up;

    // Check if node is shutting down
    if ( !Clone && MyNode->GetState() == State_Shutdown )
    {
        if ( MyNode->GetShutdownLevel() == ShutdownLevel_Abrupt )
        {
            // killing the process will not remove the process object because
            // exit processing will get queued until this completes.
            kill( Pid, SIGKILL );
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf( "%s@%d - Shutdown abrupt in process, completed kill for %s (%d, %d)\n"
                            , method_name, __LINE__, Name, Nid, os_pid);
        }
        else
        {
            struct message_def *msg;

            msg = new struct message_def;
            msg->type = MsgType_Shutdown;
            msg->noreply = true;
            msg->u.request.type = ReqType_Notice;
            msg->u.request.u.shutdown.nid = Nid;
            msg->u.request.u.shutdown.pid = -1;
            msg->u.request.u.shutdown.level = MyNode->GetShutdownLevel();
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
               trace_printf( "%s@%d" " - Sending shutdown notice, level=%d\n"
                           , method_name, __LINE__, MyNode->GetShutdownLevel() );
            // Send shutdown notice
            SQ_theLocalIOToClient->putOnNoticeQueue( Pid, Verifier, msg, NULL );
        }
    }

    // some special handling for native processes
    if ( !Clone )
    {
        ssmpNoticesLock_.lock();
        if ( Type == ProcessType_SSMP && !ssmpNotices_.empty())
        {   // Some death notices are queued for this SSMP process.  Signal
            // the notifier to get to work on delivering them.
            SQ_theLocalIOToClient->nudgeNotifier ();
        }
        ssmpNoticesLock_.unlock();

        if ( Type == ProcessType_SMS )
        {
            // let healthcheck thread know that the SMService process is up and running.
            HealthCheck.setState(HC_UPDATE_SMSERVICE, (long long)this);
        }
        else if ( Type == ProcessType_DTM )
        {
            MyNode->SetPrimitiveDtmUp();
        }
        else if ( Type == ProcessType_Watchdog )
        {
            MyNode->SetPrimitiveWdgUp();
            // let healthcheck thread know that the watchdog process is up and running.
            HealthCheck.setState(HC_UPDATE_WATCHDOG, (long long)this);
            // start the watchdog timer
            HealthCheck.setState(MON_START_WATCHDOG);
        }
        else if ( Type == ProcessType_PSD )
        {
            MyNode->SetPrimitivePsdUp();
            if(IsRealCluster)
            {
                MyNode->StartPStartDPersistent();
            }

             if (trace_settings & (TRACE_RECOVERY | TRACE_REQUEST | TRACE_INIT))
                 trace_printf("%s%d: Sent start persistent processes event to PSD process %s (pid=%d)\n", method_name, __LINE__, GetName(), GetPid());
        }
    }

    TRACE_EXIT;
}
#endif

void CProcess::CompleteRequest( int status )
{
#ifndef NAMESERVER_PROCESS
    struct message_def *msg;
#endif

    const char method_name[] = "CProcess::CompleteRequest";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
       trace_printf("%s@%d - Process %s (%d,%d:%d), status %d\n",
                    method_name, __LINE__, Name, Nid, Pid, Verifier, status);

#ifndef NAMESERVER_PROCESS
    if ( !Clone )
    {
        msg = parentContext();
        if ( msg )
        { // reply pending, so send reply
            msg->noreply = false;
            msg->u.reply.type = ReplyType_Generic;
            msg->u.reply.u.generic.nid = Nid;
            msg->u.reply.u.generic.pid = Pid;
            msg->u.reply.u.generic.verifier = Verifier;
            msg->u.reply.u.generic.process_name[0] = '\0';
            msg->u.reply.u.generic.return_code = status;

            CRequest::lioreply (msg, Pid);
            parentContext( NULL );
        }
    }
#endif

    TRACE_EXIT;
}

bool CProcess::PickStdfile(PickStdFile_t whichStdfile,
                           char (&Destfile)[MAX_PROCESS_PATH],
                           int &ancestorNid, int &ancestorPid)
{
    const char method_name[] = "CProcess::PickStdfile";
    TRACE_ENTRY;

    CLNode *node = NULL;
    CProcess *ancestor;
    int nextNid = -1;
    int nextPid = 0;

    if (whichStdfile == PICK_STDOUT)
    {
        if (!outfile_.empty())
        {
            STRCPY(Destfile, outfile_.c_str());
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf( "%s@%d - Process %s, Destfile=%s, outfile_=%s\n"
                            , method_name, __LINE__
                            , Name, Destfile, outfile_.c_str());
            }
            TRACE_EXIT;
            return true;
        }
    }
    else
    {
        if (!infile_.empty())
        {
            STRCPY(Destfile, infile_.c_str());
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf( "%s@%d - Process %s, Destfile=%s, outfile_=%s\n"
                            , method_name, __LINE__
                            , Name, Destfile, infile_.c_str());
            }
            TRACE_EXIT;
            return true;
        }
    }

    nextNid = Parent_Nid;
    nextPid = Parent_Pid;
    Destfile[0] = '\0';
    bool retVal = true;

    // Keep track of process creation times to avoid looping forever.
    struct timespec earlyCreationTime;
    earlyCreationTime.tv_sec = CreationTime.tv_sec;
    earlyCreationTime.tv_nsec = CreationTime.tv_nsec;

    while(true)
    {
        node = Nodes->GetLNode (nextNid);
        if (node)
        {
            ancestor = node->GetProcessL(nextPid);
            if ( ancestor  &&
                 (( ! MyNode->IsMyNode(ancestor->GetNid())) ||
                  (ancestor->CreationTime.tv_sec  < earlyCreationTime.tv_sec ||
                   (ancestor->CreationTime.tv_sec == earlyCreationTime.tv_sec  &&
                    ancestor->CreationTime.tv_nsec < earlyCreationTime.tv_nsec))) )
            {
                earlyCreationTime.tv_sec  = ancestor->CreationTime.tv_sec;
                earlyCreationTime.tv_nsec = ancestor->CreationTime.tv_nsec;

                if (whichStdfile == PICK_STDOUT && (ancestor->outfile())[0])
                {
                    // The ancestor specified a standard outfile
                    if ( MyNode->IsMyNode(nextNid) )
                    {   // The ancestor and this process are on the same node
                        STRCPY(Destfile, ancestor->outfile());
                    }
                    else
                    {   // The ancestor is on a different node.
                        ancestorNid = nextNid;
                        ancestorPid = nextPid;
                    }

                    break;
                }
                else if (whichStdfile == PICK_STDIN && (ancestor->infile())[0])
                {
                    // The ancestor specified a standard outfile
                    if ( MyNode->IsMyNode(nextNid) )
                    {   // The ancestor and this process are on the same node
                        STRCPY(Destfile, ancestor->infile());
                    }
                    else
                    {   // The ancestor is on a different node.
                        ancestorNid = nextNid;
                        ancestorPid = nextPid;
                    }

                    break;
                }
                else
                {   // The ancestor process did not specify a stdout file
                    // so next examine ancestor's parent.
                    if (Backup || ancestor->Backup)
                    {
                        nextNid = ancestor->PairParentNid;
                        nextPid = ancestor->PairParentPid;
                    }
                    else
                    {
                        nextNid = ancestor->Parent_Nid;
                        nextPid = ancestor->Parent_Pid;
                    }
                }
            }
            else
            {
                if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
                    trace_printf("%s@%d could not find process object for "
                                 "pid=%d\n",
                                 method_name, __LINE__, nextPid);
                retVal = false;
                break;
            }
        }
        else
        {  // Unexpectedly could not find node object
           // log error
            if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
                trace_printf("%s@%d could not find node object for nid=%d\n",
                             method_name, __LINE__, nextNid);

            if (nextNid != -1)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "%s, Unable to find node object for nid=%d\n",
                        method_name, nextNid);
                mon_log_write(MON_PROCESS_PICKSTDFILE_2, SQ_LOG_ERR, buf);
            }
            retVal = false;
            break;
        }
    }

    TRACE_EXIT;

    return retVal;
}

// for attached processes,
//   set CreationTime to last modification time of /proc/<pid>/cmdline
// for unattached process,
//   set CreationTime to current time (fork time)
void CProcess::SetCreationTime(int os_pid)
{
    if (os_pid == -1)
    {
        struct timespec ts;
        int err = clock_gettime(CLOCK_REALTIME, &ts);
        if (err == 0)
            CreationTime = ts;
    } else
    {
        char statline[40];
        struct stat statbuf;
        snprintf(statline, sizeof(statline), "/proc/%d/cmdline", os_pid);
        int err = stat(statline, &statbuf);
        if (err == 0)
            CreationTime = statbuf.st_mtim;
    }
}

void CProcess::SetVerifier()
{
   Verifier = Monitor->incrGetVerifierNum();
   return;
}

#ifndef NAMESERVER_PROCESS
void CProcess::SetupFifo(int attachee_nid, int attachee_pid)
{
    const char method_name[] = "CProcess::SetupFifo";
    TRACE_ENTRY;

    // reset umask (group needs write permissions for fifo)
    mode_t prev_mask;
    prev_mask = umask(S_IWOTH);


    // Get the file name for the attached process's current standard in file
    char std_name[MAX_PROCESS_PATH];
    char filepath[30];
    ssize_t std_name_len;
    snprintf (filepath, sizeof(filepath), "/proc/%d/fd/0", attachee_pid);
    std_name_len = readlink (filepath, std_name, MAX_PROCESS_PATH-1);
    if (std_name_len < 0) std_name_len = 0;
    std_name[std_name_len] = '\0';

    if ((std_name_len >= 9)
     && (strcmp(&std_name[std_name_len-9], "(deleted)") != 0))
    {
        // Record the infile name in the process object
        infile_ = std_name;
    }
    else if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
        trace_printf("%s@%d Not saving stdin file %s for pid=%d\n",
                     method_name, __LINE__, std_name, attachee_pid);

    // Get the file name for the attached process's current standard out file
    snprintf (filepath, sizeof(filepath), "/proc/%d/fd/1", attachee_pid);
    std_name_len = readlink (filepath, std_name, MAX_PROCESS_PATH-1);
    if (std_name_len < 0) std_name_len = 0;
    std_name[std_name_len] = '\0';

    // Record the outfile name in the process object.  Any child
    // process created by it may write to the pipe.
    if (strncmp(std_name, "pipe:", 5) != 0)
    {   // The attach process has a device or file for its standard output.
        outfile_ = std_name;
    }
    else
    {   // The attached process has a pipe for its standard output.
        outfile_ = filepath;
    }

    // Create unique fifo name, store in process object
    bool fifo_ok = true;
    char fifo_stdout[50];
    strcpy(fifo_stdout, "/tmp/sqmp.XXXXXX");
    int fifo_stdout_fd = mkstemp(fifo_stdout);
    if (fifo_stdout_fd == -1)
    {   // Unexpected mkstemp problem
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], mkstemp(%s) error, %s.\n", method_name,
                fifo_stdout, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPFIFO_1, SQ_LOG_ERR, buf);
        fifo_ok = false;
    }
    if (fifo_ok)
    {
        fifo_stdout_ = fifo_stdout;
        // unlink so mkfifo works
        int err = unlink(fifo_stdout);
        if (err == -1)
        {   // Unexpected unlink problem
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unlink(%s) error, %s.\n", method_name,
                    fifo_stdout, strerror(errno));
            mon_log_write(MON_PROCESS_SETUPFIFO_2, SQ_LOG_ERR, buf);
            fifo_ok = false;
        }
    }
    if (fifo_ok)
    {
        if (mkfifo(fifo_stdout, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        {   // Unexpected fifo creation problem
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], mkfifo(%s) error, %s.\n", method_name,
                    fifo_stdout, strerror(errno));
            mon_log_write(MON_PROCESS_SETUPFIFO_3, SQ_LOG_ERR, buf);
        }
        else
        {
            // Open the fifo for reading.  Use non-blocking mode because
            // otherwise open would not complete until attached process
            // opens fifo for writing.
            fd_stdout_ = open (fifo_stdout, O_RDONLY | O_NONBLOCK);
            if (fd_stdout_ == -1)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], fifo open(%s) error, %s.\n", method_name,
                        fifo_stdout, strerror(errno));
                mon_log_write(MON_PROCESS_SETUPFIFO_4, SQ_LOG_ERR, buf);
            }
            else
            {
                // close the unlinked file
                close(fifo_stdout_fd);
            }
        }

#ifndef NAMESERVER_PROCESS
        Redirector.stdoutFd(attachee_nid, attachee_pid, fd_stdout_, outfile_.c_str(),
                            -1, -1);
#endif
    }

    // Create unique stderr fifo name, store in process object
    fifo_ok = true;
    char fifo_stderr[50];
    strcpy(fifo_stderr, "/tmp/sqmp.XXXXXX");
    int fifo_stderr_fd = mkstemp(fifo_stderr);
    if (fifo_stderr_fd == -1)
    {   // Unexpected mkstemp problem
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], mkstemp(%s) error, %s.\n", method_name,
                fifo_stderr, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPFIFO_5, SQ_LOG_ERR, buf);
        fifo_ok = false;
    }

    if (fifo_ok)
    {
        fifo_stderr_ = fifo_stderr;
        // unlink so mkfifo works
        int err = unlink(fifo_stderr);
        if (err == -1)
        {   // Unexpected unlink problem
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf), "[%s], unlink(%s) error, %s.\n", method_name,
                    fifo_stderr, strerror(errno));
            mon_log_write(MON_PROCESS_SETUPFIFO_6, SQ_LOG_ERR, buf);
            fifo_ok = false;
        }
    }
    if (fifo_ok)
    {
        if (mkfifo(fifo_stderr, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], mkfifo(%s) error, %s.\n", method_name, fifo_stderr,
                    strerror(errno));
            mon_log_write(MON_PROCESS_SETUPFIFO_7, SQ_LOG_ERR, buf);
        }
        else
        {
            fd_stderr_ = open (fifo_stderr, O_RDONLY | O_NONBLOCK);
            if (fd_stderr_ == -1)
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf(buf, sizeof(buf),
                         "[%s], fifo open(%s) error, %s.\n", method_name,
                        fifo_stderr, strerror(errno));
                mon_log_write(MON_PROCESS_SETUPFIFO_8, SQ_LOG_ERR, buf);
            }
            else
            {
                // close the unlinked file
                close(fifo_stderr_fd);
            }

#ifndef NAMESERVER_PROCESS
            Redirector.stderrFd(MyNode->GetHostname(), Name, Nid, attachee_pid, fd_stderr_);
#endif
        }
    }

    if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
        trace_printf("%s@%d Process=%s, Pid=%d, Infile=[%s], "
                     "Outfile=[%s], fifo_stdout=%s, fd_stdout=%d, "
                     "fifo_stderr=%s, fd_stderr=%d\n",
                     method_name, __LINE__, Name, attachee_pid,
                     infile_.c_str(), outfile_.c_str(), fifo_stdout_.c_str(),
                     fd_stdout_, fifo_stderr_.c_str(), fd_stderr_);

    // Restore previous umask
    umask(prev_mask);

    TRACE_EXIT;
}
#endif

// LCOV_EXCL_START
// Methods CProcess::SetupPipe and CProcess::RedirectStdFiles are
// excluded from code coverage measurement.   They are executed only
// by a monitor child process not the monitor process itself.  Therefore
// they do not show up as covered lines when monitor code coverage measurement
// is done.


#ifndef NAMESERVER_PROCESS
void CProcess::SetupPipe(int orig_fd, int unused_pipe_fd, int pipe_fd)
{
    int newfd;
    char buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcess::SetupPipe";
    TRACE_ENTRY;

    // Close original file descriptor
    if (close(orig_fd))
    {
        snprintf(buf, sizeof(buf), "[%s], close(%d) error, %s.\n",
                 method_name, orig_fd, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPPIPE_1, SQ_LOG_ERR, buf);
    }

    // Close unused pipe file descriptor
    if (close(unused_pipe_fd))
    {
        snprintf(buf, sizeof(buf), "[%s], close(%d) error, %s.\n", method_name,
                unused_pipe_fd, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPPIPE_2, SQ_LOG_ERR, buf);
    }

    // Duplicate pipe file desciptor to original file descriptor number
    newfd = dup2(pipe_fd, orig_fd);
    if (newfd == -1)
    {
        snprintf(buf, sizeof(buf), "[%s], dup2(%d, %d) error, %s.\n",
                 method_name, pipe_fd, orig_fd, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPPIPE_3, SQ_LOG_ERR, buf);
    }

    // Close the pipe file descriptor
    if (close(pipe_fd))
    {
        snprintf(buf, sizeof(buf), "[%s], close(%d) error, %s.\n", method_name,
                pipe_fd, strerror(errno));
        mon_log_write(MON_PROCESS_SETUPPIPE_4, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::RedirectStdFiles(int pfds_stdin[2], int pfds_stdout[2],
                                int pfds_stderr[2])

{
    const char method_name[] = "CProcess::RedirectStdFiles";
    TRACE_ENTRY;

    SetupPipe(0, pfds_stdin[1], pfds_stdin[0]);

    SetupPipe(1, pfds_stdout[0], pfds_stdout[1]);

    SetupPipe(2, pfds_stderr[0], pfds_stderr[1]);

    TRACE_EXIT;
}
#endif

// LCOV_EXCL_STOP

void CProcess::setEnvStr ( char **envp, int &countEnv, const char *str )
{
    envp[countEnv] = new char [ strlen(str)+1 ];
    strcpy ( envp[countEnv], str );
    ++countEnv;
}

void CProcess::setEnvStrVal ( char **envp, int &countEnv, const char *str,
                    const char *val)
{
    envp[countEnv] = new char [ strlen(str)+strlen(val)+2 ];
    sprintf ( envp[countEnv], "%s=%s", str, val );
    ++countEnv;
}

void CProcess::setEnvIntVal ( char **envp, int &countEnv, const char *str,
                              int val)
{
    envp[countEnv] = new char [ strlen(str)+13 ];
    sprintf ( envp[countEnv], "%s=%d", str, val );
    ++countEnv;
}

void CProcess::setEnvRegGroupVals(CConfigGroup *group, char **envp,
                                  int &countEnv)
{
    CConfigKey *key;

    const char method_name[] = "CProcess::setEnvRegGroupVals";
    TRACE_ENTRY;

    if (group)
    {
        key = group->GetKey((char *) "");
        while (key)
        {
            if (strncasecmp(key->GetName(), "~US_", 4) != 0)
            {  // Not an internal monitor unique string, ok to set
                setEnvStrVal(envp, countEnv, key->GetName(), key->GetValue());
            }
            if (countEnv >= MAX_CHILD_ENV_VARS)
            {
                break;
            }

            key = key->GetNext();
        }
    }
    TRACE_EXIT;
}

void CProcess::setEnvFromRegistry ( char **envp, int &countEnv )
{
    CConfigGroup *group;

    group = Config->GetClusterGroup();
    setEnvRegGroupVals ( group, envp, countEnv );

    group = Config->GetLocalNodeGroup();
    setEnvRegGroupVals ( group, envp, countEnv );

    group = Config->GetGroup(Name);
    setEnvRegGroupVals ( group, envp, countEnv );
}

#ifndef NAMESERVER_PROCESS
bool CProcess::Create (CProcess *parent, void* tag, int & result)
{
    bool monAltLogEnabled = false;
    bool seamonsterEnabled = false;
    bool shellTrace = false;
    bool successful = false;
    bool wdtDumpMonitor = false;
    bool wdtTraceCmd = false;
    bool wdtTraceInit = false;
    bool wdtTraceLio = false;
    bool wdtTraceEntryExit = false;
    bool wdtKeepAliveTimer = false;
    bool wdtMonProcRate = false;
    bool wdtLunmgrHangDelay = false;
    bool wdtLinuxWatchdog = false;
    bool wdtStartupTimer = false;
    int numProcessThreads = 0;
    int keepAliveValue = 0;
    int monitorCheckRateValue = 0;
    int lunmgrHangDelayValue = 0;
    int startupTimerValue = 0;
    int i;
    int j;
    int rc = -1;
    int rc2 = -1;
    char *env;
    char **argv;
    char *childEnv[MAX_CHILD_ENV_VARS + 1];
    int nextEnv = 0;
    int maxClientBuffers = SQ_LIO_MAX_BUFFERS;

    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcess::Create";
    TRACE_ENTRY;

    result = MPI_SUCCESS;

    if (Debug)
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d" " - Starting process through debugger" "\n", method_name, __LINE__);
    }

    pid_t os_pid;
    char sonardir[MAX_PROCESS_PATH];
    char user[50];
    char filename[MAX_PROCESS_PATH];
    char home[MAX_PROCESS_PATH];
    char mpiroot[MAX_PROCESS_PATH];
    char mpitmpdir[MAX_PROCESS_PATH];
    char mpiflags[20];
    char mpi_ic_order[10];
    char mpi_test_delay[10];
    char mpi_error_level[10];
    char sq_ic[5];
    char term[20];
    char tz[100];
    bool tz_exists;
    char xauthority[MAX_PROCESS_PATH];
    char *display;
    char *vnodes;
    char nsCommPort[10];
    char nsSyncPort[10];
    char nsMon2NsPort[10];
    char nsConfigDb[MAX_PROCESS_PATH];
    MON_Props xprops(true);
    MON_Props xprops_exe(true);
    char *xprops_exe_file;

    // get last used default environment
    env = getenv ("TERM");
    STRCPY (term, (env?env:"ansi"));
    env = getenv ("TZ");
    tz_exists = (env != NULL);
    if (tz_exists)
    {
      STRCPY (tz, env); // see note regarding TZ below
    }
    env = getenv ("USER");
    STRCPY (user, (env?env:""));
    env = getenv ("HOME");
    STRCPY (home, (env?env:""));
    env = getenv ("SONAR_ROOT");
    STRCPY (sonardir, (env?env:""));
    env = getenv ("MPI_ROOT");
    STRCPY (mpiroot, (env?env:""));
    env = getenv ("MPI_TMPDIR");
    STRCPY (mpitmpdir, (env?env:home));
   // strcpy (mpiflags, "l,y0,Eon");
    strcpy (mpiflags, "y0");
    if (Debug)
    {
        strcat(mpiflags,",egdb");
    }
    env = getenv ("MPI_TEST_DELAY");
    STRCPY(mpi_test_delay,(env?env:"2"));
    env = getenv ("MPI_ERROR_LEVEL");
    strcpy(mpi_error_level,(env?env:"2"));
    STRCPY (xauthority, home);
    strcat (xauthority, "/.Xauthority");
    display = getenv ("DISPLAY");
    vnodes = getenv("SQ_VIRTUAL_NODES");
    env=getenv("SQ_IC");
    if(env)
    {
        if ((strcmp(env,"IBV")==0) || (strcmp(env,"-IBV")==0))
        {
            strcpy(sq_ic, "-IBV");
            strcpy(mpi_ic_order, "IBV");
        }
        else
        {
            strcpy(sq_ic, "-TCP");
            strcpy(mpi_ic_order, "TCP");
        }
    }
    else
    {
        strcpy(sq_ic, "-TCP");
        strcpy(mpi_ic_order, "TCP");
    }

    env = getenv( "SQ_LIO_MAX_BUFFERS" );
    if (env)
    {
        maxClientBuffers = atoi( env );
    }

    env = getenv( "SQ_LOCAL_IO_SHELL_TRACE" );
    if (env && strcmp( env, "1" ) == 0)
       shellTrace = true;

    if ( Type == ProcessType_NameServer )
    {
        env = getenv ("NS_COMM_PORT");
        STRCPY (nsCommPort, (env?env:""));
        env = getenv ("NS_SYNC_PORT");
        STRCPY (nsSyncPort, (env?env:""));
        env = getenv ("NS_M2N_COMM_PORT");
        STRCPY (nsMon2NsPort, (env?env:""));
        env = getenv ("SQ_CONFIGDB");
        STRCPY (nsConfigDb, (env?env:""));
    }
    if ( Type == ProcessType_Watchdog )
    {
        env = getenv( "WDT_TRACE_CMD" );
        if (env && strcmp( env, "1" ) == 0)
           wdtTraceCmd = true;
        env = getenv( "WDT_TRACE_INIT" );
        if (env && strcmp( env, "1" ) == 0)
           wdtTraceInit = true;
        env = getenv( "WDT_TRACE_LIO" );
        if (env && strcmp( env, "1" ) == 0)
           wdtTraceLio = true;
        env = getenv( "WDT_TRACE_ENTRY_EXIT" );
        if (env && strcmp( env, "1" ) == 0)
           wdtTraceEntryExit = true;
        env = getenv( "SQ_WDT_KEEPALIVETIMERVALUE" );
        if (env && isdigit(*env))
        {
            wdtKeepAliveTimer = true;
            keepAliveValue = atoi(env);
        }
        env = getenv( "SQ_WDT_MONITOR_PROCESS_CHECKRATE" );
        if (env && isdigit(*env))
        {
            wdtMonProcRate = true;
            monitorCheckRateValue = atoi(env);
        }
        env = getenv( "SQ_WDT_LUNMGR_PROCESS_HANGDELAY" );
        if (env && isdigit(*env))
        {
            wdtLunmgrHangDelay = true;
            lunmgrHangDelayValue = atoi(env);
        }
        env = getenv( "SQ_LINUX_WATCHDOG" );
        if (env && strcmp( env, "1" ) == 0)
           wdtLinuxWatchdog = true;
        env = getenv( "SQ_WDT_STARTUPTIMERVALUE" );
        if (env && isdigit(*env))
        {
            wdtStartupTimer = true;
            startupTimerValue = atoi(env);
        }
        env = getenv( "SQ_WDT_DUMP_MONITOR" );
        if (env && strcmp( env, "1" ) == 0)
           wdtDumpMonitor = true;
    }

    env = getenv( "SQ_MON_ALTLOG" );
    if (env && strcmp( env, "1" ) == 0)
       monAltLogEnabled = true;

    env = getenv( "SQ_SEAMONSTER" );
    if (env && strcmp( env, "1" ) == 0)
       seamonsterEnabled = true;

    env = getenv( "SQ_LIO_PROCESS_THREADS" );
    if (env && isdigit(*env))
       numProcessThreads = atoi(env);

    env = getenv( "TRAF_ROOT_ZNODE" );
    if (env)
    {
        trafRootZnode_ = env ;
    }

    env = getenv( "TRAF_CONF" );
    if (env)
    {
        trafConf_ = env ;
    }
    env = getenv( "TRAF_HOME" );
    if (env)
    {
        trafHome_ = env ;
    }
    env = getenv( "TRAF_LOG" );
    if (env)
    {
        trafLog_ = env ;
    }
    env = getenv( "TRAF_VAR" );
    if (env)
    {
        trafVar_ = env ;
    }

    // setup default environment variables from monitor or last CreateProcess call
    if (maxClientBuffers)
    {
        setEnvIntVal ( childEnv, nextEnv, "SQ_LIO_MAX_BUFFERS", maxClientBuffers );
    }
    if (numProcessThreads)
    {
        setEnvIntVal ( childEnv, nextEnv, "SQ_LIO_PROCESS_THREADS",
                       numProcessThreads );
    }
    if (shellTrace)
    {
        setEnvStr ( childEnv, nextEnv, "SQ_LOCAL_IO_SHELL_TRACE=1" );
    }

    setEnvStrVal ( childEnv, nextEnv, "MPI_ROOT", mpiroot );

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
       trace_printf("%s@%d - MPI_ROOT = %s\n", method_name, __LINE__, mpiroot);

    setEnvStrVal ( childEnv, nextEnv, "MPI_TMPDIR", mpitmpdir );

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
       trace_printf("%s@%d - MPI_TMPDIR=%s\n", method_name, __LINE__,
                    mpitmpdir);

    setEnvStrVal ( childEnv, nextEnv, "MPI_FLAGS", mpiflags );

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
       trace_printf("%s@%d - MPI_FLAGS=%s\n", method_name, __LINE__, mpiflags);

    setEnvStrVal ( childEnv, nextEnv, "MPI_IC_ORDER", mpi_ic_order );

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
       trace_printf("%s@%d - MPI_IC_ORDER=%s\n", method_name, __LINE__,
                    mpi_ic_order);

    setEnvStrVal ( childEnv, nextEnv, "MPI_TEST_DELAY", mpi_test_delay );

    setEnvStrVal ( childEnv, nextEnv, "MPI_ERROR_LEVEL", mpi_error_level );

    setEnvStr ( childEnv, nextEnv, "MPI_RDMA_MSGSIZE=32768,131072,4194304" );

    setEnvStr ( childEnv, nextEnv, "HPMP_SQ=1" );

    setEnvStr ( childEnv, nextEnv, "MALLOC_ARENA_MAX=1" );

    setEnvStr ( childEnv, nextEnv, "HPMP_SINGLETON_HA=1" );

    if ( strcmp( mpi_ic_order, "IBV" ) == 0 )
    {
        setEnvStr ( childEnv, nextEnv, "MPI_HASIC_IBV=1" );
    }

    if ( Measure == 1 )
    {
        snprintf(filename,sizeof(filename),"%s/%s", mpitmpdir, Name);
        setEnvStrVal ( childEnv, nextEnv, "MPI_INSTR", filename );
    }
    else if ( Measure == 2 )
    {
        snprintf(filename,sizeof(filename),"%s/%s.cpu:cpu", mpitmpdir, Name);
        setEnvStrVal ( childEnv, nextEnv, "MPI_INSTR", filename );
    }

    setEnvIntVal ( childEnv, nextEnv, "TRAF_CLUSTER_ID", ClusterId );
    setEnvIntVal ( childEnv, nextEnv, "TRAF_INSTANCE_ID", InstanceId );

    setEnvStrVal ( childEnv, nextEnv, "TRAF_ROOT_ZNODE", trafRootZnode_.c_str() );
    setEnvStrVal ( childEnv, nextEnv, "TRAF_CONF", trafConf_.c_str() );
    setEnvStrVal ( childEnv, nextEnv, "TRAF_HOME", trafHome_.c_str() );
    setEnvStrVal ( childEnv, nextEnv, "TRAF_LOG", trafLog_.c_str() );
    setEnvStrVal ( childEnv, nextEnv, "TRAF_VAR", trafVar_.c_str() );
    setEnvStrVal ( childEnv, nextEnv, "USER", user );
    setEnvStrVal ( childEnv, nextEnv, "HOME", home );
    setEnvStrVal ( childEnv, nextEnv, "TERM", term );
    if (tz_exists)
    {
      // Note that if TZ does not exist, we don't want to set it.
      // The absence of TZ causes the glib localtime function to
      // use the local time as defined in /etc/localtime. But,
      // an invalid TZ setting (such as the empty string) causes
      // the localtime function to use UTC. So, the semantics of
      // an unset TZ are not the same as the semantics of
      // TZ=<empty string>.
      setEnvStrVal ( childEnv, nextEnv, "TZ", tz );
    }
    setEnvStrVal ( childEnv, nextEnv, "CLASSPATH", getenv("CLASSPATH"));

    if ( display )
    {
        setEnvStrVal ( childEnv, nextEnv, "DISPLAY", display );
    }
    setEnvStrVal ( childEnv, nextEnv, "XAUTHORITY", xauthority );
    setEnvStrVal ( childEnv, nextEnv, "SQ_IC", sq_ic );
    if ( vnodes && *vnodes )
    {
        setEnvStrVal ( childEnv, nextEnv, "SQ_VIRTUAL_NODES", vnodes );
        setEnvIntVal ( childEnv, nextEnv, "SQ_VIRTUAL_NID", MyPNID );
        setEnvIntVal ( childEnv, nextEnv, "SQ_LIO_VIRTUAL_NID", MyPNID );
    }

    if ( Type == ProcessType_NameServer )
    {
        setEnvStr ( childEnv, nextEnv, "SQ_MON_CREATOR=MPIRUN" );
        setEnvStr ( childEnv, nextEnv, "SQ_MON_RUN_MODE=AGENT" );
        if ( nsCommPort[0] )
            setEnvStrVal ( childEnv, nextEnv, "NS_COMM_PORT", nsCommPort );
        if ( nsSyncPort[0] )
            setEnvStrVal ( childEnv, nextEnv, "NS_SYNC_PORT", nsSyncPort );
        if ( nsMon2NsPort[0] )
            setEnvStrVal ( childEnv, nextEnv, "NS_M2N_COMM_PORT", nsMon2NsPort );
        if (nsConfigDb[0] )
            setEnvStrVal ( childEnv, nextEnv, "SQ_CONFIGDB", nsConfigDb );
    }
    if ( Type == ProcessType_Watchdog )
    {
        if ( wdtTraceCmd )
        {
            setEnvStr ( childEnv, nextEnv, "WDT_TRACE_CMD=1" );
        }
        if ( wdtTraceInit )
        {
            setEnvStr ( childEnv, nextEnv, "WDT_TRACE_INIT=1" );
        }
        if ( wdtTraceLio )
        {
            setEnvStr ( childEnv, nextEnv, "WDT_TRACE_LIO=1" );
        }
        if ( wdtTraceEntryExit )
        {
            setEnvStr ( childEnv, nextEnv, "WDT_TRACE_ENTRY_EXIT=1" );
        }
        if ( wdtKeepAliveTimer )
        {
            setEnvIntVal ( childEnv, nextEnv, "SQ_WDT_KEEPALIVETIMERVALUE", keepAliveValue );
        }
        if ( wdtMonProcRate )
        {
            setEnvIntVal ( childEnv, nextEnv, "SQ_WDT_MONITOR_PROCESS_CHECKRATE", monitorCheckRateValue );
        }
        if ( wdtLunmgrHangDelay )
        {
            setEnvIntVal ( childEnv, nextEnv, "SQ_WDT_LUNMGR_PROCESS_HANGDELAY", lunmgrHangDelayValue );
        }
        if ( wdtLinuxWatchdog )
        {
            setEnvStr ( childEnv, nextEnv, "SQ_LINUX_WATCHDOG=1" );
        }
        if ( wdtStartupTimer )
        {
            setEnvIntVal ( childEnv, nextEnv, "SQ_WDT_STARTUPTIMERVALUE", startupTimerValue );
        }
        if ( wdtDumpMonitor )
        {
            setEnvStr ( childEnv, nextEnv, "SQ_WDT_DUMP_MONITOR=1" );
        }
        if ( monAltLogEnabled )
        {
            setEnvStr ( childEnv, nextEnv, "SQ_MON_ALTLOG=1" );
        }
    }
    if ( Type == ProcessType_PSD || Type == ProcessType_SMS )
    {
        if ( monAltLogEnabled )
        {
            setEnvStr ( childEnv, nextEnv, "SQ_MON_ALTLOG=1" );
        }
    }
    if ( seamonsterEnabled )
    {
        setEnvStr ( childEnv, nextEnv, "SQ_SEAMONSTER=1" );
    }

    string LDpath;
    Config->strIdToString( ldpathStrId_, LDpath );

    if (!LDpath.empty())
    {
        setEnvStrVal( childEnv, nextEnv, "LD_LIBRARY_PATH", LDpath.c_str( ) );
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
        {
            trace_printf( "%s@%d - LD_LIBRARY_PATH = %s\n", method_name, __LINE__, LDpath.c_str() );
            trace_printf( "%s@%d - ldpathStrId_ = stringId(nid=%d, id=%d)\n"
                        , method_name, __LINE__
                        , ldpathStrId_.nid, ldpathStrId_.id );
        }
    }

    setEnvStr ( childEnv, nextEnv, "LD_BIND_NOW=true" );

    string program;
    Config->strIdToString ( programStrId_, program );
    if (!program.empty( ))
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
        {
            trace_printf( "%s@%d - Program = %s\n", method_name, __LINE__, program.c_str( ) );
            trace_printf( "%s@%d - programStrId_ = stringId(nid=%d, id=%d)\n"
                        , method_name, __LINE__
                        , programStrId_.nid, programStrId_.id );
        }
    }
   
    // temp for performance investigation
    if ( strstr(program.c_str(), "tdm_arkcmp") != NULL
      || strstr(program.c_str(), "tdm_arkesp") != NULL )
    {
        cmpOrEsp_ = true;
    }
    // Save actual program filename and set PWD environment variable
    size_t lastSlash = program.rfind('/');
    if (lastSlash == string::npos)
    {   // At top level directory
        STRCPY(filename, program.c_str());
    }
    else
    {
        STRCPY(filename, &program[lastSlash+1]);
    }
    if (lastSlash == string::npos || lastSlash == 0)
    {
        setEnvStr ( childEnv, nextEnv, "PWD=/" );

        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - PWD=/\n", method_name, __LINE__);
    }
    else
    {
        string pwd = program.substr(0, lastSlash);
        setEnvStrVal ( childEnv, nextEnv, "PWD", pwd.c_str() );
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d - PWD=%s\n", method_name, __LINE__,
                         pwd.c_str());
    }

    string path;
    Config->strIdToString( pathStrId_, path );

    setEnvStrVal( childEnv, nextEnv, "PATH", path.c_str( ) );
    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d" " - PATH = " "%s" "\n", method_name, __LINE__, path.c_str() );
        trace_printf( "%s@%d - pathStrId_ = stringId(nid=%d, id=%d)\n"
                    , method_name, __LINE__
                    , pathStrId_.nid, pathStrId_.id );
    }

    // Set values from registry as environment variables
    setEnvFromRegistry ( childEnv, nextEnv );

    xprops_exe_file = NULL;
    char *envfile = new char [strlen(trafVar_.c_str())+9];
    strcpy(envfile, trafVar_.c_str());
    strcat(envfile, "/mon.env");
    xprops.load(envfile);
    delete [] envfile;
    MON_Smap_Enum xenum(&xprops);
    if (xenum.more())
    {
        snprintf(la_buf, sizeof(la_buf),
                 "[CProcess::Create], Warning: using mon.env.\n");
    }
    while (xenum.more())
    {
        char *xkey = xenum.next();
        const char *xvalue = xprops.get(xkey);
        if (memcmp(xkey, "SQ_PROPS_", 9) == 0)
        {
            if (strcasecmp(&xkey[9], filename) == 0)
                xprops_exe_file = (char *) xvalue;
        }
        setEnvStrVal ( childEnv, nextEnv, xkey, xvalue );
        if (nextEnv > MAX_CHILD_ENV_VARS)
        {   // Exceeded array size
            nextEnv = MAX_CHILD_ENV_VARS;
            break;
        }
        if (trace_settings
            & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf("%s@%d - mon.env %s=%s\n", method_name, __LINE__, xkey,
                         xvalue );
    }
    if (xprops_exe_file != NULL)
    {
        // load exe-property-file
        xprops_exe.load(xprops_exe_file);
        MON_Smap_Enum xenum(&xprops_exe);
        while (xenum.more())
        {
            char *xkey = xenum.next();
            const char *xvalue = xprops_exe.get(xkey);
            setEnvStrVal ( childEnv, nextEnv, xkey, xvalue );
            if (nextEnv > MAX_CHILD_ENV_VARS)
            {   // Exceeded array size
                nextEnv = MAX_CHILD_ENV_VARS;
                break;
            }
            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - %s %s=%s\n", method_name, __LINE__, xprops_exe_file, xkey, xvalue);
        }
    }
    // Add environment array terminator required by execve.
    childEnv[nextEnv] = NULL;
    ++nextEnv;

    if ( !SMSIntegrating && Type == ProcessType_SMS && !Clone && !argc_ )
    {
        argv = new char *[13];
    }
    else
    {
        argv = new char *[argc_ + 13];
    }
    argv[0] = new char [strlen(filename)+1];
    strcpy(argv[0], filename);
    j = 1;

    // finish setting up arguments for process after <filename> in argv[0]
    // "SQMON1.0" <pnid> <nid> <pid> <pname> <port> <ptype> <zid> <verifier> "SPARE"
    //     [1]     [2]    [3]   [4]    [5]    [6]    [7]     [8]      [9]     [10]
    argv[j] = new char[9];
    sprintf (argv[j], "SQMON1.1");

    argv[j + 1] = new char[6];
    sprintf (argv[j + 1], "%5.5d", MyPNID);

    argv[j + 2] = new char[6];
    sprintf (argv[j + 2], "%5.5d", Nid);

    argv[j + 3] = new char[10];
    //sprintf (argv[j + 3], "%6.6d", Pid);
    strcpy(argv[j + 3],"????????"); // The Pid will be assigned later, but we can't print it then.

    argv[j + 4] = new char[strlen(Name) ? strlen(Name)+1 : MAX_PROCESS_NAME_STR];
    strcpy (argv[j + 4], Name);

    argv[j + 5] = new char[strlen (MyCommPort) + 1];
    strcpy (argv[j + 5], MyCommPort);

    argv[j + 6] = new char[6];
    sprintf (argv[j + 6], "%5.5d", Type);

    argv[j + 7] = new char[6];
    sprintf (argv[j + 7], "%5.5d", MyNode->GetZone());

    SetVerifier(); // CProcess::Create
    argv[j + 8] = new char[6];
    sprintf (argv[j + 8], "%5.5d", Verifier);

    argv[j + 9] = new char[6];
    sprintf (argv[j + 9], "SPARE");

    if ( !SMSIntegrating && Type == ProcessType_SMS && !Clone && !argc_ )
    {
        argc_ = 1;
        argv[j + 10] = new char[7];
        sprintf (argv[j + 10], "sminit");
        argv[j + 11] = NULL;
    }
    else
    {
        // now append user args
        const char *pUserArgv = userArgv_;
        int arglen;
        for (i = 0; i < argc_; i++)
        {
            arglen = strlen (pUserArgv) + 1;
            argv[i + j + 10] = new char[arglen];
            strcpy (argv[i + j + 10], pUserArgv);
            pUserArgv += arglen;
        }
        argv[i + j + 10] = NULL;
    }

    // start process and place in list
    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
    {
        trace_printf("%s@%d" " - Program='" "%s" "' argc=" "%d" "\n", method_name, __LINE__, program.c_str(), argc_ + j + 10);
        i = 0;
        while (argv[i] != NULL)
        {
            trace_printf("%s@%d" " - argv[" "%d" "]="  "%s" "\n", method_name, __LINE__, i, argv[i]);
            i++;
        }
    }

    // Create pipes for inter-process communication between new process
    // and the monitor.
    int pfds_stdin[2];
    if (pipe (pfds_stdin))
    {  // Error creating pipe
        snprintf(la_buf, sizeof(la_buf), "[%s], stdin pipe error, %s.\n",
                 method_name, strerror(errno));
        mon_log_write(MON_PROCESS_CREATE_1, SQ_LOG_ERR, la_buf);
        pfds_stdin[0] = -1;
        pfds_stdin[1] = -1;
    }

    int pfds_stdout[2];
    if (pipe (pfds_stdout))
    {  // Error creating pipe
        snprintf(la_buf, sizeof(la_buf), "[%s], stdout pipe error, %s.\n",
                 method_name, strerror(errno));
        mon_log_write(MON_PROCESS_CREATE_2, SQ_LOG_ERR, la_buf);
        pfds_stdout[0] = -1;
        pfds_stdout[1] = -1;
    }

    int pfds_stderr[2];
    if (pipe (pfds_stderr))
    {  // Error creating pipe
        snprintf(la_buf, sizeof(la_buf), "[%s], stderr pipe error, %s.\n",
                 method_name, strerror(errno));
        mon_log_write(MON_PROCESS_CREATE_3, SQ_LOG_ERR, la_buf);
        pfds_stderr[0] = -1;
        pfds_stderr[1] = -1;
    }

    MemModLock.lock();

    // make all child variable accessed only from heap
    int priority = Priority;

#ifdef USE_FORK_SUSPEND_RESUME
    mon_thread_suspend_all();
#endif // USE_FORK_SUSPEND_RESUME

    sigset_t forkMask;
    sigset_t oldMask;
    sigemptyset(&forkMask);
    sigaddset(&forkMask, SIGCHLD);
    rc = pthread_sigmask(SIG_BLOCK, &forkMask, &oldMask);
    if (rc != 0)
    {
        snprintf(la_buf, sizeof(la_buf),
                 "[%s], pthread_sigmask() error: %s (%d)\n",
                 method_name, strerror(rc), rc );
        mon_log_write(MON_PROCESS_CREATE_4, SQ_LOG_ERR, la_buf);
    }

    // this pipe is used to tell the child to go away if monitor detects
    // a duplicate pid. This can occur if there is a pending child death signal.
    int pipefd[2];
    pipe(pipefd);
    bool childGoAway = false;

    SetCreationTime(-1);
    os_pid = fork ();
    if (os_pid == -1)
    {
        // can't start a process
        rc = result = MPI_ERR_SPAWN;
    }
    else if (os_pid)
    {
        // I am monitor

        rc = pthread_sigmask(SIG_SETMASK, &oldMask, NULL);
        if (rc != 0)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], pthread_sigmask() error: %s (%d)\n",
                     method_name, strerror(rc), rc );
            mon_log_write(MON_PROCESS_CREATE_5, SQ_LOG_ERR, la_buf);
        }

        // check if process already exists with the same pid.
        if (MyNode->GetProcess(os_pid) != NULL)
        {
            rc = result = MPI_ERR_SPAWN;
            // tell the child to go away
            childGoAway = true;
            snprintf(la_buf, sizeof(la_buf),
                  "[%s], pid already exists, aborting process create: pid = %d\n",
                  method_name, os_pid );
            mon_log_write(MON_PROCESS_CREATE_4, SQ_LOG_ERR, la_buf);
        }

        // tell the child to stay or go away
        close(pipefd[0]); // close the read-end of the pipe, not going to use
        write(pipefd[1], &childGoAway, sizeof(childGoAway));
        close(pipefd[1]); // close the write-end of the pipe, sending EOF.

        if (childGoAway)
        {   // no need to continue connecting with child
            goto forkExit;
        }

        // I'm the monitor ... connect to child
        rc = MPI_SUCCESS;

        // Save process id and build process name if not already named
        Pid = os_pid;
        if (Name[0] == '\0')
        {   // No name assigned to the process so generate one based on
            // the node-id and process-id.
            MyNode->BuildOurName(Nid, os_pid, Name);

            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - No process name specified, generated name=%s for process (%d, %d)\n", method_name, __LINE__, Name, Nid, os_pid);
        }

        if (NameServerEnabled && tag != NULL)
        {
            // Send actual pid and process name back to parent
            // STDIO Redirection requires that clone process in parent node
            // have the actual pid
            rc2 = PtpClient->ProcessInit( this
                                        , tag
                                        , 0
                                        , parent->Nid );
            if (rc2)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                CLNode *parentLNode = NULL;
                parentLNode = Nodes->GetLNode( parent->Nid );
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Can't send process create success "
                          "for process %s (%d, %d) "
                          "to parent node %s, nid=%d\n"
                        , method_name
                        , GetName()
                        , GetNid()
                        , GetPid()
                        , parentLNode->GetNode()->GetName()
                        , parentLNode->GetNid() );
                mon_log_write(MON_PROCESS_CREATE_12, SQ_LOG_ERR, la_buf);
            }
        }

        if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
            trace_printf("%s@%d Process=%s, Infile=[%s], Outfile=[%s]\n",
                         method_name, __LINE__, Name, infile_.c_str(),
                         outfile_.c_str());

        // stdin pipe to child:
        //    We don't need read end of pipe.
        //    Add the write end of file descriptor to list of file
        //       descriptors monitored.
        if (pfds_stdin[1] != -1)
        {
            close(pfds_stdin[0]);

            // Decide on standard input source for the
            // process.  It will either be a filename on this node
            // or handled by a specific process on another node.
            int AncestorNid = -1;
            int AncestorPid = -1;
            char Stdfile[MAX_PROCESS_PATH] = {0};
            if (PickStdfile(PICK_STDIN, Stdfile, AncestorNid, AncestorPid))
            {
                Redirector.stdinFd(Nid, os_pid, pfds_stdin[1], Stdfile,
                                   AncestorNid, AncestorPid);
                fd_stdin_ = pfds_stdin[1];
            }
            else
            {
                if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
                    trace_printf("%s@%d Unable to find stdin file for "
                                 "Process=%s, pid=%d.  Closing stdin pipe "
                                 "fd=%d\n", method_name, __LINE__, Name,
                                 os_pid, pfds_stdin[1]);
                close ( pfds_stdin[1] );
            }
        }

        // stdout pipe to child:
        //    We don't need write end of pipe.
        //    Add the read end of file descriptor to list of file
        //       descriptors monitored.
        if (pfds_stdout[0] != -1)
        {
            close(pfds_stdout[1]);

            // Decide on standard output destination for the
            // process.  It will either be a filename on this node
            // or handled by a specific process on another node.
            int AncestorNid = -1;
            int AncestorPid = -1;
            char Stdfile[MAX_PROCESS_PATH] = {0};
            if (!PickStdfile(PICK_STDOUT, Stdfile, AncestorNid, AncestorPid))
            {  // Unable to locate stdout file.  So create a file based
               // on the process name and use that for output.
                strcpy(Stdfile, "stdout_");
                strcat(Stdfile, Name);
                if (trace_settings & (TRACE_PROCESS | TRACE_REDIRECTION))
                    trace_printf("%s@%d Unable to find stdout file for "
                             "process=%s, pid=%d.  Using file %s for stdout.\n",
                                 method_name, __LINE__, Name, os_pid, Stdfile);
            }
            Redirector.stdoutFd(Nid, os_pid, pfds_stdout[0], Stdfile,
                                AncestorNid, AncestorPid);

            fd_stdout_ = pfds_stdout[0];
        }

        // stderr pipe to child:
        //    We don't need write end of pipe.
        //    Add the read end of file descriptor to list of file
        //       descriptors monitored.
        if (pfds_stderr[0] != -1)
        {
            close(pfds_stderr[1]);
            Redirector.stderrFd(MyNode->GetHostname(), Name, Nid, os_pid, pfds_stderr[0]);
            fd_stderr_ = pfds_stderr[0];
        }

    forkExit:
        // release fork semaphore so child can get it
        if ( sem_post(MyNode->GetMutex()) == -1 )
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[CProcess::Create], Parent can't put mutex.\n");
            mon_log_write(MON_PROCESS_CREATE_7, SQ_LOG_ERR, la_buf);
        }
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d - Parent put mutex so child can proceed.\n",
                        method_name, __LINE__);

#ifdef USE_FORK_SUSPEND_RESUME
        mon_thread_resume_suspended();
#endif // USE_FORK_SUSPEND_RESUME

    }
// LCOV_EXCL_START
// Exclude the following from monitor code coverage measurement since
// it is executed by a child process not the monitor process.
    else
    {
        // I'm the child process

        // Take fork semaphore.  We need to wait until parent indicates
        // it is ok to proceed.  Pipes between parent and child need to
        // be set up before child can continue.
        bool sem_log_error = false;
        int sem_rc;
        int err = 0;
        struct timeval logTime;
        struct tm *ltime;
        struct timespec ts;

        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            err = errno;
            gettimeofday(&logTime, NULL);
            ltime = localtime(&logTime.tv_sec);
            snprintf(la_buf, sizeof(la_buf),
                     "%02d/%02d/%02d-%02d:%02d:%02d "
                     "[CProcess::Create], clock_gettime(CLOCK_REALTIME),"
                     " Child can't get time, %s (%d), program %s, (pid=%d).\n"
                     , ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year-100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec
                     , strerror(err), err
                     , filename, getpid());
            write (2, la_buf, strlen(la_buf));
        }
        ts.tv_sec  += 1;

        env = getenv( "MON_CREATE_SEM_DELAY" );
        if (env && isdigit(*env))
        {
            ts.tv_sec = atol(env);
        }

        env = getenv( "MON_CREATE_SEM_LOG_ERROR" );
        if (env && isdigit(*env))
        {
            int val = atoi(env);
            sem_log_error = (val != 0) ? true : false;
        }
        do
        {
            sem_rc = sem_timedwait(MyNode->GetMutex(), &ts);
            err = errno;
            if ( sem_log_error && err == ETIMEDOUT )
            {
                gettimeofday(&logTime, NULL);
                ltime = localtime(&logTime.tv_sec);
                snprintf(la_buf, sizeof(la_buf),
                         "%02d/%02d/%02d-%02d:%02d:%02d "
                         "[CProcess::Create], Child can't take mutex,"
                         " %s (%d), program %s, (pid=%d).\n"
                         , ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year-100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec
                         , strerror(err), err
                         , filename, getpid());
                write (2, la_buf, strlen(la_buf));
            }
        }
        while (sem_rc == -1 && (err == EINTR || err == ETIMEDOUT));

        if ( sem_log_error && sem_rc == -1 && !(err == EINTR || err == ETIMEDOUT))
        {
            gettimeofday(&logTime, NULL);
            ltime = localtime(&logTime.tv_sec);
            snprintf(la_buf, sizeof(la_buf),
                     "%02d/%02d/%02d-%02d:%02d:%02d "
                     "[CProcess::Create], Child can't take mutex,"
                     " %s (%d), program %s, (pid=%d).\n"
                     , ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year-100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec
                     , strerror(errno), errno
                     , filename, getpid());
            write (2, la_buf, strlen(la_buf));
        }

        // check if monitor wanted child to stay or go away
        close(pipefd[1]); // close the write-end, not going to use
        // read till EOF
        while (read(pipefd[0], &childGoAway, sizeof(childGoAway)) > 0);
        close(pipefd[0]); // close the read-end of the pipe

        if (childGoAway)
        {
            _exit( ENOEXEC );
        }

        // set the process's process id to the os process id for compatability
        pid_t myPid = getpid();
        sprintf (argv[j + 3], "%6.6d", myPid);

        char *pName = argv[j + 4];
        if (pName[0] == '\0')
        {   // No name assigned to the process so generate one based on
            // the node-id and process-id.
            MyNode->BuildOurName(Nid, myPid, pName);
        }

        // Unmask all allowed signals in the child
        // except SIGUSR1
        sigset_t              mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
        if (rc != 0)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[CProcess::Create], pthread_sigmask() error:"
                     " %s (%d), program %s.\n", strerror(rc), rc, filename);
            write (2, la_buf, strlen(la_buf));
        }

        // set child process's priority based on minimums and specified value
        nice(priority);

        // Redirect standard input, standard output, standard error
        RedirectStdFiles(pfds_stdin, pfds_stdout, pfds_stderr);

        // Close file descriptors opened by the monitor parent except
        // for stdin, stdout, stderr.
        MyNode->close_fds ();

        char *name;
        size_t pathlen;

        // Get program search path
        pathlen = path.length();

        size_t len;
        len = strlen(filename) + 1;

        // Allocate space to hold the pathnames + filename
        size_t alloclen;
        alloclen = pathlen + len + 1;
        name = new char[alloclen];

        // Place the program filename at the end of the buffer preceeded
        // by a slash.
        name =  (char *) memcpy(name + pathlen + 1, filename, len);
        *--name = '/';

        // Try to find the program in the directories specified by PATH.
        // Each element of the path is tried until we find the program
        // or run out of elements to try.
        const char *pEnd;
        const char *pStart;

        pEnd = path.c_str();
        do
        {
            char *startp;

            pStart = pEnd;
            pEnd = strchr(pStart, ':');
            if (!pEnd)
                pEnd = strchr(pStart, '\0');

            if (pEnd == pStart)
                // Two adjacent colons, or a colon at the beginning or the end
                // of `PATH' means to search the current directory.
                startp = name + 1;
            else
                // Copy the next path into the buffer just before the
                // program filename.
                startp = (char *) memcpy(name - (pEnd - pStart), pStart, pEnd - pStart);

            // Try to execute this name.  If it works, execve will not return.
            execve( startp, argv, childEnv);

            switch  (errno)
            {
            case EACCES:
            case ENOENT:
            case ESTALE:
            case ENOTDIR:
            case ENODEV:
            case ETIMEDOUT:
            case ENOEXEC:
                // Those errors indicate the file is missing or not
                // executable by us, in which case we want to just try
                // the next path directory.
                break;

            default:
                // Some other error means we found an executable file, but
                // something went wrong executing it; return the error to
                // our caller.
                goto execFailed;
            }
        }  while (*pEnd++ != '\0');

    execFailed:
        // The specified program could not be executed.  Note that at this
        // point we are executing as the child process.  We will exit and
        // the monitor will get a "child death" signal and take the
        // appropriate actions.
        //
        // It's probably not possible to log an error at this point
        // since the error logging mechanism is probably not available
        // at this early stage of child process startup.  We can write to
        // the standard error file descriptor since the monitor has set that
        // up as a pipe back to itself.

        snprintf(la_buf, sizeof(la_buf),
                 "Unable to execute program %s, %s (%d).\n",
                 filename, strerror(errno), errno);
        write (2, la_buf, strlen(la_buf));

        _exit( errno );
    }
// LCOV_EXCL_STOP

    MemModLock.unlock();

    if (rc == MPI_SUCCESS && result == MPI_SUCCESS)
    {
        successful = true;
        PidAtFork_  = os_pid;

        // Indicate that process exists but has not yet completed initialization.
        State_ = State_Initializing;

        MyNode->SetAffinity( this );

        if (Backup)
        {
            if ( !parent )
            {   // Unexpectedly have null parent pointer
                snprintf(la_buf, sizeof(la_buf),
                         "[CProcess::CProcess], No Primary for Backup process!\n");
                mon_log_write(MON_PROCESS_PROCESS_2, SQ_LOG_ERR, la_buf);
            }
            else if (strcmp (parent->Name, Name) != 0)
            {
                snprintf(la_buf, sizeof(la_buf),
                         "[CProcess::Create], Primary & Backup process name "
                         "don't match!\n");
                mon_log_write(MON_PROCESS_CREATE_10, SQ_LOG_ERR, la_buf);
            }
            else
            {
                // primary & backup processes are parent's of each other
                parent->Parent_Nid = Nid;
                parent->Parent_Pid = Pid;
                parent->Backup = false;
                if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                   trace_printf("%s@%d" " - Assigning parent nid=" "%d" ", pid=" "%d"  " with (really child) parent nid=" "%d" ", parent pid=" "%d" "\n", method_name, __LINE__, parent->Nid, parent->Pid, Nid, Pid);
            }
        }

        if ( Backup )
        {   // For a backup process the "parent" is the CProcess object
            // for the primary process.  So find the real parent process
            // object.
            parent = Nodes->GetLNode ( PairParentNid )
                          ->GetProcessL( PairParentPid );
        }

        if ( !UnHooked &&  parent  && !Backup )
        {   // Parent process object keeps track of child processes
            // created on this node.  Needed in case parent process
            // exits abnormally.

            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL
                                  | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - Child process %s (%d, %d) added to "
                             "parent %s (%d, %d)\n", method_name, __LINE__,
                             Name, Nid, Pid, parent->GetName(),
                             parent->GetNid(), parent->GetPid());

            parent->childAdd ( Nid, Pid );

        }

        Monitor->writeProcessMapBegin( Name, Nid, Pid, Verifier,
                                       parent ? parent->GetNid() : -1,
                                       parent ? parent->GetPid() : -1,
                                       parent ? parent->GetVerifier() : -1,
                                       program.c_str() );
    }
    else
    {
        successful = false;
        result = MPI_ERR_SPAWN;

        if (NameServerEnabled)
        {
            rc2 = PtpClient->ProcessInit( this
                                        , tag
                                        , result
                                        , parent->Nid );
            if (rc2)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                CLNode *parentLNode = NULL;
                parentLNode = Nodes->GetLNode( parent->Nid );
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] - Can't send process create failure "
                          "for process %s (%d, %d) "
                          "result to parent node %s, nid=%d, result=%d\n"
                        , method_name
                        , GetName()
                        , GetNid()
                        , GetPid()
                        , parentLNode->GetNode()->GetName()
                        , parentLNode->GetNid(), result );
                mon_log_write(MON_PROCESS_CREATE_13, SQ_LOG_ERR, la_buf);
            }
        }

        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[CProcess::Create], Failed to start process %s path= %s.\n", Name, path.c_str());
        mon_log_write(MON_PROCESS_CREATE_11, SQ_LOG_ERR, buf);
    }

    // release allocated memory
    for (i = 0; argv[i]; i++)
    {
        delete [] argv[i];
    }
    delete [] argv;

    for (i = 0; childEnv[i]; i++)
    {
        delete [] childEnv[i];
    }

    TRACE_EXIT;

    return successful;
}
#endif

#ifndef NAMESERVER_PROCESS
bool CProcess::Dump (CProcess *dumper, char *core_path)
{
    bool status = FAILURE;
    CReplDump *repl;

    const char method_name[] = "CProcess::Dump";
    TRACE_ENTRY;

    switch (DumpState)
    {
        case Dump_Ready:
            DumpState = Dump_Pending;
            dumpFile_ = core_path;
            DumperNid = dumper->Nid;
            DumperPid = dumper->Pid;
            DumperVerifier = dumper->Verifier;
            status = SUCCESS;
            if (trace_settings & TRACE_PROCESS)
                trace_printf("%s@%d - DumpState=Dump_Pending, "
                             "target %s (%d, %d:%d), "
                             "dumper %s (%d, %d:%d), core path:%s\n"
                             , method_name, __LINE__
                             , Name
                             , Nid
                             , Pid
                             , Verifier
                             , dumper->Name
                             , dumper->Nid
                             , dumper->Pid
                             , dumper->Verifier
                             , core_path?core_path:"" );
            if ( NameServerEnabled )
            {
                int rc = -1;

                rc = PtpClient->ProcessDump(this);
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Dump process request failed: "
                              "target %s (%d, %d:%d)\n"
                            , method_name
                            , Name
                            , Nid
                            , Pid
                            , Verifier );
                    mon_log_write(MON_PROCESS_DUMP_1, SQ_LOG_ERR, la_buf);
                }
            }
            else
            {
                repl = new CReplDump(this);
                Replicator.addItem(repl);
            }
            break;

        default:
            if (trace_settings & TRACE_PROCESS)
                trace_printf("%s@%d - Dump already in progress, pid=%d\n",
                             method_name, __LINE__, Pid);
            break;
    }

    TRACE_EXIT;

    return status;
}
#endif

#ifndef NAMESERVER_PROCESS
static void cprocess_dump_cb(void *ctx, pid_t pid, int status)
{
    CLNode   *lnode = static_cast<CLNode *>(ctx);
    lnode->DumpCallback( lnode->GetNid(), pid, status );
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::DumpBegin (int nid, int pid, Verifier_t verifier, char *core_path)
{
    char           *argv[6];
    char           *cmd;
    char            core_file[MAX_PROCESS_PATH];
    char            core_pid[20];
    char            date[20];
    int             err;
    struct timeval  tv;
    struct tm       tx;

    const char method_name[] = "CProcess::DumpBegin";
    TRACE_ENTRY;

    DumperNid = nid;
    DumperPid = pid;
    DumperVerifier = verifier;
    if (Clone)
    {
        DumpState = Dump_InProgress;
    }
    else
    {
        // Increment reference count for process object until DumpEnd
        incrReplRef();

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tx);
        snprintf(date, sizeof(date), "%d-%02d-%02d_%02d-%02d-%02d",
                tx.tm_year + 1900,
                tx.tm_mon + 1,
                tx.tm_mday,
                tx.tm_hour,
                tx.tm_min,
                tx.tm_sec);

        string program;
        Config->strIdToString ( programStrId_, program );

        cmd = rindex((char *) program.c_str(), '/');
        if (cmd == NULL)
            cmd = (char *) program.c_str();
        else
            cmd++; // past '/'

        // Override core_path if directory specified in /proc/sys/kernel/core_pattern
        FILE * procCorePatternFile;  // "/proc/sys/kernel/core_pattern" file descriptor

        procCorePatternFile = fopen("/proc/sys/kernel/core_pattern", "r");
        if (!procCorePatternFile)
        {
            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], Cannot open /proc/sys/kernel/core_pattern, %s (%d)\n",
                    method_name, strerror(errno), errno);
            mon_log_write( MON_PROCESS_DUMP_BEGIN_1, SQ_LOG_ERR, buf );
        }
        else
        {
            char corePattern[132] = {0};
            char corePath[132] = {0};

            fgets( corePattern, 132, procCorePatternFile );
            if (strlen( corePattern) )
            {
                strncpy( corePath, corePattern, sizeof(corePath) );
                if (corePath[0] == '/')
                {
                    char *pch = strrchr( corePath, '/' );
                    if (pch)
                    {
                        *pch = 0;
                        strcpy( core_path, corePath );
                    }
                }
                if (trace_settings & TRACE_PROCESS)
                {
                    trace_printf( "%s@%d - corePath=%s\n"
                                , method_name, __LINE__, corePath );
                    trace_printf( "%s@%d - core_path=%s\n"
                                , method_name, __LINE__, core_path );
                    trace_printf( "%s@%d - corePattern=%s\n"
                                , method_name, __LINE__, corePattern );
                }
            }
            else
            {
                if (trace_settings & TRACE_PROCESS)
                {
                    trace_printf("%s@%d - undefined corePattern=%s\n",
                                 method_name, __LINE__, corePattern);
                }
            }
            fclose(procCorePatternFile);
        }

        // date=%Y-%m-%d_%H-%M-%S
        // core_file=<path>/core.<date>.<pname>.<pid>.<cmd>
        snprintf(core_file, sizeof(core_file), "%s/core.%s.%s.%d.%d.%s",
                core_path,
                date,
                &Name[1],
                Nid,
                Pid,
                cmd);
        corefile_ = core_file;

        if (trace_settings & TRACE_PROCESS)
            trace_printf( "%s@%d - starting mondump - "
                          "target %s (%d, %d:%d), "
                          "dumper (%d, %d:%d), "
                          "core-file=%s\n"
                        , method_name, __LINE__
                        , Name
                        , Nid
                        , Pid
                        , Verifier
                        , DumperNid
                        , DumperPid
                        , DumperVerifier
                        , core_file);

        argv[0] = (char *) "mondump";
        snprintf(core_pid, sizeof(core_pid), "%d", Pid);
        argv[1] = core_pid;
        argv[2] = core_file;
        argv[3] = NULL;
        CLNode   *lnode = Nodes->GetLNode( Nid );
        err = IntProcess.create(argv[0],
                                argv,
                                cprocess_dump_cb, // cb
                                Pid,              // cb_pid
                                lnode,            // cb_ctx
                                NULL);
        if (err == 0)
        {
            dumpFile_ = core_file;
            DumpState = Dump_InProgress;
        }
        else
        {
            DumpState = Dump_Complete;
            if ( NameServerEnabled )
            {
                int rc = -1;
    
                rc = PtpClient->ProcessDumpComplete(this);
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Dump complete reply to dumper failed: "
                              "target %s (%d, %d:%d), "
                              "dumper (%d, %d:%d)\n"
                            , method_name
                            , Name
                            , Nid
                            , Pid
                            , Verifier
                            , DumperNid
                            , DumperPid
                            , DumperVerifier );
                    mon_log_write(MON_PROCESS_DUMP_BEGIN_2, SQ_LOG_ERR, la_buf);
                }
            }
            else
            {
                CReplDumpComplete *repl = new CReplDumpComplete(this);
                Replicator.addItem(repl);
            }
            CompleteDump(Dump_Failed, NULL);
        }
    }

    if (trace_settings & TRACE_PROCESS)
    {
        if (DumpState == Dump_InProgress)
            trace_printf( "%s@%d - DumpState=Dump_InProgress, "
                          "target %s (%d, %d:%d), "
                          "dumper (%d, %d:%d)\n"
                        , method_name, __LINE__
                        , Name
                        , Nid
                        , Pid
                        , Verifier
                        , DumperNid
                        , DumperPid
                        , DumperVerifier );
        else
            trace_printf( "%s@%d - DumpState=Dump_Complete, "
                          "target %s (%d, %d:%d), "
                          "dumper (%d, %d:%d)\n"
                        , method_name, __LINE__
                        , Name
                        , Nid
                        , Pid
                        , Verifier
                        , DumperNid
                        , DumperPid
                        , DumperVerifier );
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
const char *DumpStateString( DUMPSTATE state)
{
    const char *str;

    switch( state )
    {
        case Dump_Unknown:
            str = "Dump_Unknown";
            break;
        case Dump_Ready:
            str = "Dump_Ready";
            break;
        case Dump_Pending:
            str = "Dump_Pending";
            break;
        case Dump_InProgress:
            str = "Dump_InProgress";
            break;
        case Dump_Complete:
            str = "Dump_Complete";
            break;
        default:
            str = "DumpState - Undefined";
            break;
    }

    return( str );
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::DumpEnd (DUMPSTATUS status, char *core_file)
{
    const char method_name[] = "CProcess::DumpEnd";
    TRACE_ENTRY;

    if (trace_settings & TRACE_PROCESS)
        trace_printf("%s@%d - name=%s, DumpState=%s, DumpStatus=%d, pid=%d, core_file=%s\n",
                     method_name, __LINE__, Name, DumpStateString(DumpState), status, Pid, core_file);

    if ( DumpState != Dump_Ready )
    {
        CompleteDump(status, core_file);
    }

    // Decrement reference count for process object
    decrReplRef();

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
struct message_def * CProcess::DeathMessage( )
{
    struct message_def *msg;

    const char method_name[] = "CProcess::DeathMessage";
    TRACE_ENTRY;

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->notice_death_Incr();

    msg = new struct message_def;
    msg->type = MsgType_ProcessDeath;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    msg->u.request.u.death.nid = Nid;
    msg->u.request.u.death.pid = Pid;
    msg->u.request.u.death.verifier = Verifier;
    msg->u.request.u.death.trans_id.txid[0] = 0;
    msg->u.request.u.death.trans_id.txid[1] = 0;
    msg->u.request.u.death.trans_id.txid[2] = 0;
    msg->u.request.u.death.trans_id.txid[3] = 0;
    msg->u.request.u.death.aborted = IsAbended();
    strcpy(msg->u.request.u.death.process_name, Name);
    msg->u.request.u.death.type = Type;
#ifdef USE_SEQUENCE_NUM
    msg->u.request.u.death.seqnum = Monitor->GetTimeSeqNum();
#endif

    if (trace_settings & ( TRACE_TMSYNC | TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Death notice for process %s (%d, %d)\n",
                     method_name, __LINE__, Name, Nid, Pid );
    TRACE_EXIT;

    return msg;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::Exit( CProcess *parent )
{
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcess::Exit";
    TRACE_ENTRY;

    if ( DumpState != Dump_Ready )
    {
        DumpEnd( Dump_Failed, (char *)corefile_.c_str() );
    }

    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
       trace_printf( "%s@%d" " - Process %s (%d,%d:%d) is exiting, parent process %s (%d,%d:%d)\n"
                   , method_name, __LINE__
                   , GetName(), GetNid(), GetPid(), GetVerifier()
                   , parent?parent->GetName():""
                   , parent?parent->GetNid():-1
                   , parent?parent->GetPid():-1
                   , parent?parent->GetVerifier():-1 );

    SetState(State_Stopped);

    if (!Clone && parent && NameServerEnabled)
    {
        if (parent->GetNid() != GetNid())
        { // parent is remote
            if (parent->childCount() == 0)
            { // process is parent's last child
                if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                {
                    trace_printf( "%s@%d - Parent's last child, deleting clone process %s, (%d,%d:%d)\n"
                                , method_name, __LINE__
                                , parent->GetName()
                                , parent->GetNid()
                                , parent->GetPid()
                                , parent->GetVerifier() );
                }
                Nodes->DeleteCloneProcess( parent );
                parent = NULL;
            }
            else
            {
                ProcessInfoNs_reply_def processInfo;
                int rc = Nodes->GetProcessInfoNs( parent->GetNid()
                                                , parent->GetPid()
                                                , parent->GetVerifier()
                                                , &processInfo);
                if (rc == MPI_ERR_NAME)
                { // parent exited
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC | TRACE_TMSYNC))
                    {
                        trace_printf( "%s@%d - Deleting clone process %s, (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , parent->GetName()
                                    , parent->GetNid()
                                    , parent->GetPid()
                                    , parent->GetVerifier() );
                    }
                    Nodes->DeleteCloneProcess( parent );
                    parent = NULL;
                }
            }
        }
    }

    // if the env is set to not deliver death messages upon node down,
    // check the state of the process' node.
    bool supplyProcessDeathNotices = true;
    if (!Monitor->IsNodeDownDeathNotices())
    {
        CNode * node = Nodes->GetLNode(GetNid())->GetNode();
        // if process' node is being killed, do not supply process death notices
        supplyProcessDeathNotices = !node->IsKillingNode();
    }

    if(  NoticeHead &&
        !MyNode->IsKillingNode() &&
        !(Type == ProcessType_DTM && IsAbended()) &&
        supplyProcessDeathNotices )
    {
        if ( !Clone && NameServerEnabled )
        {
            // Notify all remote registered nodes of this process' death
            NoticeHead->NotifyRemote();
        }
        // Notify all local registered processes of this process' death
        NoticeHead->NotifyAll();
    }

    if ( !Clone && !Paired )
    {
        switch (Type)
        {
            case ProcessType_TSE:
            case ProcessType_ASE:
                MyNode->delFromQuiesceExitPids( GetPid(), GetVerifier() );

                if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
                    trace_printf("%s%d: pid %d deleted from quiesce exit list\n", method_name, __LINE__, GetPid());

                if (MyNode->isInQuiesceState())
                {
                    if (MyNode->isQuiesceExitPidsEmpty())
                    {
                        HealthCheck.setState(MON_SCHED_NODE_DOWN);  // schedule a node down req
                    }
                }
                else
                {   // unmount volumes only if node is not quiescing.
                    Devices->UnMountVolume( Name, Backup );
                }
                break;
            case ProcessType_DTM:
                if ( IsAbended() )
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf( "%s@%d - DTM abended %s (%d, %d:%d)\n"
                                   , method_name, __LINE__, Name, Nid, Pid, Verifier);
                    if ( !MyNode->IsKillingNode() &&
                         !IsPersistent() &&
                          MyNode->GetShutdownLevel() != ShutdownLevel_Abrupt )
                    {
                        MyNode->SetDTMAborted( true );
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - DTM stopped normally" "\n", method_name, __LINE__);
                    if ( !MyNode->IsKillingNode() &&
                         !IsPersistent() &&
                          MyNode->GetShutdownLevel() == ShutdownLevel_Undefined )
                    {
                        MyNode->SetDTMAborted( true );
                    }
                    else
                    {
                        if ( Monitor->GetTmLeader() == MyPNID )
                        {
                            // set the clean shutdown condition
                            char key[MAX_KEY_NAME];
                            char value[10];
                            strcpy(key,"Clean_Shutdown");
                            strcpy(value,"True");
                            Config->GetClusterGroup()->Set( key, value );
                        }
                    }
                }
                break;
            case ProcessType_SMS:
                if ( IsAbended() )
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf( "%s@%d - SMS abended %s (%d, %d:%d)\n"
                                   , method_name, __LINE__, Name, Nid, Pid, Verifier);
                    if ( !MyNode->IsKillingNode() &&
                          MyNode->GetShutdownLevel() != ShutdownLevel_Abrupt )
                    {
                        MyNode->SetSMSAborted( true );
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - SMS stopped normally" "\n", method_name, __LINE__);
                    if ( !MyNode->IsKillingNode() &&
                          MyNode->GetShutdownLevel() == ShutdownLevel_Undefined )
                    {
                        MyNode->SetSMSAborted( true );
                    }
                }
                break;
            case ProcessType_NameServer:
                if ( IsAbended() )
                {
                    if (!Clone)
                    {
                        NameServer->NameServerExited();
                    }
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - NameServer abended" "\n", method_name, __LINE__);
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - NameServer stopped normally" "\n", method_name, __LINE__);
                }
                break;
            case ProcessType_Watchdog:
                if ( IsAbended() )
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - Watchdog abended" "\n", method_name, __LINE__);
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                       trace_printf("%s@%d" " - Watchdog stopped normally" "\n", method_name, __LINE__);
                }
                break;
            case ProcessType_MXOSRVR:
            case ProcessType_Generic:
                if ( MyNode->GetState() == State_Up &&
                    !MyNode->IsKillingNode() &&
                     MyNode->GetShutdownLevel() == ShutdownLevel_Undefined )
                {
                    // Send logical node's SSMP process this process' death message
                    CLNode *lnode = MyNode->GetLNode( Nid );
                    if ( lnode )
                    {
                        CProcess *ssmpProcess = lnode->GetSSMProc();
                        if ( ssmpProcess && Pid != -1 )
                        {
                            if (trace_settings & TRACE_PROCESS)
                                trace_printf("%s@%d: Queueing death notice for SSMP process for %s (%d, %d:%d)\n",
                                             method_name, __LINE__, Name, Nid, Pid, Verifier);

                            ssmpProcess->ssmpNoticesLock_.lock();
                            ssmpProcess->ssmpNotices_.push_back( DeathMessage() );
                            ssmpProcess->ssmpNoticesLock_.unlock();
                            SQ_theLocalIOToClient->nudgeNotifier ();
                        }
                        else
                        {
                            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL | TRACE_NOTICE ))
                                trace_printf("%s@%d: No SSMP process found in nid=%d\n",
                                             method_name, __LINE__, lnode->GetNid());
                        }
                    }
                }
                break;

            case ProcessType_SSMP:
                // Indicate no SSM process on this node.
                Nodes->GetLNode ( Nid )->SetSSMProc ( NULL );
                break;

            case ProcessType_AMP:
            case ProcessType_Backout:
            case ProcessType_VolumeRecovery:
            case ProcessType_SPX:
            case ProcessType_PSD:
            case ProcessType_PERSIST:
            case ProcessType_TMID:
                // No special handling needed on exit
                break;
            default:
                snprintf(la_buf, sizeof(la_buf)
                        , "[CProcess::Exit], Invalid process type(%d)! "
                          "%s (%d,%d:%d)\n"
                        , Type, Name , Nid, Pid, Verifier);
                mon_log_write(MON_PROCESS_EXIT_1, SQ_LOG_ERR, la_buf);
        }

        // Remove this child process from parent's child-process-list.
        if ( (parent != NULL) && (parent->GetState() == State_Up) )
        {
            parent->childRemove( Nid, Pid);
            if (NameServerEnabled)
            {
                parent->childUnHookedRemove( Nid, Pid);
            }
        }

        // Check if we need to output a entry into the process id map log file
        if ( PidMap )
        {
            Monitor->writeProcessMapEnd( Name, Nid, Pid, Verifier,
                                         parent ? parent->GetNid() : -1,
                                         parent ? parent->GetPid() : -1,
                                         parent ? parent->GetVerifier() : -1,
                                         program() );
        }
    }

    if ( Clone && Pid != -1 )
    {
        if ( Type == ProcessType_SPX &&
             MyNode->GetShutdownLevel() == ShutdownLevel_Undefined &&
             supplyProcessDeathNotices )
        {
            // Send local SPX this SPX's death message
            CLNode *lnode = MyNode->GetFirstLNode();
            for ( ; lnode; lnode = lnode->GetNextP() )
            {
                CProcess *spxProcess = lnode->GetProcessLByType( ProcessType_SPX );
                if ( spxProcess && MyNode->GetState() == State_Up )
                {
                    SQ_theLocalIOToClient->putOnNoticeQueue( spxProcess->Pid
                                                           , spxProcess->Verifier
                                                           , DeathMessage()
                                                           , NULL);

                    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                       trace_printf( "%s@%d" " - Sending death message of %s (%d,%d:%d) to %s (%d,%d:%d)\n"
                                   , method_name, __LINE__
                                   , GetName(), GetNid(), GetPid(), GetVerifier()
                                   , spxProcess->GetName(), spxProcess->GetNid()
                                   , spxProcess->GetPid(), spxProcess->GetVerifier());
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
                        trace_printf("%s@%d: No SPX process found in nid=%d\n",
                                     method_name, __LINE__, lnode->GetNid());
                }
            }
        }

        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
            trace_printf( "%s@%d" " - Death message check of %s (%d,%d:%d) type=%s, My node phase=%s, send death notices=%d\n"
                        , method_name, __LINE__
                        , GetName(), GetNid(), GetPid(), GetVerifier()
                        , ProcessTypeString(GetType()), NodePhaseString( MyNode->GetPhase() )
                        , supplyProcessDeathNotices );
    }

    if ( parent && !parent->IsClone() && Pid != -1 )
    {

        // If process and parent are DTMs suppress death
        // message here, it was delivered above
        if ( parent->IsSystemMessages()   &&
             parent->GetState() == State_Up &&
             !MyNode->IsKillingNode() &&
             !(GetType() == ProcessType_DTM &&
               parent->GetType()  == ProcessType_DTM) &&
             supplyProcessDeathNotices )
        {
            SQ_theLocalIOToClient->putOnNoticeQueue( parent->Pid
                                                   , parent->Verifier
                                                   , DeathMessage()
                                                   , NULL);

            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
               trace_printf( "%s@%d" " - Sending death message of %s (%d,%d:%d) to %s (%d,%d:%d) \n"
                           , method_name, __LINE__
                           , GetName(), GetNid(), GetPid(), GetVerifier()
                           , parent->GetName(), parent->GetNid()
                           , parent->GetPid(), parent->GetVerifier());
        }
        else
        {
            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
               trace_printf("%s@%d" " - Parent doesn't want Death message" "\n", method_name, __LINE__);
        }
    }

    if (NameServerEnabled)
    {
        if ( parent )
        {
            if ( parent->IsClone() && Pid != -1 )
            {
                int targetNid = parent->GetNid();
                CLNode *targetLNode = Nodes->GetLNode( targetNid );
                // Send the process exit to the parent node
                int rc = PtpClient->ProcessExit( this
                                               , targetNid
                                               , targetLNode->GetNode()->GetName() );
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send process exit "
                              "for process %s (%d, %d) "
                              "to parent node %s, nid=%d\n"
                            , method_name
                            , GetName()
                            , GetNid()
                            , GetPid()
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_PROCESS_PROCEXIT_1, SQ_LOG_ERR, la_buf);
                }
            }
        }
        else
        {
            if (GetParentNid() != -1)
            {
                int targetNid = GetParentNid();
                CLNode *targetLNode = Nodes->GetLNode( targetNid );
                // Send the process exit to the parent node
                int rc = PtpClient->ProcessExit( this
                                               , targetNid
                                               , targetLNode->GetNode()->GetName() );
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send process exit "
                              "for process %s (%d, %d) "
                              "to parent node %s, nid=%d\n"
                            , method_name
                            , GetName()
                            , GetNid()
                            , GetPid()
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_PROCESS_PROCEXIT_2, SQ_LOG_ERR, la_buf);
                }
            }
        }
        procExitNotifierNodes();
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::GenerateEvent( int event_id, int length, char *data )
{
    struct message_def *msg;

    const char method_name[] = "CProcess::GenerateEvent";
    TRACE_ENTRY;
    if( Clone )
    {
        if ( Event_messages )
        {
            // Replicate the event to other nodes
            CReplEvent *repl = new CReplEvent(event_id, length, data, Nid, Pid, Verifier);
            Replicator.addItem(repl);
        }
    }
    else
    {
        if ( Event_messages )
        {
            msg = new struct message_def;
            msg->type = MsgType_Event;
            msg->noreply = true;
            msg->u.request.type = ReqType_Notice;
            msg->u.request.u.event_notice.event_id = event_id;
            msg->u.request.u.event_notice.length = length;
            memset( msg->u.request.u.event_notice.data, 0, MAX_SYNC_DATA );
            if (length && data)
            {
                memmove( msg->u.request.u.event_notice.data, data, (length>MAX_SYNC_DATA)?MAX_SYNC_DATA:length );
            }

            SQ_theLocalIOToClient->putOnNoticeQueue( Pid
                                                   , Verifier
                                                   , msg
                                                   , NULL);
        }
    }
    TRACE_EXIT;
}
#endif

CProcess *CProcess::GetBackup (void)
{
    CLNode *node = NULL;
    CProcess *parent = NULL;
    CProcess *backup = NULL;

    node = Nodes->GetLNode (Parent_Nid);
    if (node)
    {
        parent = node->GetProcessL(Parent_Pid);
        if (parent)
        {
            backup = (parent->Backup ? parent : NULL);
        }
    }

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
       trace_printf("CProcess::GetBackup" "@%d" " - name= %s(%d:%d), parent=%p(%s), backup=%p" "\n", __LINE__, Name, Parent_Nid, Parent_Pid, parent, parent ? parent->Name : "None", backup);

    return backup;
}


// see: CProcessContainer::GetProcess (int pid)
// see: CProcessContainer::GetProcess (char *name, bool checkstate)

CProcess *CProcess::GetProcessByType( PROCESSTYPE type )
{
    CProcess *entry = this;

    const char method_name[] = "CProcess::GetProcessByType";
    TRACE_ENTRY;
    while (entry)
    {
        if (entry->Type == type)
        {
            // Only return entry if it has completed startup
            if (entry->State_ != State_Up)
            {
                entry = NULL;
            }
            break;
        }
        entry = entry->next_;
    }
    TRACE_EXIT;
    return entry;
}

// see: CLNode::GetProcessL (int pid)
// see: CLNode::GetProcessL (char *name, bool checkstate)

CProcess *CProcess::GetProcessLByType( PROCESSTYPE type )
{
    CProcess *entry = this;

    const char method_name[] = "CProcess::GetProcessLByType";
    TRACE_ENTRY;
    while (entry)
    {
        if (entry->Type == type)
        {
            // Only return entry if it has completed startup
            if (entry->State_ != State_Up)
            {
                entry = NULL;
            }
            break;
        }
        entry = entry->nextL_;
    }
    TRACE_EXIT;
    return entry;
}

bool CProcess::MakePrimary (void)
{
    bool successful;
    CLNode *node = NULL;
    CProcess *primary = NULL;
    CProcess *backup = NULL;

    const char method_name[] = "CProcess::MakePrimary";
    TRACE_ENTRY;
    if (Backup)
    {
        backup = this;
        if (Parent_Nid != -1)
        {
            node = Nodes->GetLNode (Parent_Nid);
            if (node)
            {
                if (Parent_Pid != -1)
                {
                    primary = node->GetProcessL(Parent_Pid);
                    if (!primary)
                    {
                        if (trace_settings & TRACE_REQUEST_DETAIL)
                           trace_printf("%s@%d" " - Can't find Primary process" "\n", method_name, __LINE__);
                    }
                }
            }
            else
            {
                if (trace_settings & TRACE_REQUEST_DETAIL)
                   trace_printf("%s@%d" " - Can't find Primary process's node" "\n", method_name, __LINE__);
            }
        }
    }
    else
    {
        primary = this;
        if (Parent_Nid != -1)
        {
            node = Nodes->GetLNode (Parent_Nid);
            if (node)
            {
                if (Parent_Pid != -1)
                {
                    backup = node->GetProcessL(Parent_Pid);
                    if (backup)
                    {
                        backup = (backup->Backup ? backup : NULL);
                    }
                }
                else
                {
                    if (trace_settings & TRACE_REQUEST_DETAIL)
                       trace_printf("%s@%d" " - Can't find Backup process" "\n", method_name, __LINE__);
                }
            }
            else
            {
                if (trace_settings & TRACE_REQUEST_DETAIL)
                   trace_printf("%s@%d" " - Can't find Backup process's node" "\n", method_name, __LINE__);
            }
        }
    }

    if (primary == this)
    {
        if (trace_settings & TRACE_REQUEST_DETAIL)
           trace_printf("%s@%d" "- Primary process will continue as Primary" "\n", method_name, __LINE__);
        if (!backup)
        {
            primary->Parent_Nid = -1;
            primary->Parent_Pid = -1;
        }
        successful = true;
    }
    else if (backup == this)
    {
        backup->Backup = false;
        if (primary)
        {
            primary->Backup = true;
            if (trace_settings & TRACE_REQUEST_DETAIL)
               trace_printf("%s@%d" "- Old Primary process is now the Backup" "\n", method_name, __LINE__);
        }
        else
        {
            backup->Parent_Nid = -1;
            backup->Parent_Pid = -1;
        }
        if (trace_settings & TRACE_REQUEST_DETAIL)
           trace_printf("%s@%d" "- Backup process is now the Primary" "\n", method_name, __LINE__);
        successful = true;
    }
    else
    {
        successful = false;
    }

    TRACE_EXIT;
    return successful;
}

#ifndef NAMESERVER_PROCESS
bool CProcess::Open (CProcess * opened_process, int death_notification)
{
    const char method_name[] = "CProcess::Open";
    TRACE_ENTRY;

    bool status;

    if ((opened_process->StartupCompleted) &&
        (opened_process->State_ == State_Up) && (State_ == State_Up))
    {
        if ( death_notification
             && !((opened_process->Parent_Nid == Nid) &&
                  (opened_process->Parent_Pid == Pid)) )
        {
            _TM_Txid_External transid;
            transid = null_trans();
            opened_process->RegisterDeathNotification( Nid
                                                     , Pid
                                                     , Verifier
                                                     , Name
                                                     , transid);
        }
        status = SUCCESS;
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[CProcess::Open], Can't Open Process %s "
                 "has not completed startup protocol!\n", opened_process->Name);
        mon_log_write(MON_PROCESS_OPEN_1, SQ_LOG_ERR, buf);

        status = FAILURE;
    }
    TRACE_EXIT;

    return status;
}
#endif

void CProcessContainer::close_fds ( void )
{
    DIR *dirp = opendir("/proc/self/fd");
    for (;;)
    {
        if (dirp == NULL)
            break;
        struct dirent *direntp = readdir(dirp);
        if (direntp == NULL)
            break;
        if (direntp->d_ino == 0) // invalid inode-number
            continue;
        if (direntp->d_name[0] == '.') // relative
            continue;
        int fd;
        sscanf(direntp->d_name, "%d", &fd);
        if (fd > 2)
            close(fd);
    }
    if (dirp != NULL)
        closedir(dirp);
}

#ifndef NAMESERVER_PROCESS
CNotice *CProcess::RegisterDeathNotification( int nid
                                            , int pid
                                            , Verifier_t verifier
                                            , const char *name
                                            , _TM_Txid_External trans_id )
{
    CNotice *notice = NULL;

    const char method_name[] = "CProcess::RegisterDeathNotification";
    TRACE_ENTRY;

    deathInterestLock_.lock();

    if ( NoticeHead )
    {
        notice = NoticeHead->GetNotice( nid, pid, verifier, trans_id );
    }
    if ( notice == NULL )
    {
        notice = new CNotice (nid, pid, verifier, name, trans_id, this);
        if (NoticeHead == NULL)
        {
            NoticeHead = NoticeTail = notice;
        }
        else
        {
            NoticeTail = NoticeTail->Link (notice);
        }
    }
    else
    {
        // We have a duplicate registation request for notification.
        // Just return original notice object without error.
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST))
           trace_printf("%s@%d" " - Already have registered for this notice" "\n", method_name, __LINE__);
    }

    deathInterestLock_.unlock();

    TRACE_EXIT;
    return notice;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcess::ReplyNewProcess (struct message_def * reply_msg,
                                CProcess * process, int result)
{
    const char method_name[] = "CProcess::ReplyNewProcess";
    TRACE_ENTRY;

    // the parent gets a new_process reply
    reply_msg->type = MsgType_Service;
    reply_msg->noreply = false;
    reply_msg->reply_tag = process->ReplyTag;
    reply_msg->u.reply.type = ReplyType_NewProcess;
    reply_msg->u.reply.u.new_process.nid = process->Nid;
    reply_msg->u.reply.u.new_process.pid = process->Pid;
    reply_msg->u.reply.u.new_process.verifier = process->Verifier;
    strcpy (reply_msg->u.reply.u.new_process.process_name,process->Name);
    reply_msg->u.reply.u.new_process.return_code = result;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_SYNC))
        trace_printf("%s@%d - Created process %s (%d, %d:%d), sending reply to "
                     "%s (%d, %d), result=%d\n", method_name, __LINE__,
                     process->Name, process->Nid, process->Pid, process->Verifier,
                     Name, Nid, Pid, result);

    // send reply to the parent
    SQ_theLocalIOToClient->sendCtlMsg
        ( Pid, MC_SReady, ((SharedMsgDef*)reply_msg)-> trailer.index );

    TRACE_EXIT;
}
#endif


#ifndef NAMESERVER_PROCESS
void CProcess::SendProcessCreatedNotice(CProcess *parent, int result)
{
    const char method_name[] = "CProcess::SendProcessCreatedNotice";
    TRACE_ENTRY;

    struct message_def *reply_msg;

    reply_msg = new struct message_def;

    // the parent gets a child started notice
    reply_msg->type = MsgType_ProcessCreated;
    reply_msg->noreply = true;
    reply_msg->u.request.type = ReqType_Notice;
    reply_msg->u.request.u.process_created.nid = Nid;
    reply_msg->u.request.u.process_created.pid = Pid;
    reply_msg->u.request.u.process_created.verifier = Verifier;
    reply_msg->u.request.u.process_created.tag = Tag;
    strcpy(reply_msg->u.request.u.process_created.port, Port);
    strcpy(reply_msg->u.request.u.process_created.process_name, Name);
    reply_msg->u.request.u.process_created.return_code = result;
    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS | TRACE_SYNC))
        trace_printf("%s@%d - Created process %s (%d, %d), sending process "
                     "created notice to %s (%d, %d), result=%d\n",
                     method_name, __LINE__, Name, Nid, Pid,
                     parent->Name, parent->Nid, parent->Pid, result);

    // send notice to the parent
    SQ_theLocalIOToClient->putOnNoticeQueue( parent->Pid
                                           , parent->Verifier
                                           , reply_msg
                                           , NULL);

    TRACE_EXIT;
}
#endif

struct message_def * CProcess::GetDeathNotice( void )
{
    const char method_name[] = "CProcess::GetDeathNotice";
    TRACE_ENTRY;

    struct message_def *notice = NULL;

    ssmpNoticesLock_.lock();
    if ( ! ssmpNotices_.empty() )
    {
        notice = ssmpNotices_.front();
        if ( notice )
        {
            ssmpNotices_.pop_front();
        }
    }
    ssmpNoticesLock_.unlock();

    TRACE_EXIT;

    return notice;
}

void CProcess::PutDeathNotice( struct message_def * notice)
{
    const char method_name[] = "CProcess::PutDeathNotice";
    TRACE_ENTRY;

    ssmpNoticesLock_.lock();
    ssmpNotices_.push_front ( notice );
    ssmpNoticesLock_.unlock();

    TRACE_EXIT;
}

void CProcess::Switch( CProcess *parent )
{
    const char method_name[] = "CProcess::Switch";
    TRACE_ENTRY;

    if (parent)
    {
        if (IsBackup())
        {
            if (GetPid() == parent->GetParentPid())
            {
                // The parent now doesn't have a backup
                parent->SetParentNid ( -1 );
                parent->SetParentPid ( -1 );
                parent->SetParent ( NULL );
            }
            else
            {
               if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                   trace_printf("%s@%d" " - Parent not our primary" "\n", method_name, __LINE__);
            }
        }
        if (parent->IsBackup())
        {
            if (GetPid() == parent->GetParentPid())
            {
                // The parent is now the primary
                parent->SetBackup ( false );
                parent->SetParentNid ( -1 );
                parent->SetParentPid ( -1 );
                parent->SetParent ( NULL );
                if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf("%s@%d" " - Backup taking over, Name=" "%s" "\n", method_name, __LINE__, parent->GetName());
            }
            else
            {
                if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf("%s@%d" " - Parent not our backup" "\n", method_name, __LINE__);
            }
        }
    }

    TRACE_EXIT;
}


CProcessContainer::CProcessContainer (void)
                  :numProcs_(0)
                  ,nodeContainer_(false)
                  ,processNameFormatLong_(true)
                  ,nameMap_(NULL)
                  ,pidMap_(NULL)
                  ,head_(NULL)
                  ,tail_(NULL)
{
    const char method_name[] = "CProcessContainer::CProcessContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PCTR", 4);

    //create & initialize existing semaphore
    char sem_name[MAX_PROCESS_PATH];
    snprintf(sem_name,sizeof(sem_name), "/monitor.sem.%s", getenv("USER"));
    Mutex = sem_open(sem_name,O_CREAT,0644,0);
    if(Mutex == SEM_FAILED)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf(buf, sizeof(buf), "[%s], Can't create semaphore %s!\n",
                 method_name, sem_name);
        mon_log_write(MON_PROCESSCONT_PROCESSCONT_1, SQ_LOG_ERR, buf);

        sem_unlink(sem_name);

        mon_failure_exit();
    }

#ifndef NAMESERVER_PROCESS
    char *env = getenv("SQ_MON_PROCESS_NAME_FORMAT_LONG");
    if ( env && isdigit(*env) )
    {
        int val = atoi(env);
        processNameFormatLong_ = (val != 0) ? true : false;
    }
#endif

    TRACE_EXIT;
}

CProcessContainer::CProcessContainer( bool nodeContainer )
                  :numProcs_(0)
                  ,nodeContainer_(nodeContainer)
                  ,processNameFormatLong_(true)
                  ,nameMap_(NULL)
                  ,pidMap_(NULL)
                  ,head_(NULL)
                  ,tail_(NULL)
{
    const char method_name[] = "CProcessContainer::CProcessContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PCTR", 4);

    //create & initialize existing semaphore
    char sem_name[MAX_PROCESS_PATH];
    snprintf(sem_name,sizeof(sem_name), "/monitor.sem.%s", getenv("USER"));
    Mutex = sem_open(sem_name,O_CREAT,0644,0);
    if(Mutex == SEM_FAILED)
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = errno;
        snprintf(buf, sizeof(buf), "[%s], Can't create semaphore %s! (%s)\n",
                 method_name, sem_name, strerror(err));
        mon_log_write(MON_PROCESSCONT_PROCESSCONT_3, SQ_LOG_ERR, buf);

        err = sem_unlink(sem_name);
        if (err == -1)
        {
            int err = errno;
            snprintf(buf, sizeof(buf), "[%s], Can't unlink semaphore %s! (%s)\n",
                     method_name, sem_name, strerror(err));
            mon_log_write(MON_PROCESSCONT_PROCESSCONT_4, SQ_LOG_ERR, buf);
        }

        mon_failure_exit();
    }

#ifndef NAMESERVER_PROCESS
    char *env = getenv("SQ_MON_PROCESS_NAME_FORMAT_LONG");
    if ( env && isdigit(*env) )
    {
        int val = atoi(env);
        processNameFormatLong_ = (val != 0) ? true : false;
    }
#endif

    if ( nodeContainer_ )
    {
        nameMap_ = new nameMap_t;
        pidMap_ = new pidMap_t;
    }

    TRACE_EXIT;
}

CProcessContainer::~CProcessContainer (void)
{
    const char method_name[] = "CProcessContainer::~CProcessContainer";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        CleanUpProcesses();
        if ( nameMap_ )
        {
            delete nameMap_;
        }
        if ( pidMap_ )
        {
            delete pidMap_;
        }
    }

    sem_close(Mutex);
    char sem_name[MAX_PROCESS_PATH];
    snprintf(sem_name,sizeof(sem_name), "/monitor.sem.%s", getenv("USER"));
    sem_unlink(sem_name);

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "pctr", 4);

    TRACE_EXIT;
}

void CProcessContainer::AddToPidMap(int pid, CProcess *process)
{
    const char method_name[] = "CProcessContainer::AddToPidMap";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    pair<pidMap_t::iterator, bool> ret;

    if (pid != -1)
    {
        // temp trace, remove once USE_PROCESS_MAPS is default
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d inserting into pidMap %p: %d, %s (%d, %d)\n"
                        , method_name, __LINE__
                        , pidMap_, pid
                        , process->GetName(), process->GetNid(), process->GetPid());
        }

        pidMapLock_.lock();
        ret = pidMap_->insert( pidMap_t::value_type ( pid, process ));
        pidMapLock_.unlock();
        if (ret.second == false)
        {   // Already had an entry with the given key value
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d pid map already contained process %d\n",
                             method_name, __LINE__, pid);
            }
        }

        // temp trace
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d pidMap_ (%p) now has %d entries\n",
                         method_name, __LINE__, pidMap_, (int)pidMap_->size());
        }

    }

    TRACE_EXIT;
}

void CProcessContainer::DelFromPidMap( CProcess *process )
{
    const char method_name[] = "CProcessContainer::DelFromPidMap";
    TRACE_ENTRY;

    pidMapLock_.lock();
    int count = pidMap_->erase ( process->GetPid() );
    pidMapLock_.unlock();

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {
        if (count != 0)
        {
            trace_printf("%s@%d removed from pidMap %p: %s (%d, %d)\n",
                         method_name, __LINE__, pidMap_,
                         process->GetName(), process->GetNid(), process->GetPid());
        }
    }

    if ( process->GetPid() != process->GetPidAtFork() )
    {   // Process id changed after fork(). [This could happen if, for
        // example, a shell script was the originally started process
        // and it then started the actual process.
        pidMapLock_.lock();
        int count = pidMap_->erase ( process->GetPidAtFork() );
        pidMapLock_.unlock();

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            if (count != 0)
            {
                trace_printf("%s@%d removed from pidMap %p: %s (%d, %d)\n",
                             method_name, __LINE__, pidMap_,
                             process->GetName(), process->GetNid(),
                             process->GetPidAtFork());
            }
        }
    }

    TRACE_EXIT;
}

void CProcessContainer::AddToNameMap( CProcess *process )
{
    const char method_name[] = "CProcessContainer::AddToNameMap";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    pair<nameMap_t::iterator, bool> ret1;

    if ( strlen(process->GetName()) != 0 )
    {

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d inserting into nameMap %p: %s (%d, %d)\n", method_name, __LINE__, nameMap_, process->GetName(), process->GetNid(), process->GetPid());
        }

        nameMapLock_.lock();
        ret1 = nameMap_->insert( nameMap_t::value_type ( process->GetName(),
                                                        process ));
        nameMapLock_.unlock();
        if (ret1.second == false)
        {   // Already had an entry with the given key value.  This is not
            // necessarily an error.  One sceario where this can happen is
            // if a new process request contains a user assigned process
            // name and the process is to be created on another node.
            // When the InternalType_ProcInit replication message is
            // processed on the originating node we'll attempt to re-add
            // the name (a system generated name will be added for the first
            // time at this point.)
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d nameMap %p already contained process %s\n",
                             method_name, __LINE__, nameMap_, process->GetName());
            }
        }

        // temp trace
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d nameMap_ (%p) now has %d entries\n",
                         method_name, __LINE__, nameMap_,
                         (int)nameMap_->size());
        }

    }

    TRACE_EXIT;
}

void CProcessContainer::DelFromNameMap( CProcess *process )
{
    const char method_name[] = "CProcessContainer::DelFromNameMap";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    CProcess *p2 = GetProcess ( process->GetName(), false );
    if ( p2 == NULL)
    {  // Process was not in the map, no need to erase
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d not removing from nameMap %p: %s (%d, %d)."
                         "  No such mapping\n",
                         method_name, __LINE__, nameMap_,
                         process->GetName(), process->GetNid(), process->GetPid());
        }
    }
    else if (p2 != process)
    {
        // Name was in map but process object is not what we were expecting
        // so leave it alone
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d not removing from nameMap %p: %s (%d, %d)."
                         "  Map contains %s (%d, %d)\n",
                         method_name, __LINE__, nameMap_,
                         process->GetName(), process->GetNid(), process->GetPid(),
                         p2->GetName(), p2->GetNid(), p2->GetPid());
        }
    }
    else
    {

        nameMapLock_.lock();
        int count = nameMap_->erase ( process->GetName() );
        nameMapLock_.unlock();

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            if (count != 0)
            {
                trace_printf("%s@%d removed from nameMap %p: %s (%d, %d)\n",
                             method_name, __LINE__, nameMap_,
                             process->GetName(), process->GetNid(), process->GetPid());
            }
        }
    }

}

void CProcessContainer::AddToList(CProcess *process)
{
    const char method_name[] = "CProcessContainer::AddToList";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    if (process)
    {
        // link it to the CNode container
        if (head_ == NULL)
        {
            head_ = tail_ = process;
            process->prev_ = NULL;
        }
        else
        {
            tail_->next_ = process;
            process->prev_ = tail_;
            tail_ = process;
        }
        process->next_ = NULL;
        numProcs_++;

        // link it to the CLNode container
        CLNode *lnode = Nodes->GetLNode( process->Nid );
        lnode->AddToListL( process );

        if (trace_settings & (TRACE_PROCESS_DETAIL))
        {
            CNode *node = lnode->GetNode();
            trace_printf("%s@%d" " container %p - pnid=%d, process count=%d, pnode=%d" "\n", method_name, __LINE__, this, node->GetPNid(), numProcs_, nodeContainer_);
        }

        AddToNameMap(process);
        if ( process->Pid != -1 )
        {
            AddToPidMap(process->Pid, process);
        }

    }

    TRACE_EXIT;
}

void CProcessContainer::AddToListL(CProcess *process)
{
    const char method_name[] = "CProcessContainer::AddToListL";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CLNode (the logical node)
        abort();
    }

    if (process)
    {
        // link it to the CLNode container
        if (head_ == NULL)
        {
            head_ = tail_ = process;
            process->prevL_ = NULL;
        }
        else
        {
            tail_->nextL_ = process;
            process->prevL_ = tail_;
            tail_ = process;
        }
        process->nextL_ = NULL;
        numProcs_++;

        if (trace_settings & (TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d" " - container %p nid=%d, process count=%d, pnode=%d" "\n", method_name, __LINE__, this, process->Nid, numProcs_, nodeContainer_);
        }
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CProcessContainer::AttachProcessCheck ( struct message_def *msg )
{
    CProcess *process;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcessContainer::AttachProcessCheck";
    TRACE_ENTRY;

    assert ( msg != NULL);

    if ( msg->u.request.u.startup.startup_size != sizeof(msg->u.request.u.startup) )
    {
        snprintf(la_buf, sizeof(la_buf), "[%s], Startup message from %s has invalid size=%d, expecting size=%d\n",
                 method_name, msg->u.request.u.startup.process_name,
                 msg->u.request.u.startup.startup_size,
                 (int) sizeof(msg->u.request.u.startup));
        mon_log_write(MON_PROCESSCONT_ATTACHPCHECK_1, SQ_LOG_ERR, la_buf);

        abort(); // TODO: revisit
    } else if ((MyNode->GetState() != State_Up &&
                MyNode->GetState() != State_Shutdown) &&
               ( strcmp(msg->u.request.u.startup.program,"shell")!=0 )   )
    {
        // Check if we can accept a connection
        snprintf(la_buf, sizeof(la_buf), "[%s], Can't accept %s because node is logically down\n", method_name, msg->u.request.u.startup.process_name);
        mon_log_write(MON_PROCESSCONT_ATTACHPCHECK_1, SQ_LOG_ERR, la_buf);

        msg->u.reply.type = ReplyType_Generic;
        msg->u.reply.u.generic.nid = -1;
        msg->u.reply.u.generic.pid = -1;
        msg->u.reply.u.generic.verifier = -1;
        msg->u.reply.u.generic.process_name[0] = '\0';
        msg->u.reply.u.generic.return_code = MPI_ERR_OP;
    }

    // shell is trying to attach across all nodes
    else if (msg->u.request.u.startup.paired)
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_SYNC | TRACE_INIT | TRACE_PROCESS))
           trace_printf("%s@%d" " - paired attach" "\n", method_name, __LINE__);
        Nodes->GetLNode (msg->u.request.u.startup.process_name, &process);
        if (process)
        {
            process->SetPaired ( true );
            process->SetClone( false );
            msg->u.reply.type = ReplyType_Generic;
            msg->u.reply.u.generic.nid = process->GetNid();
            msg->u.reply.u.generic.pid = process->GetPid();
            msg->u.reply.u.generic.verifier = process->GetVerifier();
            strcpy (msg->u.reply.u.generic.process_name, process->GetName());
            msg->u.reply.u.generic.return_code = MPI_SUCCESS;
        }
        else
        {
            // Can't find process
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Can't find or clone Process %s to pair attach!\n",
                     method_name, msg->u.request.u.startup.process_name);
            mon_log_write(MON_PROCESSCONT_ATTACHPCHECK_2, SQ_LOG_ERR, la_buf);

            msg->u.reply.type = ReplyType_Generic;
            msg->u.reply.u.generic.nid = -1;
            msg->u.reply.u.generic.pid = -1;
            msg->u.reply.u.generic.verifier = -1;
            msg->u.reply.u.generic.process_name[0] = '\0';
            msg->u.reply.u.generic.return_code = MPI_ERR_NAME;
        }
    }
    // check if its an attach request, if so setup the process
    else if ((msg->u.request.u.startup.nid == -1) &&
             (msg->u.request.u.startup.pid == -1)   )
    {
        Nodes->GetLNode (msg->u.request.u.startup.process_name, &process);
        if (!process)
        {

            if (trace_settings & (TRACE_REQUEST | TRACE_SYNC | TRACE_PROCESS))
               trace_printf("%s@%d" " - process attaching" "\n", method_name, __LINE__);
            if ( ! nodeContainer_ )
            {
                // Programmer bonehead :^)
                // This must only be called from MyNode (the local physical node)
                abort();
            }
            if ( ! MyNode->IsSpareNode() )
            {
                int nid = MyNode->AssignNid();
                if ( nid == -1 )
                {
                    snprintf( la_buf, sizeof(la_buf),
                            "[%s], Can't attach the pid %d (program: %s) - the monitor is not up yet (curr state: %d).\n",
                            method_name,
                            msg->u.request.u.startup.os_pid,
                            msg->u.request.u.startup.program,
                            MyNode->GetState() );
                    mon_log_write( MON_PROCESSCONT_ATTACHPCHECK_4, SQ_LOG_ERR, la_buf );

                    msg->u.reply.type = ReplyType_Generic;
                    msg->u.reply.u.generic.nid = -1;
                    msg->u.reply.u.generic.pid = -1;
                    msg->u.reply.u.generic.verifier = -1;
                    msg->u.reply.u.generic.process_name[0] = '\0';
                    msg->u.reply.u.generic.return_code = MPI_ERR_NAME;
                }
                else
                {
                    strId_t progStrId = MyNode->GetStringId( msg->u.request.u.startup.program );
                    strId_t nullStrId = { -1, -1 };
                    process =
                        new CProcess( NULL, nid, msg->u.request.u.startup.os_pid, ProcessType_Generic, 0, 0, false, true, (char *) "",
                        nullStrId, nullStrId, progStrId, (char *) "", (char *) "" );
                    if ( process == NULL )
                    {
                        //TODO: Log event
                        abort();
                    }
                    if ( process )
                    {
                        char user_argv[MAX_ARGS][MAX_ARG_SIZE];
                        process->userArgs( 0, user_argv );
                    }
                    if ( msg->u.request.u.startup.process_name[0] == '\0' )
                    {   // Create a name for the process and place it in the
                        // Name member of the process object);
                        char pname[MAX_KEY_NAME];
                        MyNode->BuildOurName( nid, process->GetPid( ), pname );
                        process->SetName( pname );
                    }
                    else
                    {
                        process->SetName(
                            MyNode->NormalizeName( msg->u.request.u.startup.process_name ) );
                    }
                    process->SetAttached( true );
                    process->SetupFifo( process->GetNid( ), msg->u.request.u.startup.os_pid );
                    process->SetCreationTime( msg->u.request.u.startup.os_pid );
                    process->SetVerifier( ); // CProcessContainer::AttachProcessCheck
                    AddToList( process );
                    process->CompleteProcessStartup( msg->u.request.u.startup.port_name, // CProcessContainer::AttachProcessCheck
                                                     msg->u.request.u.startup.os_pid,
                                                     msg->u.request.u.startup.event_messages,
                                                     msg->u.request.u.startup.system_messages,
                                                     false,
                                                     NULL,
                                                     MyPNID );

                    msg->u.reply.type = ReplyType_Startup;
                    msg->u.reply.u.startup_info.nid = process->GetNid( );
                    msg->u.reply.u.startup_info.pid = process->GetPid( );
                    msg->u.reply.u.startup_info.verifier = process->GetVerifier( );
                    strcpy( msg->u.reply.u.startup_info.process_name, process->GetName( ) );
                    msg->u.reply.u.startup_info.return_code = MPI_SUCCESS;
                    STRCPY( msg->u.reply.u.startup_info.fifo_stdin,
                            process->fifo_stdin() );
                    STRCPY( msg->u.reply.u.startup_info.fifo_stdout,
                            process->fifo_stdout() );
                    STRCPY( msg->u.reply.u.startup_info.fifo_stderr,
                            process->fifo_stderr() );

                    Monitor->writeProcessMapBegin( process->GetName( )
                                                 , process->GetNid( )
                                                 , process->GetPid( )
                                                 , process->GetVerifier( )
                                                 , -1, -1, -1
                                                 , msg->u.request.u.startup.program );
                }
            }
            else
            {
                snprintf( la_buf, sizeof(la_buf),
                        "[%s], Can't attach, node is a spare node!\n",
                        method_name );
                mon_log_write( MON_PROCESSCONT_ATTACHPCHECK_3, SQ_LOG_ERR, la_buf );

                msg->u.reply.type = ReplyType_Startup;
                msg->u.reply.u.startup_info.nid = -1;
                msg->u.reply.u.startup_info.pid = -1;
                msg->u.reply.u.startup_info.verifier = -1;
                msg->u.reply.u.startup_info.process_name[0] = '\0';
                msg->u.reply.u.startup_info.return_code = MPI_ERR_NO_MEM;
            }
        }
        else
        {
            // Find the duplicate process
            snprintf( la_buf, sizeof(la_buf),
                     "[%s], Can't attach duplicate process %s!\n",
                     method_name, msg->u.request.u.startup.process_name );
            mon_log_write( MON_PROCESSCONT_ATTACHPCHECK_4, SQ_LOG_ERR, la_buf );

            msg->u.reply.type = ReplyType_Generic;
            msg->u.reply.u.generic.nid = -1;
            msg->u.reply.u.generic.pid = -1;
            msg->u.reply.u.generic.verifier = -1;
            msg->u.reply.u.generic.process_name[0] = '\0';
            msg->u.reply.u.generic.return_code = MPI_ERR_NAME;
        }
    }
    // complete a monitor child process startup
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_SYNC | TRACE_PROCESS))
           trace_printf("%s@%d" " - child attach" "\n", method_name, __LINE__);
        Monitor->CompleteProcessStartup(msg);
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::Bcast (struct message_def *msg)
{
    CProcess *process = NULL;
    SharedMsgDef *shm = NULL;
    SQ_LocalIOToClient::bcastPids_t *bcastPids = NULL;
    unsigned int msgSize;

    const char method_name[] = "CProcessContainer::Bcast";
    TRACE_ENTRY;

    // Prepare a broadcast notice for sending by the local io "pending
    // notice thread".   Do this by formatting an image of the message
    // to be sent along with a the list of process ids that will receive
    // the notice.
    pidMapLock_.lock();
    pidMap_t::iterator pidMapIt;
    for ( pidMapIt = pidMap_->begin(); pidMapIt != pidMap_->end() ; pidMapIt++ )
    {
        process = pidMapIt->second;
        assert( process );
        if (process->IsSystemMessages() &&
            process->GetState() == State_Up)
        {
            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_RECOVERY | TRACE_SYNC_DETAIL | TRACE_TMSYNC | TRACE_PROCESS_DETAIL))
                trace_printf( "%s@%d - Send notice to %s (%d, %d:%d)\n"
                            , method_name, __LINE__
                            , process->GetName()
                            , process->GetNid()
                            , process->GetPid()
                            , process->GetVerifier() );

            if (!shm)
            {   // First process, allocate a buffer for the notice image
                // and initialize it.
                shm = new SharedMsgDef;
                memset( &shm->trailer, 0, sizeof(shm->trailer) );
                bcastPids = new SQ_LocalIOToClient::bcastPids_t;
                assert(bcastPids);

                msgSize = SQ_theLocalIOToClient->getSizeOfMsg( msg );

                if ( msgSize > sizeof ( message_def ) )
                {   // Not expected to occur but guard against client
                    // buffer overrun
                    msgSize = sizeof ( message_def );
                }

                memcpy( &shm->msg, msg, msgSize );
                shm->trailer.OSPid = BCAST_PID;
                shm->trailer.verifier = -1;
            }
            // Add this process id to the list.
            SQ_LocalIOToClient::pidVerifier_t pv;
            pv.pv.pid = process->GetPid();
            pv.pv.verifier = process->GetVerifier();
            bcastPids->insert( pv.pnv );
        }
    }
    pidMapLock_.unlock();

    if (shm)
    {
        SQ_theLocalIOToClient->putOnNoticeQueue( BCAST_PID
                                               , -1
                                               , &shm->msg
                                               , bcastPids);
    }

    TRACE_EXIT;
}
#endif

char *CProcessContainer::BuildOurName( int nid, int pid, char *name )
{
    const char method_name[] = "CProcessContainer::BuildOurName";
    TRACE_ENTRY;

    // We are skipping 'A', 'I', 'O', and 'U' to distinguish between zero
    // and one digits, and for political correctness in generated names
    static char b32table[32] =  {'0','1','2','3','4','5','6','7','8','9'
                                ,'B','C','D','E','F','G','H','J','K','L','M'
                                ,'N','P','Q','R','S','T','V','W','X','Y','Z' };

    int i;
    int rem;
    int cnt[6];

    if (!processNameFormatLong_)
    {
        // Convert Pid into base 35 acsii
        cnt[0] = pid / 42875;    // (35 * 35 * 35)
        rem = pid - ( cnt[0] * 42875 );
        cnt[1] = rem / 1225;     // (35 * 35)
        rem -= ( cnt[1] * 1225 );
        cnt[2] = rem / 35;
        rem -= ( cnt[2] * 35 );
        cnt[3] = rem;
    
        // Process name format long: '$Zxxpppp' xx = nid, pppp = pid

        // Convert Nid into base 16 acsii
        sprintf(name,"$Z%2.2X",nid);

        // Convert Pid into base 36 ascii
        for(i=3; i>=0; i--)
        {
            if( cnt[i] < 10 )
            {
                name[i+4] = '0'+cnt[i];
            }
            else
            {
                cnt[i] -= 10;
                // we are skipping cap 'o' because it looks like zero.
                if( cnt[i] >= 14 )
                {
                    name[i+4] = 'P'+(cnt[i]-14);
                }
                else
                {
                    name[i+4] = 'A'+cnt[i];
                }
            }
        }
        name[8] = '\0';
    }
    else
    {
        // Process name format long: '$Zxxxxpppppp' xxxx = <nid>, pppppp = <pid>
        sprintf(name,"$Z");
    
        // Convert <nid> into base 32 (1,048,575)
        cnt[0] = nid / 32768;       // (32 * 32 * 32)
        rem = nid - ( cnt[0] * 32768 );
        cnt[1] = rem / 1024;        // (32 * 32)
        rem -= ( cnt[1] * 1024 );
        cnt[2] = rem / 32;
        rem -= ( cnt[2] * 32 );
        cnt[3] = rem;
    
        // Convert <nid> into base 32 ascii
        for(i=3; i>=0; i--)
        {
            name[i+2] = static_cast<char>(b32table[cnt[i]]);
        }
    
        // Convert <pid> into base 32 (1,073,741,823)
        cnt[0] = pid / 33554432;    // (32 * 32 * 32 * 32 * 32)
        rem = pid - ( cnt[0] * 33554432 );
        cnt[1] = rem / 1048576;     // (32 * 32 * 32 * 32)
        rem -= ( cnt[1] * 1048576 );
        cnt[2] = rem / 32768;       // (32 * 32 * 32)
        rem -= ( cnt[2] * 32768 );
        cnt[3] = rem / 1024;        // (32 * 32)
        rem -= ( cnt[3] * 1024 );
        cnt[4] = rem / 32;
        rem -= ( cnt[4] * 32 );
        cnt[5] = rem;
    
        // Convert <pid> into base 32 ascii
        for(i=5; i>=0; i--)
        {
            name[i+6] = static_cast<char>(b32table[cnt[i]]);
        }
        name[12] = '\0';
    }

    TRACE_EXIT;
    return name;
}

#ifndef NAMESERVER_PROCESS
bool CProcessContainer::CancelDeathNotification( int nid
                                               , int pid
                                               , int verifier
                                               , _TM_Txid_External trans_id)
{
    bool status = FAILURE;
    CProcess *process = head_;

    // we will loop through all processes on the node ... return FAILURE
    // only if we don't find any notices to cancel.
    while (process)
    {
        status = process->CancelDeathNotification (nid, pid, verifier, trans_id);
        process = process->GetNext ();
    }

    return status;
}
#endif

#ifndef NAMESERVER_PROCESS
// Child_Exit terminates all child processes created by the parent process
// unless the child process is Unhooked from the parent process
void CProcessContainer::Child_Exit ( CProcess * parent )
{
    CProcess *process;

    const char method_name[] = "CProcessContainer::Child_Exit";
    TRACE_ENTRY;
    if (trace_settings & TRACE_ENTRY_EXIT)
        trace_printf("%s@%d with parent (%d, %d)\n", method_name, __LINE__, parent->GetNid(), parent->GetPid() );

    if ( parent &&
           ((MyNode->GetState() != State_Shutdown &&
             MyNode->GetShutdownLevel() == ShutdownLevel_Undefined)
           || (parent->GetType() == ProcessType_SPX) ) )
    {
        CProcess::nidPid_t child;
        CLNode * childLNode;

        while ( parent->childRemoveFirst ( child ))
        {

            childLNode = Nodes->GetLNode( child.nid );
            process = (childLNode != NULL )
                         ? childLNode->GetNode()->GetProcess( child.pid ) : NULL;

            if ( process && (!process->IsUnhooked()) )
            {

                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Child process %s (%d, %d) exits due "
                                 "to parent death (%d, %d)\n",
                                 method_name, __LINE__, process->GetName(),
                                 process->GetNid(), process->GetPid(),
                                 parent->GetNid(), parent->GetPid());

                childLNode->SetProcessState( process, State_Down, true );
                if ( !process->IsClone() )
                {
                    if ( parent->GetType() == ProcessType_SPX )
                    {
                        kill (process->GetPid(), SIGKILL);
                    }
                    else
                    {
                        kill (process->GetPid(), Monitor->GetProcTermSig());
                    }
                }
                else
                {
                    if (NameServerEnabled)
                    {
                        CNode* childNode = childLNode->GetNode();
                        // Forward the process kill to the target node
                        int rc = PtpClient->ProcessKill( process
                                                       , process->GetAbort()
                                                       , childLNode->GetNid()
                                                       , childNode->GetName() );
                        if (rc)
                        {
                            char la_buf[MON_STRING_BUF_SIZE];
                            snprintf( la_buf, sizeof(la_buf)
                                    , "[%s] - Can't send process kill "
                                      "request for child process %s (%d, %d) "
                                      "to child node %s, nid=%d\n"
                                    , method_name
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , childNode->GetName()
                                    , childLNode->GetNid() );
                            mon_log_write(MON_PROCESSCONT_CHILDEXIT_1, SQ_LOG_ERR, la_buf);
                        }
                    }
                }
                
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf( "%s@%d - Completed kill for child process %s (%d, %d)\n"
                                , method_name, __LINE__
                                , process->GetName()
                                , process->GetNid()
                                , process->GetPid());
            }
            else
            {
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                {
                    if (process)
                    {
                        trace_printf("%s@%d - Child process %s (%d, %d), not "
                                    "killed, state=%d, unhooked=%d\n",
                                    method_name, __LINE__, process->GetName(),
                                    process->GetNid(), process->GetPid(),
                                    process->GetState(), process->IsUnhooked());

                    }
                }

            }
        }
    }
    TRACE_EXIT;
}

void CProcessContainer::ChildUnHooked_Exit( CProcess* parent )
{
    const char method_name[] = "CProcessContainer::ChildUnHooked_Exit";
    TRACE_ENTRY;

    CProcess *process;

    if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
        trace_printf( "%s@%d with parent %s (%d,%d:%d)\n"
                    , method_name, __LINE__
                    , parent->GetName()
                    , parent->GetNid()
                    , parent->GetPid()
                    , parent->GetVerifier() );

    if (NameServerEnabled)
    {
        if ( parent && !parent->IsClone()
           && ((MyNode->GetState() != State_Shutdown
             && MyNode->GetShutdownLevel() == ShutdownLevel_Undefined)) )
        {
            CProcess::nidPid_t child;
            CLNode* childLNode;

            while ( parent->childUnHookedRemoveFirst( child ))
            {
                childLNode = Nodes->GetLNode( child.nid );
                process = (childLNode != NULL )
                             ? childLNode->GetNode()->GetProcess( child.pid ) : NULL;
                if (process)
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    {
                        trace_printf( "%s@%d - Telling unhooked child process %s (%d,%d:%d) "
                                      "of parent death %s (%d,%d:%d)\n"
                                    , method_name, __LINE__
                                    , process->GetName()
                                    , process->GetNid()
                                    , process->GetPid()
                                    , process->GetVerifier()
                                    , parent->GetName()
                                    , parent->GetNid()
                                    , parent->GetPid()
                                    , parent->GetVerifier() );
                    }
    
                    CNode* childNode = childLNode->GetNode();
                    // Forward the parent's process exit to the child's node
                    int rc = PtpClient->ProcessExit( parent
                                                   , childLNode->GetNid()
                                                   , childNode->GetName() );
                    if (rc)
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        snprintf( la_buf, sizeof(la_buf)
                                , "[%s] - Can't send process exit "
                                  "request for parent process %s (%d,%d:%d) "
                                  "to child's node %s, nid=%d\n"
                                , method_name
                                , parent->GetName()
                                , parent->GetNid()
                                , parent->GetPid()
                                , parent->GetVerifier()
                                , childNode->GetName()
                                , childLNode->GetNid() );
                        mon_log_write(MON_PROCESSCONT_CHILDEXIT_1, SQ_LOG_ERR, la_buf);
                    }
                    else
                    {
                        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                            trace_printf( "%s@%d - Completed kill for parent process %s (%d,%d:%d)\n"
                                        , method_name, __LINE__
                                        , parent->GetName()
                                        , parent->GetNid()
                                        , parent->GetPid()
                                        , parent->GetVerifier() );
                    }
                }
            }
        }
    }
    TRACE_EXIT;
}
#endif

void CProcessContainer::CleanUpProcesses( void )
{
    CProcess *process = head_;

    const char method_name[] = "CProcessContainer::CleanUpProcesses";
    TRACE_ENTRY;

    while (process)
    {
        DelFromNameMap ( process );
        DelFromPidMap ( process );

        DeleteFromList(process);
        process = head_;
    }
    numProcs_ = 0;

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
       trace_printf("%s@%d" " - process count=%d" "\n", method_name, __LINE__, numProcs_);

    TRACE_EXIT;
}

CProcess *CProcessContainer::CloneProcess (int nid,
                                           PROCESSTYPE type,
                                           int priority,
                                           int backup,
                                           bool unhooked,
                                           char *process_name,
                                           char *port,
                                           int os_pid,
                                           int verifier,
                                           int parent_nid,
                                           int parent_pid,
                                           int parent_verifier,
                                           bool event_messages,
                                           bool system_messages,
#ifdef NAMESERVER_PROCESS
                                           char *path, 
                                           char *ldpath, 
                                           char *program, 
#else
                                           strId_t pathStrId,
                                           strId_t ldpathStrId,
                                           strId_t programStrId,
#endif
                                           char *infile,
                                           char *outfile,
                                           struct timespec *creation_time,
                                           int origPNidNs)
{
    char pname[MAX_PROCESS_NAME];
    CProcess *process;
    CProcess *parent = NULL;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcessContainer::CloneProcess";
    TRACE_ENTRY;

    // load & normalize process name
    if( process_name[0] == '\0' )
    {
        pname[0] = '\0';
    }
    else
    {
        STRCPY (pname, NormalizeName (process_name));
    }

    if (parent_nid != -1)
    {
        parent = Nodes->GetLNode (parent_nid)->GetProcessL(parent_pid);
    }

    if (backup)
    {
        if (!parent)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Backup does not have parent's name.\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_CLONEPROCESS_1, SQ_LOG_ERR, la_buf);
            return NULL;
        }
        if (parent_nid == nid)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Backup can't be in parent's node.\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_CLONEPROCESS_2, SQ_LOG_ERR, la_buf);
            return NULL;
        }
    }
    else
    {
        if (pname[0] != '\0')
        {
            Nodes->GetLNode (pname, &process);
            if (process)
            {
                snprintf(la_buf, sizeof(la_buf),
                         "[%s], Failed, Duplicate processname (%s).\n",
                         method_name, process_name);
                mon_log_write(MON_PROCESSCONT_CLONEPROCESS_3, SQ_LOG_ERR, la_buf);
                return NULL;
            }
        }
    }
    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_PROCESS_DETAIL | TRACE_REQUEST_DETAIL))
        trace_printf("%s@%d - Process name=%s (%d, %d), port=%s, "
                     "parent (%d, %d)\n", method_name,
                     __LINE__, pname, nid, os_pid, port, parent_nid, parent_pid);

#ifdef NAMESERVER_PROCESS
    process =
        new CProcess( parent
                    , nid
                    , os_pid
                    , verifier
                    , type
                    , priority
                    , backup
                    , false
                    , unhooked
                    , pname
                    , path
                    , ldpath
                    , program
                    , infile
                    , outfile);
#else
    process =
        new CProcess (parent, nid, os_pid, type, priority, backup, false, unhooked, pname, pathStrId, ldpathStrId,
                      programStrId, infile, outfile);
#endif

    if (process)
    {
        process->SetVerifier(verifier); // CProcessContainer::CloneProcess
        process->SetParentVerifier(parent_verifier);

        AddToList( process );

        process->CompleteProcessStartup (port, os_pid, event_messages, system_messages, os_pid==-1, creation_time, origPNidNs); // CProcessContainer::CloneProcess
    }

    TRACE_EXIT;
    return process;
}


CProcess *CProcessContainer::CompleteProcessStartup (char *process_name,
                                                     char *port,
                                                     int os_pid,
                                                     bool event_messages,
                                                     bool system_messages,
                                                     struct timespec *creation_time,
                                                     int origPNidNs)
{
    CProcess *process = NULL;

    const char method_name[] = "CProcessContainer::CompleteProcessStartup";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        process = GetProcess(process_name,false);
    }
    else
    {
        // Not supposed to be able to get here.
        abort();

    }
    if (process)
    {
        if (process->GetPid() != os_pid)
        { // Process id changed from when we started the process.
#ifndef NAMESERVER_PROCESS
            if ( !process->IsUnhooked() )
            {   // Parent process object keeps track of child processes
                // created on this node.  Needed in case parent process
                // exits abnormally.
                int parentNid;
                int parentPid;
                if ( ! process->IsBackup() )
                {
                    parentNid = process->GetParentNid();
                    parentPid = process->GetParentPid();
                }
                else
                {
                    parentNid = process->GetPairParentNid();
                    parentPid = process->GetPairParentPid();
                }

                if ( parentNid != -1 && parentPid != -1 )
                {
                    CProcess* parent;
                    parent = Nodes->GetLNode ( parentNid )
                                ->GetProcessL( parentPid );
                    if ( parent && !process->IsBackup() )
                    {
                        parent->childRemove ( process->GetNid(),
                                              process->GetPid() );
                        parent->childAdd ( process->GetNid(), os_pid );
                    }
                }
            }
            if (NameServerEnabled)
            {
                if (process->IsUnhooked())
                {   // Parent process object keeps track of child processes
                    // created. Needed when parent process exits to clean up
                    // parent clone process object in remote nodes.
                    int parentNid;
                    int parentPid;
                    CProcess* parent;
                    if ( !process->IsBackup() )
                    {
                        parentNid = process->GetParentNid();
                        parentPid = process->GetParentPid();
                    }
                    else
                    {
                        parentNid = process->GetPairParentNid();
                        parentPid = process->GetPairParentPid();
                    }
    
                    if ( parentNid != -1 && parentPid != -1 )
                    {
                        parent = Nodes->GetLNode(parentNid)->GetProcessL(parentPid);
                        if ( parent && !parent->IsClone() && !process->IsBackup() )
                        {
                            parent->childUnHookedRemove( process->GetNid()
                                                       , process->GetPid() );
                            parent->childUnHookedAdd( process->GetNid()
                                                    , os_pid );
                        }
                    }
                }
            }
#endif
            // Process id changed from when we started the process.  So
            // remap using the new pid.  [This could happen if, for example,
            // a shell script was the originally started process and it
            // then started the process that is now sending its startup message]
            if (trace_settings & TRACE_PROCESS)
            {
                trace_printf("%s@%d - process id changed, new pid at process"
                             " startup=%d, original pid=%d\n",
                             method_name, __LINE__, os_pid,
                             process->GetPid() );
            }
            AddToPidMap ( os_pid, process );
        }
        process->CompleteProcessStartup (port, os_pid, event_messages, system_messages, false, creation_time, origPNidNs); // CProcessContainer::CompleteProcessStartup
    }
    // When using process maps do not log an error if the process is
    // not found.  This method can be called from
    // CCluster::HandleOtherNodeMsg to check if process exists.
    TRACE_EXIT;
    return process;
}

#ifndef NAMESERVER_PROCESS
CProcess *CProcessContainer::CreateProcess (CProcess * parent,
                                            int nid,
                                            PROCESSTYPE type,
                                            int debug,
                                            int priority,
                                            int backup,
                                            bool unhooked,
                                            char *process_name,
                                            strId_t pathStrId,
                                            strId_t ldpathStrId,
                                            strId_t programStrId,
                                            char *infile,
                                            char *outfile,
                                            void *tag,
                                            int &result)
{
    CProcess *process = NULL;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcessContainer::CreateProcess";
    TRACE_ENTRY;

    result = MPI_SUCCESS;

    // load & normalize process name
    if( process_name[0] != '\0' )
    {
        NormalizeName (process_name);
    }

    if (backup)
    {
        if ( !parent || (strcmp (parent->GetName(), process_name) != 0) )
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Backup does not have parent's name.\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_CREATEPROCESS_1, SQ_LOG_ERR, la_buf);

            result = MPI_ERR_NAME;

            return NULL;
        }
        if (parent->GetNid() == nid)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Backup can't be in parent's node.\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_CREATEPROCESS_2, SQ_LOG_ERR, la_buf);

            result = MPI_ERR_RANK;

            return NULL;
        }
    }
    else
    {
        Nodes->GetLNode (process_name, &process, false);
        if (process)
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Duplicate processname (%s).\n",
                     method_name, process_name);
            mon_log_write(MON_PROCESSCONT_CREATEPROCESS_3, SQ_LOG_ERR, la_buf);

            result = MPI_ERR_NAME;
            return NULL;
        }
    }

    process =
        new CProcess (parent, nid, -1, type, priority, backup, debug, unhooked, process_name,
                      pathStrId, ldpathStrId, programStrId, infile, outfile);
    if (process)
    {
        AddToList( process );
        if (type == ProcessType_DTM ||
            type == ProcessType_NameServer ||
            type == ProcessType_Watchdog ||
            type == ProcessType_PSD ||
            type == ProcessType_SMS )
        {
            if (type == ProcessType_NameServer)
            {
                process->userArgs ( monitorArgc, monitorArgv );
            }
            if (process->Create (parent, tag, result)) // monitor
            {
                AddToPidMap(process->GetPid(), process);
            }
        }
        else if ( type == ProcessType_SSMP )
        {
            Nodes->GetLNode ( nid )->SetSSMProc ( process );
        }
    }
    TRACE_EXIT;

    return process;
}
#endif

#ifdef NAMESERVER_PROCESS
void CProcessContainer::DeleteAllDown()
{
    CProcess *process  = NULL;
    int nid = -1;
    int pid = -1;

    const char method_name[] = "CProcessContainer::DeleteAllDown";
    TRACE_ENTRY;

    nameMap_t::iterator nameMapIt;

    while ( true )
    {
        nameMapLock_.lock();
        nameMapIt = nameMap_->begin();

        if (nameMap_->size() == 0)
        {
            nameMapLock_.unlock();
            break; // all done
        }

        process = nameMapIt->second;

        // Delete name map entry
        nameMap_->erase (nameMapIt);

        nameMapLock_.unlock();

        nid = process->GetNid();
        pid = process->GetPid();

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d removed from nameMap %p: %s (%d, %d)\n",
                         method_name, __LINE__, nameMap_,
                         process->GetName(), nid, pid);
        }

        // Delete pid map entry
        DelFromPidMap ( process );

        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
        {
            trace_printf( "%s@%d - Completed delete for %s (%d, %d)\n"
                        , method_name, __LINE__
                        , process->GetName(), nid, pid);
        }

        // Remove all processes
        // PSD will re-create persistent processes on spare node activation
        Exit_Process( process, true, nid );
    }

    TRACE_EXIT;
}
#endif

void CProcessContainer::DeleteFromList( CProcess *process )
{
    const char method_name[] = "CProcessContainer::DeleteFromList";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    if (process)
    {
        RemoveFromList( process );

        if (process->replRefCount() == 0)
        {   // Process object is not in replication queue so ok to
            // delete.
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d - Deleting process %s (%d, %d)\n", method_name, __LINE__, process->Name, process->Nid, process->Pid );
            }
            delete process;
        }
        else
        {   // Process object is in replication queue.  Replication
            // queueing logic will delete the object once the replication
            // has completed.   Set the state here to indicate that
            // the object is no longer on the process list.
            process->SetState (State_Unlinked);
            if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
            {
                trace_printf("%s@%d - Setting process %s (%d, %d) state to State_Unlinked\n", method_name, __LINE__, process->Name, process->Nid, process->Pid );
            }
        }
    }

    TRACE_EXIT;
}

void CProcessContainer::RemoveFromList( CProcess *process )
{
    const char method_name[] = "CProcessContainer::RemoveFromList";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    if (process)
    {
        CLNode *lnode = Nodes->GetLNode( process->Nid );
        lnode->RemoveFromListL( process );

        if (head_ == process)
            head_ = process->next_;
        if (tail_ == process)
            tail_ = process->prev_;
        if (process->prev_)
            process->prev_->next_ = process->next_;
        if (process->next_)
            process->next_->prev_ = process->prev_;
        numProcs_--;
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            CNode *node = lnode->GetNode();
            trace_printf("%s@%d" " - container %p pnid=%d, process count=%d, pnode=%d" "\n", method_name, __LINE__, this, node->GetPNid(), numProcs_, nodeContainer_);
        }

    }

    TRACE_EXIT;
}

void CProcessContainer::RemoveFromListL( CProcess *process )
{
    const char method_name[] = "CProcessContainer::RemoveFromListL";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CLNode (the logical node)
        abort();
    }

    if (process)
    {

        if (head_ == process)
            head_ = process->nextL_;
        if (tail_ == process)
            tail_ = process->prevL_;
        if (process->prevL_)
            process->prevL_->nextL_ = process->nextL_;
        if (process->nextL_)
            process->nextL_->prevL_ = process->prevL_;
        numProcs_--;
        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d" " - container %p nid=%d, process count=%d, pnode=%d" "\n", method_name, __LINE__, this, process->Nid, numProcs_, nodeContainer_);
        }
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
bool CProcessContainer::Dump_Process (CProcess *dumper, CProcess *process, char *core_path)
{
    bool status;

    const char method_name[] = "CProcessContainer::Dump_Process";
    TRACE_ENTRY;

    status = process->Dump(dumper, core_path);

    TRACE_EXIT;
    return status;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::DumpCallback( int nid, pid_t pid, int status )
{
    const char method_name[] = "CProcessContainer::DumpCallback";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CLNode (the logical node)
        abort();
    }

    CLNode   *lnode = Nodes->GetLNode( nid );
    CNode    *node = lnode->GetNode();

    CProcess *process = node->GetProcess( pid );
    if ( process )
    {
        if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
        {
            if (trace_settings & TRACE_PROCESS)
            {
                trace_printf("%s@%d - dump successful, nid=%d, pid=%d\n",
                             method_name, __LINE__, nid, pid );
            }
            process->SetDumpStatus( Dump_Success );
        }
        else
        {
            if (trace_settings & TRACE_PROCESS)
            {
                trace_printf("%s@%d - dump failed, nid=%d, pid=%d\n",
                             method_name, __LINE__, nid, pid );
            }
            process->SetDumpStatus( Dump_Failed );
        }
        process->SetDumpState( Dump_Complete );

        CReplDumpComplete *repl = new CReplDumpComplete( process );
        Replicator.addItem(repl);
    }
    else
    {
        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf("%s@%d - dump process not found, nid=%d, pid=%d\n",
                         method_name, __LINE__, nid, pid );
        }
    }

    TRACE_EXIT;
}
#endif


#ifndef NAMESERVER_PROCESS
CProcess * CProcessContainer::ParentNewProcReply ( CProcess *process, int result )
{
    const char method_name[] = "CProcessContainer::ParentNewProcReply";
    TRACE_ENTRY;

    CProcess *parent = NULL;

    if (process->GetParentNid() != -1)
    {
        parent = Nodes->GetProcess( process->GetParentNid(),
                                    process->GetParentPid() );
    }

    // If we have a parent process then it is expecting a reply
    if (parent && !parent->IsClone() && !parent->IsPaired())
    {
        if (!process->IsNowait())
        {   // The new process request was "waited" so send reply now
            struct message_def *reply_msg;
            reply_msg = process->parentContext();

            if ( reply_msg )
            {
                // send reply to the parent
                parent->ReplyNewProcess ( reply_msg, process, result );
                // Since we have replied parent context (i.e the request
                // buffer) is no longer valid.
                process->parentContext( NULL );
            }
        }
        else
        {   // The new process request was "no-wait" so send notice now
            process->SendProcessCreatedNotice(parent, result);
        }
    }

    TRACE_EXIT;

    return parent;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::Exit_Process (CProcess *process, bool abend, int downNode)
{
    bool restarted = false;
    char la_buf[MON_STRING_BUF_SIZE];
    CProcess *parent = NULL;

    const char method_name[] = "CProcessContainer::Exit_Process(process)";
    TRACE_ENTRY;

    if (process)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf( "%s@%d - Process %s (abended=%d) is exiting, abend=%d, downNode=%d\n"
                        , method_name, __LINE__
                        , process->GetName()
                        , process->IsAbended()
                        , abend
                        , downNode );

        if ( process->GetState() == State_Down && abend && !process->IsAbended() )
        {
            process->SetAbended( abend );
        }
        if (process->GetNid() == downNode && !process->IsAbended() )
        {
            process->SetAbended( abend );
        }

        if ( numProcs_ <= 0 )
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Node's process count is invalid, aborting\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_EXITPROCESS_1, SQ_LOG_ERR, la_buf);
            abort();
        }

        if ( process->GetState() == State_Stopped )
        {
            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d" " - Process " "%s" " already exited." "\n", method_name, __LINE__, process->GetName());
            return;
        }

        if (!process->IsStartupCompleted())
        {
            parent = ParentNewProcReply ( process, MPI_ERR_SPAWN );

            char buf[MON_STRING_BUF_SIZE];
            snprintf(buf, sizeof(buf),
                     "[%s], Exiting process %s (%d, %d) did not complete "
                     "startup\n",
                     method_name, process->GetName(), process->GetNid(),
                     process->GetPid());
            mon_log_write(MON_PROCESSCONT_EXITPROCESS_2, SQ_LOG_ERR, buf);
        }

        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf( "%s@%d - Process %s is exiting, persistent=%d, abended=%d\n"
                        , method_name, __LINE__
                        , process->GetName()
                        , process->IsPersistent()
                        , process->IsAbended() );

        if ( process->IsPersistent() &&
            (process->IsAbended() || process->GetType() == ProcessType_SPX))
        {
            Child_Exit(process);
        }

        if (!process->IsClone() && NameServerEnabled)
        {
            if (process->childUnHookedCount() > 0)
            {
                ChildUnHooked_Exit(process);
            }
        }
    
        if ( parent == NULL)
        {
            parent = Nodes->GetProcess( process->GetParentNid(),
                                        process->GetParentPid() );
        }

        // Unregister any interest in other process' death
        _TM_Txid_External transid;
        transid = invalid_trans();
        process->procExitUnregAll( transid );

        // Handle the process termination
        process->Exit( parent );

        process->Switch( parent ); // switch process pair roles if needed

        if ( process->IsPersistent() &&
             process->GetAbort() == false &&
            !MyNode->IsActivatingSpare() &&
            !MyNode->IsKillingNode() &&
             MyNode->GetShutdownLevel() == ShutdownLevel_Undefined &&
            (process->IsAbended()||
             process->GetNid() == downNode ||
             process->GetType() == ProcessType_SPX))
        {
            // see if we can restart the process
            restarted = RestartPersistentProcess( process, downNode );
            if ( !restarted )
            {
                if (!process->IsClone() && !MyNode->isInQuiesceState())
                {
                    // Replicate the exit to other nodes
                    if (!NameServerEnabled)
                    {
                        // Replicate the exit to other nodes
                        CReplExit *repl = new CReplExit(process->GetNid(),
                                                    process->GetPid(),
                                                    process->GetVerifier(),
                                                    process->GetName(),
                                                    process->IsAbended());
                        Replicator.addItem(repl);
                    }
                }
                else
                {
                    if (trace_settings & TRACE_SYNC)
                    {
                        trace_printf("%s@%d - not queuing process exit for clone %s\n", method_name, __LINE__, process->GetName());
                    }
                }
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d" " - Persistent Process " "%s" " did not re-start on nid=" "%d" "\n", method_name, __LINE__, process->GetName(), process->GetNid());

                CNode * node;
                node = Nodes->GetLNode(process->GetNid())->GetNode();
                node->DeleteFromList( process );
            }
        }
        else
        {
            process->SetState (State_Stopped);
            if ( !process->IsClone() &&
                 !MyNode->IsKillingNode() &&
                 !MyNode->isInQuiesceState() &&
                 !(process->GetType() == ProcessType_DTM &&
                   process->IsAbended() &&
                   MyNode->GetShutdownLevel() == ShutdownLevel_Undefined) )
            {
                if (!NameServerEnabled)
                {
                    // Replicate the exit to other nodes
                    CReplExit *repl = new CReplExit(process->GetNid(),
                                                process->GetPid(),
                                                process->GetVerifier(),
                                                process->GetName(),
                                                process->IsAbended());
                    Replicator.addItem(repl);
                }
            }
            else
            {
                if (trace_settings & TRACE_SYNC)
                {
                    trace_printf("%s@%d - not queuing process exit for clone %s\n", method_name, __LINE__, process->GetName());
                }
            }
            process->SetDeletePending ( true );
            if (process->IsAbended() || process->GetType() == ProcessType_SPX)
            {
                Child_Exit(process);
            }

            if (!process->IsClone() && process->GetType() == ProcessType_Watchdog)
            {
                HealthCheck.setState(HC_UPDATE_WATCHDOG, (long long)NULL);
            }
            CNode * node;
            node = Nodes->GetLNode(process->GetNid())->GetNode();
            node->DeleteFromList( process );

        }
    }
    TRACE_EXIT;

    return;
}
#endif

#ifdef NAMESERVER_PROCESS
void CProcessContainer::Exit_Process (CProcess *process, bool abend, int downNode)
{
    const char method_name[] = "CProcessContainer::Exit_Process(process)";
    TRACE_ENTRY;

    char la_buf[MON_STRING_BUF_SIZE];
    CProcess *parent = NULL;

    if (process)
    {
        if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
            trace_printf( "%s@%d - Process %s (abended=%d) is exiting, abend=%d, downNode=%d\n"
                        , method_name, __LINE__
                        , process->GetName()
                        , process->IsAbended()
                        , abend
                        , downNode );

        if ( process->GetState() == State_Down && abend && !process->IsAbended() )
        {
            process->SetAbended( abend );
        }
        if (process->GetNid() == downNode && !process->IsAbended() )
        {
            process->SetAbended( abend );
        }

        if ( numProcs_ <= 0 )
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Node's process count is invalid, aborting\n",
                     method_name);
            mon_log_write(MON_PROCESSCONT_EXITPROCESS_1, SQ_LOG_ERR, la_buf);
            abort();
        }

        if ( process->GetState() == State_Stopped )
        {
            if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d" " - Process " "%s" " already exited." "\n", method_name, __LINE__, process->GetName());
            return;
        }

        if ( parent == NULL)
        {
            parent = Nodes->GetProcess( process->GetParentNid(),
                                        process->GetParentPid() );
        }

        // Handle the process termination
        process->Switch( parent ); // switch process pair roles if needed
        process->SetDeletePending ( true );

        CNode *node;
        node = Nodes->GetLNode(process->GetNid())->GetNode();
        node->DelFromNameMap ( process );
        node->DelFromPidMap ( process );
        node->DeleteFromList( process );
    }
    TRACE_EXIT;

    return;
}
#endif

CProcess *CProcessContainer::GetProcess (int pid)
{
    const char method_name[] = "CProcessContainer::GetProcess (pid)";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    pidMap_t::iterator it;
    CProcess *entry = NULL;

    pidMapLock_.lock();
    it = pidMap_->find(pid);
    if (it != pidMap_->end())
    {
        entry = it->second;

        // bugcatcher, temp call
        entry->validateObj();
    }
    pidMapLock_.unlock();

    if (trace_settings & TRACE_PROCESS_DETAIL)
    {
        trace_printf("%s@%d - pidmap_ (%p) entry=%p, pid=%d, Name=%s\n",
                     method_name, __LINE__, pidMap_, entry, pid,
                     ((entry != NULL) ? entry->GetName(): ""));
    }

    TRACE_EXIT;
    return entry;
}

CProcess *CProcessContainer::GetProcess (const char *name, bool checkstate)
{
    const char method_name[] = "CProcessContainer::GetProcess (name)";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    nameMap_t::iterator it;
    CProcess *entry = NULL;

    if ( ! strlen( name ) )
    {
        TRACE_EXIT;
        return entry;
    }
    char pname[MAX_PROCESS_NAME];
    strncpy(pname, name, MAX_PROCESS_NAME);
    pname[MAX_PROCESS_NAME-1] = '\0';

    NormalizeName (pname);

    // Look up name in process-name-to-process-object map.
    nameMapLock_.lock();
    it = nameMap_->find( pname );

    if (it != nameMap_->end())
    {
        entry = it->second;

        // bugcatcher, temp call
        entry->validateObj();

        if (trace_settings & TRACE_PROCESS_DETAIL)
            trace_printf("%s@%d - Name=%s, checkstate=%d, state=%d, backup=%d\n",
                         method_name, __LINE__, entry->GetName(), checkstate,
                         entry->GetState(), entry->IsBackup());

        if ( checkstate &&  entry->GetState() != State_Up)
        {   // Only return entry if it has completed startup
            if (trace_settings & TRACE_PROCESS)
                trace_printf( "%s@%d - Process %s (%d,%d:%d) not in 'Up' state"
                              ", checkstate=%d, state=%d, backup=%d\n"
                            ,  method_name, __LINE__
                            , entry->GetName()
                            , entry->GetNid()
                            , entry->GetPid()
                            , entry->GetVerifier()
                            , checkstate
                            , entry->GetState()
                            , entry->IsBackup());
            entry = NULL;
        }
    }
    nameMapLock_.unlock();

    TRACE_EXIT;

    return entry;
}

CProcess *CProcessContainer::GetProcess( int pid
                                       , Verifier_t verifier
                                       , bool checkstate )
{
    const char method_name[] = "CProcessContainer::GetProcess(pid, verifier)";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    CProcess *entry = NULL;

    if ( pid != -1 )
    {
        entry = CProcessContainer::GetProcess( pid );
    }

    if ( entry )
    {
        if ( (verifier != -1) && (verifier != entry->GetVerifier()) )
        {
           if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           {
              trace_printf( "%s@%d - Get (%d:%d) failed -- verifier mismatch (%d)\n"
                          , method_name, __LINE__
                          , pid
                          , verifier
                          , entry->GetVerifier() );
           }
           entry = NULL;
        }
    }

    if ( entry && checkstate &&  entry->GetState() != State_Up)
    {   // Only return entry if it has completed startup
        if (trace_settings & TRACE_PROCESS)
            trace_printf( "%s@%d - Process %s (%d,%d:%d) not in 'Up' state"
                          ", checkstate=%d, state=%d, backup=%d\n"
                        ,  method_name, __LINE__
                        , entry->GetName()
                        , entry->GetNid()
                        , entry->GetPid()
                        , entry->GetVerifier()
                        , checkstate
                        , entry->GetState()
                        , entry->IsBackup());
        entry = NULL;
    }

    TRACE_EXIT;
    return entry;
}

CProcess *CProcessContainer::GetProcess( const char *name
                                       , Verifier_t verifier
                                       , bool checkstate )
{
    const char method_name[] = "CProcessContainer::GetProcess(name, verifier)";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    CProcess *entry = NULL;

    if ( strlen( name ) )
    {
        entry = CProcessContainer::GetProcess( name, checkstate );
    }

    if ( entry )
    {
        if ( (verifier != -1) && (verifier != entry->GetVerifier()) )
        {
           if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           {
              trace_printf( "%s@%d - Get (%s:%d) failed -- verifier mismatch (%d)\n"
                          , method_name, __LINE__
                          , name
                          , verifier
                          , entry->GetVerifier() );
           }
           entry = NULL;
        }
    }

    TRACE_EXIT;
    return entry;
}

CProcess *CProcessContainer::GetProcessByType (PROCESSTYPE type)
{
    CProcess *entry = head_;

    const char method_name[] = "CProcessContainer::GetProcessByType";
    TRACE_ENTRY;

    if ( ! nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CNode (the physical node)
        abort();
    }

    entry = entry->GetProcessByType( type );
    TRACE_EXIT;

    return entry;
}

// see: CLNode::GetProcessL (int pid)
// see: CLNode::GetProcessL (char *name, bool checkstate)

CProcess *CProcessContainer::GetProcessLByType(PROCESSTYPE type)
{
    CProcess *entry = head_;

    const char method_name[] = "CProcessContainer::GetProcessByType";
    TRACE_ENTRY;

    if ( nodeContainer_ )
    {
        // Programmer bonehead :^)
        // This must only be called from CLNode (the logical node)
        abort();
    }

    entry = entry->GetProcessLByType( type );

    TRACE_EXIT;

    return entry;
}

#ifndef NAMESERVER_PROCESS
void CProcessContainer::KillAll( STATE node_State, CProcess *requester )
{
    CProcess *process = NULL;
    int nid;

    const char method_name[] = "CProcessContainer::KillAll";
    TRACE_ENTRY;

    nameMapLock_.lock();

    nameMap_t::iterator nameMapIt;
    nameMap_t::iterator nameMapItSave;
    for ( nameMapIt = nameMap_->begin(); nameMapIt != nameMap_->end(); )
    {
        process = nameMapIt->second;
        assert( process );
        nameMapItSave = nameMapIt;
        ++nameMapIt;

        nid = process->GetNid();
        if (( process->GetType() != ProcessType_Watchdog ) &&
            ( process != requester                  )   )
        {
            if (node_State == State_Down)
            {
                int killedNid = process->GetNid();
                int killedPid = process->GetPid();
                bool killedIsClone = process->IsClone();

                // Delete name map entry
                nameMap_->erase(nameMapItSave);

                if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
                {
                    trace_printf("%s@%d removed from nameMap %p: %s (%d, %d)\n",
                                 method_name, __LINE__, nameMap_,
                                 process->GetName(), killedNid,
                                 killedPid);
                }

                // Delete pid map entry
                DelFromPidMap ( process );

                // Set process to "stopped" state.  SetProcessState
                // will invoke Exit_Process so "process" is not
                // valid after SetProcessState returns.
                SetProcessState( process, State_Stopped, true, -1);
                if ( nid == killedNid )
                {
                    if ( !killedIsClone && killedPid != -1)
                    {
                        kill (killedPid, SIGKILL);
                        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                            trace_printf("%s@%d - Completed kill for (%d, %d)\n", method_name, __LINE__, killedNid, killedPid);
                    }
                }
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_RECOVERY | TRACE_SYNC_DETAIL | TRACE_TMSYNC | TRACE_PROCESS_DETAIL))
                    trace_printf("%s@%d change process (%d, %d) state to down\n", method_name, __LINE__, process->GetNid(), process->GetPid());
                process->SetState (State_Down);
                // Replicate the kill to other nodes
                CReplKill *repl = new CReplKill( process->GetNid()
                                               , process->GetPid()
                                               , process->GetVerifier()
                                               , process->GetAbort());
                Replicator.addItem(repl);
            }
        }
    }

    nameMapLock_.unlock();

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::KillAllDown()
{
    CProcess *process  = NULL;
    int nid = -1;
    int pid = -1;

    const char method_name[] = "CProcessContainer::KillAllDown";
    TRACE_ENTRY;

    nameMap_t::iterator nameMapIt;

    while ( true )
    {
        nameMapLock_.lock();
        nameMapIt = nameMap_->begin();

        if (nameMap_->size() == 0)
        {
            nameMapLock_.unlock();
            break; // all done
        }

        process = nameMapIt->second;

        // Delete name map entry
        nameMap_->erase (nameMapIt);

        nameMapLock_.unlock();

        nid = process->GetNid();
        pid = process->GetPid();

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d removed from nameMap %p: %s (%d, %d)\n",
                         method_name, __LINE__, nameMap_,
                         process->GetName(), nid, pid);
        }

        // Delete pid map entry
        DelFromPidMap ( process );

        // valid for virtual cluster only.
        if ( !process->IsClone() && pid != -1 )
        {
            // killing the process will not remove the process object because
            // exit processing will get queued until this completes.
            kill( pid, SIGKILL );
            PROCESSTYPE type = process->GetType();
            if ( type == ProcessType_TSE ||
                 type == ProcessType_ASE )
            {
                // unmount volume would acquire nameMapLock_ internally.
                Devices->UnMountVolume( process->GetName(), process->IsBackup() );
            }
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d - Completed kill for %s (%d, %d)\n", method_name, __LINE__, process->GetName(), nid, pid);
        }

        // Remove all processes
        // PSD will re-create persistent processes on spare node activation
        Exit_Process( process, true, nid );
    }

    // clean up clone processes on this node that do not have entries in
    // nameMap_ or pidMap_ yet and restart persistent processes
    CProcess *nextProc = NULL;
    process = head_;

    while (process)
    {
        nextProc = process->GetNext();

        // Delete pid map entry
        DelFromPidMap ( process );

        Exit_Process( process, true, nid );

        process = nextProc;
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::KillAllDownSoft()
{
    const char method_name[] = "CProcessContainer::KillAllDownSoft";
    TRACE_ENTRY;

    CProcess *process  = NULL;
    int nid = -1;
    int pid = -1;
    PROCESSTYPE type;
    nameMap_t::iterator nameMapIt;

    while ( true )
    {
        nameMapLock_.lock();
        nameMapIt = nameMap_->begin();

        if (nameMap_->size() == 0)
        {
            nameMapLock_.unlock();
            break; // all done
        }

        process = nameMapIt->second;

        // Delete name map entry
        nameMap_->erase (nameMapIt);

        nameMapLock_.unlock();

        nid = process->GetNid();
        pid = process->GetPid();
        type = process->GetType();

        if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d removed from nameMap %p: %s (%d, %d)\n",
                         method_name, __LINE__, nameMap_,
                         process->GetName(), nid, pid);
        }

        // valid for virtual cluster or soft node down only.
        if ( type != ProcessType_DTM && type != ProcessType_NameServer )
        {
            // Delete pid map entry
            DelFromPidMap ( process );

            // valid for virtual cluster only.
            if ( !process->IsClone() && pid != -1 )
            {
                // killing the process will not remove the process object because
                // exit processing will get queued until this completes.
                kill( pid, SIGKILL );
                PROCESSTYPE type = process->GetType();
                if ( type == ProcessType_TSE ||
                     type == ProcessType_ASE )
                {
                    // unmount volume would acquire nameMapLock_ internally.
                    Devices->UnMountVolume( process->GetName(), process->IsBackup() );
                }
                if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Completed kill for %s (%d, %d)\n", method_name, __LINE__, process->GetName(), nid, pid);
            }
            // Remove all processes
            // PSD will re-create persistent processes on spare node activation
            Exit_Process( process, true, nid );
        }
    }

    // clean up clone processes on this node that do not have entries in
    // nameMap_ or pidMap_ yet and restart persistent processes
    CProcess *nextProc = NULL;
    process = head_;

    while (process)
    {
        nextProc = process->GetNext();

        PROCESSTYPE type = process->GetType();
        if ( type != ProcessType_DTM && type != ProcessType_NameServer )
        {
            // Delete pid map entry
            DelFromPidMap ( process );

            Exit_Process( process, true, nid );
        }

        process = nextProc;
    }

    TRACE_EXIT;
}
#endif

char *CProcessContainer::NormalizeName (char *name)
{
    char *ptr;

    const char method_name[] = "CProcessContainer::NormalizeName";
    TRACE_ENTRY;
    ptr = name;
    while (*ptr)
    {
        *ptr = toupper (*ptr);
        ptr++;
    }
    TRACE_EXIT;

    return name;
}

#ifndef NAMESERVER_PROCESS
bool CProcessContainer::Open_Process (int nid, int pid, Verifier_t verifier, int death_notification, CProcess * process)
{
    bool status = FAILURE;
    CProcess *opener_process = NULL;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CProcessContainer::Open_Process";
    TRACE_ENTRY;
    if (process)
    {
        opener_process = Nodes->GetLNode (nid)->GetProcessL(pid);
        if (opener_process)
        {
            if ( (verifier != -1) && (verifier != process->GetVerifier()) )
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                   trace_printf("%s@%d - Opener (%d, %d:%d) not found -- verifier mismatch (%d)\n",
                                method_name, __LINE__,
                                nid,
                                pid,
                                verifier,
                                opener_process->GetVerifier());
                }
            }
            else
            {
                status = opener_process->Open (process,death_notification);
            }
        }
        else
        {
            snprintf(la_buf, sizeof(la_buf),
                     "[%s], Failed, Can't find opener process, Pid=%d.\n",
                     method_name, pid);
            mon_log_write(MON_PROCESSCONT_OPENPROCESS_1, SQ_LOG_ERR, la_buf);
        }
    }
    else
    {
        snprintf(la_buf, sizeof(la_buf),
                "[%s], Failed, Can't find process.\n", method_name);
        mon_log_write(MON_PROCESSCONT_OPENPROCESS_2, SQ_LOG_ERR, la_buf);
    }
    TRACE_EXIT;

    return status;
}
#endif

#ifdef NAMESERVER_PROCESS
bool CProcessContainer::RestartPersistentProcess( CProcess *, int )
{
    return false;
}
#else
//
// Persistent process re-creation logic:
//
// o Process object is target of re-create
// o Process object type determines persist configuration template
// o Persist configuration template determines re-creation rules
//
//  Re-creation rules:
//
//  PROCESS_NAME format defines (%nid+/%nid) node re-creation scope
//          $<prefix>%nid+ or $<prefix>%nid or $<name>
//
//  PERSIST_ZONES format defines (%zid+/%zid) rules of re-creation within scope
//
//  (%nid+) Nid_ALL = one process in each node
//              Zid_ALL       = n/a
//              Zid_RELATIVE  = recreate only in initial <nid> assigned
//  (%nid ) Nid_RELATIVE = one process in cluster
//              Zid_ALL       = recreate in current up <nid> or next up <nid>
//              Zid_RELATIVE  = recreate only in initial <nid> assigned (non-HA)
//  (     ) Nid_Undefined = one process in cluster
//              Zid_ALL       = recreate in current up <nid> or next up <nid>
//              Zid_RELATIVE  = recreate only in initial <nid> assigned (non-HA)
//
bool CProcessContainer::RestartPersistentProcess( CProcess *process, int downNid )
{
    const char method_name[] = "CProcessContainer::RestartPersistentProcess";
    TRACE_ENTRY;

    bool successful = false;
    bool restart = false;
    int nid = -1;
    int max_retries = 3;
    int retry_max_time = 1;
    CNode *currenNode;
    CNode *newNode;
    CLNode *currentLNode;
    CLNode *newLNode;
    CProcess *parent = NULL;
    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CPersistConfig *persistConfig = NULL;

    assert(clusterConfig != NULL);

    persistConfig = clusterConfig->GetPersistConfig( process->GetType()
                                                   , process->GetName()
                                                   , process->GetNid() );
    if (persistConfig)
    {
        max_retries = persistConfig->GetPersistRetries();
        retry_max_time = persistConfig->GetPersistWindow();
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Persistent process %s (%s) not restarted on nid=%d "
                  "because the persist configuration is missing.\n"
                , method_name
                , process->GetName()
                , ProcessTypeString( process->GetType() )
                , process->GetNid() );
        mon_log_write(MON_PROCESS_PERSIST_2, SQ_LOG_ERR, buf);
        return false;
    }

    if (!process->IsClone())
    {
        if ( process->GetType() == ProcessType_DTM )
        {
            MyNode->ResetPrimitiveDtmUp();
        }
        else if ( process->GetType() == ProcessType_Watchdog )
        {
            MyNode->ResetPrimitiveWdgUp();
        }
        else if ( process->GetType() == ProcessType_PSD )
        {
             MyNode->ResetPrimitivePsdUp() ;
        }
    }

    // if 1st time retrying to restart process
    if (process->GetPersistentCreateTime() == 0)
    {
        process->SetPersistentCreateTime ( time(NULL) );
    }

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
       trace_printf( "%s@%d - Persistent process %s, retryCount=%d, max_retries = %d, "
                     "time limit = %d, createTime=%ld, down nid=%d\n"
                   , method_name, __LINE__
                   , process->GetName()
                   , process->GetPersistentRetries()
                   , max_retries
                   , retry_max_time
                   , process->GetPersistentCreateTime()
                   , downNid);

    // get the parent process if any
    if (process->GetParentNid() != -1 && process->GetParentPid() != -1)
    {
        parent = Nodes->GetLNode( process->GetParentNid())->GetProcessL(process->GetParentPid() );
    }

    currentLNode = Nodes->GetLNode( process->GetNid() );
    newLNode     = Nodes->GetLNodeNext( process->GetNid() );

    switch (persistConfig->GetProcessNameNidFormat())
    {
    case Nid_ALL:       // one process in each <nid>
        switch (persistConfig->GetZoneZidFormat())
        {
        case Zid_ALL:       // n/a
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], Persistent process %s not "
                      "restarted because the persist configuration is "
                      "inconsistent for key %s.\n"
                    , method_name
                    , process->GetName()
                    , persistConfig->GetPersistPrefix() );
            mon_log_write(MON_PROCESS_PERSIST_2, SQ_LOG_ERR, buf);
            return false;
        case Zid_RELATIVE:  // recreate only in initial <nid> assigned
        default:
            // Is this a node down and node going down is process' node?
            if ( downNid != -1 && currentLNode->GetNid() == downNid )
            {
                if (trace_settings &
                    (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf( "%s@%d - original node is not available, nid=%d, downNid=%d\n"
                                , method_name, __LINE__
                                , currentLNode->GetNid()
                                , downNid );
            }
            else
            {
                if ( currentLNode->GetState() == State_Up)
                {
                    if (trace_settings &
                        (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    {
                        trace_printf( "%s@%d - original node is available, nid=%d\n"
                                    , method_name, __LINE__, process->GetNid());
                    }

                    if ( MyNode->IsMyNode(process->GetNid()) )
                    {
                        restart = true;
                    }
                }
                else
                {
                    if (trace_settings &
                        (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        trace_printf( "%s@%d - original node is not available, nid=%d, downNid=%d\n"
                                    , method_name, __LINE__
                                    , currentLNode->GetNid()
                                    , downNid );
                }
            }
        } // switch
        break;
    case Nid_RELATIVE:  // one process in cluster
    case Nid_Undefined: // one process in cluster
    default:
        switch (persistConfig->GetZoneZidFormat())
        {
        case Zid_ALL:       // recreate in current up <nid> or next up <nid>
            // check if we need to do something because the node is down and
            // spare node is not activating
            if ((downNid != -1 && !currentLNode->GetNode()->IsSpareNode()) ||
                 currentLNode->GetState() == State_Down )
            {
                nid = (newLNode) ? newLNode->GetNid() : -1;
                if ( newLNode &&
                    (newLNode->GetState() == State_Up &&
                     newLNode->GetNid() != downNid ) )
                {
                    if (MyNode->IsMyNode(nid))
                    {
                        // OK we need to move the process to our node
                        if (trace_settings &
                            (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                            trace_printf( "%s@%d - Moving process from nid=%d to new nid=%d\n"
                                        , method_name, __LINE__
                                        , process->GetNid(), nid);
                        currenNode = currentLNode->GetNode();
                        currenNode->RemoveFromList(process);
                        process->SetNid ( nid );
                        process->SetPid ( -1 );
                        newNode = newLNode->GetNode();
                        newNode->AddToList( process );
                        process->SetClone( false );
                        // Replicate the clone to other nodes
                        CReplClone *repl = new CReplClone(process);
                        Replicator.addItem(repl);
                        restart = true;
                    }
                    else
                    {
                        if (trace_settings &
                            (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                            trace_printf( "%s@%d - Not moving process from nid=%d to nid=%d""\n"
                                        , method_name, __LINE__
                                        , process->GetNid(), nid);
                    }
                }
                else
                {
                    if (trace_settings &
                        (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                       trace_printf( "%s@%d - Next possible node is not available, nid=%d\n"
                                    , method_name, __LINE__, nid);
                }
            }
            else
            {
                if (trace_settings &
                    (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf( "%s@%d - original node is available, nid=%d\n"
                                , method_name, __LINE__, process->GetNid());

                if ( MyNode->IsMyNode(process->GetNid()) )
                {
                    restart = true;
                }
            }
            break;
        case Zid_RELATIVE:  // recreate only in initial <nid> assigned (non-HA)
        default:
            // Is this a node down and node going down is process' node?
            if ( downNid != -1 && currentLNode->GetNid() == downNid )
            {
                if (trace_settings &
                    (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf( "%s@%d - original node is not available, nid=%d, downNid=%d\n"
                                , method_name, __LINE__
                                , currentLNode->GetNid()
                                , downNid );
            }
            else
            {
                if ( currentLNode->GetState() == State_Up)
                {
                    if (trace_settings &
                        (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    {
                        trace_printf( "%s@%d - original node is available, nid=%d\n"
                                    , method_name, __LINE__, process->GetNid());
                    }

                    if ( MyNode->IsMyNode(process->GetNid()) )
                    {
                        restart = true;
                    }
                }
                else
                {
                    if (trace_settings &
                        (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        trace_printf( "%s@%d - original node is not available, nid=%d, downNid=%d\n"
                                    , method_name, __LINE__
                                    , currentLNode->GetNid()
                                    , downNid );
                }
            }
        }
        break;
    }

    if ( Nodes->IsShutdownActive() )
    {
        if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
            trace_printf("%s@%d" " - Shutdown process " "%s" " on nid=" "%d"   "\n", method_name, __LINE__, process->GetName(), process->GetNid());
        successful = false;
    }
    else
    {
        // Re-initialize process flags
        process->SetState (State_Unknown);

        if (( restart                    ) &&
            ( MyNode->IsMyNode(process->GetNid()) ))
        {
            // check if we should retry to create the process
            if ( (time(NULL) - process->GetPersistentCreateTime()) < retry_max_time )
            {
                int retryCount = process->GetPersistentRetries();
                ++retryCount;
                if ( retryCount <= max_retries )
                {
                    process->SetPersistentRetries ( retryCount );
                }
                else
                {
                    if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                        trace_printf("%s@%d - Max retries exceeded for "
                                     "process %s, retry count=%d, max "
                                     "retries=%d\n", method_name, __LINE__,
                                     process->GetName(), retryCount,
                                     max_retries);

                    char buf[MON_STRING_BUF_SIZE];

                    snprintf( buf, sizeof(buf)
                            , "[%s], Persistent process %s not restarted because "
                              "the maximum retry count has been exceeded. "
                              "(retryCount=%d, maxRetryCount=%d) \n"
                            , method_name, process->GetName()
                            , retryCount, max_retries );
                    mon_log_write(MON_PROCESS_PERSIST_1, SQ_LOG_INFO, buf);

                    if ( process->GetType() == ProcessType_DTM ||
                         process->GetType() == ProcessType_PSD ||
                         process->GetType() == ProcessType_TMID ||
                         process->GetType() == ProcessType_Watchdog ||
                         process->GetType() == ProcessType_SMS )
                    {
                        if ( process->GetType() == ProcessType_DTM )
                        {
                            MyNode->SetDTMAborted( true );
                        }
                        if ( process->GetType() == ProcessType_SMS )
                        {
                            MyNode->SetSMSAborted( true );
                        }

                        snprintf(buf, sizeof(buf), "[%s], Critial persistent process %s "
                                 "not restarted, "
                                 "scheduling node down on node %s (%d)!\n",
                                 method_name, process->GetName(), MyNode->GetName(), MyPNID);
                        mon_log_write(MON_PROCESS_PERSIST_4, SQ_LOG_CRIT, buf);

                        ReqQueue.enqueueDownReq(MyPNID);
                    }

                    return false;
                }
            }
            else
            {
                if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf( "%s@%d - Retries count reset for process %s, "
                                  "retry: count=%d, time elapsed=%ld, time max=%d\n"
                                , method_name, __LINE__
                                , process->GetName()
                                , process->GetPersistentRetries()
                                , (time(NULL) - process->GetPersistentCreateTime())
                                , retry_max_time);
                process->SetPersistentRetries( 1 );
            }

            // OK ... just restart the process on the same node
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d" " - Restarting process " "%s" " on nid=" "%d"   "\n", method_name, __LINE__, process->GetName(), process->GetNid());
            process->SetDeletePending ( false );
            process->SetStartupCompleted ( false );
            process->SetPriorPid( !MyNode->IsSpareNode() ? process->GetPid() : 0 );
            process->SetClone( false );
            int result;
            successful = process->Create(parent, 0, result);
            if (successful)
            {
                process->SetAbended( false );
                Nodes->GetLNode (process->GetNid())->GetNode()
                    ->AddToNameMap(process);
                Nodes->GetLNode (process->GetNid())->GetNode()
                    ->AddToPidMap(process->GetPid(), process);
                if (process->GetPersistentRetries() == 1)
                {
                    process->SetPersistentCreateTime ( time(NULL) );
                }
                if ( process->GetType() == ProcessType_SSMP )
                {
                    Nodes->GetLNode ( process->GetNid() )->SetSSMProc ( process );
                }
            }
            else
            {
                if ( process->GetType() == ProcessType_DTM )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], DTM (%s) persistent restart failed, Node %s going down\n"
                            , method_name, process->GetName(), MyNode->GetName());
                    mon_log_write(MON_PROCESS_PERSIST_6, SQ_LOG_INFO, buf);

                    snprintf( buf, sizeof(buf),
                              "DTM (%s) persistent restart failed, Node %s going down\n",
                              process->GetName(), MyNode->GetName());
                    genSnmpTrap( buf );

                    // DTM just died unexpectedly, so bring the node down
                    Monitor->HardNodeDown(MyPNID, true);
                }
            }
        }
        else
        {
            if (trace_settings & (TRACE_SYNC | TRACE_REQUEST | TRACE_PROCESS))
                trace_printf("%s@%d" " - Not restarting process " "%s" " on nid=" "%d"   "\n", method_name, __LINE__, process->GetName(), process->GetNid());
            successful = restart;
        }
    }

    TRACE_EXIT;

    return successful;
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::PidHangupSet ( int pid )
{
    hungupPidsLock_.lock();
    hungupPids_.insert ( pid );
    hungupPidsLock_.unlock();
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::PidHangupClear ( int pid )
{
    hungupPidsLock_.lock();
    hungupPids_.erase ( pid );
    hungupPidsLock_.unlock();
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::CheckFdState ( int fd )
{
    const char method_name[   ] = "CProcessContainer::CheckFdState";
    char buf[MON_STRING_BUF_SIZE];

    int epollfd = epoll_create(5);
    if (epollfd == -1)
    {
        snprintf(buf, sizeof(buf), "[%s], epoll_create error, %s (%d)\n",
                method_name, strerror(errno), errno);
        mon_log_write(MON_PROCESS_CHECKFDSTATE_1, SQ_LOG_ERR, buf);

        return;
    }

    // Add file descriptor to epoll set
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    if ((epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
        && (errno != EEXIST))
    {
        snprintf(buf, sizeof(buf),
                 "[%s], epoll_ctl error, adding fd=%d, %s (%d)\n",
                method_name, fd, strerror(errno), errno);
        mon_log_write(MON_PROCESS_CHECKFDSTATE_2, SQ_LOG_ERR, buf);

        return;
    }

    // see if hangup is still asserted on stderr
    struct epoll_event event_list[10];
    int ready_fds = epoll_wait (epollfd, event_list, 10, 0);

    if (ready_fds == -1)
    {  // epoll_wait error
        snprintf(buf, sizeof(buf), "[%s], epoll_wait error, %s (%d)\n",
                method_name, strerror(errno), errno);
        mon_log_write(MON_PROCESS_CHECKFDSTATE_3, SQ_LOG_ERR, buf);
    }
    else if (ready_fds != 0)
    {
        for (int n=0; n < ready_fds; n++)
        {
            snprintf(buf, sizeof(buf),
                     "[%s], for fd=%d, events=%d\n", method_name,
                     event_list[n].data.fd, event_list[n].events);
            mon_log_write(MON_PROCESS_CHECKFDSTATE_4, SQ_LOG_INFO, buf);
        }
    }
    else
    {  // Indicate the epoll hangup no longer asserted
        snprintf(buf, sizeof(buf),
                 "[%s], No events pending for fd=%d\n", method_name, fd);
        mon_log_write(MON_PROCESS_CHECKFDSTATE_5, SQ_LOG_INFO, buf);
    }

    close( epollfd );
}
#endif

#ifndef NAMESERVER_PROCESS
void CProcessContainer::PidHangupCheck ( time_t now )
{
    const char method_name[   ] = "CProcessContainer::PidHangupCheck";
    TRACE_ENTRY;
    char buf[MON_STRING_BUF_SIZE];

    // Examine the list of processes for which we have received a
    // pipe hangup indication but have not received a child death
    // signal.
    hungupPidsLock_.lock();
    int pid;
    for (hungupPids_t::const_iterator it = hungupPids_.begin();
         it != hungupPids_.end();)
    {
        pid = *it;
        ++it;

        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf("%s@%d process %d is in hangup list\n",
                         method_name, __LINE__, pid);
        }

        CProcess * process = GetProcess (pid);
        time_t hangupTime = 0;

        if (process)
        {
            hangupTime = process->GetHangupTime();
            if ( now < (hangupTime + PROCESS_DEATH_MARGIN) )
            {   // Process hangup detected recently.  Wait a while before
                // taking action on this process.  This allows time for
                // child death signal to arrive.

                // temp trace
                if (trace_settings & TRACE_PROCESS)
                {
                    trace_printf("%s@%d process %d not yet ripe\n",
                                 method_name, __LINE__, pid);
                }

                continue;
            }
        }

        // See if process is still alive
        if (kill(pid,0) == -1)
        {
            if (errno == ESRCH)
            {   // Process no longer exists
                if (trace_settings & (TRACE_PROCESS_DETAIL | TRACE_EVLOG_MSG))
                {
                    trace_printf("%s@%d process %d no longer exists, initiating "
                                 "exit processing\n"
                                , method_name, __LINE__, pid);
                }

                // Remove from set
                hungupPids_.erase ( pid );

                // set state process
                // Queue request for processing by worker thread
                ReqQueue.enqueueChildDeathReq ( pid );

                // release buffers
                // todo
            }
            else
            {
                int saveerrno = errno;

                if (trace_settings & TRACE_PROCESS)
                    trace_printf("%s@%d process %d, errno=%d (%p)\n",
                                 method_name, __LINE__, pid, saveerrno,
                                 strerror(saveerrno));

                // Log info
                snprintf(buf, sizeof(buf),
                         "[%s], error getting process %d info, %s (%d)\n",
                        method_name, pid, strerror(saveerrno), saveerrno);
                mon_log_write(MON_PROCESS_PIDHANGUPCHECK_2, SQ_LOG_INFO, buf);
            }
        }
        else
        {
            char timestring[50];
            if (process)
            {
                strcpy(timestring, ctime ( &hangupTime ));
                timestring[strlen(timestring)-1] = '\0';
            }
            else
            {
                strcpy(timestring, "unknown");
            }

            if (trace_settings & TRACE_PROCESS)
                trace_printf("%s@%d process %d (%s) still running, no child "
                             "death indication received (hangup at %s)\n",
                             method_name, __LINE__, pid,
                             ((process != NULL) ? process->GetName(): "unknown"),
                             timestring);

            // Log info
            snprintf(buf, sizeof(buf),
                     "[%s], process %d (%s) still running, no child death "
                     "indication received (hangup at %s)\n", method_name, pid,
                     ((process != NULL) ? process->GetName() : "unknown"),
                     timestring);
            mon_log_write(MON_PROCESS_PIDHANGUPCHECK_3, SQ_LOG_INFO, buf);


            if (process)
                CheckFdState( process->FdStderr() );

            // Possibly kill process after sufficient time has elapsed
            // todo
        }
    }
    hungupPidsLock_.unlock();

    TRACE_EXIT;
}
#endif

void CProcessContainer::SetProcessState( CProcess *process, STATE state, bool abend, int downNode )
{
    const char method_name[] = "CProcessContainer::SetProcessState(process)";
    TRACE_ENTRY;

    if ( process )
    {
        switch ( state )
        {
        case State_Down:
            // Process intends to exits, when the child death arrives the
            // State_Stopped is processed
            if (trace_settings & TRACE_PROCESS)
                trace_printf( "%s@%d Setting State_Down for process %s(%d,%d:%d),"
                              "state=%s, abend=%d, down=%d\n"
                            , method_name, __LINE__
                            , process->GetName()
                            , process->GetNid()
                            , process->GetPid()
                            , process->GetVerifier()
                            , StateString(process->GetState())
                            , abend, downNode );
            process->SetState( State_Down );
            if ( abend && !process->IsAbended() )
            {
                process->SetAbended( abend );
            }
            break;
        case State_Stopped:
            if ( process->GetState() != State_Stopped )
            {
                // Process terminated so handle the exit processing.
                // Termination detected through a child death signal or
                // a broken stderr pipe for an attached process.

                // Note: Exit_Process() will delete the process object, so
                //       save the process information needed before the call
#ifndef NAMESERVER_PROCESS
                PROCESSTYPE processType = process->GetType();
#endif
                string      processName = process->GetName();
                int         processNid  = process->GetNid();
                int         processPid  = process->GetPid();
                Verifier_t  processVerifier = process->GetVerifier();
#ifndef NAMESERVER_PROCESS
                Exit_Process( process, abend, downNode );
#endif
                if (trace_settings & TRACE_PROCESS)
                    trace_printf( "%s@%d Set State_Stopped for process %s(%d,%d:%d), abend=%d, down=%d, "
                                  "killingMyNode=%d,DTM aborted=%d, SMS aborted=%d\n"
                                , method_name, __LINE__
                                , processName.c_str(), processNid, processPid, processVerifier
                                , abend, downNode
                                , MyNode->IsKillingNode(), MyNode->IsDTMAborted(), MyNode->IsSMSAborted());
#ifndef NAMESERVER_PROCESS
                if ( !MyNode->IsKillingNode() )
                {
                    switch ( processType )
                    {
                    case ProcessType_DTM:
                        if ( MyNode->GetState() != State_Shutdown &&
                             MyNode->IsDTMAborted() )
                        {
                            char buf[MON_STRING_BUF_SIZE];
                            snprintf(buf, sizeof(buf),
                                     "[%s], DTM (%s) aborted, Node %s going down\n",
                                     method_name, processName.c_str(), MyNode->GetName());
                            mon_log_write(MON_PROCESS_SETSTATE_1, SQ_LOG_INFO, buf);

                            snprintf( buf, sizeof(buf),
                                      "DTM (%s) aborted, Node %s going down\n",
                                      processName.c_str(), MyNode->GetName());
                            genSnmpTrap( buf );

                            // DTM just died unexpectedly, so bring the node down
                            Monitor->HardNodeDown(MyPNID, true);
                        }
                        break;
                    case ProcessType_SMS:
                        if ( MyNode->GetState() != State_Shutdown &&
                             MyNode->IsSMSAborted() )
                        {
                            char buf[MON_STRING_BUF_SIZE];
                            snprintf(buf, sizeof(buf),
                                     "[%s], SMS (%s) aborted, Node %s going down\n",
                                     method_name, processName.c_str(), MyNode->GetName());
                            mon_log_write(MON_PROCESS_SETSTATE_2, SQ_LOG_INFO, buf);

                            snprintf( buf, sizeof(buf),
                                      "SMS (%s) aborted, Node %s going down\n",
                                      processName.c_str(), MyNode->GetName());
                            genSnmpTrap( buf );

                            // SMS just died unexpectedly, so bring the node down
                            Monitor->HardNodeDown(MyPNID, true);
                        }
                        break;
                    default: // no special handling
                        break;
                    }
                }
#endif
            }
            break;
        default:
            process->SetState( state );
            break;
        }
    }

    TRACE_EXIT;
}

