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

#ifndef WATCHDOG_H 
#define WATCHDOG_H

#include <sched.h>
#include <list>
#include <vector>

#include "sdtimer.h"
//#include "wdtimer.h"


class CWatchdog : public CLock
{
private:
    int            eyecatcher_;      // Debuggging aid -- leave as first
                                     // member variable of the class
public:
    CWatchdog( void );
    ~CWatchdog( void );

    void    ExpireSoftdogTimer( bool dontSleep=false );
    void    ShutdownSoftdogTimer( bool dontSleep=false );
    WatchdogEvent_t GetEvent( void );
    bool    IsNodeDown( void );
    void    ResetSoftdogTimer( void );
    void    SetEvent( WatchdogEvent_t event );
    void    SetNodeDown( void );
    void    StartSoftdogTimer( void );
    int     StartWorkerThreads( void );
    void    StopSoftdogTimer( void );
    int     StopWorkerThreads( void );
    void    WaitForEvent( void );

protected:

private:
    WatchdogEvent_t  event_;        // Last monitor event
    bool             nodeDown_;     // SdTimer expired and node down executed
    CSdTimer         sdTimer_;        // softdog timer
    //CWdTimer       *wdTimer_;         // Linux Watchdog timer

};


#endif // WATCHDOG_H
