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

#ifndef WDTIMER_H 
#define WDTIMER_H

#include <sched.h>
#include <list>
#include <vector>

#include "internal.h"
#include "clusterconf.h"
#include "lnode.h"

typedef enum {
    Wdt_Disabled=0,                      // Watchdog timer disabled
    Wdt_Pending,                         // Watchdog timer disable pending
    Wdt_Active                           // Watchdog timer active
} WdtState_t;

class CWdTimer : public CLock
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    typedef enum {
        WDT_DISABLED=0,
        WDT_START,         // Monitor Event to start the Watchdog timer (WDT)
        WDT_RESET,         // Monitor Event to restart the WDT
        WDT_EXPIRE,        // Monitor Event to expire the WDT 
        WDT_SHUTDOWN,      // Monitor Event to shutdown the WDT
        WDT_STOP,          // Monitor Event to stop the WDT
        WDT_EXIT           // Monitor Event telling the Watchdog process to exit 
    } WdTimerState_t;

    CWdTimer();
    ~CWdTimer( void );

    void    DisableWatchdogTimerRefresh( void );

    inline WdTimerState_t GetState( void ) { return( state_ ); }
    inline int   GetWDTKeepAliveTimerValue( void ) { return( wdtKeepAliveTimerValue_ ); }

    inline bool  IsKillingNode( void ) { return( killingNode_ ); }
    inline bool  IsWatchdogEnabled( void ) { return( watchdog_ ); }

    void    ResetWatchdogTimer( void );
    void    RestoreWatchdogTimer( void );
    inline void SetKillingNode( bool killingNode ) { killingNode_ = killingNode; }
    inline void SetState( WdTimerState_t state ) { state_ = state; }
    void    SetWatchdogEnabled( bool watchdog ) { watchdog_ = watchdog; }
    void    SetWatchdogTimerMin( void );
    void    StartWatchdogTimer( void );
    void    StopWatchdogTimer( void );
    void    SuspendWatchdogTimerRefresh( void );

protected:

private:
    WdTimerState_t  state_;        // Physical node's current operating state
    bool            killingNode_;  // true when down node in process


    // The following Watchdog boolean value is set to true when the environment variable SQ_WATCHDOG
    // is defined.  This makes the new watchdog timer facility active on the node.  Ultimately, we should
    // transition this, and all watchdog timer related defines, to be set/retrieved via the registry.
    // The new watchdog timer facility uses the existence of a file /dev/watchdog that should exist if the
    // necessary driver package is installed properly on the system.   The WDTfd field is the descriptor
    // returned when the file is opened.    

    bool            watchdog_;
    WdtState_t      wdtRefresh_;
    int             wdtFd_;
    int             wdtKeepAliveTimerValue_;

    struct timeval  wdTimerStart_;    // time of last watchdog reset
    
};

#endif // WDTIMER_H
