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

#ifndef HEALTHCHECK_H
#define HEALTHCHECK_H

#include <pthread.h>
#include <list>
#include <vector>
#include <signal.h>

#include "lock.h"
#include "msgdef.h"
#include "internal.h"
#include "clusterconf.h"
#include "lnode.h"


typedef enum
{
  HC_AVAILABLE = 1, // default state
  HC_UPDATE_SMSERVICE, 
  HC_UPDATE_WATCHDOG, 
  MON_READY,
  MON_SHUT_DOWN,
  MON_NODE_QUIESCE,
  MON_SCHED_NODE_DOWN,
  MON_NODE_DOWN,
  MON_STOP_WATCHDOG,
  MON_START_WATCHDOG,
  MON_EXIT_PRIMITIVES,
  HC_EXIT
} HealthCheckStates;

class CHealthCheck
{
public:

    CHealthCheck();
    virtual ~CHealthCheck();

    inline HealthCheckStates GetState( void ) { CAutoLock alock(healthCheckLock_.getLocker()); return( state_ ); }
    inline int getSyncTimeout( void ) { return( monSyncTimeout_ ); }
    void start();
    void shutdownWork();
    void setState(HealthCheckStates st, long long param1 = 0);
    void updateSMServiceProcess();
    void updateWatchdogProcess();
    void healthCheckThread();
    void initializeVars();
    static void sigusr2SignalHandler (int , siginfo_t *, void *);

    pthread_t tid() { return thread_id_; }
    void timeToLogHealth(struct timespec &ts);
    void triggerTimeToLogHealth( void );

    enum { HEALTH_LOGGING_FREQUENCY_DEFAULT = 3600 }; // Default 1 hour between health log messages
    enum { HEALTH_LOGGING_FREQUENCY_MIN = 60 };       // Mininum 1 minute between health log messages
    enum { QUIESCE_TIMEOUT_DEFAULT = 30 };  // Max seconds to wait for SE processes to exit 
    enum { SYNC_TIMEOUT_DEFAULT = 900 };    // Max seconds to wait for synchThread(main) Allgather IO completion

private:

    void setTimeToWakeUp( struct timespec &ts);
    void sendEventToSMService(SMServiceEvent_t event);
    void sendEventToWatchDog(WatchdogEvent_t event);
    void processTimerEvent();
    void startQuiesce();
    void scheduleNodeDown();
    const char *getStateStr(HealthCheckStates state);
#ifdef NAMESERVER_PROCESS
    void stopNameServer();
#endif

    HealthCheckStates state_;           // current state of the health check thread
    long long param1_;                  // optional param
    CLock healthCheckLock_;             // lock required to update/read the state
    pthread_t thread_id_;               // thread id of health check thread
    struct timespec currTime_;          // current time 
    struct timespec quiesceStartTime_;  // time when quiescing started
    struct timespec lastReqCheckTime_;  // last time when request was checked for responsiveness
    struct timespec lastSyncCheckTime_; // last time when sync thread was checked for responsiveness
    struct timespec nonresponsiveTime_; // start time when Sync thread became unresponsive
    struct timespec nextHealthLogTime_; // next time this monitor's health is to be logged
    int  healthLoggingFrequency_;       // Seconds between health log messages
    long long wakeupTimeSaved_;         // time when healthcheck thread should wakeup, in secs.
    CProcess * watchdogProcess_;        // ptr to the watchdog process object
    CProcess * smserviceProcess_;       // ptr to the smservice process object
    int quiesceTimeoutSec_;             // quiesce timeout (in secs) 
    bool quiesceCountingDown_;          // started quiesce count down 
    bool nodeDownScheduled_;            // node down req scheduled or not
    bool enableMonDebugging_;           // enable monitor debugging
    bool checkReqResponsive_;           // should req thread be checked for responsiveness or not
    int  monSyncTimeout_;               // timeout (in secs) for sync thread responsiveness
    int  refreshCounter_;               // monitor heartbeats, updated every second.
    bool cpuSchedulingDataEnabled_;     // monitors exchange CPU scheduling data when enabled
};

#endif
