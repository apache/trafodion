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
#include <unistd.h>

using namespace std;

#include "clio.h"
#include "monlogging.h"
#include "montrace.h"
#include "msgdef.h"
#include "lock.h"
#include "pkillall.h"
#include "procmon.h"
#include "watchdog.h"
#include "sdtimer.h"
#include "gentrap.h"

#define LUNMGR_RETRY_MAX           3

extern CWatchdog       *Watchdog;
extern CProcessMonitor *ProcessMonitor;

static void *SoftdogThread( void *arg )
{
    const char method_name[] = "SoftdogThread";
    TRACE_ENTRY;

    // Parameter passed to the thread is the CSdTimer object
    CSdTimer *sdTimer = (CSdTimer *) arg;
    
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
        monproc_log_write( MON_SDTIMER_SOFTDOG_TH_1, SQ_LOG_ERR, buf );
    }

    sdTimer->SoftdogTimer();

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d EXIT thread\n", method_name, __LINE__ );
    }
       
    TRACE_EXIT;
    pthread_exit( (void *)errno );
    return( (void *)errno );
}


CSdTimer::CSdTimer()
         :CLock()
         ,state_(SDT_DISABLED)
         ,dumpMonitor_(false)
         ,killingNode_(false)
         ,softdog_(false)
         ,sdtKeepAliveTimerValue_(WDT_KEEPALIVETIMERDEFAULT)
         ,threadId_(0)
         ,sdtLastMonRefreshCtr_(0)
{
    const char method_name[] = "CSdTimer::CSdTimer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "SDTM", 4);

    char *wdtKeepAliveTimerValueC;
    int wdtKeepAliveTimerValue;
    if ( (wdtKeepAliveTimerValueC = getenv( "SQ_WDT_KEEPALIVETIMERVALUE" )) )
    {
        // in seconds
        wdtKeepAliveTimerValue = atoi( wdtKeepAliveTimerValueC );
        sdtKeepAliveTimerValue_ = wdtKeepAliveTimerValue;
    }

    clock_gettime(CLOCK_REALTIME, &expiredTime_);
    expiredTime_.tv_sec += sdtKeepAliveTimerValue_;
    
    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d" " - KeepAlive Timer in seconds =%ld\n"
                    , method_name, __LINE__, sdtKeepAliveTimerValue_ );
        trace_printf("%s@%d" " - Start time %ld(secs):%ld(nsecs)\n"
                    , method_name, __LINE__, expiredTime_.tv_sec, expiredTime_.tv_nsec);
    }

    char *env = getenv( "SQ_WDT_DUMP_MONITOR" );
    if (env && strcmp( env, "1" ) == 0)
    {
        dumpMonitor_ = true;
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf( "%s@%d" " - Dump monitor process, dumpMonitor_=%d\n"
                    , method_name, __LINE__, dumpMonitor_ );
    }

    TRACE_EXIT;
}

CSdTimer::~CSdTimer( void )
{
    const char method_name[] = "CSdTimer::~CSdTimer";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "sdtm", 4);

    TRACE_EXIT;
}

int CSdTimer::DumpMonitorProcess( void )
{
    const char method_name[] = "CSdTimer::DumpMonitorProcess";
    TRACE_ENTRY;

    int rc = 0;
    
    if ( IsDumpMonitor() )
    {
        CUtility gCore( "gcore" );
    
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf("%s@%d" " - Generating monitor core!\n", method_name, __LINE__);
        }

        char pidstr[5];
        sprintf(pidstr, "%d", gp_local_mon_io->get_monitor_pid());
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "Generating monitor core %s\n", pidstr);
        monproc_log_write( MON_SDTIMER_STOPPROCESSES_1, SQ_LOG_ERR, la_buf);

        // save, close and restore stdin 
        int savedStdIn = dup(STDIN_FILENO);
        close(STDIN_FILENO);

        // kill all processes
        rc = gCore.ExecuteCommand( pidstr );
        if ( rc == -1 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't execute 'gcore' command!\n", method_name);
            monproc_log_write( MON_SDTIMER_DUMPMONITORPROC_1, SQ_LOG_ERR, la_buf);
            dup2(savedStdIn, STDIN_FILENO);
            close(savedStdIn);

            TRACE_EXIT;
            return( rc );
        }

        dup2(savedStdIn, STDIN_FILENO);
        close(savedStdIn);
    }
    
    TRACE_EXIT;
    return( rc );
}

bool CSdTimer::IsMonitorInDebug( void )
{
    const char method_name[] = "CSdTimer::IsMonitorInDebug";
    TRACE_ENTRY;

    bool inDebug = false;
    char buffer[132];
    char filepath[MAX_PROCESS_PATH];
    FILE *procMonitorStatusFile; // "/proc/%d/status" file pointer

    memset( buffer, 0, sizeof(buffer) );
    sprintf (filepath, "/proc/%d/status", gp_local_mon_io->get_monitor_pid());
    procMonitorStatusFile = fopen(filepath, "r");
    if ( !procMonitorStatusFile )
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = errno;
        sprintf(buf, "[%s], Cannot monitor process status open %s, %s (%d)\n"
                , method_name, filepath, strerror(err), err);
        monproc_log_write(MON_SDTIMER_MONITORINDEBUG_1, SQ_LOG_ERR, buf);
        TRACE_EXIT;
        return( false );
    }

    if ( procMonitorStatusFile != NULL )
    {
        int totalProcStatusFound = 0;
        int value;
        const char *procGdbPidString = "TracerPid";
        size_t procGdbPidStringLen = strlen ( procGdbPidString );

        // Examine each /proc/%d/status item for "TracerPid"  
        // to determine if monitor process is in debug
        while( !feof( procMonitorStatusFile ) )
        {
            fgets( buffer, 132, procMonitorStatusFile );
            if ( strncmp( buffer, procGdbPidString, procGdbPidStringLen) == 0 )
            {
                value = atoi( &buffer[procGdbPidStringLen+1] );
                if ( value != 0 )
                {
                    inDebug = true;
                }
            }
            ++totalProcStatusFound;
            if ( totalProcStatusFound == procFinalItem )
            {
                break;
            }
        }

        fclose ( procMonitorStatusFile );
    }

    TRACE_EXIT;
    return( inDebug );
}

void CSdTimer::NodeFailSafe( bool timerExpired, bool shutdown )
{
    const char method_name[] = "CSdTimer::NodeFailSafe";
    TRACE_ENTRY;

    if( getenv("SQ_VIRTUAL_NODES") )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Node fail safe is not supported in a virtual cluster!\n", method_name);
        monproc_log_write( MON_SDTIMER_NODEFAILSAFE_1, SQ_LOG_INFO, la_buf);
        return;
    }
    else
    {
        if ( shutdown )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Node shutdown triggered - Node shutting down! \n", method_name );
            monproc_log_write( MON_SDTIMER_NODEFAILSAFE_2, SQ_LOG_CRIT, la_buf);
         }
         else
         {
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Node fail safe triggered - Node going down! Last refreshed at %ld(secs) %ld(nsecs). \n", 
                      method_name, expiredTime_.tv_sec - sdtKeepAliveTimerValue_, expiredTime_.tv_nsec);
            monproc_log_write( MON_SDTIMER_NODEFAILSAFE_3, SQ_LOG_CRIT, la_buf);
         }
    }
    
    int rc = 0;
    
    if ( IsSoftdogEnabled() )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf("%s@%d" " - Node going down!\n", method_name, __LINE__);
        }

        if ( timerExpired )
        {
            SuspendMonitorProcess();
        }
        SetKillingNode( true );
        rc = StopProcesses();
        if ( rc == 0 )
        {
            // Stop monitoring the monitor process
            ProcessMonitor->SetCheckMonitor( false );
            StopMonitorProcess();
        }
    }

    
    if (trace_settings & TRACE_REQUEST)
    {
        trace_printf("%s@%d" " - Node down processing complete! (rc=%d)\n", method_name, __LINE__, rc);
    }

    TRACE_EXIT;
}

void CSdTimer::ResetSoftdogTimer( struct timespec &timeout )
{
    const char method_name[] = "CSdTimer::ResetSoftdogTimer";
    TRACE_ENTRY;

    if ( IsSoftdogEnabled() )
    {
        clock_gettime(CLOCK_REALTIME, &expiredTime_);
        expiredTime_.tv_sec += sdtKeepAliveTimerValue_;
        timeout = expiredTime_;
        SetState( SDT_FAIL );
    }

    TRACE_EXIT;
}

bool CSdTimer::CheckMonitorRefresh()
{
    const char method_name[] = "CSdTimer::CheckMonitorRefresh";
    TRACE_ENTRY;

    bool result = true; // assume refreshed

    int monRefreshCtr = gp_local_mon_io->getLastMonRefresh();
    
    // the current refresh counter has to be greater than or equal to the saved refresh counter
    assert(monRefreshCtr >= sdtLastMonRefreshCtr_);

    if (monRefreshCtr == sdtLastMonRefreshCtr_)
    {   // no increment from last time
        result = false;
    }
    else
    {
        result = true; 
        sdtLastMonRefreshCtr_ = monRefreshCtr;
    }

    TRACE_EXIT;

    return result;
}

void CSdTimer::SetTimeToWakeUp( struct timespec &ts )
{
    const char method_name[] = "CSdTimer::SetTimeToWakeUp";
    TRACE_ENTRY;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += sdtKeepAliveTimerValue_;

    TRACE_EXIT;
}

void CSdTimer::SoftdogTimer( void )
{
    const char method_name[] = "CSdTimer::SoftdogTimer";
    TRACE_ENTRY;

    int rc;
    struct timespec   timeout;
    bool timerExpired = false;

    SetTimeToWakeUp( timeout );
    
    // until there is an exit event from the monitor or timer expires
    while ( GetState() != SDT_EXIT ) 
    {
        lock();
        if ( !IsSoftdogEnabled() )
        {
            // Wait until timer started
            CLock::wait();
        }
        else
        {
            // Wait until signaled or timer expires
            rc = CLock::timedWait( &timeout );
            if ( rc == ETIMEDOUT )
            {
                timerExpired = true;

                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf("%s@%d" " - Softdog Timer refresh expired" "\n", method_name, __LINE__);
                }

                if ( GetState() != SDT_FAIL && GetState() != SDT_EXIT )
                {
                    SetState( SDT_FAIL );
                }
            }
            else
            {
                if (trace_settings & TRACE_REQUEST)
                {
                    trace_printf("%s@%d" " - Softdog Timer not expired, state_=%d\n", method_name, __LINE__, GetState());
                }
            }
        }

        switch ( GetState() )
        {
            case SDT_START:
                StartSoftdogTimer();
                SetTimeToWakeUp( timeout );
                break;
            case SDT_FAIL:
                if ( timerExpired )
                {
                    if ( CheckMonitorRefresh() || IsMonitorInDebug() )
                    {
                        ResetSoftdogTimer( timeout );
                    }
                    else
                    {
                        if( getenv("SQ_VIRTUAL_NODES") )
                        { // Ignore expired timer in virtual cluster
                            ResetSoftdogTimer( timeout );
                        }
                        else
                        {
                            DumpMonitorProcess();
    
                            char buf[MON_STRING_BUF_SIZE];
                            snprintf( buf, sizeof(buf), "Node %d going down, "
                                      "failed to get refresh event from monitor\n",
                                      gv_ms_su_nid);
                            genSnmpTrap( buf );
    
                            NodeFailSafe( timerExpired );
                            StopSoftdogTimer();
                            Watchdog->SetNodeDown();
                            Watchdog->CLock::wakeOne();
                        }
                    }
                }
                break;
            case SDT_EXPIRE:
                NodeFailSafe( timerExpired );
                StopSoftdogTimer();
                Watchdog->SetNodeDown();
                Watchdog->CLock::wakeOne();
                break;
            case SDT_SHUTDOWN:
                NodeFailSafe( timerExpired, true /*shutdown*/ );
                StopSoftdogTimer();
                Watchdog->SetNodeDown();
                Watchdog->CLock::wakeOne();
                break;
            case SDT_RESET:
                ResetSoftdogTimer( timeout );
                break;
            case SDT_STOP:
                StopSoftdogTimer();
                break;
            default:
                break;
        }
        unlock();
    }       

    TRACE_EXIT;
}

void CSdTimer::StartSoftdogTimer( void )
{
    const char method_name[] = "CSdTimer::StartSoftdogTimer";
    TRACE_ENTRY;

    if ( !IsSoftdogEnabled()  )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d Timer started!\n", method_name, __LINE__ );
        }

        char la_buf[MON_STRING_BUF_SIZE];
        sprintf( la_buf
               , "[%s], KeepAlive Timer in seconds = %ld\n"
               , method_name, sdtKeepAliveTimerValue_ );
        monproc_log_write( MON_SDTIMER_STARTSOFTDOGTIMER_1, SQ_LOG_INFO, la_buf);
    
        clock_gettime(CLOCK_REALTIME, &expiredTime_);
        expiredTime_.tv_sec += sdtKeepAliveTimerValue_;
        SetSoftdog( true );
        SetState( SDT_FAIL );
    }

    TRACE_EXIT;
}

void CSdTimer::StopSoftdogTimer( void )
{
    const char method_name[] = "CSdTimer::StopSoftdogTimer";
    TRACE_ENTRY;

    if ( IsSoftdogEnabled() )
    {
        if (trace_settings & TRACE_REQUEST)
        {
            trace_printf( "%s@%d Timer stopped!\n", method_name, __LINE__ );
        }
        SetSoftdog( false );
        SetState( SDT_DISABLED );
    }

    TRACE_EXIT;
}

int CSdTimer::StartWorker( void )
{
    const char method_name[] = "CSdTimer::StartWorker";
    TRACE_ENTRY;

    int rc = pthread_create( &threadId_, NULL, SoftdogThread, this );
    if (rc != 0)
    {
        char la_buf[MON_STRING_BUF_SIZE];
        int err = rc;
        sprintf(la_buf, "[%s], Error= Can't create thread! - errno=%d (%s)\n", method_name, err, strerror(err));
        monproc_log_write( MON_SDTIMER_STARTWORKER_1, SQ_LOG_ERR, la_buf);
        TRACE_EXIT;
        return( rc );
    }

    if (trace_settings & TRACE_INIT)
    {
        trace_printf("%s@%d" " SoftdogThread created, threadId=%lx" "\n", method_name, __LINE__, threadId_);
    }

    TRACE_EXIT;
    return( rc );
}


int CSdTimer::StopProcesses( void )
{
    const char method_name[] = "CSdTimer::StopProcesses";
    TRACE_ENTRY;

    CPKillAll pKillAll( "pkillall" );
    
    if (trace_settings & TRACE_REQUEST)
    {
        trace_printf("%s@%d" " - Killing all processes!\n", method_name, __LINE__);
    }

    // save, close and restore stdin 
    int savedStdIn = dup(STDIN_FILENO);
    close(STDIN_FILENO);

    // kill all processes
    int rc = pKillAll.ExecuteCommand( "-safekill" );
    if ( rc == -1 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't execute 'pkillall -safekill' command!\n", method_name);
        monproc_log_write( MON_SDTIMER_STOPPROCESSES_1, SQ_LOG_ERR, la_buf);
        dup2(savedStdIn, STDIN_FILENO);
        close(savedStdIn);

        TRACE_EXIT;
        return( rc );
    }

    dup2(savedStdIn, STDIN_FILENO);
    close(savedStdIn);
    
    TRACE_EXIT;
    return( rc );
}

int CSdTimer::StopMonitorProcess( void )
{
    const char method_name[] = "CSdTimer::StopMonitorProcess";
    TRACE_ENTRY;

    int monPid = gp_local_mon_io->get_monitor_pid();
    int rc = kill( monPid, SIGKILL ); 
    if ( rc == -1 && errno == ESRCH)
    {
        if ( errno != ESRCH )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = rc;
            sprintf(buf, "[%s], Error= Can't kill monitor process! - errno=%d (%s)\n", method_name, err, strerror(err));
            monproc_log_write(MON_SDTIMER_STOPMONITORPROC_1, SQ_LOG_ERR, buf);
        }
        else
        {
            rc = 0;  // It's already dead
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CSdTimer::SuspendMonitorProcess( void )
{
    const char method_name[] = "CSdTimer::SuspendMonitorProcess";
    TRACE_ENTRY;

    int monPid = gp_local_mon_io->get_monitor_pid();

    if (trace_settings & TRACE_REQUEST)
    {
        trace_printf("%s@%d" " - Suspending monitor process, pid=%d\n", method_name, __LINE__, monPid);
    }

    int rc = kill( monPid, SIGUSR2 ); 
    if ( rc == -1 && errno == ESRCH)
    {
        if ( errno != ESRCH )
        {
            char buf[MON_STRING_BUF_SIZE];
            int err = rc;
            sprintf(buf, "[%s], Error= Can't signal monitor process! - errno=%d (%s)\n", method_name, err, strerror(err));
            monproc_log_write(MON_SDTIMER_SUSPENDMONITORPROC_1, SQ_LOG_ERR, buf);
        }
        else
        {
            rc = 0;  // It's dead
        }
    }

    TRACE_EXIT;
    return( rc );
}

int CSdTimer::ShutdownWork( void )
{
    const char method_name[] = "CSdTimer::ShutdownWork";
    TRACE_ENTRY;

    int rc;
    
    while ( IsSoftdogEnabled() ) 
    {
        StopSoftdogTimer();

        char buf[MON_STRING_BUF_SIZE];
        sprintf(buf, "[%s], Watchdog process timer stopped!\n", method_name);
        monproc_log_write(MON_SDTIMER_SHUTDOWNWORK_1, SQ_LOG_INFO, buf);
    }

    // Wake up Softdog thread to exit.
    SetState( SDT_EXIT );
    CLock::wakeOne();

    if (trace_settings & TRACE_INIT)
        trace_printf( "%s@%d waiting for Softdog check thread=%lx to exit.\n",
                      method_name, __LINE__, threadId_ );

    // Wait for Softdog thread to exit
    if ((rc = pthread_join(threadId_, NULL)) != 0)
    {
        char buf[MON_STRING_BUF_SIZE];
        int err = rc;
        sprintf(buf, "[%s], Error= Can't join thread! - errno=%d (%s)\n", method_name, err, strerror(err));
        monproc_log_write(MON_SDTIMER_SHUTDOWNWORK_2, SQ_LOG_ERR, buf);
    }

    TRACE_EXIT;
    return( rc );
}


