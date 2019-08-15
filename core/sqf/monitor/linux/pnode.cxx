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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include "/usr/include/linux/watchdog.h"
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "redirector.h"
#include "monitor.h"
#include "clusterconf.h"
#include "device.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "mlio.h"
#include "nameserver.h"
#include "replicate.h"
#include "reqqueue.h"
#include "healthcheck.h"
#ifndef NAMESERVER_PROCESS
#include "zclient.h"
#include "ptpclient.h"
#endif

extern CReqQueue ReqQueue;
extern char MyPath[MAX_PROCESS_PATH];
extern int MyPNID;
extern bool IsRealCluster;
extern bool SpareNodeColdStandby;
extern bool Emulate_Down;

extern CConfigContainer *Config;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
#ifndef NAMESERVER_PROCESS
extern CDeviceContainer *Devices;
#endif
extern CNode *MyNode;
extern CMonStats *MonStats;
#ifndef NAMESERVER_PROCESS
extern CRedirector Redirector;
#endif
extern CReplicate Replicator;
extern CHealthCheck HealthCheck;
extern CMonTrace *MonTrace;
extern bool IsAgentMode;
extern bool IAmIntegrating;
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern CClusterConfig *ClusterConfig;

const char *StateString( STATE state);
#ifndef NAMESERVER_PROCESS
extern const char *ProcessTypeString( PROCESSTYPE type );
const char *SyncStateString( SyncState state);
extern CPtpClient *PtpClient;
extern CNameServer *NameServer;
extern CProcess *NameServerProcess;
extern bool NameServerEnabled;
extern bool ZClientEnabled;
extern bool IsMaster;
extern CZClient *ZClient;
#endif
extern CNameServerConfigContainer *NameServerConfig;

// The following defines are necessary for the new watchdog timer facility.  They should really be
// ultimately placed in watchdog.h in my opinion, especially so people know not to re-use values 16,17
// as they are specified for our higher resolution timer values.   These defines are used as parameters 
// to the ioctl calls.   Please not that the _IO[W]R_BAD  versions need to be used rather than the 
// mostly equivalent _IO[W]R version due to the fact that the latter version has some compile time parameter
// validation checking that is not liked by the INTEL compiler.   The BAD version avoids the check.  I've
// confirmed that the values passed in to satisfy the necessary space limit criteria for passing the test.
// They use GCC which does not experience the same compiler issue.

#define WATCHDOG_IOCTL_BASE     'W'

#define WDIOC_SQ_GETSUPPORT        _IOR(WATCHDOG_IOCTL_BASE, 0, struct watchdog_info)
#define WDIOC_SQ_GETSTATUS         _IOR(WATCHDOG_IOCTL_BASE, 1, int)
#define WDIOC_SQ_GETBOOTSTATUS     _IOR(WATCHDOG_IOCTL_BASE, 2, int)
#define WDIOC_SQ_GETTEMP           _IOR(WATCHDOG_IOCTL_BASE, 3, int)
#define WDIOC_SQ_SETOPTIONS        _IOR(WATCHDOG_IOCTL_BASE, 4, int)
#define WDIOC_SQ_KEEPALIVE         _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SQ_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_SQ_GETTIMEOUT        _IOR(WATCHDOG_IOCTL_BASE, 7, int)


// Default interval used by GetSchedulingData (in milliseconds)
unsigned long int CNode::minSchedDataInterval_ = 500;

// Strings for use in obtaining /proc/meminfo data (see
// CNode::GetSchedulingData).  Spelling is important.  Must match
// items in /proc/meminfo.
const char * CNode::memInfoString_[] = {
    "MemTotal:",
    "MemFree:",
    "Buffers:",
    "Cached:",
    "SwapFree:",
    "Active:",
    "Inactive:",
    "Dirty:",
    "Writeback:",
    "VmallocUsed:"
};

size_t CNode::memInfoStringLen_[memFinalItem];


CNode::CNode( char *name
            , char *domain
            , char *fqdn
            , int pnid
            , int rank )
      :CLNodeContainer(this)
      ,CProcessContainer(true)
      ,pnid_(pnid)
      ,changeState_(false)
      ,numCores_(0)
      ,freeCache_(0)
      ,state_(rank == -1 ? State_Down : State_Up)
      ,phase_(Phase_Ready)
      ,killingNode_(false)
      ,dtmAborted_(false)
      ,smsAborted_(false)
      ,pendingNodeDown_(false)
      ,primitiveDtmUp_(false)
      ,primitivePsdUp_(false)
      ,primitiveWdgUp_(false)
      ,lastLNode_(NULL)
      ,lastSdLevel_(ShutdownLevel_Undefined)
      ,rankFailure_(false)
      ,creator_(false)
      ,creatorPid_(-1)
      ,creatorVerifier_(-1)
      ,joiningPhase_(JoiningPhase_Unknown)
      ,activatingSpare_(false)
      ,spareNode_(false)
      ,next_(NULL)
      ,prev_(NULL)
      ,rank_(rank)
      ,shutdownLevel_(ShutdownLevel_Undefined)
      ,shutdownNameServer_(false)
      ,wdtKeepAliveTimerValue_(WDT_KEEPALIVETIMERDEFAULT)
      ,zid_(pnid)
      ,commPort_("")
      ,syncPort_("")
#ifdef NAMESERVER_PROCESS
      ,mon2NsPort_("")
      ,mon2NsSocketPort_(-1)
      ,monConnCount_(0)
#else
      ,ptpPort_("")
      ,ptpSocketPort_(-1)
#endif
      ,commSocketPort_(-1)
      ,syncSocketPort_(-1)
      ,uniqStrId_(-1)
      ,procStatFile_(NULL)
      ,procMeminfoFile_(-1)
{
    const char method_name[] = "CNode::CNode";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PNOD", 4);

    char *p = getenv("SQ_SCHED_DATA_INTERVAL");
    if (p != NULL)
    {
        minSchedDataInterval_ = atoi(p);
    }
    if (trace_settings & TRACE_INIT)
    {
        trace_printf("%s@%d minSchedDataInterval_=%ld\n", method_name,
                     __LINE__, minSchedDataInterval_);
    }

    prevSchedData_.tv_sec = 0;
    prevSchedData_.tv_nsec = 0;

    STRCPY(name_, name);
    STRCPY(domain_, domain);
    STRCPY(fqdn_, fqdn);
    
    hostname_ = name;
    size_t pos = hostname_.find_first_of( ".:" );
    if ( pos != hostname_.npos )
    {
        hostname_.erase( hostname_.begin()+pos, hostname_.end() );
    }

    for (unsigned int i=0; i<memFinalItem; i++)
    {
        memInfoStringLen_[i] = strlen ( memInfoString_[i] );
    }

    if (pnid_ == MyPNID)
    {
        GetSchedulingData();
#ifdef HARD_AFFINITY
        int i;
        cpu_set_t mask;

        sched_getaffinity(getpid(), sizeof(affinityMask_), &affinityMask_);
        for (i=0; i < numCores_; i++)
        {
            if (CPU_ISSET(i, &affinityMask_))
            {
                if (trace_settings & TRACE_INIT)
                    trace_printf("%s@%d" " - Processor core #" "%d" " is usable" "\n", method_name, __LINE__, i);
            }
            else
            {
                if (trace_settings & TRACE_INIT)
                    trace_printf("%s@%d" " - Processor core #" "%d" " is not activated" "\n", method_name, __LINE__, i);
            }
        }
        if ( !IsRealCluster )
        {
            // set the monitor's hard affinity to a processor core
            CPU_ZERO(&mask);
            CPU_SET(MyPNID%numCores_, &mask);
            sched_setaffinity(getpid(), sizeof(mask), &mask);
        }
#else
        sched_getaffinity(getpid(), sizeof(affinityMask_), &affinityMask_);
#endif
    }
    
    CPU_ZERO( &excludedCoreMask_ );

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {   // Display pidMap location, useful for understanding later trace output
        trace_printf("%s@%d PNid %d, pidMap_ (%p)\n",
                     method_name, __LINE__, GetPNid(), GetPidMap());
    }

    gettimeofday(&todStart_, NULL);

#ifndef NAMESERVER_PROCESS
    quiesceSendPids_ = new SQ_LocalIOToClient::bcastPids_t;
    quiesceExitPids_ = new SQ_LocalIOToClient::bcastPids_t;
#endif
    internalState_ = State_Default; 

    TRACE_EXIT;
}

CNode::CNode( char *name
            , char *domain
            , char *fqdn
            , int   pnid
            , int   rank
            , int   sparePNidCount
            , int   sparePNids[]
            , cpu_set_t &excludedCoreMask )
      :CLNodeContainer(this)
      ,CProcessContainer(true)
      ,pnid_(pnid)
      ,changeState_(false)
      ,numCores_(0)
      ,freeCache_(0)
      ,state_(rank == -1 ? State_Down : State_Up)
      ,phase_(Phase_Ready)
      ,killingNode_(false)
      ,dtmAborted_(false)
      ,smsAborted_(false)
      ,pendingNodeDown_(false)
      ,lastLNode_(NULL)
      ,lastSdLevel_(ShutdownLevel_Undefined)
      ,excludedCoreMask_(excludedCoreMask)
      ,rankFailure_(false)
      ,creator_(false)
      ,creatorPid_(-1)
      ,creatorVerifier_(-1)
      ,joiningPhase_(JoiningPhase_Unknown)
      ,activatingSpare_(false)
      ,spareNode_(true)
      ,next_(NULL)
      ,prev_(NULL)
      ,rank_(rank)
      ,shutdownLevel_(ShutdownLevel_Undefined)
      ,shutdownNameServer_(false)
      ,wdtKeepAliveTimerValue_(WDT_KEEPALIVETIMERDEFAULT)
      ,zid_(-1)
      ,commPort_("")
      ,syncPort_("")
#ifdef NAMESERVER_PROCESS
      ,mon2NsPort_("")
      ,mon2NsSocketPort_(-1)
      ,monConnCount_(-1)
#else
      ,ptpPort_("")
      ,ptpSocketPort_(-1)
#endif
      ,commSocketPort_(-1)
      ,syncSocketPort_(-1)
      ,uniqStrId_(-1)
      ,procStatFile_(NULL)
      ,procMeminfoFile_(-1)
{
    const char method_name[] = "CNode::CNode";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PNOD", 4);

    STRCPY(name_, name);
    STRCPY(domain_, domain);
    STRCPY(fqdn_, fqdn);
    
    hostname_ = name;
    size_t pos = hostname_.find_first_of( ".:" );
    if ( pos != hostname_.npos )
    {
        hostname_.erase( hostname_.begin()+pos, hostname_.end() );
    }

    for (unsigned int i=0; i<memFinalItem; i++)
    {
        memInfoStringLen_[i] = strlen ( memInfoString_[i] );
    }

    if (pnid_ == MyPNID)
    {
        GetSchedulingData();
#ifdef HARD_AFFINITY
        sched_getaffinity(getpid(), sizeof(affinityMask_), &affinityMask_);
        int i;
        cpu_set_t mask;

        for (i=0; i < numCores_; i++)
        {
            if (CPU_ISSET(i, &affinityMask_))
            {
                if (trace_settings & TRACE_INIT)
                    trace_printf("%s@%d" " - Processor core #" "%d" " is usable" "\n", method_name, __LINE__, i);
            }
            else
            {
                if (trace_settings & TRACE_INIT)
                    trace_printf("%s@%d" " - Processor core #" "%d" " is not activated" "\n", method_name, __LINE__, i);
            }
        }
        if ( !IsRealCluster )
        {
            // set the monitor's hard affinity to a processor core
            CPU_ZERO(&mask);
            CPU_SET(MyPNID%numCores_, &mask);
            sched_setaffinity(getpid(), sizeof(mask), &mask);
        }
#else
        sched_getaffinity(getpid(), sizeof(affinityMask_), &affinityMask_);
#endif
    }

    CPU_ZERO( &excludedCoreMask_ );

    for ( int i = 0; i <  sparePNidCount; i++ )
    {
        sparePNids_.push_back( sparePNids[i] );
    }

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {   // Display pidMap location, useful for understanding later trace output
        trace_printf("%s@%d PNid %d, pidMap_ (%p)\n",
                     method_name, __LINE__, GetPNid(), GetPidMap());
    }
    
    gettimeofday(&todStart_, NULL);

#ifndef NAMESERVER_PROCESS
    quiesceSendPids_ = new SQ_LocalIOToClient::bcastPids_t;
    quiesceExitPids_ = new SQ_LocalIOToClient::bcastPids_t;
#endif
    internalState_ = State_Default; 

    TRACE_EXIT;
}

CNode::~CNode( void )
{
    const char method_name[] = "CNode::~CNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d pnid=%d\n", method_name, __LINE__, pnid_ );
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "pnod", 4);

    if ( procStatFile_ != NULL )
        fclose( procStatFile_ );

    if ( procMeminfoFile_ != -1 )
        close( procMeminfoFile_ );

#ifndef NAMESERVER_PROCESS
    if (quiesceSendPids_)
    {
        delete quiesceSendPids_;
    }

    if (quiesceExitPids_)
    {
        delete quiesceExitPids_;
    }
#endif
    
    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CNode::addToQuiesceSendPids( int pid, Verifier_t verifier )
{
    SQ_LocalIOToClient::pidVerifier_t pv;
    pv.pv.pid = pid;
    pv.pv.verifier = verifier;
    quiesceSendPids_->insert( pv.pnv );
}

void CNode::addToQuiesceExitPids( int pid, Verifier_t verifier )
{
    SQ_LocalIOToClient::pidVerifier_t pv;
    pv.pv.pid = pid;
    pv.pv.verifier = verifier;
    quiesceExitPids_->insert( pv.pnv );
}

void CNode::delFromQuiesceExitPids( int pid, Verifier_t verifier )
{
    SQ_LocalIOToClient::pidVerifier_t pv;
    pv.pv.pid = pid;
    pv.pv.verifier = verifier;
    quiesceExitPids_->erase( pv.pnv );
}
#endif

int CNode::AssignNid(void)
{
    const char method_name[] = "CNode::AssignNid";
    TRACE_ENTRY;

    CLNode *lnode = AssignLNode();
    
    TRACE_EXIT;
    if (lnode)
    {
        return( lnode->Nid );
    }
    else
    {
        return -1;
    }
}

CLNode *CNode::AssignLNode (void)
{
    CLNode *lnode;

    const char method_name[] = "CNode::AssignLNode";
    TRACE_ENTRY;

    assert(!spareNode_);

    // If logical nodes are configured
    lnode = GetFirstLNode();
    if ( lnode )
    {
        if ( !lastLNode_ )
        {
            lastLNode_ = lnode;
        }
        else if (lastLNode_->GetNextP())
        {
            lnode = lastLNode_->GetNextP();
        }
        else
        {
            lnode = lastLNode_;
        }
    }
    
    TRACE_EXIT;
    return lnode;
}

void CNode::CheckActivationPhase( void )
{
    int         tmCount = 0;
    CLNode     *lnode;
    CProcess   *process;
    bool        tmReady = false;

    const char method_name[] = "CNode::CheckActivationPhase";
    TRACE_ENTRY;

    // check for a TM process in each lnode
    lnode = GetFirstLNode();

    tmReady = lnode ? true : false;
    for ( ; lnode ; lnode = lnode->GetNextP() )
    {
        if ( lnode->GetState() == State_Up )
        {
            process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process )
            {
                tmCount++;
                if (trace_settings & (TRACE_INIT | TRACE_SYNC | TRACE_TMSYNC))
                    trace_printf("%s@%d - TM %s (pid=%d)\n", method_name, __LINE__, process->GetName(), process->GetPid());
            }
        }
        tmReady = (tmCount == GetLNodesCount()) ? true : false;
    }

    if ( tmReady )
    {
        if (trace_settings & (TRACE_INIT | TRACE_SYNC | TRACE_TMSYNC))
            trace_printf("%s@%d - Setting Phase_Ready on node %s, pnid=%d\n", method_name, __LINE__, GetName(), GetPNid());
        phase_ = Phase_Ready;
        HealthCheck.triggerTimeToLogHealth();
    }

    TRACE_EXIT;
}

void CNode::CheckShutdownProcessing( void )
{
    const char method_name[] = "CNode::CheckShutdownProcessing";
    TRACE_ENTRY;

#ifndef NAMESERVER_PROCESS
    struct message_def *msg;
    if ( shutdownLevel_ != lastSdLevel_ )
    {
        lastSdLevel_ = shutdownLevel_;
        msg = new struct message_def;
        msg->type = MsgType_Shutdown;
        msg->noreply = true;
        msg->u.request.type = ReqType_Notice;
        msg->u.request.u.shutdown.nid = MyPNID;
        msg->u.request.u.shutdown.pid = -1;
        msg->u.request.u.shutdown.level = shutdownLevel_;
        if (trace_settings & TRACE_SYNC)
           trace_printf("%s@%d" " - Broadcasting shutdown notice, level=" "%d" "\n", method_name, __LINE__, shutdownLevel_);
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "Broadcasting shutdown notice, level = %d\n", shutdownLevel_);
        mon_log_write(MON_NODE_SHUTDOWN_1, SQ_LOG_WARNING, buf);
        Bcast (msg);
        delete msg;
    }
#endif

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
// In virtual node configuration, empty the quiescing pids so that new ones could be added.
void CNode::EmptyQuiescingPids()
{
    const char method_name[] = "CNode::EmptyQuiescingPids";
    TRACE_ENTRY;

    if (quiesceSendPids_)
    {
        quiesceSendPids_->clear();
    }

    if (quiesceExitPids_)
    {
        quiesceExitPids_->clear();
    }

    TRACE_EXIT;
}
#endif

#ifndef NAMESERVER_PROCESS
// Send quiescing notices to pids in the QiesceSendPids list. 
void CNode::SendQuiescingNotices( void )
{
    struct message_def *msg;

    const char method_name[] = "CNode::SendQuiescingNotices";
    TRACE_ENTRY;
    
    // send quiescing notices to TSEs and TMs
    msg = new struct message_def;
    msg->type = MsgType_NodeQuiesce;
    msg->noreply = true;
    msg->u.request.type = ReqType_Notice;
    msg->u.request.u.down.nid = MyPNID;
    strcpy(msg->u.request.u.down.node_name, GetNode()->GetName());

    if (trace_settings & TRACE_NOTICE)
           trace_printf("%s@%d" " - Broadcasting Quiesce notice to TSE and DTMs" "\n", method_name, __LINE__);

    // make a copy because the notice clear will delete the pids list if empty. 
    SQ_LocalIOToClient::bcastPids_t *quiesceSendPidsCopy = new SQ_LocalIOToClient::bcastPids_t;
    *quiesceSendPidsCopy = *quiesceSendPids_;

    SQ_theLocalIOToClient->putOnNoticeQueue( BCAST_PID
                                           , -1
                                           , msg
                                           , quiesceSendPidsCopy);

    TRACE_EXIT;
}
#endif

void CNode::SetState( STATE state )
{
    if ( state != state_ )
    {
        state_ = state;
        mem_log_write(CMonLog::MON_NODE_1, pnid_, state_); 
    }
}

void CNode::DeLink( CNode ** head, CNode ** tail )
{
    const char method_name[] = "CNode::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = next_;
    if (*tail == this)
        *tail = prev_;
    if (prev_)
        prev_->next_ = next_;
    if (next_)
        next_->prev_ = prev_;
    TRACE_EXIT;
}


void CNode::GetCpuStat ( void )
{
    char buffer[132];
    char name[25];
    long long num1, num2, num3, num4, num5, num6, num7;
    int count;
    int count2;
    int cpunum;

    // Holding area for cpu statistics, one set per core
    typedef struct {
        long long user;
        long long nice;
        long long sys;
        long long idle;
        long long wait;
        long long irq;
        long long sirq;
    } cpuStats_t;
    cpuStats_t cpuStats[MAX_CORES];

    const char method_name[] = "CNode::GetCpuStat";
    TRACE_ENTRY;


    // get processor statistics
    if ( procStatFile_ == NULL )
    {   // First time, open the file
        procStatFile_ = fopen("/proc/stat", "r");
        if (!procStatFile_)
        {
            if (trace_settings & TRACE_SYNC)
                trace_printf("%s@%d Cannot open /proc/stat, %s (%d)\n",
                             method_name, __LINE__, strerror(errno), errno);

            char buf[MON_STRING_BUF_SIZE];
            sprintf(buf, "[%s], Cannot open /proc/stat, %s (%d)\n",
                    method_name, strerror(errno), errno);

            mon_log_write(MON_NODE_GETCPUSTAT_1, SQ_LOG_ERR, buf);

            return;
        }
    }
    else
    {   // Other times, go back to the beginning.
        rewind ( procStatFile_ );
    }

    while( !feof(procStatFile_) )
    {
        fgets(buffer,132,procStatFile_);

        num1 = num2 = num3 = num4 = num5 = num6 = num7 = 0;
        count = sscanf(buffer, "%s %Ld %Ld %Ld %Ld %Ld %Ld %Ld",
                       (char *) &name, &num1, &num2, &num3, &num4, &num5,
                       &num6, &num7);

        if (count <= 0)
        {   // Invalid stats line
            continue;
        }

        if( strncmp( name, "btime", 5 ) == 0 )
        {
            bTime_ = (int) num1;
        }
        else if ( strncmp ( name, "cpu", 3) == 0 )
        {   // Got cpu statistics.  Determine which cpu.
            count2 = sscanf(name, "cpu%d", &cpunum);

            // Got cpu number, store the statistics for this cpu
            if (count2 == 1)
            {
                if (trace_settings & TRACE_SYNC_DETAIL)
                    trace_printf("%s@%d For cpu %d, user=%lld, nice=%lld, "
                                 "system=%lld, idle=%lld, iowait=%lld, "
                                 "irq=%lld, softirq=%lld\n", method_name,
                                 __LINE__, cpunum, num1, num2, num3, num4,
                                 num5, num6, num7);

                if (cpunum < MAX_CORES)
                {
                    cpuStats[cpunum].user = num1;
                    cpuStats[cpunum].nice = num2;
                    cpuStats[cpunum].sys  = num3;
                    cpuStats[cpunum].idle = num4;
                    cpuStats[cpunum].wait = num5;
                    cpuStats[cpunum].irq  = num6;
                    cpuStats[cpunum].sirq = num7;
                }
            }
        }
    }

    long long totUser, totNice, totSys, totIdle, totWait, totIrq, totSirq;
    int numCoresInLnode;
    int firstCoreInLnode;

    CLNode *lnode = GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        numCoresInLnode = lnode->GetNumCores();
        firstCoreInLnode = lnode->GetFirstCore();

        // temp trace
        if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d pnode #%d, lnode #%d, numCoresInLnode=%d, "
                         "firstCoreInLnode=%d\n", method_name, __LINE__,
                         lnode->GetNode()->GetPNid(), lnode->Nid, numCoresInLnode,
                         firstCoreInLnode);

        // Aggregate stats for the logical node
        totUser = totNice = totSys = totIdle = totWait = totIrq = totSirq = 0;
        for (int i=firstCoreInLnode; i<(firstCoreInLnode+numCoresInLnode); ++i)
        {
            totUser += cpuStats[i].user;
            totNice += cpuStats[i].nice;
            totSys  += cpuStats[i].sys;
            totIdle += cpuStats[i].idle;
            totWait += cpuStats[i].wait;
            totIrq  += cpuStats[i].irq;
            totSirq += cpuStats[i].sirq;

            if (trace_settings & TRACE_SYNC_DETAIL)
                trace_printf("%s@%d pnode #%d, lnode #%d, core=%d, "
                             "totUser=%lld, totNice=%lld, totSys=%lld, "
                             "totIdle=%lld, totWait=%lld, totIrq=%lld, "
                             "totSirq=%lld\n", method_name, __LINE__,
                             lnode->GetNode()->GetPNid(), lnode->Nid, i, totUser,
                             totNice, totSys, totIdle, totWait, totIrq,
                             totSirq);

        }

        // Store average of stats over all cores comprising the logical node
        lnode->SetCpuUser   ( totUser / numCoresInLnode );
        lnode->SetCpuNice   ( totNice / numCoresInLnode );
        lnode->SetCpuSystem ( totSys  / numCoresInLnode );
        lnode->SetCpuIdle   ( totIdle / numCoresInLnode );
        lnode->SetCpuIowait ( totWait / numCoresInLnode );
        lnode->SetCpuIrq    ( totIrq  / numCoresInLnode );
        lnode->SetCpuSoftIrq( totSirq / numCoresInLnode );
    }

    TRACE_EXIT;
}

bool CNode::NextMemInfoLine( bufInfo_t &inBuf, char * dataline )
{
    char * eol;
    size_t bytes;
    ssize_t count;

    // check if another line is available in the buffer
    eol = strchr ( inBuf.bol, '\n' );

    if ( eol == NULL )
    {   // no, so read more data

        // move any residual data
        memcpy ( inBuf.buffer, inBuf.bol, inBuf.remBytes );

        // read additional data
        count = read(procMeminfoFile_, &inBuf.buffer[inBuf.remBytes],
                     inBuf.bufsize - inBuf.remBytes - 1);

        if ( count != 0 && count != -1 )
        {   // Update buffer data pointers and counts
            inBuf.buffer[inBuf.remBytes + count] = '\0';
            inBuf.remBytes += count;
            inBuf.bol = inBuf.buffer;

            eol = strchr ( inBuf.bol, '\n' );
        }
    }

    if ( eol != NULL )
    {   // Have another line, return data to caller

        bytes = eol - inBuf.bol;

        memcpy ( dataline, inBuf.bol, bytes );
        dataline[bytes] = '\0';

        inBuf.bol = ++eol;
        inBuf.remBytes -= (bytes + 1);

        return true;
    }
    else
    {   // No more data available
        return false;
    }
}

bool CNode::GetSchedulingData( void )
{
    static bool get_processor_info = true;

    const char method_name[] = "CNode::GetSchedulingData";
    TRACE_ENTRY;

    if( pnid_ != MyPNID )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[CNode::GetSchedulingData], Invalid call for remote node.\n");
        mon_log_write(MON_NODE_GETSCHDATA_1, SQ_LOG_ERR, buf);
    }
    else
    {
       // Getting scheduling data is time consuming so don't do it too
        // often.
        struct timespec now;
        long int secs;
        long int nsecs;
        unsigned long long int msecs;

        clock_gettime(CLOCK_REALTIME, &now);
        if ( (now.tv_nsec - prevSchedData_.tv_nsec )  < 0 )
        {
            secs = now.tv_sec - prevSchedData_.tv_sec - 1;
            nsecs = 1000000000 + now.tv_nsec - prevSchedData_.tv_nsec;
        }
        else
        {
            secs = now.tv_sec - prevSchedData_.tv_sec;
            nsecs = now.tv_nsec - prevSchedData_.tv_nsec;
        }
        msecs = (secs * 1000) + (nsecs / 1000000);
        if ( msecs < minSchedDataInterval_ )
        {
            if (trace_settings & TRACE_SYNC)
                trace_printf("%s@%d Defer refreshing scheduling data\n",
                             method_name, __LINE__);
            // Wait longer to get scheduling data again
            return false;
        }
        // Remember time we last got scheduling data
        prevSchedData_ = now;

        // get memory information    
        if ( procMeminfoFile_ == -1 )
        {   // First time, open the file
            procMeminfoFile_ = open("/proc/meminfo", O_RDONLY);
            if ( procMeminfoFile_ == -1 )
            {
                if (trace_settings & TRACE_SYNC)
                    trace_printf("%s@%d Cannot open /proc/meminfo, %s (%d)\n",
                                 method_name, __LINE__, strerror(errno), errno);

                char buf[MON_STRING_BUF_SIZE];
                sprintf(buf, "[%s], Cannot open /proc/meminfo, %s (%d)\n",
                        method_name, strerror(errno), errno);
                mon_log_write(MON_NODE_GETSCHDATA_7, SQ_LOG_ERR, buf);
            }
        }
        else
        {   // Other times, go back to the beginning.
            lseek ( procMeminfoFile_, 0, SEEK_SET );
        }

        if ( procMeminfoFile_ != -1 )
        {
            bool foundProcMem[memFinalItem];
            for (unsigned int i=0; i<memFinalItem; i++)
                foundProcMem[i] = false;

            int totalProcMemFound = 0;
            unsigned long int value;

            char readBuffer[132];
            char memLine[80];

            bufInfo_t bufInfo;
            readBuffer[0] = '\0';
            bufInfo.buffer = readBuffer;
            bufInfo.bol = readBuffer;
            bufInfo.bufsize = sizeof(readBuffer);
            bufInfo.remBytes = 0;

            while  ( NextMemInfoLine ( bufInfo, memLine ) )
            {
                for (unsigned int i=0; i<memFinalItem; i++)
                {
                    if ( strncmp( memLine, memInfoString_[i],
                                  memInfoStringLen_[i]) == 0 )
                    {
                        errno = 0;
                        value = strtoul ( &memLine[memInfoStringLen_[i]],
                                          NULL, 10 );
                        if ( errno == 0 )
                        {
                            memInfoData_[i] = value;
                        }
                        else
                        {   // Unexpectedly could not convert data
                            // value. (Will retain previous value.)
                            char buf[MON_STRING_BUF_SIZE];
                            sprintf(buf, "%s, possible /proc/meminfo/%s "
                                    " conversion error, memLine='%s'. errno=%d"
                                    "\n", method_name, memInfoString_[i],
                                    memLine, errno);
                            mon_log_write(MON_NODE_GETSCHDATA_3, SQ_LOG_ERR,
                                          buf);
                        }
                        foundProcMem[i] = true;
                        ++totalProcMemFound;
                        break;
                    }
                }

                if (totalProcMemFound == memFinalItem)
                    break;
            }

            freeCache_ = memInfoData_[memFree] + memInfoData_[memBuffers]
                + memInfoData_[memCached];
            if (totalProcMemFound != memFinalItem)
            {   // Failed to find some data

                for (unsigned int i=0; i<memFinalItem; i++)
                {
                    if (!foundProcMem[i])
                    {   // Did not obtain this memory counter
                        char buf[MON_STRING_BUF_SIZE];
                        sprintf(buf, "%s, %s not in /proc/meminfo.\n",
                                method_name, memInfoString_[i]);
                        mon_log_write(MON_NODE_GETSCHDATA_2, SQ_LOG_ERR, buf);
                    }
                }
            }
        }

        if( get_processor_info )
        {
            // get processor information, one time only
            FILE *proccpu = NULL;
            char buffer[132];
            numCores_ = 0;
            proccpu = fopen("/proc/cpuinfo", "r");
    
            while( !feof(proccpu) )
            {    
                fgets(buffer,132,proccpu);
                if( strncmp( buffer, "processor\t:", 11 ) == 0 )
                {
                    numCores_++;
                }
            }

            get_processor_info = false;
            if (trace_settings & TRACE_SYNC_DETAIL)
               trace_printf("%s@%d" " - NumCores=" "%d" "\n", method_name, __LINE__, GetNumCores());
            fclose( proccpu );
        }

       if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d - Processor memory=%d kB, FreeMemory=%d kB, buffers=%d kB, cached=%d kB, FreeSwap=%d kB, Active=%d kB, Inactive=%d kB, Dirty=%d kB, Writeback=%d kB, VmallocUsed=%d kB\n", method_name, __LINE__, memInfoData_[memTotal], memInfoData_[memFree], memInfoData_[memBuffers], memInfoData_[memCached], memInfoData_[memSwapFree], memInfoData_[memActive], memInfoData_[memInactive], memInfoData_[memDirty], memInfoData_[memWriteback], memInfoData_[memVmallocUsed]);

       GetCpuStat();
    }

    TRACE_EXIT;

    return true;
}


strId_t CNode::GetStringId( char *candidate, CLNode *targetLNode, bool clone )
{
    const char method_name[] = "CNode::GetStringId";
    TRACE_ENTRY;

    strId_t existStrId;
    strId_t strId;
    string  existUString;

    if ( ! Config->getUniqueStringId( pnid_, candidate, strId ) )
    {   // The candidate string is not in the configuration database
        if (uniqStrId_ == -1)
        {   // Get the last unique string id assigned
            uniqStrId_ = Config->getMaxUniqueId( pnid_ );
        }
        existStrId.nid = pnid_;
        existStrId.id  = ++uniqStrId_;
        while (Config->getUniqueString(existStrId.nid, existStrId.id, existUString))
        {
            existStrId.id  = ++uniqStrId_;
        }

        strId.nid = pnid_;
        strId.id  = uniqStrId_;
        Config->addUniqueString(strId.nid, strId.id, candidate);

#ifndef NAMESERVER_PROCESS
        if (NameServerEnabled)
        {
            if (targetLNode != NULL && !clone &&
                !MyNode->IsMyNode(targetLNode->GetNid()))
            {
                // Forward the unique string to the target node
                int rc = PtpClient->ProcessAddUniqStr( strId.nid
                                                     , strId.id
                                                     , candidate
                                                     , targetLNode->GetNid()
                                                     , targetLNode->GetNode()->GetName() );
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send unique string "
                              "to target node %s, nid=%d\n"
                            , method_name
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_NODE_GETSTRINGID_1, SQ_LOG_ERR, la_buf);
                }
            }
        }
        else
#endif
        {
#ifdef NAMESERVER_PROCESS
            clone = clone;  // Make compiler happy!
            targetLNode = targetLNode;  // Make compiler happy!
#endif
            CReplUniqStr *repl = new CReplUniqStr ( strId.nid, strId.id, candidate );
            Replicator.addItem(repl);
        }
    }
    // temp trace
    else
    {
        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf("%s@%d - unique string id=[%d,%d] (%s)\n",
                         method_name, __LINE__, strId.nid, strId.id, candidate );
        }

#ifndef NAMESERVER_PROCESS
        if (NameServerEnabled)
        {
            if (targetLNode != NULL && !clone &&
                !MyNode->IsMyNode(targetLNode->GetNid()))
            {
                // Forward the unique string to the target node
                int rc = PtpClient->ProcessAddUniqStr( strId.nid
                                                     , strId.id
                                                     , candidate
                                                     , targetLNode->GetNid()
                                                     , targetLNode->GetNode()->GetName());
                if (rc)
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    snprintf( la_buf, sizeof(la_buf)
                            , "[%s] - Can't send unique string "
                              "to target node %s, nid=%d\n"
                            , method_name
                            , targetLNode->GetNode()->GetName()
                            , targetLNode->GetNid() );
                    mon_log_write(MON_NODE_GETSTRINGID_2, SQ_LOG_ERR, la_buf);
                }
            }
        }
#endif
    }

    TRACE_EXIT;

    return strId;
}

CNode *CNode::LinkAfter( CNode * &tail, CNode * entry )
{
    const char method_name[] = "CNode::LinkAfter";
    TRACE_ENTRY;

    entry->prev_ = this;
    if (next_ == NULL)
    {
        entry->next_ = NULL;
        tail = entry;
    }
    else
    {
        entry->next_ = next_;
        next_->prev_ = entry;
    }
    next_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked physical node object "
                      "tail=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , tail->GetPNid()
                    , prev_?prev_->GetPNid():-1
                    , GetPNid()
                    , next_?next_->GetPNid():-1
                    , entry->prev_?entry->prev_->GetPNid():-1
                    , entry->GetPNid()
                    , entry->next_?entry->next_->GetPNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

CNode *CNode::LinkBefore( CNode * &head, CNode * entry )
{
    const char method_name[] = "CNode::LinkBefore";
    TRACE_ENTRY;

    entry->next_ = this;
    if (prev_ == NULL)
    {
        entry->prev_ = NULL;
        head = entry;
    }
    else
    {
        entry->prev_ = prev_;
        prev_->next_ = entry;
    }
    prev_ = entry;

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d - Linked physical node object "
                      "head=%d\n"
                      "\t\tthis: prev=%d, this=%d, next=%d\n"
                      "\t\tentry: prev=%d, entry=%d, next=%d\n"
                    , method_name, __LINE__
                    , head->GetPNid()
                    , prev_?prev_->GetPNid():-1
                    , GetPNid()
                    , next_?next_->GetPNid():-1
                    , entry->prev_?entry->prev_->GetPNid():-1
                    , entry->GetPNid()
                    , entry->next_?entry->next_->GetPNid():-1 );
    }
    
    TRACE_EXIT;
    return entry;
}

void CNode::MoveLNodes( CNode *spareNode )
{
    CLNode *lnode;
    const char method_name[] = "CNode::MoveLNodes";
    TRACE_ENTRY;

    // Move the logical nodes
    lnode = GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        // adjust each logical node to reference the 
        // new physical node's logical nodes container
        lnode->SetLNodeContainer( spareNode->GetLNodeContainer() );
    }
    spareNode->SetFirstLNode( GetFirstLNode() );
    spareNode->SetLastLNode( GetLastLNode() );
    spareNode->SetLNodesCount( GetLNodesCount() );
    SetFirstLNode( NULL );
    SetLastLNode( NULL );
    SetLNodesCount( 0 );
    lastLNode_ = NULL;

    // Move the physical node's process list
    nameMap_t *nameMap = spareNode->GetNameMap();
    nameMap->clear();
    pidMap_t *pidMap = spareNode->GetPidMap();
    pidMap->clear();
    spareNode->SetNameMap( GetNameMap() );
    SetNameMap( nameMap );
    spareNode->SetPidMap( GetPidMap() );
    SetPidMap( pidMap );
    spareNode->SetFirstProcess( GetFirstProcess() );
    spareNode->SetLastProcess( GetLastProcess() );
    spareNode->SetNumProcs( GetNumProcs() );
    spareNode->ResetSpareNode();
    // Reset the state of the down node, it is now a spare node
    SetFirstProcess( NULL );
    SetLastProcess( NULL );
    SetNumProcs( 0 );
    SetSpareNode();

    // Move zone identifier
    spareNode->SetZone( GetZone() );
    SetZone( -1 );
    
    TRACE_EXIT;
    return;
}

#ifndef NAMESERVER_PROCESS
void CNode::SetAffinity( int nid, pid_t pid, PROCESSTYPE type )
{
    CLNode  *lnode = Nodes->GetLNode( nid );

    const char method_name[] = "CNode::SetAffinity";
    TRACE_ENTRY;

    if ( lnode )
    {
        lnode->SetAffinity (pid, type);
    }
    else
    {
        abort();
    }

    TRACE_EXIT;
}

void CNode::SetAffinity( CProcess *process )
{
    CLNode  *lnode = Nodes->GetLNode( process->GetNid() );

    const char method_name[] = "CNode::SetAffinity";
    TRACE_ENTRY;

    if ( lnode )
    {
        lnode->SetAffinity( process );
    }
    else
    {
        abort();
    }

    TRACE_EXIT;
}

void CNode::GetPersistProcessAttributes( CPersistConfig *persistConfig
                                       , int             nid
                                       , PROCESSTYPE    &processType
                                       , char           *processName
                                       , char           *programName
                                       , int            &programArgc
                                       , char           *programArgs
                                       , char           *outfile
                                       , char           *persistRetries
                                       , char           *persistZones )
{
    const char method_name[] = "CNode::GetPersistProcessAttributes";
    char zoneStr[MAX_PERSIST_VALUE_STR];

    processType = persistConfig->GetProcessType();

    switch (persistConfig->GetZoneZidFormat())
    {
    case Zid_ALL:
        sprintf( zoneStr, "%d (ALL)", -1 );
        strcat( persistZones, zoneStr );
        break;
    case Zid_RELATIVE:
    default:
        sprintf( zoneStr, "%d", nid );
        strcpy( persistZones, zoneStr );
        break;
    }

    if ( nid == -1 )
    {
        sprintf( processName, "%s"
               , persistConfig->GetProcessNamePrefix() );
        sprintf( outfile, "%s"
               , persistConfig->GetStdoutPrefix() );
    }
    else
    {
        sprintf( processName, "%s%d"
               , persistConfig->GetProcessNamePrefix()
               , nid );
        sprintf( outfile, "%s%d"
               , persistConfig->GetStdoutPrefix()
               , nid );
    }

    sprintf( programName, "%s", persistConfig->GetProgramName() );

    programArgc = persistConfig->GetProgramArgc();
    if (programArgc)
    {
        sprintf( programArgs, "%s"
               , persistConfig->GetProgramArgs() );
    }

    sprintf( persistRetries, "%d,%d"
           , persistConfig->GetPersistRetries()
           , persistConfig->GetPersistWindow() );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_PROCESS | TRACE_PROCESS_DETAIL))
        trace_printf( "%s@%d Persist process Nid=%d, "
                      "processName=%s, type=%s, stdout=%s, "
                      "persistRetries=%s, persistZones=%s\n"
                    , method_name, __LINE__
                    , nid, processName
                    , ProcessTypeString(persistConfig->GetProcessType())
                    , outfile
                    , persistRetries
                    , persistZones );
}

void CNode::StartDtmProcess( void )
{
    const char method_name[] = "CNode::StartDtmProcess";
    TRACE_ENTRY;

    bool debug = false;
    bool nowait = false;
    char infile[MAX_PROCESS_PATH];
    char *ldpath = NULL;
    char path[MAX_SEARCH_PATH];
    char processName[MAX_PROCESS_NAME];
    char programArgs[MAX_VALUE_SIZE_INT];
    char programName[MAX_PROCESS_NAME];
    char outfile[MAX_PROCESS_PATH];
    char persistRetries[MAX_PERSIST_VALUE_STR];
    char persistZones[MAX_VALUE_SIZE_INT];
    char stdout[MAX_PROCESS_PATH];
    int nid = MyNode->AssignNid();
    int programArgc = 0;
    PROCESSTYPE processType = ProcessType_DTM;
    CProcess* dtmProcess;
    CClusterConfig* clusterConfig = Nodes->GetClusterConfig();
    CPersistConfig* persistConfig = NULL;
    
    assert(clusterConfig != NULL);

    persistConfig = clusterConfig->GetPersistConfig( "DTM" );
    if (persistConfig == NULL)
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s], Persistent process configuration for DTM is missing!\n"
                , method_name );
        mon_log_write(MON_NODE_STARTDTMPROCESS_1, SQ_LOG_ERR, buf);
        abort();
    }

    GetPersistProcessAttributes( persistConfig
                               , nid
                               , processType
                               , processName
                               , programName
                               , programArgc
                               , programArgs
                               , outfile
                               , persistRetries
                               , persistZones );

    const char *logpath = getenv("TRAF_LOG");
    snprintf( stdout, sizeof(stdout)
            , "%s/%s"
            , logpath, outfile );

    if (trace_settings & (TRACE_PROCESS | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d - Process %s, logpath=%s, outfile=%s, stdout=%s\n"
                    , method_name, __LINE__
                    , processName, logpath, outfile, stdout);
    }

    strcpy(path,getenv("PATH"));
    strcat(path,":");
    strcat(path,MyPath);
    ldpath = getenv("LD_LIBRARY_PATH");
    strId_t pathStrId = MyNode->GetStringId ( path );
    strId_t ldpathStrId = MyNode->GetStringId ( ldpath );
    strId_t programStrId = MyNode->GetStringId ( programName );

    int result;
    dtmProcess  = CreateProcess( NULL               // parent
                               , nid
                               , ProcessType_DTM
                               , 0                  // debug
                               , 0                  // priority
                               , 0                  // backup
                               , true               // unhooked
                               , processName
                               , pathStrId 
                               , ldpathStrId 
                               , programStrId 
                               , (char *) ""        // infile
                               , stdout             // outfile
                               , 0                  // tag
                               , result );
    if ( dtmProcess )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
           trace_printf( "%s@%d - DTM process created (%s)\n"
                       , method_name, __LINE__, processName );
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf
               , "[%s], DTM process creation failed! (%s)\n"
               , method_name, processName );
        mon_log_write( MON_NODE_STARTDTMPROCESS_2, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

void CNode::StartNameServerProcess( void )
{
    const char method_name[] = "CNode::StartNameServerProcess";
    TRACE_ENTRY;

    if ( !NameServer->IsNameServerConfigured( MyPNID ) )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf( "%s@%d" " - NameServer is not configured in my node\n"
                        , method_name, __LINE__);
        }
        return;
    }

    char path[MAX_SEARCH_PATH];
    char *ldpath = NULL; // = getenv("LD_LIBRARY_PATH");
    char filename[MAX_PROCESS_PATH];
    char name[MAX_PROCESS_NAME];
    char stdout[MAX_PROCESS_PATH];
    
    const char *logpath = getenv("TRAF_LOG");
    snprintf( name, sizeof(name), "$TNS%d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "%s/stdout_TNS%d", logpath, MyNode->GetZone() );

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf("%s@%d" " - Creating NameServer Process\n", method_name, __LINE__);
    }

    strcpy(path,getenv("PATH"));
    strcat(path,":");
    strcat(path,MyPath);
    strcpy(filename,"trafns");
    ldpath = getenv("LD_LIBRARY_PATH");
    strId_t pathStrId = MyNode->GetStringId ( path );
    strId_t ldpathStrId = MyNode->GetStringId ( ldpath );
    strId_t programStrId = MyNode->GetStringId ( filename );

    int result;
    NameServerProcess  = CreateProcess( NULL, //parent
                                        MyNode->AssignNid(),
                                        ProcessType_NameServer,
                                        0,  //debug
                                        0,  //priority
                                        0,  //backup
                                        true, //unhooked
                                        name,
                                        pathStrId,
                                        ldpathStrId,
                                        programStrId,
                                        (char *) "", //infile,
                                        stdout, //outfile,
                                        0, //tag
                                        result
                                        );
    if ( NameServerProcess )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            trace_printf("%s@%d" " - NameServer Process created\n", method_name, __LINE__);
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], NameServer Process creation failed.\n", method_name);
        mon_log_write( MON_NODE_STARTNAMESERVER_1, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
}

void CNode::StartWatchdogProcess( void )
{
    const char method_name[] = "CNode::StartWatchdogProcess";
    TRACE_ENTRY;

    char path[MAX_SEARCH_PATH];
    char *ldpath = NULL; // = getenv("LD_LIBRARY_PATH");
    char filename[MAX_PROCESS_PATH];
    char name[MAX_PROCESS_NAME];
    char stdout[MAX_PROCESS_PATH];
    CProcess * watchdogProcess;
    
    const char *logpath = getenv("TRAF_LOG");
    snprintf( name, sizeof(name), "$WDG%d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "%s/stdout_WDG%d", logpath, MyNode->GetZone() );

    // The following variables are used to retrieve the proper startup and keepalive environment variable
    // values, and to use as arguments for the lower level ioctl calls that interface with the watchdog 
    // timer package.

    char *WDT_KeepAliveTimerValueC;

    // If the SQ_WDT_KEEPALIVETIMERVALUE are not defined in this case,
    // we will use the default values defined above.

    if (!(WDT_KeepAliveTimerValueC = getenv("SQ_WDT_KEEPALIVETIMERVALUE")))
    {
        wdtKeepAliveTimerValue_ = WDT_KEEPALIVETIMERDEFAULT;
    }
    else
    {
        wdtKeepAliveTimerValue_ = atoi(WDT_KeepAliveTimerValueC);	
    }

    //Displays the startup and keep alive timer values in use for a given run.
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d" " - KeepAlive Timer in seconds =%d\n", method_name, __LINE__, (wdtKeepAliveTimerValue_));

    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d" " - Creating Watchdog Process\n", method_name, __LINE__);

    strcpy(path,getenv("PATH"));
    strcat(path,":");
    strcat(path,MyPath);
    strcpy(filename,"sqwatchdog");
    ldpath = getenv("LD_LIBRARY_PATH");
    strId_t pathStrId = MyNode->GetStringId ( path );
    strId_t ldpathStrId = MyNode->GetStringId ( ldpath );
    strId_t programStrId = MyNode->GetStringId ( filename );

    int result;
    watchdogProcess  = CreateProcess( NULL, //parent
                                      MyNode->AssignNid(),
                                      ProcessType_Watchdog,
                                      0,  //debug
                                      0,  //priority
                                      0,  //backup
                                      true, //unhooked
                                      name,
                                      pathStrId,
                                      ldpathStrId,
                                      programStrId,
                                      (char *) "", //infile,
                                      stdout, //outfile,
                                      0, //tag
                                      result
                                      );
    if ( watchdogProcess )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
           trace_printf("%s@%d" " - Watchdog Process created\n", method_name, __LINE__);

        gettimeofday(&todStart_, NULL);
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[CNode::StartWatchdogProcess], Watchdog Process creation failed. Timer disabled\n");
        mon_log_write( MON_NODE_STARTWATCHDOG_1, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
}

void CNode::StartPStartDProcess( void )
{
    const char method_name[] = "CNode::StartPStartDProcess";
    TRACE_ENTRY;

    char path[MAX_SEARCH_PATH];
    char *ldpath = NULL;
    char filename[MAX_PROCESS_PATH];
    char name[MAX_PROCESS_NAME];
    char stdout[MAX_PROCESS_PATH];
    CProcess * pstartdProcess;
    
    const char *logpath = getenv("TRAF_LOG");
    snprintf( name, sizeof(name), "$PSD%d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "%s/stdout_PSD%d", logpath, MyNode->GetZone() );

    strcpy(path,getenv("PATH"));
    strcat(path,":");
    strcat(path,MyPath);
    strcpy(filename,"pstartd");
    ldpath = getenv("LD_LIBRARY_PATH");
    strId_t pathStrId = MyNode->GetStringId ( path );
    strId_t ldpathStrId = MyNode->GetStringId ( ldpath );
    strId_t programStrId = MyNode->GetStringId ( filename );

    int result;
    pstartdProcess  = CreateProcess( NULL, //parent
                                      MyNode->AssignNid(),
                                      ProcessType_PSD,
                                      0,  //debug
                                      0,  //priority
                                      0,  //backup
                                      true, //unhooked
                                      name,
                                      pathStrId,
                                      ldpathStrId,
                                      programStrId,
                                      (char *) "", //infile,
                                      stdout, //outfile,
                                      0, //tag
                                      result
                                      );
    if ( pstartdProcess )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
           trace_printf("%s@%d - pstartd process created\n",
                        method_name, __LINE__);
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[CNode::StartPStartDProcess], pstartd creation failed.\n");
        mon_log_write( MON_NODE_STARTPSTARTD_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
}

void CNode::StartPStartDPersistent( void )
{
    const char method_name[] = "CNode::StartPStartDPersistent";
    TRACE_ENTRY;

    if ( trace_settings & (TRACE_RECOVERY | TRACE_REQUEST | TRACE_INIT) )
    {
        trace_printf( "%s@%d - Sending start persistent processes event %d (%s)\n"
                    , method_name, __LINE__
                    , PStartD_StartPersist, name_ );
    }

    int tmCount = 0;
    CNode *node;
    CLNode *lnode;
    CProcess *process;

    // Any DTMs running in cluster?
    for ( int i=0; !tmCount && i < Nodes->GetPNodesCount(); i++ )
    {
        node = Nodes->GetNodeByMap( i );
        lnode = node->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNextP() )
        {
            CProcess *process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process ) tmCount++;
        }
    }

    if ( tmCount )
    {
        lnode = GetFirstLNode();
        for ( ; lnode ; lnode = lnode->GetNextP() )
        {
            if ( lnode->GetState() == State_Up )
            {
                // Send local PSD process event to start persistent processes 
                // that don't require transactions
                process = lnode->GetProcessLByType( ProcessType_PSD );
                if ( process )
                {
                    char nidString[6];
                    sprintf(nidString,"%d",lnode->GetNid());
                    process->GenerateEvent( PStartD_StartPersist, sizeof(nidString), nidString );
            
                    if ( trace_settings & 
                        (TRACE_RECOVERY | TRACE_REQUEST | TRACE_INIT) )
                    {
                        trace_printf( "%s@%d - Sent event %d (%s) to PSD %s (pid=%d)\n"
                                    , method_name, __LINE__
                                    , PStartD_StartPersist, nidString
                                    , process->GetName(), process->GetPid());
                    }
                }
            }
        }
    }
    
    TRACE_EXIT;
}

void CNode::StartPStartDPersistentDTM( int nid )
{
    const char method_name[] = "CNode::StartPStartDPersistentDTM";
    TRACE_ENTRY;

    assert( GetState() == State_Up );

    int tmCount = 0;
    CNode *node;
    CLNode *lnode;
    CProcess *process;

    // Any DTMs running in cluster?
    for ( int i=0; !tmCount && i < Nodes->GetPNodesCount(); i++ )
    {
        node = Nodes->GetNodeByMap( i );
        lnode = node->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNextP() )
        {
            process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process ) tmCount++;
        }
    }

    if ( tmCount )
    {
        lnode = GetFirstLNode();
        for ( ; lnode ; lnode = lnode->GetNextP() )
        {
            if ( lnode->GetNid() == nid )
            {
                // Send local PSD process event to start persistent processes 
                // that do require transactions
                process = lnode->GetProcessLByType( ProcessType_PSD );
                if ( process )
                {
                    char nidString[6];
                    sprintf( nidString, "%d", nid );
                    process->GenerateEvent( PStartD_StartPersistDTM, sizeof(nidString), nidString );
            
                    if ( trace_settings & 
                        (TRACE_RECOVERY | TRACE_REQUEST | TRACE_INIT) )
                    {
                        trace_printf( "%s@%d - Sending event %d (%s) to PSD %s (pid=%d)\n"
                                    , method_name, __LINE__
                                    , PStartD_StartPersistDTM, nidString
                                    , process->GetName(), process->GetPid());
                    }
                }
            }
        }
    }
    
    TRACE_EXIT;
}

void CNode::StartSMServiceProcess( void )
{
    const char method_name[] = "CNode::StartSMServiceProcess";
    TRACE_ENTRY;

    char path[MAX_SEARCH_PATH];
    char *ldpath = NULL;
    char filename[MAX_PROCESS_PATH];
    char name[MAX_PROCESS_NAME];
    char stdout[MAX_PROCESS_PATH];
    CProcess *smsProcess;
    
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d" " - Creating SMService Process\n", method_name, __LINE__);

    const char *logpath = getenv("TRAF_LOG");
    snprintf( name, sizeof(name), "$SMS%03d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "%s/stdout_SMS%03d", logpath, MyNode->GetZone() );

    strcpy(path,getenv("PATH"));
    strcat(path,":");
    strcat(path,MyPath);
    strcpy(filename,"smservice");
    ldpath = getenv("LD_LIBRARY_PATH");
    strId_t pathStrId = MyNode->GetStringId ( path );
    strId_t ldpathStrId = MyNode->GetStringId ( ldpath );
    strId_t programStrId = MyNode->GetStringId ( filename );

    int result;
    smsProcess  = CreateProcess( NULL, //parent
                                 MyNode->AssignNid(),
                                 ProcessType_SMS,
                                 0,  //debug
                                 0,  //priority
                                 0,  //backup
                                 true, //unhooked
                                 name,
                                 pathStrId,
                                 ldpathStrId,
                                 programStrId,
                                 (char *) "", //infile,
                                 stdout, //outfile,
                                 0, //tag
                                 result
                                 );
    if ( smsProcess )
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
           trace_printf("%s@%d - smservice process (%s) created\n",
                        method_name, __LINE__, name);
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], smservice process (%s)creation failed.\n", 
                method_name, name);
        mon_log_write( MON_NODE_STARTSMSERVICE_1, SQ_LOG_CRIT, buf );
    }

    TRACE_EXIT;
}
#endif

CNodeContainer::CNodeContainer( void )
               :CLNodeContainer(NULL)
               ,Node(NULL)
               ,pnodeCount_(0)
               ,indexToPnid_(NULL)
               ,clusterConfig_(NULL)
               ,nameServerConfig_(NULL)
               ,head_(NULL)
               ,tail_(NULL)
               ,syncBufferFreeSpace_(MAX_SYNC_SIZE)
               ,lastSyncBuffer_(NULL)
               ,SyncBuffer(NULL)
{
    const char method_name[] = "CNodeContainer::CNodeContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "NCTR", 4);

    lastSyncBuffer_ = new struct sync_buffer_def;
    SyncBuffer = new struct sync_buffer_def;

    // Load cluster configuration
    LoadConfig();

    // Allocate logical and physical node arrays
    Node = new CNode *[clusterConfig_->GetPNodesConfigMax()];
    LNode = new CLNode *[clusterConfig_->GetLNodesConfigMax()];
    indexToPnid_ = new int[clusterConfig_->GetPNodesConfigMax()];
    for (int i = 0; i < clusterConfig_->GetPNodesConfigMax(); i++ )
    {
        Node[i] = NULL;
        indexToPnid_[i] = -1;
    }
    indexToNid_ = new int[clusterConfig_->GetLNodesConfigMax()];
    for (int i = 0; i < clusterConfig_->GetLNodesConfigMax(); i++ )
    {
        LNode[i] = NULL;
        indexToNid_[i] = -1;
    }

    TRACE_EXIT;
}

CNodeContainer::~CNodeContainer( void )
{
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::~CNodeContainer";
    TRACE_ENTRY;
    if (lastSyncBuffer_)
    {
        delete lastSyncBuffer_;
    }
    if (SyncBuffer)
    {
        delete SyncBuffer;
    }
    while (head_)
    {
        node->DeLink (&head_, &tail_);
        delete node;
        node = head_;
    }

    if (clusterConfig_)
    {
        delete clusterConfig_;
        clusterConfig_ = NULL;
    }
    if (nameServerConfig_)
    {
        delete nameServerConfig_;
    }
    if (Node)
    {
        delete [] Node;
    }
    if (LNode)
    {
        delete [] LNode;
    }

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "nctr", 4);
    
    TRACE_EXIT;
}


int CNodeContainer::GetPNodesUpCount( int &readyCount )
{
    const char method_name[] = "CNodeContainer::GetPNodesUpCount";
    TRACE_ENTRY;

    int upCount = 0;
    readyCount = 0;

    CNode *node = head_;
    while (node)
    {
        if ( node->GetState() == State_Up )
        { 
            upCount++;
            if (node->GetPhase() == Phase_Ready)
            {
                readyCount++;
            }
        }
        node = node->GetNext();
    }

    TRACE_EXIT;
    return( upCount );
}

int CNodeContainer::GetPNid( char *nodeName )
{
    const char method_name[] = "CNodeContainer::GetPNid";
    TRACE_ENTRY;

    int pnid = clusterConfig_->GetPNid( nodeName );

    TRACE_EXIT;
    return( pnid );
}

void CNodeContainer::AddedNode( CNode *node )
{
    const char method_name[] = "CNodeContainer::AddedNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_SYNC))
    {
        trace_printf( "%s@%d - node %s, pnid=%d, zone=%d\n"
                    , method_name, __LINE__
                    , node->GetName(), node->GetPNid(), node->GetZone() );
    }

    assert( node->GetState() == State_Down );

    // Broadcast node added notice to local processes
    CLNode *lnode = node->GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        lnode->Added();
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
CProcess *CNodeContainer::AddCloneProcess( ProcessInfoNs_reply_def *processInfo )
{
    const char method_name[] = "CNodeContainer::AddNode";
    TRACE_ENTRY;

    CLNode   *lnode = Nodes->GetLNode(processInfo->nid);
    CNode    *node = lnode->GetNode();

    strId_t pathStrId = MyNode->GetStringId ( processInfo->path, lnode, true );
    strId_t ldpathStrId = MyNode->GetStringId (processInfo->ldpath, lnode, true );
    strId_t programStrId = MyNode->GetStringId ( processInfo->program, lnode, true );

    CProcess *process = node->CloneProcess( processInfo->nid
                                          , processInfo->type
                                          , processInfo->priority
                                          , processInfo->backup
                                          , processInfo->unhooked
                                          , processInfo->process_name
                                          , processInfo->port_name
                                          , processInfo->pid
                                          , processInfo->verifier
                                          , processInfo->parent_nid
                                          , processInfo->parent_pid
                                          , processInfo->parent_verifier
                                          , processInfo->event_messages
                                          , processInfo->system_messages
                                          , pathStrId
                                          , ldpathStrId
                                          , programStrId
//                                          , processInfo->pathStrId
//                                          , processInfo->ldpathStrId
//                                          , processInfo->programStrId
                                          , processInfo->infile
                                          , processInfo->outfile
                                          , &processInfo->creation_time
                                          , -1 );//processInfo->origPNidNs_);

    TRACE_EXIT;
    return(process);
}

void CNodeContainer::AddConfiguredZNodes( void )
{
    const char method_name[] = "CNodeContainer::AddConfiguredZNodes";
    TRACE_ENTRY;

    if (ZClientEnabled)
    {
        if (!IsAgentMode || (IsAgentMode && IsMaster))
        {
            CPNodeConfig *pnodeConfig = clusterConfig_->GetFirstPNodeConfig();
            for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
            {
                ZClient->ConfiguredZNodeCreate( pnodeConfig->GetName() );
            }
        }
    }

    TRACE_EXIT;
}
#endif

CNode *CNodeContainer::AddNode( int pnid )
{
    const char method_name[] = "CNodeContainer::AddNode";
    TRACE_ENTRY;

    CNode        *node = NULL;
    CPNodeConfig *pnodeConfig = clusterConfig_->GetPNodeConfig( pnid );
    if (pnodeConfig)
    {
        node = GetNode(pnid);
        if (!node)
        {
            node = new CNode( (char *)pnodeConfig->GetName()
                            , (char *)pnodeConfig->GetDomain()
                            , (char *)pnodeConfig->GetFqdn()
                            , pnodeConfig->GetPNid()
                            , -1 );
            assert( node != NULL );
    
            if ( node )
            {
                // Add node to monitor's view of the cluster
                AddNode( node );

                // Broadcast node added notice to local processes
                AddedNode( node );

                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Node %s added to configuration, pnid=%d\n"
                        , method_name, __LINE__
                        , node->GetName(), node->GetPNid() );
                mon_log_write(MON_NODE_ADDNODE_4, SQ_LOG_INFO, buf);
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s@%d] Could not allocate physical node object "
                          ", pnid=%d\n"
                        , method_name, __LINE__, pnid);
                mon_log_write(MON_NODE_ADDNODE_1, SQ_LOG_ERR, buf);
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Found existing physical node "
                      "in monitor's view of cluster, pnid=%d\n"
                    , method_name, __LINE__, pnid);
            mon_log_write(MON_NODE_ADDNODE_2, SQ_LOG_ERR, buf);
        }       
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] Could not find physical node in configuration, "
                  "pnid=%d\n"
                , method_name, __LINE__, pnid);
        mon_log_write(MON_NODE_ADDNODE_3, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return(node);
}

void CNodeContainer::AddNode( CNode *node )
{
    const char method_name[] = "CNodeContainer::AddNode";
    TRACE_ENTRY;

    assert( node != NULL );

    if ( node )
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d - Adding physical node object "
                          "(pnid=%d, nodename=%s) to container, "
                          "pnodesCount=%d, lnodesCount=%d\n"
                        , method_name, __LINE__
                        , node->GetPNid()
                        , node->GetName()
                        , GetPNodesCount()
                        , GetLNodesCount() );
        }

        // add physical node to physical nodes array
        Node[node->GetPNid()] = node;
        pnodeCount_++;

        if (head_ == NULL)
        {
            head_ = tail_ = node;
        }
        else
        {
            // add to list in pnid sort order
            if (node->GetPNid() < head_->GetPNid())
            { // link new node to the begining
                head_->LinkBefore( head_, node );
            }
            else if (node->GetPNid() > tail_->GetPNid())
            { // link new node to the end
                tail_->LinkAfter( tail_, node );
            }
            else
            {
                CNode *entry = head_;
                CNode *prevEntry = NULL;
                while (entry)
                { // walk the list
                    if (node->GetPNid() > entry->GetPNid())
                    { // new node is greater than current list entry
                        prevEntry = entry;
                        entry = prevEntry->GetNext();
                    }
                    else
                    { // new node is less than current list entry
                        prevEntry->LinkAfter( tail_, node );
                        entry = NULL;
                    }
                }
            }
        }
#ifdef NAMESERVER_PROCESS
        AddLNodes( node );
#else
        // now add logical nodes to physical node
        if (IAmIntegrating)
        {
            // do nothing.
            // lnodes will be created in the revive phase
        }
        else
        {
            AddLNodes( node );
        }
#endif

        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d - Added physical node object "
                          "(pnid=%d, nodename=%s) to container, "
                          "pnodesCount=%d, lnodesCount=%d\n"
                        , method_name, __LINE__
                        , node->GetPNid()
                        , node->GetName()
                        , GetPNodesCount()
                        , GetLNodesCount() );
        }
    }

    TRACE_EXIT;
}

void CNodeContainer::AddNodes( )
{
    const char method_name[] = "CNodeContainer::AddNodes";
    TRACE_ENTRY;

    CNode *node;
    int pnid;
    int rank;
    int *sparePNids = NULL;
    
    //  only relevant on a workstation acting as a single node
    const char* envVar = getenv("SQ_MAX_RANK"); 
    int maxNode;
    if (envVar != NULL)
        maxNode = atoi (envVar);
    else
        maxNode = MAX_NODES; 

    CPNodeConfig *pnodeConfig = clusterConfig_->GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        pnid = pnodeConfig->GetPNid();
        rank = 0;

        if ( pnodeConfig->IsSpareNode() )
        {
            sparePNids = new int [pnodeConfig->GetSparesCount()+1]; 
            assert( sparePNids != NULL );

            pnodeConfig->GetSpareList( sparePNids );
            // add the spare node's pnid to the spare set
            sparePNids[pnodeConfig->GetSparesCount()] = pnid;
            node = new CNode( (char *)pnodeConfig->GetName()
                            , (char *)pnodeConfig->GetDomain()
                            , (char *)pnodeConfig->GetFqdn()
                            , pnid
                            , rank 
                            , pnodeConfig->GetSparesCount()+1
                            , sparePNids
                            , pnodeConfig->GetExcludedCoreMask() );
            assert( node != NULL );
            for ( int i = 0; i < (pnodeConfig->GetSparesCount()+1) ; i++ )
            {
                if ( Node[sparePNids[i]] && sparePNids[i] != pnid)
                {
                    // Replicate the spare set in each memmber of the set
                    Node[sparePNids[i]]->SetSparePNids( node->GetSparePNids() );
                }
            }
        }
        else
        {
            if (pnid >= maxNode) // only for workstation acting as single node
            {
                rank = -1; // -1 creates node in down state
            }
            node = new CNode( (char *)pnodeConfig->GetName()
                            , (char *)pnodeConfig->GetDomain()
                            , (char *)pnodeConfig->GetFqdn()
                            , pnid
                            , rank );
            assert( node != NULL );
        }
        
        if ( node )
        {
            AddNode( node );
        }

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
        {
            if ( pnodeConfig->IsSpareNode() )
            {
                for ( int i = 0; i < (pnodeConfig->GetSparesCount()+1) ; i++ )
                {
                    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                        trace_printf( "%s@%d - spare set member %s (pnid=%d), spare set size=%d, contains:\n"
                                    , method_name, __LINE__
                                    , Node[sparePNids[i]]->GetName()
                                    , Node[sparePNids[i]]->GetPNid()
                                    , (pnodeConfig->GetSparesCount()+1));
                    PNidVector sparePNidsVector = Node[sparePNids[i]]->GetSparePNids();
                    for ( unsigned int jj = 0; jj < sparePNidsVector.size(); jj++ )
                    {
                        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                            trace_printf( "%s@%d - pnid=%d in spare pnids vector, size=%ld\n"
                                        , method_name, __LINE__
                                        , sparePNids[jj]
                                        , sparePNidsVector.size());
                    }
                }
            }
        }

        if ( sparePNids != NULL )
        {
            delete [] sparePNids;
            sparePNids = NULL;
        }
    }

    TRACE_EXIT;
}

void CNodeContainer::AddLNodes( )
{
    const char method_name[] = "CNodeContainer::AddLNodes";
    TRACE_ENTRY;

    CNode *node;
    int pnid;

    CPNodeConfig *pnodeConfig = clusterConfig_->GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        pnid = pnodeConfig->GetPNid();

        node = Node[pnid];
        assert( node != NULL );

        // now add logical nodes to physical node
        if (!IAmIntegrating)
        {
             AddLNodes( node );
        }
    }

    TRACE_EXIT;
}

void CNodeContainer::AddLNodes( CNode  *node )
{
    const char method_name[] = "CNodeContainer::AddLNodes";
    TRACE_ENTRY;

    CLNode         *lnode;
    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;

    pnodeConfig = clusterConfig_->GetPNodeConfig( node->GetPNid() );
    
    if (pnodeConfig )
    {
        // Create logical nodes configured for the physical node passed in
        lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
        if (lnodeConfig)
        {
            for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNextP() )
            {
                // add logical node to 
                //   logical nodes array and logical nodes container
                lnode = AddLNode( lnodeConfig, node );
                LNode[lnodeConfig->GetNid()] = lnode;
                // add logical node to physical node's logical node container 
                node->AddLNodeP( lnode );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Could not find logical node in "
                      "configuration of physical node, pnid=%d\n"
                    , method_name, __LINE__, node->GetPNid() );
            mon_log_write(MON_NODE_ADDLNODES_1, SQ_LOG_ERR, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] Could not find physical node in configuration, "
                  "pnid=%d\n"
                , method_name, __LINE__, node->GetPNid());
        mon_log_write(MON_NODE_ADDLNODES_2, SQ_LOG_ERR, buf);
    }


    TRACE_EXIT;
}

// add configured lnodes of physical node node2 to node1. 
void CNodeContainer::AddLNodes( CNode  *node1, CNode *node2 )
{
    const char method_name[] = "CNodeContainer::AddLNodes(node1, node2)";
    TRACE_ENTRY;

    CLNode         *lnode;
    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;

    pnodeConfig = clusterConfig_->GetPNodeConfig( node2->GetPNid() );
    if (pnodeConfig )
    {
        // Create logical nodes configured for the physical node passed in
        lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
        if (lnodeConfig)
        {
            for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNextP() )
            {
                // add logical node to 
                //   logical nodes array and logical nodes container
                lnode = AddLNode( lnodeConfig, node1 );
                LNode[lnodeConfig->GetNid()] = lnode;
                // add logical node to physical node's logical node container 
                node1->AddLNodeP( lnode );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Could not find logical node in "
                      "configuration of physical node, pnid=%d\n"
                    , method_name, __LINE__, node2->GetPNid() );
            mon_log_write(MON_NODE_ADDLNODES_3, SQ_LOG_ERR, buf);

            mon_failure_exit();
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] Could not find physical node in configuration, "
                  "pnid=%d\n"
                , method_name, __LINE__, node2->GetPNid());
        mon_log_write(MON_NODE_ADDLNODES_4, SQ_LOG_ERR, buf);

        mon_failure_exit();
    }

    TRACE_EXIT;
}

void CNodeContainer::AddToSpareNodesList( int pnid )
{
    const char method_name[] = "CNodeContainer::AddToSpareNodesList";
    TRACE_ENTRY;

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d - adding pnid=%d to spare node list\n", method_name, __LINE__, pnid);

    if ( SpareNodeColdStandby )
    {
        spareNodesList_.push_back( Node[pnid] );
    }
    else
    {
        if ( Node[pnid]->GetState() == State_Up && Node[pnid]->IsSpareNode() )
        {
            spareNodesList_.push_back( Node[pnid] );
        }
    }

    if (trace_settings & TRACE_INIT)
    {
        CNode *spareNode;
        NodesList::iterator itSn;
        for ( itSn = spareNodesList_.begin(); itSn != spareNodesList_.end() ; itSn++ ) 
        {
            spareNode = *itSn;
            trace_printf("%s@%d - pnid=%d is in spare node list\n", method_name, __LINE__, spareNode->GetPNid());
        }
    }

    TRACE_EXIT;
}

int CNodeContainer::PackSpareNodesList( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::PackSpareNodesList";
    TRACE_ENTRY;

    NodesList::iterator itSn;
    for ( itSn = spareNodesList_.begin(); itSn != spareNodesList_.end() ; itSn++ )
    {
        *buffer = (*itSn)->GetPNid();

        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d - packing spare node pnid=%d \n", method_name, __LINE__, *buffer);

        ++buffer;
    }

    TRACE_EXIT;
    return spareNodesList_.size();
}

void CNodeContainer::UnpackSpareNodesList( intBuffPtr_t &buffer, int spareNodesCount )
{
    const char method_name[] = "CNodeContainer::UnpackSpareNodesList";
    TRACE_ENTRY;

    // make sure the list is empty.
    assert(spareNodesList_.size() == 0);

    // reset spareNode_ flag in all nodes.
    for ( int pnid = 0; pnid < GetPNodesCount(); pnid++ )
    {
        Node[indexToPnid_[pnid]]->ResetSpareNode();
    }

    for ( int i=0; i < spareNodesCount; i++ )
    {
        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d - unpacking spare node pnid=%d \n", method_name, __LINE__, *buffer);

        spareNodesList_.push_back( Node[indexToPnid_[*buffer]] );
        Node[indexToPnid_[*buffer]]->SetSpareNode();
        ++buffer;
    }

    TRACE_EXIT;
    return;
}

int CNodeContainer::PackNodeMappings( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::PackNodeMappings";
    TRACE_ENTRY;

    int pnid, pnidConfig;
    CLNode *lnode = NULL;
    CLNodeConfig *lnodeConfig = NULL;
    int count = 0;

    // go thru all configured physical nodes; and for their logical nodes,
    // find the physical nodes on which they are running.
    CPNodeConfig *pnodeConfig = clusterConfig_->GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        pnidConfig = pnodeConfig->GetPNid();
        lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
        if ( lnodeConfig )
        {
            lnode = Nodes->GetLNode( lnodeConfig->GetNid() );
            pnid =  lnode->GetNode()->GetPNid();

            *buffer++ = pnidConfig;
            *buffer++ = pnid;

            ++count;

            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
                trace_printf("%s@%d - Packing node mapping, pnidConfig=%d, pnid=%d \n",
                            method_name, __LINE__, pnidConfig, pnid);
        }
    }

    TRACE_EXIT;

    return count;
}

void CNodeContainer::UnpackNodeMappings( intBuffPtr_t &buffer, int nodeMapCount )
{
    const char method_name[] = "CNodeContainer::UnpackNodeMappings";
    TRACE_ENTRY;

    int pnid, pnidConfig;

    // lock sync thread since we are making a change the monitor's
    // operational view of the cluster
    if ( !Emulate_Down )
    {
        Monitor->EnterSyncCycle();
    }

    for (int count = 0; count < nodeMapCount; count++)
    {
        pnidConfig = *buffer++;
        pnid = *buffer++;

        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf("%s@%d - Unpacking node mapping, pnidConfig=%d, pnid=%d \n",
                        method_name, __LINE__, pnidConfig, pnid);

#ifndef NAMESERVER_PROCESS
        Nodes->AddLNodes( Nodes->GetNode(pnid), Nodes->GetNode(pnidConfig) );
#endif
    }

    UpdateCluster();

    // unlock sync thread
    if ( !Emulate_Down )
    {
        Monitor->ExitSyncCycle();
    }

    TRACE_EXIT;
    return;
}

void CNodeContainer::PackZids( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::PackZids";
    TRACE_ENTRY;

    // Pack in pairs of pnid:zid to account for missing pnid(s)
    for ( int index = 0; index < GetPNodesCount(); index++ )
    {
        *buffer = Node[indexToPnid_[index]]->GetPNid();
        ++buffer;
        *buffer = Node[indexToPnid_[index]]->GetZone();
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Packing zid=%d for indexToPnid_[%d]=%d\n"
                         , method_name, __LINE__
                         , *buffer
                         , index
                         , indexToPnid_[index]);
        ++buffer;
    }

    TRACE_EXIT;
    return;
}

void CNodeContainer::UnpackZids( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::UnpackZids";
    TRACE_ENTRY;

    // Unpack in pairs of pnid:zid to account for missing pnid(s)
    for ( int index = 0; index < GetPNodesCount(); index++ )
    {
        int pnid = *buffer;
        ++buffer;
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Unpacking zid=%d pnid=%d\n"
                         , method_name, __LINE__
                         , *buffer
                         , pnid);
        Node[pnid]->SetZone(*buffer);
        ++buffer;
    }

    TRACE_EXIT;
    return;
}

CLNode *CNodeContainer::AssignLNode( CProcess *requester, PROCESSTYPE type, int nid, int not_nid)
{
    int not_zone;
    CLNode *lnode = NULL;
    char la_buf[MON_STRING_BUF_SIZE];

    const char method_name[] = "CNodeContainer::AssignLNode";
    TRACE_ENTRY;

    not_zone = (not_nid == -1?-1:LNode[not_nid]->GetZone());

    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS))
       trace_printf("%s@%d" " - nid=%d, not_nid=%d, not_zone=%d\n", method_name, __LINE__, nid, not_nid, not_zone);

    switch (type)
    {
    case ProcessType_Backout:
    case ProcessType_VolumeRecovery:
        if (nid == -1)
        {
            lnode = SelectLNode(requester, ZoneType_Storage, nid, not_zone, true);
        }
        else
        {
            if ((LNode[nid]->GetState() == State_Up || 
                 LNode[nid]->GetState() == State_Shutdown) &&
                !LNode[nid]->IsKillingNode() &&
                (LNode[nid]->GetZoneType() == ZoneType_Any ||
                 LNode[nid]->GetZoneType() == ZoneType_Backend ||
                 LNode[nid]->GetZoneType() == ZoneType_Storage) )
            {
                lnode = LNode[nid]; // use the configured nid passed in
            }
        }
        break;
    case ProcessType_TSE:
    case ProcessType_ASE:
        if (nid == -1)
        {
            lnode = SelectLNode(requester, ZoneType_Storage, nid, not_zone, true);
        }
        else
        {
            if ((LNode[nid]->GetState() == State_Up || 
                 LNode[nid]->GetState() == State_Shutdown) &&
                !LNode[nid]->IsKillingNode() &&
                (LNode[nid]->GetZoneType() == ZoneType_Any ||
                 LNode[nid]->GetZoneType() == ZoneType_Backend ||
                 LNode[nid]->GetZoneType() == ZoneType_Storage) )
            {
                lnode = LNode[nid]; // use the configured nid passed in
            }
        }
        break;
    case ProcessType_DTM:
    case ProcessType_SPX:
    case ProcessType_SSMP:
    case ProcessType_Watchdog:
    case ProcessType_PSD:
        if (nid == -1)
        {
            sprintf(la_buf, "[CNodeContainer::AssignLNode], Can't dynamically assign Nid for type (DTM, SPX, or Watchdog) assuming nid=0.\n");
            mon_log_write(MON_NODECONT_ASSIGN_LNODE, SQ_LOG_ERR, la_buf);

            nid = 0;
        }
        if ((LNode[nid]->GetState() == State_Up || 
             LNode[nid]->GetState() == State_Shutdown) &&
            !LNode[nid]->IsKillingNode() )
        {
            lnode = LNode[nid]; // use the configured nid passed in
        }
        break;
    case ProcessType_Generic:
        if (nid == -1)
        {
            lnode = SelectLNode(requester, ZoneType_Aggregation, nid, not_zone, true);
        }
        else
        {
            if ((LNode[nid]->GetState() == State_Up || 
                 LNode[nid]->GetState() == State_Shutdown) &&
                !LNode[nid]->IsKillingNode() )
            {
                lnode = LNode[nid]; // use the configured nid passed in
            }
        }
        break;
    case ProcessType_MXOSRVR:
        if (nid == -1)
        {
            lnode = SelectLNode(requester, ZoneType_Edge, nid, not_zone, false);
        }
        else
        {
            if ((LNode[nid]->GetState() == State_Up || 
                 LNode[nid]->GetState() == State_Shutdown)  &&
                !LNode[nid]->IsKillingNode() &&
                (LNode[nid]->GetZoneType() == ZoneType_Any ||
                 LNode[nid]->GetZoneType() == ZoneType_Frontend ||
                 LNode[nid]->GetZoneType() == ZoneType_Edge) )
            {
                lnode = LNode[nid]; // use the configured nid passed in
            }
        }
        break;
    default:
        if (trace_settings & TRACE_REQUEST)
           trace_printf("%s@%d" " - ProcessType=" "%d" "\n", method_name, __LINE__, type);
        lnode = SelectLNode(requester, ZoneType_Any, nid, not_zone, true);
    }

    TRACE_EXIT;
    return lnode;
}

void CNodeContainer::AvgNodeData(ZoneType type, int *avg_pcount, unsigned int *avg_memory)
{
    int     lnodes = 0;
    CNode  *node;
    
    const char method_name[] = "CNodeContainer::AvgNodeData";
    TRACE_ENTRY;

    CLNode *lnode = Nodes->GetFirstLNode();
    for ( ; lnode ; lnode = lnode->GetNext() )
    {
        node = lnode->GetNode();
        // average only over node of requested type and available logical nodes
        if ( (lnode->GetZoneType() & type) &&
             (lnode->GetState() == State_Up &&
             !lnode->IsKillingNode()) )
        {
            *avg_pcount += lnode->GetNumProcs();
            *avg_memory += (node->GetFreeCache() + node->GetFreeSwap());
            lnodes++;
        }
    }

    if ( lnodes )
    {
        *avg_pcount /= lnodes;
        *avg_memory /= lnodes;
    }
    else
    {
        *avg_pcount = 0;
        *avg_memory = 0;
    }

    TRACE_EXIT;
}

void CNodeContainer::ChangedNode( CNode *node )
{
    const char method_name[] = "CNodeContainer::ChangedNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_SYNC))
    {
        trace_printf( "%s@%d - node_name=%s, pnid=%d, zone=%d\n"
                    , method_name, __LINE__
                    , node->GetName()
                    , node->GetPNid()
                    , node->GetZone() );
    }

    assert( node->GetState() == State_Down );

    CClusterConfig *clusterConfig = Nodes->GetClusterConfig();
    CLNodeConfig   *lnodeConfig = NULL;

    // Broadcast node changed notice to local processes
    CLNode *lnode = node->GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        lnodeConfig = clusterConfig->GetLNodeConfig( lnode->GetNid() );
        lnode->Changed( lnodeConfig );
    }

    TRACE_EXIT;
}

#ifndef NAMESERVER_PROCESS
void CNodeContainer::CancelDeathNotification( int nid
                                            , int pid
                                            , int verifier
                                            , _TM_Txid_External trans_id)
{
    CNode *node;
    
    const char method_name[] = "CNodeContainer::CancelDeathNotification";
    TRACE_ENTRY;

    for ( node=head_; node; node=node->GetNext() )
    {
        node->CLNodeContainer::CancelDeathNotification( nid
                                                      , pid
                                                      , verifier
                                                      , trans_id);
    }

    TRACE_EXIT;
}
#endif
   
#ifndef NAMESERVER_PROCESS
CProcess *CNodeContainer::CloneProcessNs( int nid
                                        , int pid
                                        , Verifier_t verifier )
{
    const char method_name[] = "CNodeContainer::CloneProcessNs";
    TRACE_ENTRY;

    CProcess *process = NULL;

    struct message_def msg;
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = REPLY_TAG;
    msg.u.request.type = ReqType_ProcessInfoNs;

    struct ProcessInfo_def *processInfo = &msg.u.request.u.process_info;
    processInfo->nid = -1;
    processInfo->pid = -1;
    processInfo->verifier = -1;
    processInfo->process_name[0] = 0;
    processInfo->target_nid = nid;
    processInfo->target_pid = pid;
    processInfo->target_verifier = verifier;
    processInfo->target_process_name[0] = 0;
    processInfo->target_process_pattern[0] = 0;
    processInfo->type = ProcessType_Undefined;
    
    int error = NameServer->ProcessInfoNs(&msg); // in reqQueue thread (CExternalReq)
    if (error == 0)
    {
        if ( (msg.type == MsgType_Service) &&
             (msg.u.reply.type == ReplyType_ProcessInfoNs) )
        {
            if ( msg.u.reply.u.process_info_ns.return_code == MPI_SUCCESS )
            {
                process = AddCloneProcess( &msg.u.reply.u.process_info_ns );
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                   trace_printf( "%s@%d - ProcessInfoNs(%d, %d:%d) -- can't find target process\n"
                               , method_name, __LINE__
                               , msg.u.reply.u.process_info_ns.nid
                               , msg.u.reply.u.process_info_ns.pid
                               , msg.u.reply.u.process_info_ns.verifier);
                }

                if ( msg.u.reply.u.process_info_ns.return_code != MPI_ERR_NAME )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf),
                              "[%s] ProcessInfo(%d, %d:%d) failed, rc=%d\n"
                            , method_name
                            , msg.u.reply.u.process_info_ns.nid
                            , msg.u.reply.u.process_info_ns.pid
                            , msg.u.reply.u.process_info_ns.verifier
                            , msg.u.reply.u.process_info_ns.return_code );
                    mon_log_write( MON_NODE_CLONEPROCESSNS_1, SQ_LOG_ERR, buf );
                }
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfoNs\n"
                    , method_name, msg.type, msg.u.reply.type );
            mon_log_write( MON_NODE_CLONEPROCESSNS_2, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] - Process info request to Name Server failed\n"
                , method_name );
        mon_log_write( MON_NODE_CLONEPROCESSNS_3, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
    return( process );
}
#endif
   
#ifndef NAMESERVER_PROCESS
CProcess *CNodeContainer::CloneProcessNs( const char *name, Verifier_t verifier )
{
    const char method_name[] = "CNodeContainer::CloneProcessNs";
    TRACE_ENTRY;

    CProcess *process = NULL;

    struct message_def msg;
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = REPLY_TAG;
    msg.u.request.type = ReqType_ProcessInfoNs;

    struct ProcessInfo_def *processInfo = &msg.u.request.u.process_info;
    processInfo->nid = -1;
    processInfo->pid = -1;
    processInfo->verifier = -1;
    processInfo->process_name[0] = 0;
    processInfo->target_nid = -1;
    processInfo->target_pid = -1;
    processInfo->target_verifier = verifier;
    STRCPY( processInfo->target_process_name, name);
    processInfo->target_process_pattern[0] = 0;
    processInfo->type = ProcessType_Undefined;

    int error = NameServer->ProcessInfoNs(&msg); // in reqQueue thread (CExternalReq)
    if (error == 0)
    {
        if ( (msg.type == MsgType_Service) &&
             (msg.u.reply.type == ReplyType_ProcessInfoNs) )
        {
            if ( msg.u.reply.u.process_info_ns.return_code == MPI_SUCCESS )
            {
                process = AddCloneProcess( &msg.u.reply.u.process_info_ns );
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                {
                   trace_printf( "%s@%d - ProcessInfoNs(%s:%d) -- can't find target process\n"
                               , method_name, __LINE__
                               , msg.u.reply.u.process_info_ns.process_name
                               , msg.u.reply.u.process_info_ns.verifier);
                }

                if ( msg.u.reply.u.process_info_ns.return_code != MPI_ERR_NAME )
                {
                    char buf[MON_STRING_BUF_SIZE];
                    snprintf( buf, sizeof(buf),
                              "[%s] ProcessInfo(%s:%d) failed, rc=%d\n"
                            , method_name
                            , msg.u.reply.u.process_info_ns.process_name
                            , msg.u.reply.u.process_info_ns.verifier
                            , msg.u.reply.u.process_info_ns.return_code );
                    mon_log_write( MON_NODE_CLONEPROCESSNS_4, SQ_LOG_ERR, buf );
                }
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfo\n"
                    , method_name, msg.type, msg.u.reply.type );
            mon_log_write( MON_NODE_CLONEPROCESSNS_5, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] - Process info request to Name Server failed\n"
                , method_name );
        mon_log_write( MON_NODE_CLONEPROCESSNS_6, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
    return( process );
}
#endif
   
#ifndef NAMESERVER_PROCESS
void CNodeContainer::DeleteCloneProcess( CProcess *process )
{
    const char method_name[] = "CNodeContainer::DeleteCloneProcess";
    TRACE_ENTRY;

    CNode *node;
    node = Nodes->GetLNode(process->GetNid())->GetNode();
    node->DelFromNameMap ( process );
    node->DelFromPidMap ( process );
    node->DeleteFromList( process );

    TRACE_EXIT;
}
#endif

void CNodeContainer::DeletedNode( CNode *node )
{
    const char method_name[] = "CNodeContainer::DeletedNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST | TRACE_SYNC))
    {
        trace_printf( "%s@%d - node %s, pnid=%d, zone=%d\n"
                    , method_name, __LINE__
                    , node->GetName(), node->GetPNid(), node->GetZone() );
    }

    assert( node->GetState() == State_Down );

    // Broadcast node deleted notice to local processes
    CLNode *lnode = node->GetFirstLNode();
    for ( ; lnode; lnode = lnode->GetNextP() )
    {
        lnode->Deleted();
    }

    TRACE_EXIT;
}

bool CNodeContainer::DeleteNode( int pnid )
{
    const char method_name[] = "CNodeContainer::DeleteNode";
    TRACE_ENTRY;

    int rs = true;
    string nodeName;

    CNode *pnode = GetNode( pnid );
    if ( pnode )
    {
        nodeName = pnode->GetName();
        // Broadcast node deleted notice to local processes
        Nodes->DeletedNode( pnode );

        // Now delete it from the monitor's view
        Nodes->DeleteNode( pnode );

        // Verify it was deleted, sanity check!
        if ((pnode = Nodes->GetNode( pnid )) == NULL )
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s@%d] Node %s deleted from configuration, pnid=%d\n"
                    , method_name, __LINE__, nodeName.c_str() , pnid);
            mon_log_write(MON_NODE_DELETENODE_2, SQ_LOG_INFO, buf);
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s@%d] Could not find physical node "
                  "in monitor's view of cluster, pnid=%d\n"
                , method_name, __LINE__, pnid);
        mon_log_write(MON_NODE_DELETENODE_1, SQ_LOG_ERR, buf);
        rs = false;
    }

    TRACE_EXIT;
    return( rs );
}

void CNodeContainer::DeleteNode( CNode  *node )
{
    const char method_name[] = "CNodeContainer::DeleteNode";
    TRACE_ENTRY;

    int pnid = node->GetPNid();

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d deleting (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                     , method_name, __LINE__
                     , pnid
                     , GetPNodesCount()
                     , GetLNodesCount() );
    }

    // lock sync thread
    if ( !Emulate_Down )
    {
        Monitor->EnterSyncCycle();
    }

    // delete logical nodes owned by physical node
    DeleteNodeLNodes( node );

    // delete physical node and remove from physical nodes array
    node->DeLink (&head_, &tail_);
    delete node;
    Node[pnid] = NULL;

    // Decrement the physical node count
    pnodeCount_--;
    
    // unlock sync thread
    if ( !Emulate_Down )
    {
        Monitor->ExitSyncCycle();
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d deleted (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                     , method_name, __LINE__
                     , pnid
                     , GetPNodesCount()
                     , GetLNodesCount() );
    }

    TRACE_EXIT;
}

void CNodeContainer::DeleteNodeLNodes( CNode *node )
{
    const char method_name[] = "CNodeContainer::DeleteNodeLNode";
    TRACE_ENTRY;


    CLNode *lnode = node->GetFirstLNode();
    while ( lnode )
    {
        int nid = lnode->GetNid();
        LNode[nid] = NULL;
        // Remove logical node from physical node
        node->RemoveLNodeP( lnode );
        // Delete logical node
        Nodes->DeleteLNode( lnode );
        lnode = node->GetFirstLNode();
    }
    
    TRACE_EXIT;
}

int CNodeContainer::GetFirstNid( void )
{
    const char method_name[] = "CNodeContainer::GetFirstNid";
    TRACE_ENTRY;

    int target = -1;
    for (int i = 0; i <  clusterConfig_->GetLNodesConfigMax(); i++ )
    {
        CLNode *lnode = LNode[i];
        if ( lnode )
        {
            target = lnode->GetNid();
            break;
        }
    }

    TRACE_EXIT;
    return( target );
}

int CNodeContainer::GetNextNid( int nid )
{
    const char method_name[] = "CNodeContainer::GetNextNid";
    TRACE_ENTRY;

    int target = -1;
    for (int i = (nid+1); i <  clusterConfig_->GetLNodesConfigMax(); i++ )
    {
        CLNode *lnode = LNode[i];
        if ( lnode )
        {
            target = lnode->GetNid();
            break;
        }
    }

    TRACE_EXIT;
    return( target );
}

CNode *CNodeContainer::GetNode(int pnid)
{
    const char method_name[] = "CNodeContainer::GetNode";
    TRACE_ENTRY;

    CNode *node = head_;
    while (node)
    {
        if ( node->GetPNid() == pnid )
        { 
            break;
        }
        node = node->GetNext();
    }

    TRACE_EXIT;
    return node;
}

CNode *CNodeContainer::GetNodeByMap(int index )
{
    const char method_name[] = "CNodeContainer::GetNodeByMap";
    TRACE_ENTRY;

    CNode *node = NULL;
    
    if( index >= 0 && index < GetPNodesCount() )
    {
        node = Node[indexToPnid_[index]];
    }

    TRACE_EXIT;
    return node;
}

CNode *CNodeContainer::GetNode(char *name )
{
    CNode *node = head_;
    const char method_name[] = "CNodeContainer::GetNode";
    TRACE_ENTRY;

    while (node)
    {
        if ( CPNodeConfigContainer::hostnamecmp( node->GetName(), name) == 0 )
        { 
            break;
        }
        node = node->GetNext();
    }

    TRACE_EXIT;
    return node;
}

CNode *CNodeContainer::GetNode(char *process_name, CProcess ** process,
                               bool checkstate)
{
    CNode *node = head_;
    CProcess *p_process;
    const char method_name[] = "CNodeContainer::GetNode";
    TRACE_ENTRY;

    while (node)
    {
        *process = node->CProcessContainer::GetProcess (process_name,
                                                        checkstate);
        if (*process)
        { 
            p_process = *process;
            if (!p_process->IsBackup())
                break;
        }
        node = node->GetNext ();
    }

    TRACE_EXIT;
    return node;
}

CProcess *CNodeContainer::GetProcess( int nid, int pid, bool checknode )
{
    const char method_name[] = "CNodeContainer::GetProcess";
    TRACE_ENTRY;

    CProcess *process = NULL;
    CLNode   *lnode = GetLNode( nid );

    if ( lnode )
    {
        if ( checknode )
        {
            if ( lnode->GetState() == State_Up ||
                 lnode->GetState() == State_Shutdown )
            {
                process = lnode->GetProcessL( pid );
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                   trace_printf("%s@%d" " - Node is not up, nid=%d, state=%d, \n", method_name, __LINE__, lnode->GetNid(), lnode->GetState());
            }
        }
        else
        {
            process = lnode->GetProcessL( pid );
        }
    }

    TRACE_EXIT;
    return( process );
}

CProcess *CNodeContainer::GetProcess( int nid
                                    , int pid
                                    , Verifier_t verifier
                                    , bool checknode
                                    , bool checkprocess
                                    , bool backupOk )
{
    const char method_name[] = "CNodeContainer::GetProcess(nid,pid,verifier)";
    TRACE_ENTRY;

    CProcess *process = NULL;

    if ( nid != -1 )
    {
        CLNode   *lnode = GetLNode( nid );
        if ( lnode )
        {
            if ( checknode )
            {
                if ( lnode->GetState() == State_Up ||
                     lnode->GetState() == State_Shutdown )
                {
                    process = lnode->GetProcessL( pid, verifier, checkprocess );
                    if ( process ) 
                    {
                        if ( nid != process->GetNid() )
                        {
                           if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                           {
                              trace_printf( "%s@%d - Get (%d, %d:%d) failed -- nid mismatch (%d)\n"
                                          , method_name, __LINE__
                                          , nid
                                          , pid
                                          , verifier
                                          , process->GetNid() );
                           }            
                           process = NULL;
                        }
                        else if ( !backupOk && process->IsBackup() )
                        {
                            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                            {
                                trace_printf( "%s@%d - Get (%d, %d:%d) failed -- backupOk=%d backup=%d\n"
                                            , method_name, __LINE__
                                            , nid
                                            , pid
                                            , verifier
                                            , backupOk
                                            , process->IsBackup() );
                            }
                            process = NULL;
                        }
                    }
                    else
                    {
                        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        {
                           trace_printf( "%s@%d - Get (%d, %d:%d) failed\n"
                                       , method_name, __LINE__
                                       , nid
                                       , pid
                                       , verifier );
                        }
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                       trace_printf("%s@%d" " - Node is not up, nid=%d, state=%d, \n", method_name, __LINE__, lnode->GetNid(), lnode->GetState());
                }
            }
            else
            {
                process = lnode->GetProcessL( pid, verifier, checkprocess );
                if ( process ) 
                {
                    if ( nid != process->GetNid() )
                    {
                       if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                       {
                          trace_printf( "%s@%d - Get (%d, %d:%d) failed -- nid mismatch (%d)\n"
                                      , method_name, __LINE__
                                      , nid
                                      , pid
                                      , verifier
                                      , process->GetNid() );
                       }            
                       process = NULL;
                    }
                    else if ( !backupOk && process->IsBackup() )
                    {
                        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        {
                            trace_printf( "%s@%d - Get (%d, %d:%d) failed -- backupOk=%d backup=%d\n"
                                        , method_name, __LINE__
                                        , nid
                                        , pid
                                        , verifier
                                        , backupOk
                                        , process->IsBackup() );
                        }
                        process = NULL;
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    {
                       trace_printf( "%s@%d - Get (%d, %d:%d) failed\n"
                                   , method_name, __LINE__
                                   , nid
                                   , pid
                                   , verifier );
                    }            
                }
            }
        }
    }

    TRACE_EXIT;
    return( process );
}

CProcess *CNodeContainer::GetProcess( const char *name
                                    , Verifier_t verifier
                                    , bool checknode
                                    , bool checkprocess
                                    , bool backupOk )
{
    CNode *node = head_;
    CProcess *process = NULL;
    const char method_name[] = "CNodeContainer::GetProcess(name,verifier)";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
    {
        trace_printf( "%s@%d Getting %s:%d, "
                      "checknode=%d, checkprocess=%d, backupOk=%d\n"
                    , method_name, __LINE__
                    , name
                    , verifier
                    , checknode
                    , checkprocess
                    , backupOk );
    }

    while ( node )
    {
        if ( checknode )
        {
            if ( node->GetState() == State_Up ||
                 node->GetState() == State_Shutdown )
            {
                process = node->CProcessContainer::GetProcess( name
                                                             , verifier
                                                             , checkprocess );
                if ( process ) 
                {
                    if ( !backupOk && process->IsBackup() )
                    {
                        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        {
                            trace_printf( "%s@%d - Get (%s:%d) failed -- backupOk=%d backup=%d\n"
                                        , method_name, __LINE__
                                        , name
                                        , verifier
                                        , backupOk
                                        , process->IsBackup() );
                        }
                        process = NULL;
                    }
                    else
                    {
                        if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                        {
                            trace_printf( "%s@%d - Found (%s:%d) (%d, %d) backup=%d\n"
                                        , method_name, __LINE__
                                        , name
                                        , verifier
                                        , process->GetNid()
                                        , process->GetPid()
                                        , process->IsBackup() );
                        }
                        break;
                    }
                }
                else
                {
                    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    {
                       trace_printf( "%s@%d - Get (%s:%d) in pnid=%d failed\n"
                                   , method_name, __LINE__
                                   , name
                                   , verifier
                                   , node->GetPNid() );
                    }            
                }
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                   trace_printf("%s@%d" " - Node is not up, pnid=%d, state=%d, \n", method_name, __LINE__, node->GetPNid(), node->GetState());
            }
        }
        else
        {
            process = node->CProcessContainer::GetProcess( name
                                                         , verifier
                                                         , checkprocess );
            if ( process)
            { 
                if ( !backupOk && process->IsBackup() )
                {
                    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    {
                        trace_printf( "%s@%d - Get (%s:%d) failed -- backupOk=%d backup=%d\n"
                                    , method_name, __LINE__
                                    , name
                                    , verifier
                                    , backupOk
                                    , process->IsBackup() );
                    }
                    process = NULL;
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                {
                   trace_printf( "%s@%d - Get (%s:%d) failed\n"
                               , method_name, __LINE__
                               , name
                               , verifier );
                }            
            }
        }
        node = node->GetNext ();
    }

    TRACE_EXIT;
    return( process );
}

CProcess *CNodeContainer::GetProcessByName( const char *name, bool checkstate )
{
    CNode *node = head_;
    CProcess *process = NULL;
    const char method_name[] = "CNodeContainer::GetProcessByName";
    TRACE_ENTRY;

    while (node)
    {
        process = node->CProcessContainer::GetProcess( name, checkstate );
        if (process)
        { 
            if (!process->IsBackup())
                break;
        }
        node = node->GetNext ();
    }

    TRACE_EXIT;
    return( process );
}

#ifndef NAMESERVER_PROCESS
int CNodeContainer::GetProcessInfoNs( int nid
                                    , int pid
                                    , Verifier_t verifier
                                    , ProcessInfoNs_reply_def *processInfo )
{
    const char method_name[] = "CNodeContainer::GetProcessInfoNs";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;

    struct message_def msg;
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = REPLY_TAG;
    msg.u.request.type = ReqType_ProcessInfoNs;

    struct ProcessInfo_def *process_info = &msg.u.request.u.process_info;
    process_info->nid = -1;
    process_info->pid = -1;
    process_info->verifier = -1;
    process_info->process_name[0] = 0;
    process_info->target_nid = nid;
    process_info->target_pid = pid;
    process_info->target_verifier = verifier;
    process_info->target_process_name[0] = 0;
    process_info->target_process_pattern[0] = 0;
    process_info->type = ProcessType_Undefined;
    
    int error = NameServer->ProcessInfoNs(&msg); // in reqQueue thread (CExternalReq)
    if (error == 0)
    {
        if ( (msg.type == MsgType_Service) &&
             (msg.u.reply.type == ReplyType_ProcessInfoNs) )
        {
            if ( msg.u.reply.u.process_info_ns.return_code == MPI_SUCCESS )
            {
                *processInfo = msg.u.reply.u.process_info_ns;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s] ProcessInfo failed, rc=%d\n"
                        , method_name, msg.u.reply.u.process_info_ns.return_code );
                mon_log_write( MON_NODE_GETPROCESSNS_1, SQ_LOG_ERR, buf );
            }
            rc = msg.u.reply.u.process_info_ns.return_code;
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfoNs\n"
                    , method_name, msg.type, msg.u.reply.type );
            mon_log_write( MON_NODE_GETPROCESSNS_2, SQ_LOG_ERR, buf );
            rc = MPI_ERR_OP;
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] - Process info request to Name Server failed\n"
                , method_name );
        mon_log_write( MON_NODE_GETPROCESSNS_3, SQ_LOG_ERR, la_buf );
        rc = MPI_ERR_OP;
    }

    TRACE_EXIT;
    return( rc );
}

int CNodeContainer::GetProcessInfoNs( const char *name
                                    , Verifier_t verifier
                                    , ProcessInfoNs_reply_def *processInfo )
{
    const char method_name[] = "CNodeContainer::GetProcessInfoNs";
    TRACE_ENTRY;

    int rc = MPI_SUCCESS;

    struct message_def msg;
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = REPLY_TAG;
    msg.u.request.type = ReqType_ProcessInfoNs;

    struct ProcessInfo_def *process_info = &msg.u.request.u.process_info;
    process_info->nid = -1;
    process_info->pid = -1;
    process_info->verifier = -1;
    process_info->process_name[0] = 0;
    process_info->target_nid = -1;
    process_info->target_pid = -1;
    process_info->target_verifier = verifier;
    STRCPY( process_info->target_process_name, name);
    process_info->target_process_pattern[0] = 0;
    process_info->type = ProcessType_Undefined;

    int error = NameServer->ProcessInfoNs(&msg); // in reqQueue thread (CExternalReq)
    if (error == 0)
    {
        if ( (msg.type == MsgType_Service) &&
             (msg.u.reply.type == ReplyType_ProcessInfoNs) )
        {
            if ( msg.u.reply.u.process_info_ns.return_code == MPI_SUCCESS )
            {
                *processInfo = msg.u.reply.u.process_info_ns;
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s] ProcessInfo failed, rc=%d\n"
                        , method_name, msg.u.reply.u.process_info_ns.return_code );
                mon_log_write( MON_NODE_GETPROCESSNS_4, SQ_LOG_ERR, buf );
            }
            rc = msg.u.reply.u.process_info_ns.return_code;
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfo\n"
                    , method_name, msg.type, msg.u.reply.type );
            mon_log_write( MON_NODE_GETPROCESSNS_5, SQ_LOG_ERR, buf );
            rc = MPI_ERR_OP;
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] - Process info request to Name Server failed\n"
                , method_name );
        mon_log_write( MON_NODE_GETPROCESSNS_6, SQ_LOG_ERR, la_buf );
        rc = MPI_ERR_OP;
    }

    TRACE_EXIT;
    return( rc );
}

CProcess *CNodeContainer::GetProcessLByTypeNs( int nid, PROCESSTYPE type )
{
    const char method_name[] = "CNodeContainer::GetProcessLByTypeNs";
    TRACE_ENTRY;

    CProcess *process = NULL;

    struct message_def msg;
    msg.type = MsgType_Service;
    msg.noreply = false;
    msg.reply_tag = REPLY_TAG;
    msg.u.request.type = ReqType_ProcessInfoNs;

    struct ProcessInfo_def *processInfo = &msg.u.request.u.process_info;
    processInfo->nid = -1;
    processInfo->pid = -1;
    processInfo->verifier = -1;
    processInfo->process_name[0] = 0;
    processInfo->target_nid = nid;
    processInfo->target_pid = -1;
    processInfo->target_verifier = -1;
    processInfo->target_process_name[0] = 0;
    processInfo->target_process_pattern[0] = 0;
    processInfo->type = type;

    if ( trace_settings & ( TRACE_PROCESS | TRACE_REQUEST) )
    {
        trace_printf( "%s@%d - Received monitor request process-info-ns data.\n"
                      "        process_info.nid=%d\n"
                      "        process_info.pid=%d\n"
                      "        process_info.verifier=%d\n"
                      "        process_info.target_nid=%d\n"
                      "        process_info.target_pid=%d\n"
                      "        process_info.target_verifier=%d\n"
                      "        process_info.target_process_name=%s\n"
                      "        process_info.target_process_pattern=%s\n"
                      "        process_info.type=%d\n"
                    , method_name, __LINE__
                    , processInfo->nid
                    , processInfo->pid
                    , processInfo->verifier
                    , processInfo->target_nid
                    , processInfo->target_pid
                    , processInfo->target_verifier
                    , processInfo->target_process_name
                    , processInfo->target_process_pattern
                    , processInfo->type
                    );
    }

    int error = NameServer->ProcessInfoNs(&msg); // in reqQueue thread (CExternalReq)
    if (error == 0)
    {
        if ( (msg.type == MsgType_Service) &&
             (msg.u.reply.type == ReplyType_ProcessInfoNs) )
        {
            if ( msg.u.reply.u.process_info_ns.return_code == MPI_SUCCESS )
            {
                process = AddCloneProcess( &msg.u.reply.u.process_info_ns );
            }
            else
            {
                char buf[MON_STRING_BUF_SIZE];
                snprintf( buf, sizeof(buf),
                          "[%s] ProcessInfo failed, rc=%d\n"
                        , method_name, msg.u.reply.u.process_info_ns.return_code );
                mon_log_write( MON_NODE_GETPROCESSLBYTYPENS_1, SQ_LOG_ERR, buf );
            }
        }
        else
        {
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s], Invalid MsgType(%d)/ReplyType(%d) for "
                      "ProcessInfo\n"
                    , method_name, msg.type, msg.u.reply.type );
            mon_log_write( MON_NODE_GETPROCESSLBYTYPENS_2, SQ_LOG_ERR, buf );
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] - Process info request to Name Server failed\n"
                , method_name );
        mon_log_write( MON_NODE_GETPROCESSLBYTYPENS_3, SQ_LOG_ERR, la_buf );
    }

    TRACE_EXIT;
    return( process );
}
#endif

CNode *CNodeContainer::GetZoneNode(int zid)
{
    CNode *node = NULL;
    const char method_name[] = "CNodeContainer::GetZoneNode";
    TRACE_ENTRY;

    for ( int pnid = 0; pnid < GetPNodesCount(); pnid++ )
    {
        if ( ! Node[indexToPnid_[pnid]]->IsSpareNode() 
            && Node[indexToPnid_[pnid]]->GetZone() == zid )
        {
            node = Node[indexToPnid_[pnid]];
            break;
        }
    }

    TRACE_EXIT;
    return node;
}

void CNodeContainer::InitRecvBuffer( struct sync_buffer_def *recvBuf )
{
    const char method_name[] = "CNodeContainer::InitRecvBuffer";
    TRACE_ENTRY;

    struct internal_msg_def *msg;
    struct sync_buffer_def  *rBuf;

    for (int i = 0; i < GetPNodesCount(); i++)
    {
        rBuf = &recvBuf[indexToPnid_[i]];

        rBuf->nodeInfo.node_state    = State_Unknown;
        rBuf->nodeInfo.sdLevel       = ShutdownLevel_Undefined;
        rBuf->nodeInfo.tmSyncState   = SyncState_Null;
        rBuf->nodeInfo.internalState = State_Default;
        rBuf->nodeInfo.change_nid    = -1;
        rBuf->nodeInfo.seq_num       = 0;
        rBuf->msgInfo.msg_count = 0;
        rBuf->msgInfo.msg_offset = 0;

        msg = (struct internal_msg_def *) &rBuf->msg[0];
        msg->type = InternalType_Null;
    }

    TRACE_EXIT;
}

struct internal_msg_def *
CNodeContainer::InitSyncBuffer( struct sync_buffer_def *syncBuf
                              , unsigned long long seqNum
                              , upNodes_t upNodes )
{
    const char method_name[] = "CNodeContainer::InitSyncBuffer";
    TRACE_ENTRY;

    struct internal_msg_def *msg;

    syncBuf->nodeInfo.node_state    = MyNode->GetState();
    syncBuf->nodeInfo.sdLevel       = MyNode->GetShutdownLevel();
    syncBuf->nodeInfo.internalState = MyNode->getInternalState();
    syncBuf->nodeInfo.change_nid    = -1;
    syncBuf->nodeInfo.seq_num       = seqNum;
    syncBuf->nodeInfo.nodeMask      = upNodes;
#ifdef NAMESERVER_PROCESS
    syncBuf->nodeInfo.monConnCount  = MyNode->GetMonConnCount();
#else
    syncBuf->nodeInfo.monProcCount  = MyNode->GetNumProcs();
#endif

    for (int i = 0; i < GetPNodesCount(); i++)
    {
        if ( Node[indexToPnid_[i]] && Node[indexToPnid_[i]]->GetChangeState())
        {
            syncBuf->nodeInfo.change_nid = indexToPnid_[i];
            Node[indexToPnid_[i]]->SetChangeState( false );
            break;
        }
    }

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_TMSYNC))
    {
#ifdef NAMESERVER_PROCESS
        trace_printf( "%s@%d - Node %s (pnid=%d) node_state=(%d)(%s), internalState=%d, change_nid=%d, seqNum_=%lld, monConnCount=%d\n"
                    , method_name, __LINE__
                    , MyNode->GetName()
                    , MyPNID
                    , syncBuf->nodeInfo.node_state
                    , StateString( MyNode->GetState() )
                    , syncBuf->nodeInfo.internalState
                    , syncBuf->nodeInfo.change_nid
                    , syncBuf->nodeInfo.seq_num
                    , syncBuf->nodeInfo.monConnCount);
#else
        trace_printf( "%s@%d - Node %s (pnid=%d) node_state=(%d)(%s), internalState=%d, TmSyncState=(%d)(%s), change_nid=%d, seqNum_=%lld, monProcCount=%d\n"
                    , method_name, __LINE__
                    , MyNode->GetName()
                    , MyPNID
                    , syncBuf->nodeInfo.node_state
                    , StateString( MyNode->GetState() )
                    , syncBuf->nodeInfo.internalState
                    , syncBuf->nodeInfo.tmSyncState
                    , SyncStateString( syncBuf->nodeInfo.tmSyncState )
                    , syncBuf->nodeInfo.change_nid
                    , syncBuf->nodeInfo.seq_num
                    , syncBuf->nodeInfo.monProcCount);
#endif
    }

    syncBuf->msgInfo.msg_count = 0;
    syncBuf->msgInfo.msg_offset = 0;

    msg = (struct internal_msg_def *) &syncBuf->msg[0];
    msg->type = InternalType_Null;

    syncBufferFreeSpace_ = ( MAX_SYNC_SIZE - 
                             (sizeof(cluster_state_def_t) + sizeof(msgInfo_t)));

    TRACE_EXIT;
    return msg;
}

bool CNodeContainer::IsMyNodeFirstInConfigUp( void )
{
    const char method_name[] = "CNodeContainer::IsMyNodeFirstInConfigUp";
    TRACE_ENTRY;

    int pnid;
    CNode *node = NULL;

    CPNodeConfig *pnodeConfig = clusterConfig_->GetFirstPNodeConfig();
    for ( ; pnodeConfig; pnodeConfig = pnodeConfig->GetNext() )
    {
        pnid = pnodeConfig->GetPNid();
        node = GetNode( pnid );
        if (node && node->GetState() == State_Up )
        {
            if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            {
                trace_printf("%s@%d" " - MyPNID=%d, config pnid=%d\n"
                            , method_name, __LINE__
                            , MyPNID, pnid );
            }

            if (pnid == MyPNID)
            {
                return( true);
            }
            else
            {
                break;
            }
        }
    }

    TRACE_EXIT;
    return( false);
}

bool CNodeContainer::IsShutdownActive (void)
{
    bool status = false;
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::IsShutdownActive";
    TRACE_ENTRY;
    
    while (node)
    {
        if (( node->GetState() == State_Shutdown ) ||
            ( node->GetState() == State_Stopped  )   )
        {
            status = true;
        }
        node = node->GetNext ();
    }

    TRACE_EXIT;

    return status;
}


struct internal_msg_def *CNodeContainer::PopMsg( struct sync_buffer_def *recvBuf)
{
    struct internal_msg_def *msg;

    const char method_name[] = "CNodeContainer::PopMsg";
    TRACE_ENTRY;

    if ( recvBuf->msgInfo.msg_count > 0 )
    {
        msg = (struct internal_msg_def *)&recvBuf->msg[recvBuf->msgInfo.msg_offset];
        recvBuf->msgInfo.msg_count --;
        recvBuf->msgInfo.msg_offset += msg->replSize;
    }
    else
    {
        // SyncBuffer is empty --- just return null
        msg = (struct internal_msg_def *)&recvBuf->msg[0];
        msg->type = InternalType_Null;
    }

    TRACE_EXIT;

    return msg;
}

void CNodeContainer::SaveMyLastSyncBuffer( void )
{
    memcpy( (void*)lastSyncBuffer_, (void*)SyncBuffer, sizeof(sync_buffer_def) );
}

bool CNodeContainer::SpaceAvail ( int msgSize )
{
    // temp trace
    const char method_name[] = "CNodeContainer::SpaceAvail";

    // Determine if there is enough space in the sync buffer to 
    // hold a message of "msgSize" bytes plus the final end-of-buffer
    // indicator.
    if (syncBufferFreeSpace_ >= (msgSize + sizeof (InternalType)))
    {   // There is enough space
        if (trace_settings & TRACE_SYNC_DETAIL)
            trace_printf("%s@%d - Sync buffer has space to hold a %d byte message\n", method_name, __LINE__, msgSize);

        return true;
    }
    else
    {   // Not enough space left
        if (trace_settings & TRACE_SYNC)
            trace_printf("%s@%d - Sync buffer does not have enough space "
                         "to hold a %d byte message, sync buffer free=%d\n",
                         method_name, __LINE__, msgSize,
                         (int) syncBufferFreeSpace_);

        return false;
    }
}

void CNodeContainer::AddMsg (struct internal_msg_def *&msg,
                             int msgSize )
{

    // Insert the message size into the message header
    msg->replSize = msgSize;

    // Account for new message that has been inserted
    SyncBuffer->msgInfo.msg_count++;
    SyncBuffer->msgInfo.msg_offset += msgSize;
    syncBufferFreeSpace_ -= msgSize;

    // temp trace
    const char method_name[] = "CNodeContainer::AddMsg";
    if (trace_settings & TRACE_SYNC_DETAIL)
    {
        trace_printf("%s@%d - Added msg of size=%d, msg_count=%d, "
                     "msg_offset=%d, sync buffer free=%d\n", method_name,
                     __LINE__, msgSize, SyncBuffer->msgInfo.msg_count,
                     SyncBuffer->msgInfo.msg_offset, (int)syncBufferFreeSpace_);
    }

    // Set end-of-buffer marker
    msg = (struct internal_msg_def *)&SyncBuffer->msg[SyncBuffer->msgInfo.msg_offset];
    msg->type = InternalType_Null;

    return;
}

#ifndef NAMESERVER_PROCESS
void CNodeContainer::KillAll( CProcess *requester )
{
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::KillAll";
    TRACE_ENTRY;

    while (node)
    {
        node->CProcessContainer::KillAll( node->GetState(), requester );
        node = node->GetNext ();
    }

    TRACE_EXIT;
}
#endif


int CNodeContainer::ProcessCount( void )
{
    int count = 0;
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::ProcessCount";
    TRACE_ENTRY;

    while (node)
    {
#ifdef NAMESERVER_PROCESS // don't check state
        count += node->GetNumProcs();
#else
        if ( node->GetState() == State_Up || node->GetState() == State_Shutdown )
        {
            count += node->GetNumProcs();
        }
#endif
        node = node->GetNext ();
    }

    if (trace_settings & TRACE_ENTRY_EXIT)
       trace_printf("%s@%d" " - Count=" "%d" ", Exit" "\n", method_name, __LINE__, count);
    TRACE_EXIT;

    return (count<=0?0:count);
}

void CNodeContainer::RemoveFromSpareNodesList( CNode *node )
{
    const char method_name[] = "CNodeContainer::RemoveFromSpareNodesList";
    TRACE_ENTRY;

    if (trace_settings & TRACE_INIT)
        trace_printf("%s@%d - removing pnid=%d from spare node list\n", method_name, __LINE__, node->GetPNid());

    spareNodesList_.remove( node );

    if (trace_settings & TRACE_INIT)
    {
        CNode *spareNode;
        NodesList::iterator itSn;
        for ( itSn = spareNodesList_.begin(); itSn != spareNodesList_.end() ; itSn++ ) 
        {
            spareNode = *itSn;
            trace_printf("%s@%d - pnid=%d is in spare node list\n", method_name, __LINE__, spareNode->GetPNid());
        }
    }

    TRACE_EXIT;
}

void CNodeContainer::SetupCluster( CNode ***pnode_list, CLNode ***lnode_list, int **indexToPnid )
{
    const char method_name[] = "CNodeContainer::SetupCluster";
    TRACE_ENTRY;

    *pnode_list = Node;
    *lnode_list = LNode;
    *indexToPnid = indexToPnid_;

    // Build list of spare nodes
    CNode *node = GetFirstNode();
    for ( int i = 0; node && i < GetPNodesCount(); i++, node = node->GetNext() )
    {
        if (node)
        {
            if (trace_settings & TRACE_INIT)
                trace_printf( "%s@%d - Node %s (pnid=%d, zid=%d, state=%s) is Spare=%d\n"
                            , method_name, __LINE__
                            , node->GetName()
                            , node->GetPNid()
                            , node->GetZone()
                            , StateString(node->GetState())
                            , node->IsSpareNode());
            if ( node->GetState() == State_Up && node->IsSpareNode() )
            {
                spareNodesConfigList_.push_back( node );
                if (IAmIntegrating)
                {
                    // do nothing. spareNodesList will get populated in the join phase.
                }
                else
                {
                    spareNodesList_.push_back( node );
                }
            }
        }
    }
    if (trace_settings & TRACE_INIT)
    {
        CNode *spareNode;
        NodesList::iterator itSn;
        for ( itSn = spareNodesList_.begin(); itSn != spareNodesList_.end() ; itSn++ ) 
        {
            spareNode = *itSn;
            trace_printf("%s@%d - pnid=%d is in spare node list\n", method_name, __LINE__, spareNode->GetPNid());
        }
    }

    UpdateCluster();

    TRACE_EXIT;
}

void CNodeContainer::LoadConfig( void )
{
    const char method_name[] = "CNodeContainer::LoadConfig";
    TRACE_ENTRY;

    // The configuration is now global.  To minimize impact for the time being, just set the local
    // pointer to the global configuration
    if ( !clusterConfig_ )
    {
        clusterConfig_ = ClusterConfig;
    }

    if ( !nameServerConfig_ )
    {
        nameServerConfig_ = NameServerConfig;
    }
    if ( nameServerConfig_ )
    {
        if ( ! nameServerConfig_->LoadConfig() )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to load nameserver configuration.\n", method_name);
            mon_log_write(MON_NODECONT_LOAD_CONFIG_4, SQ_LOG_CRIT, la_buf);
            
            mon_failure_exit();
        }
    }

    TRACE_EXIT;
}

CLNode *CNodeContainer::SelectLNode( CProcess *requester, ZoneType type, int nid, int not_zone, bool considerLoad )
{
    CLNode *lnode = NULL;
    
    const char method_name[] = "CNodeContainer::SelectLNode";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS))
       trace_printf("%s@%d" " - nid=%d, not_zone=%d\n", method_name, __LINE__, nid, not_zone);

    lnode = (nid == -1) ? NextPossibleLNode(requester, type, nid, not_zone, considerLoad) : GetLNode(nid);

    if ( lnode )
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d" " - Selected Nid=" "%d" "\n", method_name, __LINE__, lnode->Nid);
        requester->SetLastNid ( lnode->Nid );
    }
    else
    {
        if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
           trace_printf("%s@%d" " - No node selected " "\n", method_name, __LINE__);
    }

    TRACE_EXIT;

    return lnode;
}


CLNode *CNodeContainer::NextPossibleLNode( CProcess *requester, ZoneType type, int nid, int not_zone, bool considerLoad )
{
    int avg_pcount = 0;
    unsigned int avg_memory = 0;
#ifdef USE_MEMORY_LOADBALANCING
    int memory = -1;
#endif
    int cNid = nid;
    int selected_nid = -1;
    int firstViableNid = -1;
    CLNode *lnode = NULL;
    
    const char method_name[] = "CNodeContainer::NextPossibleLNode";
    TRACE_ENTRY;

    if ( considerLoad )
    {
        // get scheduling data
        AvgNodeData(type, &avg_pcount, &avg_memory);
    }

    // Initialize candidate nid (cNid) to the specified nid.  If none
    // specified use the nid assigned the previous time that "requester"
    // started a process.  [Note that the first candidate nid will be the 
    // first one after the initialized value.]
    cNid = (cNid == -1) ? requester->GetLastNid() : nid;

    // Beginning with the first nid after the initialized cNid, examine
    // nodes until find one that meets all necessary criteria:
    //   1) node must be up
    //   2) node must be of type specified ZoneType
    //   3) node must not be a "not_zone" node
    //   4) if load is considered, node must meet load criteria
    int nodesConsidered = 0;
    while ( selected_nid == -1  && ++nodesConsidered <= GetLNodesConfigMax() )
    {
        // Advance to next logical node number
        ++cNid;
        if ( cNid >= GetLNodesConfigMax() )
        {   // Wrap around to node 0
            cNid = 0;
        }

        lnode = GetLNode( cNid );
        if ( ! lnode )
        {
            continue;
        }

        if ( lnode->GetState() != State_Up ||
             lnode->IsKillingNode() )
        {
            continue;
        }

        if ( considerLoad )
        {
#ifdef USE_MEMORY_LOADBALANCING
            memory = (lnode->GetNode()->GetFreeCache() + lnode->GetNode()->GetFreeSwap())*0.95; 
            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - Nid=%d, Memory=%d, AvgMemory=%d\n", method_name, __LINE__, cNid, memory, avg_memory );
#endif
            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - Nid=%d, NumProcs=%d, AvgProcs=%d\n", method_name, __LINE__, cNid, lnode->GetNumProcs(), avg_pcount );
        }

        // *** round robin based on requester's last used nid skipping overloaded nodes
        if (( lnode->GetZone() != not_zone  ) &&
            ( lnode->NodeZoneType & type    ) )
        {
            if ( !considerLoad
                 || (
#ifdef USE_MEMORY_LOADBALANCING
                  ( memory <= avg_memory             ) && 
#endif
                  ( lnode->GetNumProcs() <= avg_pcount ))   )
            {
                selected_nid = lnode->GetNid();
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Selecting Nid=%d, ZoneType=%d, "
                                 "requested: ZoneType=%d, not_zone=%d\n",
                                 method_name, __LINE__, selected_nid,
                                 lnode->NodeZoneType, type, not_zone);
            }
            else
            {   // Node has load greater than desired.
                // Keep track of the first node that is of the correct type
                // in case no nodes meet the required load criteria.
                firstViableNid =(firstViableNid == -1) ? cNid : firstViableNid;
            }
        }
    }

    if ( selected_nid == -1 )
    {
        if ( firstViableNid != -1 )
        {
            lnode = GetLNode( firstViableNid );
            assert( lnode );
            if (lnode && trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - Selecting Nid=%d, ZoneType=%d, "
                             "requested: ZoneType=%d, not_zone=%d\n",
                             method_name, __LINE__, firstViableNid,
                             lnode->NodeZoneType,
                             type, not_zone);
            }
        }
        else if (trace_settings & (TRACE_REQUEST_DETAIL|TRACE_PROCESS_DETAIL))
        {
            trace_printf("%s@%d - no node of requested type is available. "
                         "requested: ZoneType=%d, not_zone=%d\n",
                         method_name, __LINE__, type, not_zone);
        }
    }


    TRACE_EXIT;
    return lnode;
}

void CNodeContainer::UpdateCluster( void )
{
    const char method_name[] = "CNodeContainer::UpdateCluster";
    TRACE_ENTRY;

    CLNode *lnode;
    CNode  *node;

    for (int i = 0; i < clusterConfig_->GetPNodesConfigMax(); i++ )
    {
        indexToPnid_[i] = -1;
    }

    node = GetFirstNode();
    // Refresh the index to pnid map
    for ( int i = 0; node && i < GetPNodesCount(); i++, node = node->GetNext() )
    {
        indexToPnid_[i] = node->GetPNid();
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST))
            trace_printf( "%s@%d - indexToPnid_[%d]=%d\n"
                        , method_name, __LINE__
                        , i
                        , indexToPnid_[i]);
    }

    // Refresh the index to nid map
    lnode = GetFirstLNode();
    for ( int i = 0; lnode && i < GetLNodesCount(); i++, lnode = lnode->GetNext() )
    {
        indexToNid_[i] = lnode->GetNid();
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST))
        {
            trace_printf( "%s@%d - indexToNid_[%d]=%d\n"
                        , method_name, __LINE__
                        , i
                        , indexToNid_[i]);
        }
    }

    TRACE_EXIT;
}
