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

#ifndef PROCMON_H 
#define PROCMON_H

#include <sched.h>
#include <list>
#include <vector>

class CProcessMonitor : public CLock
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    typedef enum {
        PM_DISABLED=0,    // initial state
        PM_START,         // start timer
        PM_CHECK,         // check processes
        PM_STOP,          // stop timer
        PM_EXIT           // thread exit 
    } PmState_t;

    CProcessMonitor();
    ~CProcessMonitor( void );

    void    MonitorProcesses( void );
    void    SetCheckMonitor( bool checkMonitor ) { CAutoLock lock(getLocker()); checkMonitor_ = checkMonitor; }
    void    SetState( PmState_t state ) { CAutoLock lock(getLocker()); state_ = state; }
    void    StartLunMgrMonitoring( int pid );
    void    StopLunMgrMonitoring( void );
    int     StartWorker( void );
    int     ShutdownWork( void );

protected:

private:
    void    CheckLunMgr( void );
    void    CheckMonitor( void );
    PmState_t GetState( void ) { CAutoLock lock(getLocker()); return( state_ ); }
    bool    IsEnabled( void ) { CAutoLock lock(getLocker()); return( enabled_ ); }
    bool    IsCheckLunMgr( void ) { CAutoLock lock(getLocker()); return( checkLunMgr_ ); }
    bool    IsCheckMonitor( void ) { CAutoLock lock(getLocker()); return( checkMonitor_ ); }
    bool    IsLunMgrTimerExpired( void );
    void    SetEnabled( bool enabled ) { CAutoLock lock(getLocker()); enabled_ = enabled; }
    void    SetCheckLunMgr( bool checkLunMgr ) { CAutoLock lock(getLocker()); checkLunMgr_ = checkLunMgr; }
    void    SetTimeToWakeUp( struct timespec &ts);
    void    StartProcessMonitoring( void );
    void    StopProcessMonitoring( void );

    PmState_t       state_;        // Physical node's current operating state
    bool            enabled_;      // true when process monitoring enabled
    bool            checkMonitor_; // true when monitor process monitoring enabled
    bool            checkLunMgr_;  // true when lunmgr process monitoring enabled
    int             lunMgrPid_;    // lunmgr process' pid 
    int             monitorPid_;   // monitor process' pid 
    long            pmMonitorRate_; // in nano seconds
    long            pmLunMgrHangDelay_; // in seconds
    struct timespec lunmgrIsHungTime_;  // lunmgr process started + hang delay
    struct timespec monitorCheckStart_; // time lunmgr process started
    pthread_t       threadId_;     // Softdog thread id 
    
};

#endif // PROCMON_H
