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

#include <iostream>

using namespace std;

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/resource.h>
#include <errno.h>

#include "seabed/logalt.h"
#include "monlogging.h"
#include "montrace.h"
#include "msgdef.h"

#define gettid() syscall(__NR_gettid)

bool GenCoreOnFailureExit = false;

extern bool IsRealCluster;
extern int MyPNID;
extern CMonLog *MonLog;

pthread_mutex_t       MonLogMutex = PTHREAD_MUTEX_INITIALIZER;

void mon_failure_exit( bool genCoreOnFailureExit )
{
    if (genCoreOnFailureExit || GenCoreOnFailureExit)
    {
        // Generate a core file, abort is intentional
        abort();
    }
    else
    {
        // Don't generate a core file, abort is intentional
        struct rlimit limit;
        limit.rlim_cur = 0;
        limit.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &limit);
        abort();
    }
}

int mon_log_write(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    if (MonLog->isUseAltLog())
    {
        MonLog->writeAltLog(eventType, severity, msg);
    }
    else
    {
        MonLog->writeMonLog(eventType, severity, msg);
    }

    if (trace_settings & TRACE_EVLOG_MSG)
    {
        trace_printf("Evlog event: type=%d, severity=%d, text: %s",
                     eventType, severity, msg);
    }

    return(0);
}

int monproc_log_write(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    if (MonLog->isUseAltLog())
    {
        MonLog->writeAltLog(eventType, severity, msg);
    }
    else
    {
        MonLog->writeMonProcLog(eventType, severity, msg);
    }
    return(0);
}

void mem_log_write(int eventType, int value1, int value2)
{
    MonLog->memLogWrite(eventType, value1, value2);
    return;
}

CMonLog::CMonLog( const char *log4cxxConfig
                , const char *log4cxxComponent
                , const char *logFilePrefix
                , int pnid
                , int nid
                , int pid
                , const char *processName )
       : log4cxxConfig_(log4cxxConfig)
       , log4cxxComponent_(log4cxxComponent)
       , myPNid_(pnid)
       , myNid_(nid)
       , myPid_(pid)
       , myProcessName_(processName)
       , memLogID_(0)
       , memLogHeader_(NULL)
       , memLogBase_(NULL)
{
    gettimeofday(&startTime_, NULL);

    struct tm * ltime = localtime(&startTime_.tv_sec);
    sprintf(startTimeFmt_, "%04d%02d%02d.%02d.%02d.%02d", ltime->tm_year+1900, ltime->tm_mon+1, ltime->tm_mday, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);

    char *env = getenv("SQ_MON_ALTLOG");
    useAltLog_ = ( env && *env == '1' );

    logFileNum_ = 1;
    logFileNamePrefix_.assign( logFilePrefix );
    logFileType_ = SBX_LOG_TYPE_LOGFILE;

    // Log4cxx logging
    char   hostname[MAX_PROCESSOR_NAME] = {'\0'};
    gethostname(hostname, MAX_PROCESSOR_NAME);
    char   logFileSuffix[MAX_FILE_NAME];

    // Set flag to indicate whether we are operating in a real cluster
    // or a virtual cluster.
    if ( getenv("SQ_VIRTUAL_NODES") )
    {
        IsRealCluster = false;
    }

    if (IsRealCluster)
    {
        sprintf( logFileSuffix, ".%s.log"
               , hostname );
    }
    else
    {
        if (myNid_ != -1)
        {
            sprintf( logFileSuffix, ".%d.%s.log"
                   , myNid_
                   , hostname);
        }
        else if (myPNid_ != -1)
        {
            sprintf( logFileSuffix, ".%d.%s.log"
                   , myPNid_
                   , hostname);
        }
        else
        {
            sprintf( logFileSuffix, ".%d.%s.log"
                   , myPid_
                   , hostname);
        }
    }

    CommonLogger::instance().initLog4cxx(log4cxxConfig_.c_str(), logFileSuffix);
}

CMonLog::~CMonLog()
{
    const char method_name[] = "CMonLog::~CMonLog";

    if ( memLogID_ )
    {
        int rc = shmctl( memLogID_, IPC_RMID, NULL );
        if (rc)
        {
            int err = errno;
            char la_buf[MON_STRING_BUF_SIZE];
            sprintf(la_buf, "[%s], Error= Can't remove shared memory segment! - errno=%d (%s)\n", method_name, err, strerror(err));
            mon_log_write(MON_LOG_ERROR_3, SQ_LOG_ERR, la_buf);
        }
    }
}

logLevel CMonLog::getLogLevel( posix_sqlog_severity_t severity )
{
    logLevel llevel = LL_INFO;

    switch (severity)
    {
        case SQ_LOG_EMERG:
            llevel = LL_FATAL;
            break;
        case SQ_LOG_ALERT:
            llevel = LL_WARN;
            break;
        case SQ_LOG_CRIT:
            llevel = LL_FATAL;
            break;
        case SQ_LOG_ERR:
            llevel = LL_ERROR;
            break;
        case SQ_LOG_WARNING:
            llevel = LL_WARN;
            break;
        case SQ_LOG_NOTICE:
            llevel = LL_INFO;
            break;
        case SQ_LOG_INFO:
            llevel = LL_INFO;
            break;
        case SQ_LOG_DEBUG:
            llevel = LL_DEBUG;
            break;
        default:
            llevel = LL_INFO;
    }

    return( llevel );
}

void CMonLog::writeAltLog(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    char   logFileDir[PATH_MAX];
    char  *logFileDirPtr;
    char   logFilePrefix[MAX_FILE_NAME];
    char  *logDir;

    if ( useAltLog_ )
    {
        logDir = getenv("TRAF_LOG");
        if (logDir == NULL)
        {
            logFileDirPtr = NULL;
        }
        else
        {
            sprintf(logFileDir, "%s", logDir);
            logFileDirPtr = logFileDir;
        }

        // log file prefix will be mon.<date>.hh.mm.ss.nn
        // where mon.<date>.hh.mm.ss is the startup time of the monitor and
        // nn is the file number. If the log file becomes too big,
        // LogFileNum can be incremented.
        sprintf( logFilePrefix, "%s.%s.%02d"
               , logFileNamePrefix_.c_str(), (char *)&startTimeFmt_, logFileNum_);

        SBX_log_write(logFileType_,            // log_type
                      logFileDirPtr,           // log_file_dir
                      logFilePrefix,           // log_file_prefix
                      SQEVL_MONITOR,           // component id
                      eventType,               // event id
                      SQ_LOG_SEAQUEST,         // facility
                      severity,                // severity
                      myProcessName_.c_str(),  // name
                      NULL,                    // msg_prefix
                      msg,                     // msg
                      NULL,                    // snmptrap_cmd
                      NULL,                    // msg_snmptrap
                      NULL,                    // msg_ret
                      0);                      // msg_ret size

        // write to the same file in future without opening and closing it.
        if (logFileType_ == SBX_LOG_TYPE_LOGFILE)
        {
            logFileType_ |= SBX_LOG_TYPE_LOGFILE_PERSIST;
        }
    }

    return;
}

void CMonLog::writeMonLog(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    logLevel llevel = getLogLevel( severity );

    int status = pthread_mutex_lock(&MonLogMutex);
    assert(status == 0);

    // Log4cxx logging
    CommonLogger::log( log4cxxComponent_
                     , llevel
                     , "Node Number: %u,, PIN: %u , Process Name: %s,,, TID: %d, Message ID: %u, %s"
                     , myPNid_, myPid_, myProcessName_.c_str(), gettid(), eventType,  msg);
    status = pthread_mutex_unlock(&MonLogMutex);
    assert(status == 0);
    
    return;
}

void CMonLog::writeMonProcLog(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    logLevel llevel = getLogLevel( severity );

    int status = pthread_mutex_lock(&MonLogMutex);
    assert(status == 0);

    // Log4cxx logging
    CommonLogger::log( log4cxxComponent_
                     , llevel
                     , "Node Number: %u, CPU: %u, PIN: %u , Process Name: %s,,, TID: %u, Message ID: %u, %s"
                     , myPNid_, myNid_, myPid_, myProcessName_.c_str(), gettid(), eventType, msg);

    status = pthread_mutex_unlock(&MonLogMutex);
    assert(status == 0);
    
    return;
}

void CMonLog::setLogFileNum(int value)
{
    logFileNum_ = value;
    // remove the persist flag so that the previous log file will get closed
    // and a new file will be opened (in append mode) in the next log call.
    logFileType_ = SBX_LOG_TYPE_LOGFILE; 
}

// Create a circular buffer in a shared memory segment for in-memory log data. 
// The key is of the form 0x1234<monitor's pid>. 
void CMonLog::setupInMemoryLog()
{
    const char method_name[] = "CMonLog::setupInMemoryLog";

    int mypid = getpid();

    key_t memLogKey = (key_t) MEM_LOG_KEY(mypid);
    
    int memLogPerm = 0640; // user can read/write, group can read

    int entries = MEM_LOG_NUM_ENTRIES;
    char *memLogEntries = getenv("SQ_MON_MEM_LOG_ENTRIES");
    if ( memLogEntries )
    {
       entries = atoi(memLogEntries);
    }

    memLogID_ = shmget( memLogKey, MEM_LOG_SIZE(entries), IPC_CREAT | IPC_EXCL | memLogPerm );
    
    if (memLogID_ == -1)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't create shared memory segment for in-memory log! - errno=%d (%s)\n", 
                method_name, err, strerror(err));
        mon_log_write(MON_LOG_ERROR_1, SQ_LOG_ERR, la_buf);
    
        mon_failure_exit();
    }

    memLogHeader_ = (memLogHeader_t *)shmat( memLogID_, NULL, 0 );

    if (memLogHeader_ == (void *)-1)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't map shared memory segment address! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_LOG_ERROR_2, SQ_LOG_CRIT, la_buf);
        
        mon_failure_exit();
    }

    memLogBase_ = (memLogEntry_t *)(memLogHeader_ + 1);
    memLogHeader_->currEntry_ = 0;
    memLogHeader_->cycles_ = 0;
    memLogHeader_->entries_ = entries;
    
    return;
}

// write the log entry in the shared memory.
// currEntry_ is the index of the next free entry.
// if the buffer gets full, start from the beginning
void CMonLog::memLogWrite(int eventType, int value1, int value2)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    memLogLock_.lock();

    memLogEntry_t *entry = (memLogEntry_t *)&memLogBase_[memLogHeader_->currEntry_];

    entry->eventType_ = eventType;
    entry->ts_ = ts;
    entry->value1_ = value1;
    entry->value2_ = value2;

    ++memLogHeader_->currEntry_;
    if (memLogHeader_->currEntry_ > memLogHeader_->entries_)
    {
        memLogHeader_->currEntry_ = 0;
        ++memLogHeader_->cycles_;
    }  

    memLogLock_.unlock();

    return;
}



