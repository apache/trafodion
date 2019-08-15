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

using namespace std;

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>

#include "healthcheck.h"
#include "monlogging.h"
#include "montrace.h"
#include "monitor.h"
#include "seabed/trace.h"
#include "clusterconf.h"
#include "lnode.h"
#include "pnode.h"
#include "mlio.h"
#include "reqqueue.h"
#include "process.h"
#include "redirector.h"
#include "replicate.h"
#include "zclient.h"

extern CReqQueue ReqQueue;
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CNode *MyNode;
#ifndef NAMESERVER_PROCESS
extern CRedirector Redirector;
#endif
extern CHealthCheck HealthCheck;
extern CReplicate Replicator;
extern CZClient    *ZClient;
extern int MyPNID;
extern bool IsRealCluster;
extern bool ZClientEnabled;
extern char MasterMonitorName[MAX_PROCESS_PATH];

const char *StateString( STATE state);

const char *HealthCheckStateString( HealthCheckStates state)
{
    const char *str;
    
    switch( state )
    {
        case HC_AVAILABLE:
            str = "HC_AVAILABLE";
            break;
        case HC_UPDATE_SMSERVICE:
            str = "HC_UPDATE_SMSERVICE";
            break;
        case HC_UPDATE_WATCHDOG:
            str = "HC_UPDATE_WATCHDOG";
            break;
        case MON_READY:
            str = "MON_READY";
            break;
        case MON_SHUT_DOWN:
            str = "MON_SHUT_DOWN";
            break;
        case MON_NODE_QUIESCE:
            str = "MON_NODE_QUIESCE";
            break;
        case MON_SCHED_NODE_DOWN:
            str = "MON_SCHED_NODE_DOWN";
            break;
        case MON_NODE_DOWN:
            str = "MON_NODE_DOWN";
            break;
        case MON_STOP_WATCHDOG:
            str = "MON_STOP_WATCHDOG";
            break;
        case MON_START_WATCHDOG:
            str = "MON_START_WATCHDOG";
            break;
        case MON_EXIT_PRIMITIVES:
            str = "MON_EXIT_PRIMITIVES";
            break;
        case HC_EXIT:
            str = "HC_EXIT";
            break;
        default:
            str = "HealthCheckState - Undefined";
            break;
    }

    return( str );
}


// constructor
CHealthCheck::CHealthCheck()
    : thread_id_(0)
{
    const char method_name[] = "CHealthCheck::CHealthCheck";
    TRACE_ENTRY;

    clock_gettime(CLOCK_REALTIME, &currTime_);
    lastReqCheckTime_ = currTime_;
    lastSyncCheckTime_ = currTime_;
    nextHealthLogTime_ = currTime_;
    quiesceStartTime_.tv_sec = 0;
    quiesceStartTime_.tv_nsec = 0;
    nonresponsiveTime_.tv_sec = 0;
    nonresponsiveTime_.tv_nsec = 0;

    state_ = HC_AVAILABLE; // default state
    param1_ = 0;
    watchdogProcess_ = NULL;
    smserviceProcess_ = NULL;

    initializeVars();

    monSyncTimeout_ = CHealthCheck::SYNC_TIMEOUT_DEFAULT;
    char *monSyncTimeoutC = getenv("SQ_MON_SYNC_TIMEOUT");
    if ( monSyncTimeoutC ) 
    {
       monSyncTimeout_ = atoi(monSyncTimeoutC);
    }

    checkReqResponsive_ = false;
    char *checkReqResponsiveC = getenv("SQ_MON_REQ_RESPONSIVE");
    if (checkReqResponsiveC && atoi(checkReqResponsiveC) == 1)
    {
       checkReqResponsive_ = true;
    }

    enableMonDebugging_ = false;
    char *enableDebuggingC = getenv("SQ_MON_ENABLE_DEBUG");
    if (enableDebuggingC && atoi(enableDebuggingC) == 1)
    {
       enableMonDebugging_ = true;
    }

    quiesceTimeoutSec_     = CHealthCheck::QUIESCE_TIMEOUT_DEFAULT;
    char *quiesceTimeoutC = getenv("SQ_QUIESCE_WAIT_TIME");
    if (quiesceTimeoutC)
    {
        quiesceTimeoutSec_     = atoi(quiesceTimeoutC);
    }

    healthLoggingFrequency_    = CHealthCheck::HEALTH_LOGGING_FREQUENCY_DEFAULT;
    char *healthLoggingFrequencyC = getenv("SQ_HEALTH_LOGGING_FREQUENCY");
    if (healthLoggingFrequencyC)
    {
        healthLoggingFrequency_     = atoi(healthLoggingFrequencyC);
        if (healthLoggingFrequency_ < CHealthCheck::HEALTH_LOGGING_FREQUENCY_MIN)
        {
            healthLoggingFrequency_ = CHealthCheck::HEALTH_LOGGING_FREQUENCY_MIN;
        }
    }
    nextHealthLogTime_.tv_sec += healthLoggingFrequency_;
    
#ifdef NAMESERVER_PROCESS
    cpuSchedulingDataEnabled_ = false;
#else
    cpuSchedulingDataEnabled_ = true;
    char *env;
    env = getenv("SQ_CPUSCHEDULINGDATA_ENABLED");
    if ( env && isdigit(*env) )
    {
        cpuSchedulingDataEnabled_ = atoi(env);
    }
#endif

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d quiesceTimeoutSec_ = %d, syncTimeoutSec_ = %d, workerTimeoutSec_ = %d\n", method_name, __LINE__, quiesceTimeoutSec_, CMonitor::SYNC_MAX_RESPONSIVE, CReqQueue::REQ_MAX_RESPONSIVE);

    TRACE_EXIT;
}

void CHealthCheck::initializeVars()
{
    const char method_name[] = "CHealthCheck::initializeVars";
    TRACE_ENTRY;

    quiesceCountingDown_ = false;
    nodeDownScheduled_ = false;
    wakeupTimeSaved_ = 0;
    refreshCounter_ = 0;

    TRACE_EXIT;
}

// destructor
CHealthCheck::~CHealthCheck()
{
}

const char *CHealthCheck::getStateStr(HealthCheckStates state)
{
    const char *ret;
    switch (state)
    {
    case HC_AVAILABLE:
        ret = "HC_AVAILABLE";
        break;
    case HC_UPDATE_SMSERVICE:
        ret = "HC_UPDATE_SMSERVICE";
        break;
    case HC_UPDATE_WATCHDOG:
        ret = "HC_UPDATE_WATCHDOG";
        break;
    case MON_READY:
        ret = "MON_READY";
        break;
    case MON_SHUT_DOWN:
        ret = "MON_SHUT_DOWN";
        break;
    case MON_NODE_QUIESCE:
        ret = "MON_NODE_QUIESCE";
        break;
    case MON_SCHED_NODE_DOWN:
        ret = "MON_SCHED_NODE_DOWN";
        break;
    case MON_NODE_DOWN:
        ret = "MON_NODE_DOWN";
        break;
    case MON_STOP_WATCHDOG:
        ret = "MON_STOP_WATCHDOG";
        break;
    case MON_START_WATCHDOG:
        ret = "MON_START_WATCHDOG";
        break;
    case MON_EXIT_PRIMITIVES:
        ret = "MON_EXIT_PRIMITIVES";
        break;
    case HC_EXIT:
        ret = "HC_EXIT";
        break;
    default:
        ret = "?";
        break;
    }
    return ret;
}

#ifdef NAMESERVER_PROCESS
void CHealthCheck::stopNameServer()
{
    const char method_name[] = "CHealthCheck::stopNameServer";
    TRACE_ENTRY;

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d stopping.\n", method_name, __LINE__);
    char buf[MON_STRING_BUF_SIZE];
    sprintf(buf, "[%s], stopping.\n", method_name);
    mon_log_write(MON_HEALTHCHECK_STOP_NS_1, SQ_LOG_CRIT, buf);

    mon_failure_exit();
}
#endif

// Main body:
// The health check thread waits on two conditions. A signal that is set by other monitor
// threads after updating the state, and a timer event that pops when the timer expires. 
// It goes back to the timed wait after processing the events. 
void CHealthCheck::healthCheckThread()
{
    const char method_name[] = "CHealthCheck::healthCheckThread";
    TRACE_ENTRY;

    HealthCheckStates state;
#ifndef NAMESERVER_PROCESS
    int myPid = getpid();
#endif
    struct timespec ts;

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d health check thread starting\n", method_name, __LINE__);

    setTimeToWakeUp(ts);

    bool done = false;

    if (trace_settings & (TRACE_HEALTH | TRACE_INIT | TRACE_RECOVERY))
    {
        trace_printf( "%s@%d quiesceTimeoutSec_ = %d, syncTimeoutSec_ = %d, "
                      "workerTimeoutSec_ = %d, healthLoggingFrequency_= %d (secs)\n"
                    , method_name, __LINE__
                    , quiesceTimeoutSec_
                    , monSyncTimeout_
                    , CReqQueue::REQ_MAX_RESPONSIVE
                    , healthLoggingFrequency_ );
    }

    // Wait for event or timer to expire
    while (!done) 
    {
        healthCheckLock_.lock();

        healthCheckLock_.timedWait(&ts);

        clock_gettime(CLOCK_REALTIME, &currTime_);

        // if woken late by 3 seconds, log it.
        if ( ((currTime_.tv_sec - wakeupTimeSaved_) > 3) && watchdogProcess_)
        {
            mem_log_write(MON_HEALTHCHECK_WAKEUP_2, (int)(currTime_.tv_sec - wakeupTimeSaved_));
        }

#ifndef NAMESERVER_PROCESS
#ifdef EXCHANGE_CPU_SCHEDULING_DATA
        timeToLogHealth( currTime_ );

        if (cpuSchedulingDataEnabled_)
        {
            // Replicate this host's CPU scheduling data to other nodes
            CReplSchedData *repl = new CReplSchedData();
            Replicator.addItem(repl);
        }
#endif
#endif

        state = state_;
#if 0
        if ( trace_settings & TRACE_HEALTH )
            trace_printf("%s@%d State: %s\n", method_name, __LINE__, HealthCheckStateString(state));
#endif

        switch(state)
        {
            case MON_START_WATCHDOG:
                // Start the watchdog timer. After this, WDT refresh becomes mandatory.
                sendEventToWatchDog(Watchdog_Start);
                state_ = HC_AVAILABLE;
                break;

            case MON_STOP_WATCHDOG:
                // Stop the watchdog timer. After this, WDT refresh is not needed.
                sendEventToWatchDog(Watchdog_Stop);
                state_ = HC_AVAILABLE;
                break;

            case MON_EXIT_PRIMITIVES:
                // Exit the critical primitive processes.
                sendEventToSMService(SMS_Exit); // SMS process will exit
                sendEventToWatchDog(Watchdog_Exit); // WDT process will exit
                state_ = HC_AVAILABLE;
                break;

            case MON_NODE_QUIESCE:
                // Monitor the quiescing work that has already been started.
                startQuiesce();
                state_ = HC_AVAILABLE;
                break;

            case MON_SCHED_NODE_DOWN:
                // schedule node down req on worker thread
                scheduleNodeDown();
                state_ = HC_AVAILABLE;
                break; 

            case MON_NODE_DOWN:
#ifdef NAMESERVER_PROCESS
                stopNameServer();
#endif
                if( getenv("SQ_VIRTUAL_NODES") )
                {
                    // In a virtual cluster the monitor continues to run
                    // just tell the watchdog process to exit normally
                    sendEventToWatchDog(Watchdog_Exit);
                    state_ = HC_AVAILABLE;
                }
                else
                {
                    // Bring down the node by expiring the watchdog process
                    sendEventToWatchDog(Watchdog_Expire);
                    // wait forever
                    for (;;)
                      sleep(10000); 
                }
                break;

            case MON_SHUT_DOWN:
#ifdef NAMESERVER_PROCESS
                stopNameServer();
#endif
                if( getenv("SQ_VIRTUAL_NODES") )
                {
                    // In a virtual cluster the monitor continues to run
                    // just tell the watchdog process to exit normally
                    sendEventToWatchDog(Watchdog_Exit);
                }
                else
                {
                    if ( watchdogProcess_ )
                    {
                        // Bring down the node by shutting down the watchdog process
                        sendEventToWatchDog(Watchdog_Shutdown);
                        // wait forever
                        for (;;)
                          sleep(10000); 
                    }
                    else
                    {
                        // we must be a spare as we don't have WDT process
                        assert(MyNode->IsSpareNode());
                        // set the state to shutdown so that IamAlive can drive the exit.
                        MyNode->SetState( State_Shutdown );
                    }
                }
                state_ = HC_AVAILABLE;
                break;

            case HC_AVAILABLE:
                // no event to work on. Process the timer pop.
                processTimerEvent();
                break;

            case HC_EXIT:
                // health check thread should exit.
                done = true;
                break;

            case HC_UPDATE_SMSERVICE:
                // update watchdog process object. 
                updateSMServiceProcess();
                state_ = HC_AVAILABLE;
                break;

            case HC_UPDATE_WATCHDOG:
                // update watchdog process object. 
                updateWatchdogProcess();
                state_ = HC_AVAILABLE;
                break;

            default:
                mem_log_write(MON_HEALTHCHECK_BAD_STATE, state_);
                state_ = MON_NODE_DOWN; // something is seriously wrong, bring down the node.
                break;
        }

        setTimeToWakeUp(ts);

        healthCheckLock_.unlock();
    }

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d health check thread %lx exiting\n", method_name,
                     __LINE__, pthread_self());

    pthread_exit(0);

    TRACE_EXIT;
}

// Sigusr2 is used to block request thread when WDT process is ready 
// to kill all processes, unmount and finally kill the monitor process. 
// Request thread needs to be blocked to prevent sending process death messages
// until the node is completely down.
static void sigusr2SignalHandler(int , siginfo_t *, void *)
{
    const char method_name[] = "CHealthCheck::sigusr2SignalHandler";
    TRACE_ENTRY;

    ReqQueue.enqueuePostQuiesceReq(); // this will block the worker thread.

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d sigusr2 signal triggered, work queue is now blocked.\n", 
                      method_name, __LINE__);

    if ( ZClientEnabled )
    {
        ZClient->RunningZNodeDelete( MyNode->GetName() );
        ZClient->MasterZNodeDelete( MyNode->GetName() );
    }

    char buf[MON_STRING_BUF_SIZE];
    sprintf(buf, "[CHealthCheck::sigusr2SignalHandler], work queue to now blocked.\n");
    mon_log_write(MON_HEALTHCHECK_Q_BLOCK, SQ_LOG_CRIT, buf);

    TRACE_EXIT;
}

static void *healthCheck(void *arg)
{
    const char method_name[] = "CHealthCheck::healthCheck";
    TRACE_ENTRY;

    // Parameter passed to the thread is an instance of the CHealthCheck object
    CHealthCheck *healthCheck = (CHealthCheck *) arg;

    // Set sigaction such that SIGUSR2 signal is caught.  We use this
    // to detect that WDT process wants us to block req worker thread.
    struct sigaction act;
    act.sa_sigaction = sigusr2SignalHandler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset (&act.sa_mask);
    sigaddset (&act.sa_mask, SIGUSR2);
    sigaction (SIGUSR2, &act, NULL);

    // Mask all allowed signals 
    sigset_t  mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR2);
    int rc = pthread_sigmask(SIG_SETMASK, &mask, NULL);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], pthread_sigmask error=%d\n", method_name, rc);
        mon_log_write(MON_HEALTHCHECK_HC_1, SQ_LOG_ERR, buf);
    }

    // Enter thread processing loop
    healthCheck->healthCheckThread();

    TRACE_EXIT;
    return NULL;
}

void CHealthCheck::start()
{
    const char method_name[] = "CHealthCheck::start";
    TRACE_ENTRY;

    int rc = pthread_create(&thread_id_, NULL, healthCheck, this);
    if (rc != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], thread create error=%d\n", method_name, rc);
        mon_log_write(MON_HEALTHCHECK_START_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
}


void CHealthCheck::shutdownWork(void)
{
    const char method_name[] = "CHealthCheck::shutdownWork";
    TRACE_ENTRY;

    // this will wake up healthCheck thread and make it to exit.
    setState(HC_EXIT);

    if (trace_settings & TRACE_HEALTH)
        trace_printf("%s@%d waiting for health check thread=%lx to exit.\n",
                         method_name, __LINE__, HealthCheck.tid());

    // Wait for healthCheck thread to exit
    pthread_join(HealthCheck.tid(), NULL);

    TRACE_EXIT;
}

// Sets the new state for health check thread to work on.
// Since multiple monitor threads can set the state, check if the state is available before setting one. 
// This prevents the race conditions and ensures that HC thread completes working on previous state before
// anyone can set a new one.   
void CHealthCheck::setState(HealthCheckStates st, long long param1 /* optional */)
{
    const char method_name[] = "CHealthCheck::setState";
    TRACE_ENTRY;

    bool done = false;

    while (!done)
    {
        healthCheckLock_.lock();
        if (state_ == HC_AVAILABLE) 
        {
            state_ = st;
            param1_ = param1;
            healthCheckLock_.wakeOne();  
            healthCheckLock_.unlock();
            done = true;
        }
        else
        {   // allow health check thread to complete previous event
            healthCheckLock_.unlock();
            usleep(100 * 1000); // wait for 100ms and try again
        }
    }

    TRACE_EXIT; 
}

// Send an event to the SMService process
void CHealthCheck::sendEventToSMService(SMServiceEvent_t event)
{
    const char method_name[] = "CHealthCheck::sendEventToSMService";
    TRACE_ENTRY;

    if ( smserviceProcess_ ) 
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC))
        {
            trace_printf( "%s@%d - Sending event=%d\n"
                        , method_name, __LINE__, event );
        }
#ifndef NAMESERVER_PROCESS
        smserviceProcess_->GenerateEvent( event, 0, NULL );
#endif
    }

    TRACE_EXIT;
}

// Send an event to the watch dog process
void CHealthCheck::sendEventToWatchDog(WatchdogEvent_t event)
{
    const char method_name[] = "CHealthCheck::sendEventToWatchDog";
    TRACE_ENTRY;

    if ( watchdogProcess_ ) 
    {
        if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_REQUEST | TRACE_SYNC))
        {
            trace_printf( "%s@%d - Sending event=%d\n"
                        , method_name, __LINE__, event );
        }
#ifndef NAMESERVER_PROCESS
        watchdogProcess_->GenerateEvent( event, 0, NULL );
#endif
    }

    TRACE_EXIT;
}

// Process any timer pop event such as regular watchdog refresh. 
void CHealthCheck::processTimerEvent()
{
    const char method_name[] = "CHealthCheck::processTimerEvent";
    TRACE_ENTRY;
#if 0
    if (trace_settings & TRACE_HEALTH)
    {
        trace_printf("%s@%d Timer event: wakeup=%lld, current=%ld\n"
                    , method_name, __LINE__, wakeupTimeSaved_, currTime_.tv_sec );
    }
#endif
    // check if request queue is responsive, once every REQ_MAX_RESPONSIVE secs
    if ( (currTime_.tv_sec - lastReqCheckTime_.tv_sec) > CReqQueue::REQ_MAX_RESPONSIVE )
    {
        if ( !ReqQueue.responsive(currTime_) ) 
        {
            if (trace_settings & TRACE_HEALTH)
                trace_printf("%s@%d Request worker thread not responsive.\n", method_name, __LINE__); 

            mem_log_write(MON_HEALTHCHECK_TEVENT_1);

            if (checkReqResponsive_ && !enableMonDebugging_ )
            {
                // schedule a node down request because if the lengthy request completes/aborts, 
                // the worker thread should remain blocked.
                scheduleNodeDown(); 

                // the node down request may not get scheduled to run since the worker thread is blocked.
                // force immediate node down.
                state_ = MON_NODE_DOWN;
            } 
        }
        lastReqCheckTime_ = currTime_;
    }

    // check if sync thread is responsive, once every SYNC_MAX_RESPONSIVE + 1 secs
    if ( (currTime_.tv_sec - lastSyncCheckTime_.tv_sec) > (CCluster::SYNC_MAX_RESPONSIVE) )
    {
        if ( !Monitor->responsive() )
        {
            if (nonresponsiveTime_.tv_sec == 0)
            {
                nonresponsiveTime_ = currTime_;
            }
            
            if ( (monSyncTimeout_ != -1) && !enableMonDebugging_)
            {
                if ( (currTime_.tv_sec - nonresponsiveTime_.tv_sec) > monSyncTimeout_ )
                {   
                    char buf[MON_STRING_BUF_SIZE];
                    sprintf( buf
                           , "[%s], Sync Thread Timeout detected (timeout=%d). "
                             "Scheduling all nodes down!\n"
                           , method_name, monSyncTimeout_);
                    mon_log_write(MON_HEALTHCHECK_TEVENT_4, SQ_LOG_CRIT, buf);

                    if ( ZClientEnabled )
                    {
                        ZClient->RunningZNodesDelete();
                    }
                    else
                    {
                        CNode  *node = Nodes->GetFirstNode();
                        while (node)
                        {
                            ReqQueue.enqueueDownReq( node->GetPNid() );
                        }
                    }
                    nonresponsiveTime_.tv_sec = 0;
                    nonresponsiveTime_.tv_nsec = 0;
                }
            }
        }
        else
        {
            nonresponsiveTime_.tv_sec = 0;
            nonresponsiveTime_.tv_nsec = 0;
        }

        lastSyncCheckTime_ = currTime_;
    }

    // check if quiescing has timed out, once quiescing has begun.
    if (  (quiesceCountingDown_ && 
          (currTime_.tv_sec - quiesceStartTime_.tv_sec) > quiesceTimeoutSec_)
       ) 
    {
        if (trace_settings & TRACE_HEALTH)
            trace_printf("%s@%d Quiesce timeout, node going down\n", method_name, __LINE__);

        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Quiesce timeout, node going down\n", method_name);
        mon_log_write(MON_HEALTHCHECK_TEVENT_3, SQ_LOG_WARNING, buf);

        scheduleNodeDown(); // bring the node down
    }

#ifndef NAMESERVER_PROCESS
    // refresh WDT 
    if ( SQ_theLocalIOToClient )
    {
        SQ_theLocalIOToClient->refreshWDT(++refreshCounter_);
#if 0
        if (trace_settings & TRACE_HEALTH)
            trace_printf("%s@%d Watchdog timer refreshed (%d)\n"
                        , method_name, __LINE__, refreshCounter_ );
#endif
    }
#endif

    TRACE_EXIT;
}

// sets the time when health check thread should wake up on timer pop events.
void CHealthCheck::setTimeToWakeUp( struct timespec &ts)
{
    const char method_name[] = "CHealthCheck::setTimeToWakeUp";
    TRACE_ENTRY;

    clock_gettime(CLOCK_REALTIME, &ts);

    // if this thread has been running for more than 3 seconds since it last woke up, log it.
    if ( ((ts.tv_sec - currTime_.tv_sec) > 3) && (watchdogProcess_) )
    {
       mem_log_write(MON_HEALTHCHECK_WAKEUP_1, ts.tv_sec - currTime_.tv_sec);
    }

    ts.tv_sec += 1; // wake up every second

    wakeupTimeSaved_ = ts.tv_sec;
#if 0
    if (trace_settings & TRACE_HEALTH)
    {
        trace_printf("%s@%d Set Timer event: wakeup=%lld\n"
                    , method_name, __LINE__, wakeupTimeSaved_ );
    }
#endif
    TRACE_EXIT;
}

void CHealthCheck::updateSMServiceProcess()
{
    const char method_name[] = "CHealthCheck::updateSMServiceProcess";
    TRACE_ENTRY;

    smserviceProcess_ = (CProcess *)param1_; 

    TRACE_EXIT;
}

// updates watchdog process object. Client would ask us to set this only after the watchdog process
// is in UP state. param1 has already been populated with the watchdog process object ptr.
void CHealthCheck::updateWatchdogProcess()
{
    const char method_name[] = "CHealthCheck::updateWatchdogProcess";
    TRACE_ENTRY;

    watchdogProcess_ = (CProcess *)param1_; 

    TRACE_EXIT;
}

void CHealthCheck::startQuiesce()
{
    const char method_name[] = "CHealthCheck::startQueisce";
    TRACE_ENTRY;

    ReqQueue.enqueueQuiesceReq();

#ifndef NAMESERVER_PROCESS
    if (MyNode->getNumQuiesceExitPids() > 0) // count down quiesce only if there are pids on exit list.
    {
        clock_gettime(CLOCK_REALTIME, &quiesceStartTime_);
        quiesceCountingDown_ = true;
    
        if (trace_settings & TRACE_HEALTH)
            trace_printf("%s@%d Quiesce Wait Time = %d secs\n", method_name, __LINE__, quiesceTimeoutSec_); 

        if (trace_settings & TRACE_HEALTH)
            trace_printf("%s@%d QuiesceExitPids before wait = %d\n", method_name,
                         __LINE__, MyNode->getNumQuiesceExitPids());
    }

    char buf[MON_STRING_BUF_SIZE];
    sprintf(buf, "[%s], Quiesce req queued. Send pids = %d, Exit pids = %d\n", 
            method_name, MyNode->getNumQuiesceSendPids(), MyNode->getNumQuiesceExitPids());
    mon_log_write(MON_HEALTHCHECK_QUIESCE_1, SQ_LOG_WARNING, buf);
#endif

    TRACE_EXIT;
}

// Schedule a post quiescing work on worker thread to complete the rest of the node down flow.
// It is important to schedule this on worker thread so that it can remain blocked and not process
// any requests (including process exits) while WDT process is killing the processes. 
void CHealthCheck::scheduleNodeDown()
{
    const char method_name[] = "CHealthCheck::scheduleNodeDown";
    TRACE_ENTRY;

    if (!nodeDownScheduled_)
    {
        if (quiesceCountingDown_)
        {
#ifndef NAMESERVER_PROCESS
            if (trace_settings & TRACE_HEALTH)
                trace_printf("%s@%d After wait, QuiesceSendPids = %d, QuiesceExitPids = %d\n", method_name,
                             __LINE__, MyNode->getNumQuiesceSendPids(), MyNode->getNumQuiesceExitPids());
#endif
            quiesceCountingDown_ = false;
        }
        
        ReqQueue.enqueuePostQuiesceReq();
        nodeDownScheduled_ = true;

#ifndef NAMESERVER_PROCESS
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Final node down req scheduled. QuiesceSendPids = %d, QuiesceExitPids = %d\n", 
                method_name, MyNode->getNumQuiesceSendPids(), MyNode->getNumQuiesceExitPids());
        mon_log_write(MON_HEALTHCHECK_SCH_1, SQ_LOG_WARNING, buf);
#endif
    }

    TRACE_EXIT; 
}

// Determine if it is time to log a health message and then do it if needed
void CHealthCheck::timeToLogHealth(struct timespec &currentTime)
{
    const char method_name[] = "CHealthCheck::timeToLogHealth";
    TRACE_ENTRY;

    const char message_tag[] = "Health Status";

#if 0
    if (trace_settings & TRACE_HEALTH)
    {
        trace_printf("%s@%d Timer: nextHealthLogTime=%ld, current=%ld\n"
                    , method_name, __LINE__, nextHealthLogTime_.tv_sec, currTime_.tv_sec );
    }
#endif

    if (currentTime.tv_sec >= nextHealthLogTime_.tv_sec)
    { // Time to log health status
        int readyCount = 0;
        int upCount = Nodes->GetPNodesUpCount(readyCount);
    
        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s] - Node: %s, pnid=%d, state=%s, master=%s\n"
               , message_tag
               , MyNode->GetName(), MyPNID, StateString(MyNode->GetState()), MasterMonitorName );
        mon_log_write(MON_HEALTHCHECK_TIMETOLOGHEALTH, SQ_LOG_INFO, buf);
        sprintf(buf, "[%s] - Cluster: node count=%d (State Up - count=%d, Txn Services Ready - count=%d)\n"
               , message_tag
               , Nodes->GetPNodesCount(), upCount, readyCount );
        mon_log_write(MON_HEALTHCHECK_TIMETOLOGHEALTH, SQ_LOG_INFO, buf);
        
        nextHealthLogTime_.tv_sec += healthLoggingFrequency_;
    }
    TRACE_EXIT; 
}

void CHealthCheck::triggerTimeToLogHealth( void )
{
    const char method_name[] = "CHealthCheck::triggerTimeToLogHealth";
    TRACE_ENTRY;
    if (trace_settings & (TRACE_INIT | TRACE_RECOVERY | TRACE_HEALTH))
    {
        trace_printf( "%s@%d - Trigger Health Status logging!\n"
                    , method_name, __LINE__);
    }
    clock_gettime(CLOCK_REALTIME, &currTime_);
    nextHealthLogTime_ = currTime_;
    TRACE_EXIT; 
}

