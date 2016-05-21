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

#ifndef SDTIMER_H 
#define SDTIMER_H

#include <sched.h>
#include <list>
#include <vector>

class CSdTimer : public CLock
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    typedef enum {
        SDT_DISABLED=0,
        SDT_START,         // start Softdog timer (SDT)
        SDT_RESET,         // restart SDT
        SDT_FAIL,          // timer expired SDT
        SDT_EXPIRE,        // forced expire SDT
        SDT_STOP,          // stop SDT
        SDT_SHUTDOWN,      // shutdown SDT
        SDT_EXIT           // Softdog thread exit 
    } SdTimerState_t;

    CSdTimer();
    ~CSdTimer( void );

    int     DumpMonitorProcess( void );

    inline bool  IsDumpMonitor( void ) { CAutoLock lock(getLocker()); return( dumpMonitor_ ); }
    inline bool  IsKillingNode( void ) { CAutoLock lock(getLocker()); return( killingNode_ ); }
    bool    IsSoftdogEnabled( void ) { CAutoLock lock(getLocker()); return( softdog_ ); }

    void    ResetSoftdogTimer( struct timespec &timeout );
    void    SetState( SdTimerState_t state ) { CAutoLock lock(getLocker()); state_ = state; }
    void    SoftdogTimer( void );
    int     StartWorker( void );
    int     ShutdownWork( void );

protected:

private:
    SdTimerState_t GetState( void ) { CAutoLock lock(getLocker()); return( state_ ); }
    bool    IsMonitorInDebug( void );
    void    NodeFailSafe( bool timerExpired, bool shutdown = false );
    void    SetKillingNode( bool killingNode ) { CAutoLock lock(getLocker()); killingNode_ = killingNode; }
    void    SetSoftdog( bool softdog ) { softdog_ = softdog; }
    void    SetTimeToWakeUp( struct timespec &ts );
    void    StartSoftdogTimer( void );
    int     StopMonitorProcess( void );
    int     StopProcesses( void );
    void    StopSoftdogTimer( void );
    int     SuspendMonitorProcess( void );
    void    ProcessTimerEvent( void );
    bool    CheckMonitorRefresh( void );

    SdTimerState_t  state_;        // Physical node's current operating state
    bool            dumpMonitor_;  // when true gcore the monitor process
    bool            killingNode_;  // true when down node in process
    bool            softdog_;      // true when timer started
    long            sdtKeepAliveTimerValue_; // in seconds
    struct timespec expiredTime_;  // time of last softdog reset + keep alive timer value
    pthread_t       threadId_;     // Softdog thread id 
    long            sdtLastMonRefreshCtr_; // in seconds
    
    enum itemsProcStatus 
    {
         procName       // program name 
        ,procState      // process state
        ,procSleepAvg   // sleep average
        ,procTgid       // thread id
        ,procPid        // pid
        ,procPPid       // parent pid
        ,procTracerPid  // debugger pid
        ,procFinalItem  // [** final enum item **]
    };
};

#endif // SDTIMER_H
