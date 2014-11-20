///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>

using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <seabed/logalt.h>

#include "monlogging.h"
#include "montrace.h"
#include "msgdef.h"

#define gettid() syscall(__NR_gettid)

extern int MyPNID;
extern CMonLog *MonLog;
extern CMonLog *SnmpLog;

int mon_log_write(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    MonLog->writeAltLog(eventType, severity, msg);
    return(0);
}

int wdt_log_write(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    MonLog->writeAltLog(eventType, severity, msg);
    return(0);
}

int snmp_log_write(int eventType, const char *msg)
{
    SnmpLog->writeSnmpLog(eventType, msg);
    return(0);
}

void mem_log_write(int eventType, int value1, int value2)
{
    MonLog->memLogWrite(eventType, value1, value2);
    return;
}

CMonLog::CMonLog( const char *logFilePrefix ) :
         memLogID_(0),
         memLogHeader_(NULL),
         memLogBase_(NULL)
{
    gettimeofday(&startTime_, NULL);

    struct tm * ltime = localtime(&startTime_.tv_sec);
    sprintf(startTimeFmt_, "%02d%02d%02d.%02d.%02d.%02d", ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year-100, ltime->tm_hour, ltime->tm_min, ltime->tm_sec);

    char *env = getenv("SQ_MON_ALTLOG");
    useAltLog_ = ( env && *env == '1' );

    logFileNum_ = 1;
    logFileNamePrefix_.assign( logFilePrefix );
    logFileType_ = SBX_LOG_TYPE_LOGFILE;
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

void CMonLog::writeAltLog(int eventType, posix_sqlog_severity_t severity, char *msg)
{
    char   logFileDir[PATH_MAX];
    char  *logFileDirPtr;
    char   logFilePrefix[30];
    char  *rootDir;

    rootDir = getenv("MY_SQROOT");
    if (rootDir == NULL) 
    {
        logFileDirPtr = NULL;
    }
    else 
    {
        sprintf(logFileDir, "%s/logs", rootDir);
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
                  "monitor",               // name
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

    return;
}

void CMonLog::writeSnmpLog(int eventType, const char *msg)
{
    char   logFileDir[PATH_MAX];
    char  *logFileDirPtr;
    char   logFilePrefix[30];
    char  *rootDir;

    rootDir = getenv("MY_SQROOT");
    if (rootDir == NULL) 
    {
        logFileDirPtr = NULL;
    }
    else 
    {
        sprintf(logFileDir, "%s/logs", rootDir);
        logFileDirPtr = logFileDir;
    }

    // log file prefix will be snmp.mon.nn
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
                  SQ_LOG_CRIT,             // severity
                  "monitor",               // name
                  NULL,                    // msg_prefix
                  msg,                     // msg
                  NULL,                    // snmptrap_cmd
                  NULL,                    // msg_snmptrap
                  NULL,                    // msg_ret
                  0);                      // msg_ret size

    return;
}

int CMonLog::getLogFileNum()
{
    return logFileNum_;
}

void CMonLog::setLogFileNum(int value)
{
    logFileNum_ = value;
    // remove the persist flag so that the previous log file will get closed
    // and a new file will be opened (in append mode) in the next log call.
    logFileType_ = SBX_LOG_TYPE_LOGFILE; 
}

void CMonLog::setLogFileType(int value)
{
    logFileType_ = value;
}

void CMonLog::setUseAltLog(bool value)
{
    useAltLog_ = value;
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
    
        abort(); 
    }

    memLogHeader_ = (memLogHeader_t *)shmat( memLogID_, NULL, 0 );

    if (memLogHeader_ == (void *)-1)
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error= Can't map shared memory segment address! - errno=%d (%s)\n", method_name, err, strerror(err));
        mon_log_write(MON_LOG_ERROR_2, SQ_LOG_CRIT, la_buf);
        
        abort();
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



