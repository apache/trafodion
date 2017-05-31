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

//#include <iostream>

using namespace std;

#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/unistd.h>
#include "trafconfigtrace.h"
#include "trafconfiglog.h"

#define gettid() syscall(__NR_gettid)

pthread_mutex_t       TcLogMutex = PTHREAD_MUTEX_INITIALIZER;

const char *LogLevelStr( int severity )
{
    const char *str;

    switch (severity)
    {
        case TC_LOG_EMERG:
            str = "FATAL";
            break;
        case TC_LOG_ALERT:
            str = "WARN";
            break;
        case TC_LOG_CRIT:
            str = "FATAL";
            break;
        case TC_LOG_ERR:
            str = "ERROR";
            break;
        case TC_LOG_WARNING:
            str = "WARN";
            break;
        case TC_LOG_NOTICE:
            str = "INFO";
            break;
        case TC_LOG_INFO:
            str = "INFO";
            break;
        case TC_LOG_DEBUG:
            str = "DEBUG";
            break;
        default:
            str = "INFO";
    }

    return( str );
}

int TcLogWrite(int eventType, int severity, char *msg)
{
    int status;

    char   eventTimeFmt[20];
    struct timeval eventTime;

    status = pthread_mutex_lock(&TcLogMutex);
    assert(status == 0);

    gettimeofday(&eventTime, NULL);
    struct tm *ltime = localtime(&eventTime.tv_sec);
    sprintf( eventTimeFmt
           , "%04d-%02d-%02d %02d:%02d:%02d"
           , ltime->tm_year+1900
           , ltime->tm_mon+1
           , ltime->tm_mday
           , ltime->tm_hour
           , ltime->tm_min
           , ltime->tm_sec );

    fprintf( stderr
           , "%s,, %s, TRAFCONFIG,,, PIN: %u,,,, TID: %ld, Message ID: %d, %s\n"
           , eventTimeFmt
           , LogLevelStr(severity)
           , getpid()
           , gettid()
           , eventType
           , msg );

    status = pthread_mutex_unlock(&TcLogMutex);
    assert(status == 0);
    
    if (TcTraceSettings & TC_TRACE_LOG_MSG)
    {
        trace_printf("Log Event: %d, %s\n(%s)\n",
                     eventType, LogLevelStr(severity), msg);
    }

    return(0);
}
