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
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <iostream>

using namespace std;

#include "clio.h"
#include "monlogging.h"
#include "montrace.h"
#include "msgdef.h"
#include "lock.h"
#include "watchdog.h"
#include "procmon.h"
#include "gentrap.h"

// The following defines specify the default values for the timers if the 
// process monitoring timer related variables are not defined.
#define PM_MonitorRateDefault          250 // in milliseconds
#define PM_LunMgrHangDelayDefault       45 // in seconds


extern CWatchdog *Watchdog;
extern int gv_ms_su_nid;

static void *ProcessMonitorThread( void *arg )
{
    const char method_name[] = "ProcessMonitorThread";
    TRACE_ENTRY;

    // Parameter passed to the thread is the CProcessMonitor object
    CProcessMonitor *processMonitor = (CProcessMonitor *) arg;
    
    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d Thread started\n", method_name, __LINE__ );
    }

    // Mask all allowed signals except SIGPROF
    sigset_t    mask;
    sigfillset( &mask);
    sigdelset( &mask, SIGPROF ); // allows profiling such as google profiler

    int rc = pthread_sigmask( SIG_SETMASK, &mask, NULL );
    if ( rc != 0 )
    {
        char buf[MON_STRING_BUF_SIZE];
        sprintf( buf, "[%s], pthread_sigmask error=%d\n", method_name, rc );
        monproc_log_write( MON_PROCMON_PROCESSMONITOR_TH_1, SQ_LOG_ERR, buf );
    }

    processMonitor->MonitorProcesses();

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d EXIT thread\n", method_name, __LINE__ );
    }
       
    TRACE_EXIT;
    pthread_exit( (void *)errno );
    return( (void *)errno );
}


CProcessMonitor::CProcessMonitor()
                :CLock()
                ,state_(PM_DISABLED)
                ,enabled_(false)
                ,checkMonitor_(false)
                ,checkLunMgr_(false)
                ,lunMgrPid_(-1)
                ,monitorPid_(-1)
                ,pmMonitorRate_(PM_MonitorRateDefault * 1000000)
                ,pmLunMgrHangDelay_(PM_LunMgrHangDelayDefault)
                ,threadId_(0)
{
    const char method_name[] = "CProcessMonitor::CProcessMonitor";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "PRMO", 4);

    clock_gettime(CLOCK_REALTIME, &monitorCheckStart_);
    lunmgrIsHungTime_.tv_sec = lunmgrIsHungTime_.tv_nsec = 0;
    
    char *pmMonitorRateValueC;
    int pmMonitorRateValue;
    if ( (pmMonitorRateValueC = getenv( "SQ_WDT_MONITOR_PROCESS_CHECKRATE" )) )
    {
        // in seconds
        pmMonitorRateValue = atoi( pmMonitorRateValueC );
        pmMonitorRate_ = pmMonitorRateValue * 1000000; // in nanoseconds
    }
    
    char *pmLunMgrHangDelayValueC;
    int pmLunMgrHangDelayValue;
    if ( (pmLunMgrHangDelayValueC = getenv( "SQ_WDT_LUNMGR_PROCESS_HANGDELAY" )) )
    {
        // in seconds
        pmLunMgrHangDelayValue = atoi( pmLunMgrHangDelayValueC );
        pmLunMgrHangDelay_ = pmLunMgrHangDelayValue;
    }
    
    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d" " - Monitor process monitoring rate in nanoseconds =%ld\n"
                    , method_name, __LINE__, pmMonitorRate_ );
        trace_printf( "%s@%d" " - Lunmgr process hang delay in seconds =%ld\n"
                    , method_name, __LINE__, pmLunMgrHangDelay_ );
    }
    TRACE_EXIT;
}

CProcessMonitor::~CProcessMonitor( void )
{
    const char method_name[] = "CProcessMonitor::~CProcessMonitor";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "prmo", 4);

    TRACE_EXIT;
}

void CProcessMonitor::CheckLunMgr( void )
{
    const char method_name[] = "CProcessMonitor::CheckLunMgr";
    TRACE_ENTRY;

    if ( IsCheckLunMgr() )
    {
        if ( IsLunMgrTimerExpired() )
        {
            kill( lunMgrPid_, SIGKILL );
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= lunmgr process is hung! - killing pid=%d\n"
                          , method_name, lunMgrPid_);
            monproc_log_write( MON_PROCMON_CHECKLUNMGR_1, SQ_LOG_ERR, la_buf);
            lunMgrPid_ = -1;
            SetCheckLunMgr( false );
        }
    }
    
    TRACE_EXIT;
}

void CProcessMonitor::CheckMonitor( void )
{
    const char method_name[] = "CProcessMonitor::CheckMonitor";
    TRACE_ENTRY;

    if ( IsCheckMonitor() )
    {
        if ( kill( monitorPid_,0 ) == -1 && errno == ESRCH) 
        { // monitor process is dead, bring the node down
            char la_buf[MON_STRING_BUF_SIZE];
            int err = errno;
            sprintf(la_buf, "[%s], Error= Monitor process exited! - pid=%d, errno=%d (%s)\n"
                          , method_name, monitorPid_, err, strerror(err));
            monproc_log_write( MON_PROCMON_CHECKMONITOR_1, SQ_LOG_CRIT, la_buf);
            // Disable monitor process monitoring
            SetCheckMonitor( false );

            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "Node %d going down, "
                      "monitor process unexpectedly exited.\n",
                      gv_ms_su_nid);
            genSnmpTrap( buf );

            // Bring the node down by exipiring the softdog timer
            Watchdog->ExpireSoftdogTimer( true );
        }
    }
    
    TRACE_EXIT;
}

bool CProcessMonitor::IsLunMgrTimerExpired( void )
{
    const char method_name[] = "CProcessMonitor::IsLunMgrTimerExpired";
    TRACE_ENTRY;

    bool expired = false;
    struct timespec timerStop;
    clock_gettime(CLOCK_REALTIME, &timerStop);

    long secsLeft = (lunmgrIsHungTime_.tv_sec - timerStop.tv_sec);
    long nsecsLeft = 0;
    if ( secsLeft < 0 )
    {
        expired = true;
    }
    else if ( secsLeft == 0 )
    {
        nsecsLeft = (lunmgrIsHungTime_.tv_nsec - timerStop.tv_nsec);
        if ( nsecsLeft <= 0 )
        {
            expired = true;
        }
    }

    if (trace_settings & TRACE_INIT)
    {
#if 0
        trace_printf("%s@%d" " - Stop time %ld(secs):%ld(nsecs)\n", method_name, __LINE__, timerStop.tv_sec, timerStop.tv_nsec);
        trace_printf("%s@%d" " - Hung time %ld(secs):%ld(nsecs)\n", method_name, __LINE__, lunmgrIsHungTime_.tv_sec, lunmgrIsHungTime_.tv_nsec);
#endif
        trace_printf("%s@%d" " - Timer expired=%d, secsLeft=%ld(secs), nsecsLeft=%ld(nsecs), pmLunMgrHangDelay_=%ld\n"
                    , method_name, __LINE__, expired, secsLeft, nsecsLeft, pmLunMgrHangDelay_ );
    }

    TRACE_EXIT;
    return( expired );
}

void CProcessMonitor::MonitorProcesses( void )
{
    const char method_name[] = "CProcessMonitor::MonitorProcesses";
    TRACE_ENTRY;

    int rc;
    struct timespec   timeout;

    monitorPid_ = gp_local_mon_io->get_monitor_pid();
    SetTimeToWakeUp( timeout );
    
    // until time exit  
    while ( GetState() != PM_EXIT ) 
    {
        lock();
        if ( !IsEnabled() )
        {
            // Wait until timer started
            CLock::wait();
        }
        else
        {
            // Wait until signaled or timer expires
            rc = CLock::timedWait( &timeout );
            if ( rc != ETIMEDOUT  )
            {
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf("%s@%d" " - Process Monitor Timer not expired, state_=%d\n", method_name, __LINE__, GetState());
                }
            }
        }

        switch ( GetState() )
        {
            case PM_START:
                StartProcessMonitoring();
                break;
            case PM_CHECK:
                if ( IsCheckLunMgr() )
                {
                    CheckLunMgr();
                }
                if ( IsCheckMonitor() )
                {
                    CheckMonitor();
                }
                break;
            case PM_STOP:
                StopProcessMonitoring();
                break;
            default:
                break;
        }
        SetTimeToWakeUp( timeout );
        unlock();
    }       

    TRACE_EXIT;
}

void CProcessMonitor::SetTimeToWakeUp( struct timespec &ts )
{
    const char method_name[] = "CProcessMonitor::SetTimeToWakeUp";
    TRACE_ENTRY;

    clock_gettime(CLOCK_REALTIME, &ts);
    if ( (ts.tv_nsec + pmMonitorRate_) >=  1000000000)
    {
        ts.tv_sec  += 1;
        ts.tv_nsec = ((ts.tv_nsec + pmMonitorRate_) - 1000000000);
    }
    else
    {
        ts.tv_nsec += pmMonitorRate_;
    }
    //trace_printf("%s@%d" " - Timeout time %ld(secs):%ld(nsecs)(monRate=%ld)\n", method_name, __LINE__, ts.tv_sec, ts.tv_nsec, pmMonitorRate_);

    TRACE_EXIT;
}

void CProcessMonitor::StartLunMgrMonitoring( int pid )
{
    const char method_name[] = "CProcessMonitor::StartLunMgrMonitoring";
    TRACE_ENTRY;

    if ( IsEnabled() && !IsCheckLunMgr() )
    {
        lunMgrPid_ = pid;
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d lunmgr process monitoring started! - pid=%d\n", method_name, __LINE__, lunMgrPid_ );
        }
        clock_gettime(CLOCK_REALTIME, &lunmgrIsHungTime_);
        lunmgrIsHungTime_.tv_sec += pmLunMgrHangDelay_;
        SetCheckLunMgr( true );
    }

    TRACE_EXIT;
}

void CProcessMonitor::StopLunMgrMonitoring( void )
{
    const char method_name[] = "CProcessMonitor::StopLunMgrMonitoring";
    TRACE_ENTRY;

    if ( IsEnabled() && IsCheckLunMgr() )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d lunmgr process monitoring stopped!\n", method_name, __LINE__ );
        }
        lunMgrPid_ = -1;
        SetCheckLunMgr( false );
    }

    TRACE_EXIT;
}

void CProcessMonitor::StartProcessMonitoring( void )
{
    const char method_name[] = "CProcessMonitor::StartProcessMonitoring";
    TRACE_ENTRY;

    if ( !IsEnabled() )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d Process monitoring started!\n", method_name, __LINE__ );
        }
        SetEnabled( true );
        SetCheckMonitor( true );
        SetState( PM_CHECK );
    }

    TRACE_EXIT;
}

void CProcessMonitor::StopProcessMonitoring( void )
{
    const char method_name[] = "CProcessMonitor::StopProcessMonitoring";
    TRACE_ENTRY;

    if ( IsEnabled() )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d Process monitoring stopped!\n", method_name, __LINE__ );
        }
        SetCheckMonitor( false );
        SetEnabled( false );
        SetState( PM_DISABLED );
    }

    TRACE_EXIT;
}

int CProcessMonitor::StartWorker( void )
{
    const char method_name[] = "CProcessMonitor::StartWorker";
    TRACE_ENTRY;

    int rc = pthread_create( &threadId_, NULL, ProcessMonitorThread, this );
    if (rc != 0)
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = rc;
        sprintf(la_buf, "[%s], Error= Can't create thread! - errno=%d (%s)\n", method_name, err, strerror(err));
        monproc_log_write( MON_PROCMON_STARTWORKER_1, SQ_LOG_ERR, la_buf);
        TRACE_EXIT;
        return( rc );
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf("%s@%d" " ProcessMonitorThread created, threadId=%lx" "\n", method_name, __LINE__, threadId_);
    }

    TRACE_EXIT;
    return( rc );
}


int CProcessMonitor::ShutdownWork( void )
{
    const char method_name[] = "CProcessMonitor::ShutdownWork";
    TRACE_ENTRY;

    int rc;
    
    // Wake up Softdog thread to exit.
    SetState( PM_EXIT );
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf( "%s@%d waiting for ProcessMonitorThread=%lx to exit.\n",
                      method_name, __LINE__, threadId_ );

    // Wait for ProcessMonitorThread to exit
    if ((rc = pthread_join(threadId_, NULL)) != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = rc;
        sprintf(buf, "[%s], Error= Can't join thread! - errno=%d (%s)\n", method_name, err, strerror(err));
        monproc_log_write(MON_PROCMON_SHUTDOWNWORK_1, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return( rc );
}

