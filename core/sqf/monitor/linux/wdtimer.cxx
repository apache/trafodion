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
#include <signal.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#include "monlogging.h"
#include "montrace.h"
#include "msgdef.h"
#include "lock.h"
#include "wdtimer.h"


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


// The WDT_STARTUPTIMERDEFAULT is a timeout value to use during startup 
// initialization.  The SETTIMEOUT facility only sets the timeout, but
// does not drive the lower level driver to use that value until
// a KEEPALIVE call.
// Default value is in seconds
#define WDT_STARTUPTIMERDEFAULT 30


CWdTimer::CWdTimer()
         :CLock()
         ,state_(WDT_DISABLED)
         ,killingNode_(false)
         ,watchdog_(false)
         ,wdtRefresh_(Wdt_Disabled)
         ,wdtFd_(-1)
         ,wdtKeepAliveTimerValue_(WDT_KEEPALIVETIMERDEFAULT)
{
    const char method_name[] = "CWdTimer::CWdTimer";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "WDTM", 4);

    char *p = getenv("SQ_LINUX_WATCHDOG");
    if (p != NULL)
    {
        watchdog_ = true;
    }

    gettimeofday(&wdTimerStart_, NULL);

    TRACE_EXIT;
}

CWdTimer::~CWdTimer( void )
{
    const char method_name[] = "CWdTimer::~CWdTimer";
    TRACE_ENTRY;

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "wdtm", 4);

    TRACE_EXIT;
}

void CWdTimer::DisableWatchdogTimerRefresh( void )
{
    const char method_name[] = "CWdTimer::DisableWatchdogTimerRefresh";
    TRACE_ENTRY;

    if ( watchdog_ )
    {
        // Tell the sync thread to suspend the watchdog timer
        lock();
        wdtRefresh_ = Wdt_Pending;
        unlock();
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer refresh disable pending" "\n", method_name, __LINE__);
    }
    else
    {
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer refresh disable not set pending" "\n", method_name, __LINE__);
    }

    TRACE_EXIT;
}

void CWdTimer::ResetWatchdogTimer( void )
{
    static int arg;
    bool doRefresh = false;

    const char method_name[] = "CWdTimer::ResetWatchdogTimer";
    TRACE_ENTRY;
    if (watchdog_)
    {
        if ( wdtRefresh_ == Wdt_Pending )
        {
            SuspendWatchdogTimerRefresh();
        }
        else
        {
            // Currently, MPI takes 3 seconds to deliver node failure detection 
            // and few seconds in setting up a new Comm. If refresh is done only in 
            // the last 250ms (WATCHDOG_TICKS), there is a greater chance of downing 
            // the node that is stuck in MPI calls. Therefore, code below is toggled out.
            doRefresh = true; // set unconditionally until above code is active
        }
        if ( wdtRefresh_ == Wdt_Active && doRefresh )
        {
            if (ioctl(wdtFd_, WDIOC_SQ_KEEPALIVE, &arg) == -1)
            {
                char la_buf[MON_STRING_BUF_SIZE];
                int err = errno;
                sprintf(la_buf, "[CWdTimer::ResetWatchdogTimer], Keep alive failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_RESET_WATCHTIMER, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }
            gettimeofday(&wdTimerStart_, NULL);
        }
        else
        {
            if (trace_settings & TRACE_INIT)
               trace_printf("%s@%d" " - Watchdog Timer not refreshed, wdtRefresh_=%d\n", method_name, __LINE__, wdtRefresh_);
        }
    }

    TRACE_EXIT;
}

void CWdTimer::RestoreWatchdogTimer( void )
{
    const char method_name[] = "CWdTimer::RestoreWatchdogTimer";
    TRACE_ENTRY;

    char la_buf[MON_STRING_BUF_SIZE];
    int err;
    static int  timer = wdtKeepAliveTimerValue_;
    static unsigned long arg;

    if ( watchdog_ && wdtRefresh_ == Wdt_Active )
    {
        if (ioctl(wdtFd_, WDIOC_SQ_SETTIMEOUT, &timer) == -1)
        {
            err = errno;
            sprintf(la_buf, "[CWdTimer::RestoreWatchdogTimer], Set timeout failed. (Error: %s)\n", strerror(err));
            monproc_log_write(MON_WDTIMER_RESTORE_WATCHTIMER_1, SQ_LOG_ERR, la_buf);
            exit(EXIT_FAILURE);
        }

        if (ioctl(wdtFd_, WDIOC_SQ_KEEPALIVE, &arg) == -1)
        {
            err = errno;
            sprintf(la_buf, "[CWdTimer::RestoreWatchdogTimer], Keep alive failed. (Error: %s)\n", strerror(err));
            monproc_log_write(MON_WDTIMER_RESTORE_WATCHTIMER_2, SQ_LOG_ERR, la_buf);
            exit(EXIT_FAILURE);
        }
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer set to %d" "\n", method_name, __LINE__, timer);
        gettimeofday(&wdTimerStart_, NULL);
    }
    else
    {
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer not set" "\n", method_name, __LINE__);
    }

    TRACE_EXIT;
}

void CWdTimer::SetWatchdogTimerMin( void )
{
    const char method_name[] = "CWdTimer::SetWatchdogTimerMin";
    TRACE_ENTRY;

    char la_buf[MON_STRING_BUF_SIZE];
    int err;
    long todDelta;
    long timeLeft = 0;
    struct timeval todStop;
    static int  timer = 1; // 1 second is the lowest wdt setting
    static unsigned long arg;

    if ( watchdog_ && wdtRefresh_ != Wdt_Disabled )
    {
        gettimeofday(&todStop, NULL);
        // all time calculations are in microseconds
        todDelta = ((todStop.tv_sec & 0x0FFF) * 1000000 + todStop.tv_usec) - 
                    ((wdTimerStart_.tv_sec & 0x0FFF) * 1000000 + wdTimerStart_.tv_usec);
        timeLeft = (wdtKeepAliveTimerValue_ * 1000000) - todDelta;

        if (timeLeft > 1000000) // set to 1 second only if the time remaining is greater than 1 second
        {
            if (ioctl(wdtFd_, WDIOC_SQ_SETTIMEOUT, &timer) == -1)
            {
                err = errno;
                sprintf(la_buf, "[CWdTimer::SetWatchdogTimerMin], Set timeout failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_SETMIN_WATCHTIMER_1, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }

            if (ioctl(wdtFd_, WDIOC_SQ_KEEPALIVE, &arg) == -1)
            {
                err = errno;
                sprintf(la_buf, "[CWdTimer::SetWatchdogTimerMin], Keep alive failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_SETMIN_WATCHTIMER_2, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }

            if (trace_settings & TRACE_INIT)
                trace_printf("%s@%d" " - Watchdog Timer set to %d" "\n", method_name, __LINE__, timer);

            gettimeofday(&wdTimerStart_, NULL);
        }
        else
        {
            if (trace_settings & TRACE_INIT)
                trace_printf("%s@%d" " - Watchdog Timer already at or below 1 sec. timeLeft = %ld " "\n", method_name, __LINE__, timeLeft);
        }
    }
    else
    {
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer not set to miminum, wdtRefresh_=%d" "\n", method_name, __LINE__, wdtRefresh_);
    }

    TRACE_EXIT;
}

void CWdTimer::StartWatchdogTimer( void )
{
    char la_buf[MON_STRING_BUF_SIZE];
    int  fd;
    int  err;
    
    // The following variables are used to retrieve the proper startup and keepalive environment variable
    // values, and to use as arguments for the lower level ioctl calls that interface with the watchdog 
    // timer package.

    static int  timer;
    static unsigned long arg;
    char *WDT_StartupTimerValueC;
    int   WDT_StartupTimerValue;
    char *WDT_KeepAliveTimerValueC;

    const char method_name[] = "CWdTimer::StartWatchdogTimer";
    TRACE_ENTRY;

    // See if the SQ_WATCHDOG environment variable is set.  If so, we are using the new watchdog
    // timer facility.  Otherwise, we will then check to see if the SQ_WATCHDOG2 variable is set,
    // which brings into use the "old" watchdog implementation where the monitor creates a watchdog 
    // process to handle timer expiration, etc.   If neither of these is set, no watchdog timer
    // will be in use.

    if ( getenv("SQ_LINUX_WATCHDOG") )
    {

        // Now that we're here, we are using the new watchdog timer facility.  Rather than hard code
        // timer values initially, environment variables have been established so that we can change
        // the values during [perf] testing to establish the performance impact of using different timer
        // expiration values.   A separate start timer value has been established per the advice of the lower
        // level wdt engineer.  Ultimately, we may remove this initial startup value and/or hopefully 
        // change all of these environment variables to use the registry instead.   For now, this is sufficient.
        // If the SQ_WDT_STARTUPTIMERVALUE or the SQ_WDT_KEEPALIVETIMERVALUE are not defined in this case,
        // we will use the default values defined above.

        if (!(WDT_StartupTimerValueC = getenv("SQ_WDT_STARTUPTIMERVALUE")))
        {
            WDT_StartupTimerValue = WDT_STARTUPTIMERDEFAULT;
        }
        else
        {
            WDT_StartupTimerValue = atoi(WDT_StartupTimerValueC);
        }

        if (!(WDT_KeepAliveTimerValueC = getenv("SQ_WDT_KEEPALIVETIMERVALUE")))
        {
            wdtKeepAliveTimerValue_ = WDT_KEEPALIVETIMERDEFAULT;
        }
        else
        {
            wdtKeepAliveTimerValue_ = atoi(WDT_KeepAliveTimerValueC);	
        }

        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer enabled" "\n", method_name, __LINE__);

        watchdog_ = true;

        //Displays the startup and keep alive timer values in use for a given run.
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Startup Timer in seconds =%d\n", method_name, __LINE__, (WDT_StartupTimerValue));
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - KeepAlive Timer in seconds =%d\n", method_name, __LINE__, (wdtKeepAliveTimerValue_));

        //This file is the first instance where the monitor interfaces with the low level watchdog timer package
        //If a system does not have the appropriate package installed, this file will not exist and we must 
        //abort.  The other option is to disable the watchdog timer facility.
        fd = open("/dev/watchdog", O_WRONLY);
        if (fd == -1)
        {
            err = errno;
            sprintf(la_buf, "[CWdTimer::StartWatchdogTimer], Open watchdog failed. Timer disabled (Error: %s)\n", strerror(err));
            monproc_log_write(MON_WDTIMER_START_WATCHTIMER_1, SQ_LOG_ERR, la_buf);
            watchdog_ = false;
        }
        
        if (watchdog_)
        {

            // From here on, since the package is installed, we do not tolerate problems with the ioctl
            // calls necessary for the watchdog timer facility to behave as intended.  The monitor will
            // abort if there are any problems with an ioctl call.   Also, based upon information from the
            // watchdog engineer, the opening of the watchdog file above sets the timer with a 30 second
            // default expiration.  To override this default, two timeout values have been established.
            // The first is a timeout value to use during startup initialization.  The SETTIMEOUT facility
            // only sets the timeout, but does not drive the lower level driver to use that value until
            // a KEEPALIVE call.   Thus, the sequence of SETTIMEOUT(startup), KEEPALIVE, SETTIMEOUT(keepalive)
            // is used.  The second SETTIMEOUT value comes into play only when ResetWatchdogTimer is called
            // for the first time.

            wdtFd_ = fd;
            timer = WDT_StartupTimerValue;
        
            if (ioctl(fd, WDIOC_SQ_SETTIMEOUT, &timer) == -1)
            {
                err = errno;
                sprintf(la_buf, "[CWdTimer::StartWatchdogTimer], Set timeout failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_START_WATCHTIMER_2, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }

            if (ioctl(fd, WDIOC_SQ_KEEPALIVE, &arg) == -1)
            {
                err = errno;
                sprintf(la_buf, "[CWdTimer::StartWatchdogTimer], Keep alive failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_START_WATCHTIMER_3, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }

            timer = wdtKeepAliveTimerValue_;
            if (ioctl(fd, WDIOC_SQ_SETTIMEOUT, &timer) == -1)
            {
                err = errno;
                sprintf(la_buf, "[CWdTimer::StartWatchdogTimer], Set Timeout failed. (Error: %s)\n", strerror(err));
                monproc_log_write(MON_WDTIMER_START_WATCHTIMER_4, SQ_LOG_ERR, la_buf);
                exit(EXIT_FAILURE);
            }
            else
            {
                gettimeofday(&wdTimerStart_, NULL);
            }
            wdtRefresh_ = Wdt_Active;
        }
    }
    else
    {
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer has not been enabled" "\n", method_name, __LINE__);
    }
    TRACE_EXIT;
}

void CWdTimer::StopWatchdogTimer( void )
{
    const char method_name[] = "CWdTimer::StopWatchdogTimer";
    TRACE_ENTRY;

    //Depending on watchdog timer facility in use, if any, clean up the state.  For the new facility, this 
    //involves the following sequence per the engineer instructions.  The write of "V" prepares the driver
    //to receive a close, which will disable the low level timer and prevent expiration (thus preventing
    //kernel panic).   The old facility requires an event to be generated signalling termination to the 
    //watchdog process.

    if (watchdog_)
    {
        watchdog_ = false;
        wdtRefresh_ = Wdt_Disabled;
        write(wdtFd_, "V", 1);
        fsync(wdtFd_);
        close(wdtFd_);
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer has been disabled" "\n", method_name, __LINE__);
    }

    TRACE_EXIT;
}

void CWdTimer::SuspendWatchdogTimerRefresh( void )
{
    const char method_name[] = "CWdTimer::SuspendWatchdogTimerRefresh";
    TRACE_ENTRY;

    if (watchdog_)
    {
        SetWatchdogTimerMin();
        wdtRefresh_ = Wdt_Disabled;
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer refresh suspended" "\n", method_name, __LINE__);
    }
    else
    {
        if (trace_settings & TRACE_INIT)
           trace_printf("%s@%d" " - Watchdog Timer refresh NOT suspended" "\n", method_name, __LINE__);
    }

    TRACE_EXIT;
}
