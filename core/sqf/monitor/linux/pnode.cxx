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

#include "replicate.h"
#include "reqqueue.h"
extern CReqQueue ReqQueue;
extern char MyPath[MAX_PROCESS_PATH];
extern int MyPNID;
extern bool IsRealCluster;
extern bool SpareNodeColdStandby;

extern CConfigContainer *Config;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CDeviceContainer *Devices;
extern CNode *MyNode;
extern CMonStats *MonStats;
extern CRedirector Redirector;
extern CReplicate Replicator;

extern bool IAmIntegrating;

const char *StateString( STATE state);
const char *SyncStateString( SyncState state);

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


// The following defines specify the default values for the HA
// timers if the timer related environment variables are not defined.
// Defaults to 5 second Watchdog process timer expiration
#define WDT_KeepAliveTimerDefault 5

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


CNode::CNode( char *name, int pnid, int rank )
      :CLNodeContainer(this)
      ,CProcessContainer(true)
      ,pnid_(pnid)
      ,changeState_(false)
      ,numCores_(0)
      ,freeCache_(0)
      ,state_(rank == -1 ? State_Down : State_Up)
      ,phase_(Phase_Ready)
      ,softDown_(false)
      ,killingNode_(false)
      ,dtmAborted_(false)
      ,smsAborted_(false)
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
      ,tmSyncNid_(-1)
      ,tmSyncState_(SyncState_Null)
      ,shutdownLevel_(ShutdownLevel_Undefined)
      ,wdtKeepAliveTimerValue_(WDT_KeepAliveTimerDefault)
      ,zid_(pnid)
      ,commSocketPort_(-1)
      ,syncSocketPort_(-1)
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

    quiesceSendPids_ = new SQ_LocalIOToClient::bcastPids_t;
    quiesceExitPids_ = new SQ_LocalIOToClient::bcastPids_t;
    internalState_ = State_Default; 

    uniqStrId_ = Config->getMaxUniqueId ( pnid_ ) + 1;

    TRACE_EXIT;
}

CNode::CNode( char *name
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
      ,softDown_(false)
      ,killingNode_(false)
      ,dtmAborted_(false)
      ,smsAborted_(false)
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
      ,tmSyncNid_(-1)
      ,tmSyncState_(SyncState_Suspended)
      ,shutdownLevel_(ShutdownLevel_Undefined)
      ,wdtKeepAliveTimerValue_(WDT_KeepAliveTimerDefault)
      ,zid_(-1)
      ,procStatFile_(NULL)
      ,procMeminfoFile_(-1)
{
    const char method_name[] = "CNode::CNode";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PNOD", 4);

    STRCPY(name_, name);
    
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

    quiesceSendPids_ = new SQ_LocalIOToClient::bcastPids_t;
    quiesceExitPids_ = new SQ_LocalIOToClient::bcastPids_t;
    internalState_ = State_Default; 

    uniqStrId_ = Config->getMaxUniqueId ( pnid_ ) + 1;

    TRACE_EXIT;
}

CNode::~CNode( void )
{
    const char method_name[] = "CNode::~CNode";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "pnod", 4);

    if ( procStatFile_ != NULL )
        fclose( procStatFile_ );

    if ( procMeminfoFile_ != -1 )
        close( procMeminfoFile_ );

    if (quiesceSendPids_)
    {
        delete quiesceSendPids_;
    }

    if (quiesceExitPids_)
    {
        delete quiesceExitPids_;
    }
    
    TRACE_EXIT;
}

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

int CNode::AssignNid(void)
{
    const char method_name[] = "CNode::AssignNid";
    TRACE_ENTRY;

    CLNode *lnode = AssignLNode();
    
    TRACE_EXIT;
    return( lnode->Nid );
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
        else if (lastLNode_->GetNext())
        {
            lnode = lastLNode_->GetNext();
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
    bool        tmReady;

    const char method_name[] = "CNode::CheckActivationPhase";
    TRACE_ENTRY;

    // check for a TM process in each lnode
    lnode = GetFirstLNode();
    tmReady = lnode ? true : false;
    for ( ; lnode ; lnode = lnode->GetNext() )
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
        tmReady = (tmCount == GetNumLNodes()) ? true : false;
    }
    
    if ( tmReady )
    {
        if (trace_settings & (TRACE_INIT | TRACE_SYNC | TRACE_TMSYNC))
            trace_printf("%s@%d - Activation Phase_Ready on node %s, pnid=%d\n", method_name, __LINE__, GetName(), GetPNid());
        phase_ = Phase_Ready;
        tmSyncState_ = SyncState_Null;
    }

    TRACE_EXIT;
}

void CNode::CheckShutdownProcessing( void )
{
    struct message_def *msg;

    const char method_name[] = "CNode::CheckShutdownProcessing";
    TRACE_ENTRY;
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
    TRACE_EXIT;
}

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


CNode *CNode::GetNext( void )
{
    const char method_name[] = "CNode::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return next_;
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
    for ( ; lnode; lnode = lnode->GetNext() )
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


strId_t CNode::GetStringId(char * candidate)
{
    const char method_name[] = "CProcessContainer::GetStringId";
    strId_t id;

    TRACE_ENTRY;

    if ( ! Config->findUniqueString ( pnid_, candidate, id ) )
    {   // The string is not in the configuration database, add it
        id.id  = uniqStrId_++;
        id.nid = pnid_;

        Config->addUniqueString(id.nid, id.id, candidate);

        CReplUniqStr *repl = new CReplUniqStr ( id.nid, id.id, candidate );
        Replicator.addItem(repl);
    }
    // temp trace
    else
    {
        if (trace_settings & TRACE_PROCESS)
        {
            trace_printf("%s@%d - unique string %s: id=(%d,%d)\n",
                         method_name, __LINE__, candidate, id.nid, id.id );
        }
    }

    TRACE_EXIT;

    return id;
}

CNode *CNode::Link( CNode * entry )
{
    const char method_name[] = "CNode::Link";
    TRACE_ENTRY;
    next_ = entry;
    entry->prev_ = this;

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
    for ( ; lnode; lnode = lnode->GetNext() )
    {
        // adjust each logical node to reference the 
        // new physical node's logical nodes container
        lnode->SetLNodeContainer( spareNode->GetLNodeContainer() );
    }
    spareNode->SetFirstLNode( GetFirstLNode() );
    spareNode->SetLastLNode( GetLastLNode() );
    spareNode->SetNumLNodes( GetNumLNodes() );
    SetFirstLNode( NULL );
    SetLastLNode( NULL );
    SetNumLNodes( 0 );
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
    CConfigGroup *group;
    
    snprintf( name, sizeof(name), "$WDG%03d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "stdout_WDG%03d", MyNode->GetZone() );

    group = Config->GetGroup( name );
    if (group==NULL)
    {
        char persistZones[MAX_VALUE_SIZE];
        sprintf( persistZones, "%d", MyPNID);
        // Add the watchdog persistence configuration
        group = Config->AddGroup( name, ConfigType_Process );
        group->Set( (char *) "PERSIST_ZONES" , persistZones );
        group->Set( (char *) "PERSIST_RETRIES", (char *)"10,60" );
    }
    // The following variables are used to retrieve the proper startup and keepalive environment variable
    // values, and to use as arguments for the lower level ioctl calls that interface with the watchdog 
    // timer package.

    char *WDT_KeepAliveTimerValueC;

    // If the SQ_WDT_KEEPALIVETIMERVALUE are not defined in this case,
    // we will use the default values defined above.

    if (!(WDT_KeepAliveTimerValueC = getenv("SQ_WDT_KEEPALIVETIMERVALUE")))
    {
        wdtKeepAliveTimerValue_ = WDT_KeepAliveTimerDefault;
    }
    else
    {
        wdtKeepAliveTimerValue_ = atoi(WDT_KeepAliveTimerValueC);	
    }

    //Displays the startup and keep alive timer values in use for a given run.
    if (trace_settings & TRACE_INIT)
       trace_printf("%s@%d" " - KeepAlive Timer in seconds =%d\n", method_name, __LINE__, (wdtKeepAliveTimerValue_));

    if (trace_settings & TRACE_INIT)
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
                                      result
                                      );
    if ( watchdogProcess  )
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
    CConfigGroup *group;
    
    snprintf( name, sizeof(name), "$PSD%03d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "stdout_PSD%03d", MyNode->GetZone() );

    group = Config->GetGroup( name );
    if (group==NULL)
    {
        char persistZones[MAX_VALUE_SIZE];
        sprintf( persistZones, "%d", MyPNID);
        // Add the pstartd persistence configuration
        group = Config->AddGroup( name, ConfigType_Process );
        group->Set( (char *) "PERSIST_ZONES" , persistZones );
        group->Set( (char *) "PERSIST_RETRIES", (char *)"10,60" );
    }

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
                                      result
                                      );
    if ( pstartdProcess  )
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
    for ( int i=0; !tmCount && i < Nodes->GetNodesCount(); i++ )
    {
        node = Nodes->GetNode( i );
        lnode = node->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNext() )
        {
            CProcess *process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process  ) tmCount++;
        }
    }

    if ( tmCount )
    {
        lnode = GetFirstLNode();
        for ( ; lnode ; lnode = lnode->GetNext() )
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
    for ( int i=0; !tmCount && i < Nodes->GetNodesCount(); i++ )
    {
        node = Nodes->GetNode( i );
        lnode = node->GetFirstLNode();
        for ( ; lnode; lnode = lnode->GetNext() )
        {
            process = lnode->GetProcessLByType( ProcessType_DTM );
            if ( process  ) tmCount++;
        }
    }

    if ( tmCount )
    {
        lnode = GetFirstLNode();
        for ( ; lnode ; lnode = lnode->GetNext() )
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
    CProcess * smsProcess;
    
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
       trace_printf("%s@%d" " - Creating SMService Process\n", method_name, __LINE__);

    snprintf( name, sizeof(name), "$SMS%03d", MyNode->GetZone() );
    snprintf( stdout, sizeof(stdout), "stdout_SMS%03d", MyNode->GetZone() );
#if 0
    CConfigGroup *group;
    // TODO: when Synchronized request fully implemented
    group = Config->GetGroup( name );
    if (group==NULL)
    {
        char persistZones[MAX_VALUE_SIZE];
        sprintf( persistZones, "%d", MyPNID);
        // Add the smservice persistence configuration
        group = Config->AddGroup( name, ConfigType_Process );
        group->Set( (char *) "PERSIST_ZONES" , persistZones );
        group->Set( (char *) "PERSIST_RETRIES", (char *)"2,30" );
    }
#endif
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
                                 result
                                 );
    if ( smsProcess  )
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

CNodeContainer::CNodeContainer( void )
               :NumberPNodes(0)
               ,NumberLNodes(0)
               ,Node(NULL)
               ,LNode(NULL)
               ,clusterConfig_(NULL)
               ,head_(NULL)
               ,tail_(NULL)
               ,syncBufferFreeSpace_(MAX_SYNC_SIZE)
               ,SyncBuffer(NULL)
{
    const char method_name[] = "CNodeContainer::CNodeContainer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "NCTR", 4);

    SyncBuffer = new struct sync_buffer_def;

    // Load cluster configuration from 'cluster.conf'
    LoadConfig();

    TRACE_EXIT;
}

CNodeContainer::~CNodeContainer( void )
{
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::~CNodeContainer";
    TRACE_ENTRY;
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


int CNodeContainer::GetPNid( char *nodeName )
{
    const char method_name[] = "CNodeContainer::GetPNid";
    TRACE_ENTRY;

    int pnid = clusterConfig_->GetPNid( nodeName );

    TRACE_EXIT;
    return( pnid );
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
              rank = -1;
            node = new CNode( (char *)pnodeConfig->GetName(), pnid, rank );
            assert( node != NULL );
        }
        
        if ( node )
        {
            // add physical node to physical nodes array
            Node[pnid] = node;
            NumberPNodes++;
    
            if (head_ == NULL)
            {
                head_ = tail_ = node;
            }
            else
            {
                tail_ = tail_->Link(node);
            }
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

void CNodeContainer::AddLNodes( CNode  *node )
{
    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;
    const char method_name[] = "CNodeContainer::AddLNodes";
    TRACE_ENTRY;

    pnodeConfig = clusterConfig_->GetPNodeConfig( node->GetPNid() );
    
    if ( ! pnodeConfig )
    {
        abort();
    }

    // Create logical nodes configured for the physical node passed in
    lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
    for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNextP() )
    {
        // add logical node to logical nodes array
        LNode[lnodeConfig->GetNid()] = node->AddLNode( lnodeConfig );
        NumberLNodes++;
    }

    TRACE_EXIT;
}

// add configured lnodes of physical node node2 to node1. 
void CNodeContainer::AddLNodes( CNode  *node1, CNode *node2 )
{
    CPNodeConfig   *pnodeConfig;
    CLNodeConfig   *lnodeConfig;
    const char method_name[] = "CNodeContainer::AddLNodes(node1, node2)";
    TRACE_ENTRY;

    pnodeConfig = clusterConfig_->GetPNodeConfig( node2->GetPNid() );
    
    if ( ! pnodeConfig )
    {
        abort();
    }

    // Create logical nodes configured for the physical node passed in
    lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
    for ( ; lnodeConfig; lnodeConfig = lnodeConfig->GetNextP() )
    {
        // add logical node to logical nodes array
        LNode[lnodeConfig->GetNid()] = node1->AddLNode( lnodeConfig );
        NumberLNodes++;
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
    for ( int pnid = 0; pnid < NumberPNodes; pnid++ )
    {
        Node[pnid]->ResetSpareNode();
    }

    for ( int i=0; i < spareNodesCount; i++ )
    {
        if (trace_settings & TRACE_INIT)
            trace_printf("%s@%d - unpacking spare node pnid=%d \n", method_name, __LINE__, *buffer);

        spareNodesList_.push_back( Node[*buffer] );
        Node[*buffer]->SetSpareNode();
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

            if (trace_settings & ( TRACE_INIT || TRACE_RECOVERY || TRACE_REQUEST_DETAIL) )
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

    for (int count = 0; count < nodeMapCount; count++)
    {
        pnidConfig = *buffer++;
        pnid = *buffer++;

        Nodes->AddLNodes( Nodes->GetNode(pnid), Nodes->GetNode(pnidConfig) );

        if (trace_settings & ( TRACE_INIT || TRACE_RECOVERY || TRACE_REQUEST_DETAIL) )
            trace_printf("%s@%d - Unpacking node mapping, pnidConfig=%d, pnid=%d \n",
                        method_name, __LINE__, pnidConfig, pnid);
    }

    TRACE_EXIT;
    return;
}

void CNodeContainer::PackZids( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::PackZids";
    TRACE_ENTRY;

    for ( int pnid = 0; pnid < NumberPNodes; pnid++ )
    {
        *buffer = Node[pnid]->GetZone();
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Packing zid=%d for pnid=%d\n"
                         , method_name, __LINE__
                         , Node[pnid]->GetZone(), pnid);
        ++buffer;
    }

    TRACE_EXIT;
    return;
}

void CNodeContainer::UnpackZids( intBuffPtr_t &buffer )
{
    const char method_name[] = "CNodeContainer::UnpackZids";
    TRACE_ENTRY;

    for ( int pnid = 0; pnid < NumberPNodes; pnid++ )
    {
        Node[pnid]->SetZone(*buffer);
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY))
            trace_printf( "%s@%d - Unpacking zid=%d for pnid=%d\n"
                         , method_name, __LINE__
                         , Node[pnid]->GetZone(), pnid);
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
    int     nid;
    int     lnodes = 0;
    CNode  *node;
    
    const char method_name[] = "CNodeContainer::AvgNodeData";
    TRACE_ENTRY;

    for ( nid = 0; nid < NumberLNodes; nid++ )
    {
        node = LNode[nid]->GetNode();
        // average only over node of requested type and available logical nodes
        if ( (LNode[nid]->GetZoneType() & type) &&
             (LNode[nid]->GetState() == State_Up &&
             !LNode[nid]->IsKillingNode()) )
        {
            *avg_pcount += LNode[nid]->GetNumProcs();
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
   

CLNode *CNodeContainer::GetLNode (int nid)
{
    CLNode *lnode;
    const char method_name[] = "CNodeContainer::GetLNode";
    TRACE_ENTRY;

    if( nid >= 0 && nid < NumberLNodes )
    {
        lnode = LNode[nid];
    }
    else
    {
        lnode = NULL;
    }

    TRACE_EXIT;
    return lnode;
}

CLNode *CNodeContainer::GetLNode( char *process_name, CProcess **process,
                                  bool checkstate, bool backupOk )
{
    CLNode *lnode = NULL;
    CNode *node = head_;
    CProcess *p_process;
    CLNode *b_lnode = NULL;
    CProcess *b_process = NULL;
    const char method_name[] = "CNodeContainer::GetLNode";
    TRACE_ENTRY;

    // Initialize return value
    *process = NULL;

    while (node)
    {
        if ( !node->IsSpareNode() && 
             (node->GetState() == State_Up ||
              node->GetState() == State_Shutdown) )
        {
            *process = node->CProcessContainer::GetProcess(process_name, checkstate);
            if (*process)
            { 
                p_process = *process;
                if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                    trace_printf("%s@%d - process %s (%d, %d), backup=%d, backupOk=%d\n",
                                 method_name, __LINE__,
                                 p_process->GetName(), p_process->GetNid(),
                                 p_process->GetPid(),  p_process->IsBackup(),
                                 backupOk);
                if (!p_process->IsBackup())
                {
                    lnode = LNode[p_process->GetNid()];
                    break;
                }
                else
                {
                    // Save backup process and lnode
                    b_process = *process;
                    b_lnode = LNode[b_process->GetNid()];
                }
            }
        }
        node = node->GetNext ();
    }

    if ( !*process && backupOk )
    {
        // We did not find the primary and it's ok to return the backup
        *process = b_process;
        lnode = b_lnode;
    }

    TRACE_EXIT;
    return lnode;
}

CNode *CNodeContainer::GetNode(int pnid)
{
    CNode *node;
    const char method_name[] = "CNodeContainer::GetNode";
    TRACE_ENTRY;

    if( pnid >= 0 && pnid < NumberPNodes )
    {
        node = Node[pnid];
    }
    else
    {
        node = NULL;
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
        if ( strcmp( node->GetName(), name) == 0 )
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

SyncState CNodeContainer::GetTmState ( SyncState check_state )
{
    SyncState state = check_state;
    CNode *node = head_;
    const char method_name[] = "CNodeContainer::GetTmState";
    TRACE_ENTRY;
    
    while (node)
    {
        if ( node->GetState() == State_Up && ! node->IsSpareNode() && node->GetPhase() == Phase_Ready)
        {
            if ( check_state == SyncState_Start )
            {
                if ( node->GetPNid() == MyPNID )
                {
                    if ( node->GetTmSyncState() != SyncState_Start )
                    {
                        state = SyncState_Abort;
                        if (trace_settings & TRACE_TMSYNC)
                           trace_printf("%s@%d" " - Node %s, pnid=%d" " no longer in Master Sync Start state" "\n", method_name, __LINE__, node->GetName(), node->GetPNid());
                        break;
                    }
                }
                else
                {
                    if ( node->GetTmSyncState() != SyncState_Continue )
                    {
                        state = SyncState_Abort;
                        if (trace_settings & TRACE_TMSYNC)
                           trace_printf("%s@%d" " - Node %s, pnid=%d" " doesn't agree on Sync Start state, returned state=" "%d" "\n", method_name, __LINE__, node->GetName(), node->GetPNid(), node->GetTmSyncState());
                        break;
                    }
                }
            }
            else
            {
                if ( check_state == SyncState_Suspended )
                {
                    state = node->GetTmSyncState();
                    if ( state == SyncState_Suspended )
                    {
                        if (trace_settings & TRACE_TMSYNC)
                           trace_printf("%s@%d" " - Node %s, pnid=%d" " is in TmSync Suspended state\n", method_name, __LINE__, node->GetName(), node->GetPNid());
                        break;
                    }
                }
                else if ( node->GetTmSyncState() != check_state )
                {
                    state = node->GetTmSyncState();
                    if (trace_settings & TRACE_TMSYNC)
                       trace_printf("%s@%d" " - Node %s, pnid=%d" " doesn't agree on TmState, returned state=" "%d" "\n", method_name, __LINE__, node->GetName(), node->GetPNid(), state);
                    break;
                }
            }
        }
        node = node->GetNext ();
    }
    
    TRACE_EXIT;
    return state;
}

CNode *CNodeContainer::GetZoneNode(int zid)
{
    CNode *node = NULL;
    const char method_name[] = "CNodeContainer::GetZoneNode";
    TRACE_ENTRY;

    for ( int pnid = 0; pnid < NumberPNodes; pnid++ )
    {
        if ( ! Node[pnid]->IsSpareNode() && Node[pnid]->GetZone() == zid )
        {
            node = Node[pnid];
            break;
        }
    }

    TRACE_EXIT;
    return node;
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
    syncBuf->nodeInfo.tmSyncState   = MyNode->GetTmSyncState();
    syncBuf->nodeInfo.internalState = MyNode->getInternalState();
    syncBuf->nodeInfo.change_nid    = -1;
    syncBuf->nodeInfo.seq_num       = seqNum;
    syncBuf->nodeInfo.nodeMask      = upNodes;

    for (int i = 0; i < NumberPNodes; i++)
    {
        if (Node[i]->GetChangeState())
        {
            syncBuf->nodeInfo.change_nid = i;
            Node[i]->SetChangeState( false );
            break;
        }
    }

    if (trace_settings & (TRACE_SYNC_DETAIL | TRACE_TMSYNC))
        trace_printf( "%s@%d - Node %s (pnid=%d) node_state=(%d)(%s), internalState=%d, TmSyncState=(%d)(%s), change_nid=%d, seqNum_=%lld\n"
                    , method_name, __LINE__
                    , MyNode->GetName()
                    , MyPNID
                    , syncBuf->nodeInfo.node_state
                    , StateString( MyNode->GetState() )
                    , syncBuf->nodeInfo.internalState
                    , syncBuf->nodeInfo.tmSyncState
                    , SyncStateString( syncBuf->nodeInfo.tmSyncState )
                    , syncBuf->nodeInfo.change_nid
                    , syncBuf->nodeInfo.seq_num);

    syncBuf->msgInfo.msg_count = 0;
    syncBuf->msgInfo.msg_offset = 0;

    msg = (struct internal_msg_def *) &syncBuf->msg[0];
    msg->type = InternalType_Null;

    syncBufferFreeSpace_ = ( MAX_SYNC_SIZE - 
                             (sizeof(cluster_state_def_t) + sizeof(msgInfo_t)));

    return msg;
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

    if ( recvBuf->msgInfo.msg_count )
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


int CNodeContainer::ProcessCount( void )
{
    int count = 0;
    CNode *node = head_;

    const char method_name[] = "CNodeContainer::ProcessCount";
    TRACE_ENTRY;

    while (node)
    {
        if ( node->GetState() == State_Up || node->GetState() == State_Shutdown )
        {
            count += node->GetNumProcs();
        }
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

void CNodeContainer::SetupCluster( CNode ***pnode_list, CLNode ***lnode_list )
{
    const char method_name[] = "CNodeContainer::SetupCluster";
    TRACE_ENTRY;

    *pnode_list = Node;
    *lnode_list = LNode;

    // Build list of spare nodes
    for ( int i = 0; i < GetNodesCount(); i++ )
    {
        if (trace_settings & TRACE_INIT)
            trace_printf( "%s@%d - Node %s (pnid=%d, zid=%d, state=%s) is Spare=%d\n"
                        , method_name, __LINE__
                        , Node[i]->GetName()
                        , Node[i]->GetPNid()
                        , Node[i]->GetZone()
                        , StateString(Node[i]->GetState())
                        , Node[i]->IsSpareNode());
        if ( Node[i]->GetState() == State_Up && Node[i]->IsSpareNode() )
        {
            spareNodesConfigList_.push_back( Node[i] );
            if ( IAmIntegrating )
            {
                // do nothing. spareNodesList will get populated in the join phase.
            }
            else
            {
                spareNodesList_.push_back( Node[i] );
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

    TRACE_EXIT;
}

void CNodeContainer::LoadConfig( void )
{
    const char method_name[] = "CNodeContainer::LoadConfig";
    TRACE_ENTRY;

    // Open the 'cluster.conf' file
    // Read each line and create cluster configuration objects:
    //  list of CPnid:hostname_:numCores_:spare_ (when spare_=true, nid list=null)
    //    list of CNid:ZoneType (when nid = -1, ZoneType_Excluded)
    //      list of CProcessor:firstCore_:lastCore_ (when nid = -1, processor = -1)
    if ( !clusterConfig_ )
    {
        clusterConfig_ = new CClusterConfig();
    }
    if ( clusterConfig_ )
    {
        if ( clusterConfig_->Initialize() )
        {
            if ( ! clusterConfig_->LoadConfig() )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                sprintf(la_buf, "[%s], Failed to load cluster configuration.\n", method_name);
                mon_log_write(MON_NODECONT_LOAD_CONFIG_1, SQ_LOG_CRIT, la_buf);
                
                abort();
            }
            else
            {
                // Allocate logical and physical node arrays
                Node = new CNode *[clusterConfig_->GetPNodesCount()];
                LNode = new CLNode *[clusterConfig_->GetLNodesCount()];
            }
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Failed to open cluster configuration.\n", method_name);
            mon_log_write(MON_NODECONT_LOAD_CONFIG_2, SQ_LOG_CRIT, la_buf);
            
            abort();
        }
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Failed to allocate cluster configuration.\n", method_name);
        mon_log_write(MON_NODECONT_LOAD_CONFIG_3, SQ_LOG_CRIT, la_buf);
        
        abort();
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
    while ( selected_nid == -1  && ++nodesConsidered <= NumberLNodes )
    {
        // Advance to next logical node number
        ++cNid;
        if ( cNid >= NumberLNodes )
        {   // Wrap around to node 0
            cNid = 0;
        }

        if ( LNode[cNid]->GetState() != State_Up ||
             LNode[cNid]->IsKillingNode() )
        {
            continue;
        }

        if ( considerLoad )
        {
#ifdef USE_MEMORY_LOADBALANCING
            memory = (LNode[cNid]->GetNode()->GetFreeCache() + LNode[cNid]->GetNode()->GetFreeSwap())*0.95; 
            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - Nid=%d, Memory=%d, AvgMemory=%d\n", method_name, __LINE__, cNid, memory, avg_memory );
#endif
            if (trace_settings & (TRACE_REQUEST_DETAIL | TRACE_PROCESS_DETAIL))
                trace_printf("%s@%d - Nid=%d, NumProcs=%d, AvgProcs=%d\n", method_name, __LINE__, cNid, LNode[cNid]->GetNumProcs(), avg_pcount );
        }

        // *** round robin based on requester's last used nid skipping overloaded nodes
        if (( LNode[cNid]->GetZone() != not_zone  ) &&
            ( LNode[cNid]->NodeZoneType & type    ) )
        {
            if ( !considerLoad
                 || (
#ifdef USE_MEMORY_LOADBALANCING
                  ( memory <= avg_memory             ) && 
#endif
                  ( LNode[cNid]->GetNumProcs() <= avg_pcount ))   )
            {
                lnode = LNode[cNid];
                selected_nid = cNid;
                if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
                    trace_printf("%s@%d - Selecting Nid=%d, ZoneType=%d, "
                                 "requested: ZoneType=%d, not_zone=%d\n",
                                 method_name, __LINE__, selected_nid,
                                 LNode[cNid]->NodeZoneType, type, not_zone);
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
            lnode = LNode[firstViableNid];
            if (trace_settings & (TRACE_REQUEST | TRACE_PROCESS))
            {
                trace_printf("%s@%d - Selecting Nid=%d, ZoneType=%d, "
                             "requested: ZoneType=%d, not_zone=%d\n",
                             method_name, __LINE__, firstViableNid,
                             LNode[firstViableNid]->NodeZoneType,
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
